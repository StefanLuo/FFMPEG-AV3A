/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2002-2018, Audio Video coding Standard Workgroup of China
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of Audio Video coding Standard Workgroup of China
*    nor the names of its contributors maybe used to endorse or promote products
*    derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <setjmp.h>
#include "attf_head.h"
#include "..\general\decode\crc_16.h"
#include "..\general\decode\bitstream.h"
#include "..\general\bwedec\decoder.h"
#include "..\general\decode\avs2decmain.h"
#include "..\general\bwedec\avs2BweDecMDFT.h"
#include "..\general\decode\avs2audio.h"
#include "..\general\decode\lfdec.h"
#include "..\general\decode\pca.h"
#include "..\general\decode\codebook.h"
#include "..\general\decode\maxcorr.h"
#include "..\general\decode\mc_rom.h"
#include "general_decoder_frame.h"

#define SAMPLES_PER_FRAME 1024

inbufferstruct avs_inbuffer;		//for attf tream.circle buffer
attfheadinfo avs_attfheadinfo;	//buffer for attf head information and bed audio information
threeDInfo avs_3Dinfo;			//buffer for object information

static void ob_raw_data_block();	//table 78
static void threeD_ObjectInfo(int);	//table 79
static void threeD_ObjectData(int object_index);	//table 80

jmp_buf attf_jump_buffer;
static int attf_overflag = 0;		//==1, means no enough data. program will exit.

avs2audiopack_buffer opb;		//temporary buffer for attf stream analysize
unsigned int opbbuffer[29 * 40];	//opb.buffer = opbbuffer;

int road = 0;
int index_obj;

void attf_avs2audiopack_init( unsigned int *inbuf, int storage, int endbyte, int endbit)
{
	memset(&opb, 0, sizeof(avs2audiopack_buffer));
	opb.buffer = opb.ptr = inbuf;
	opb.storage = storage;
	opb.endbit = endbit;
	opb.endbyte = endbyte;
}
//-------------------------  attf bit stream tools -----------------------------
void attf_init_inbuffer()
{
	avs_inbuffer.readbitbegin = avs_inbuffer.writebitbegin = 0;
	avs_inbuffer.databitlength = 0;
	avs_inbuffer.emptybitlength = ATTF_BUFFER_LENGTH * 32;	//bit

	avs_inbuffer.bitoffset = 0;

	memset(&avs_attfheadinfo, 0, sizeof(avs_attfheadinfo));
	memset(&avs_3Dinfo, 0, sizeof(avs_3Dinfo));
}

//description: just move the point to the border of BYTE.
unsigned int attf_bytealignment()	//将位索引偏移量调整到字节单位
{
	int tmp;
	if (avs_inbuffer.bitoffset / 8 * 8 != avs_inbuffer.bitoffset)
	{
		tmp = ((avs_inbuffer.bitoffset / 8) + 1) * 8;	//move to next byte's beginning position
		avs_inbuffer.databitlength -= (tmp - avs_inbuffer.bitoffset);
		avs_inbuffer.emptybitlength += (tmp - avs_inbuffer.bitoffset);
		avs_inbuffer.bitoffset = tmp % (ATTF_BUFFER_LENGTH * 32);
		
	}
	return avs_inbuffer.bitoffset;
}

//返回当前位索引位置
unsigned int attf_getbitoffset()	//返回当前位索引位置
{
	return avs_inbuffer.bitoffset;
}

//just move forward, do nothing. bits can be larger than 32;
unsigned int attf_forward(int bits)
{
	if (avs_inbuffer.databitlength < bits)	
	{
		attf_waitfordata(bits);	//need enough words for analysis
	}
	avs_inbuffer.bitoffset += bits;

	avs_inbuffer.bitoffset %= (ATTF_BUFFER_LENGTH * 32);

	avs_inbuffer.databitlength -= bits;
	avs_inbuffer.emptybitlength += bits;

	return avs_inbuffer.bitoffset;
}

//just move backward, do nothing. bits can be larger than 32;
//	attention: do not judge whether there is data in buffer. User should do the judgement themselves 
unsigned int attf_backward(unsigned int bits)
{
	if (avs_inbuffer.bitoffset < bits)
		avs_inbuffer.bitoffset = avs_inbuffer.bitoffset + ATTF_BUFFER_LENGTH * 32 - bits;
	else
		avs_inbuffer.bitoffset -= bits;

	avs_inbuffer.databitlength += bits;
	avs_inbuffer.emptybitlength -= bits;

	return avs_inbuffer.bitoffset;
}

//Description: read next bits from stream: "avs_inbuffer.inbuffer" in bslbf order.
//In:  bits: the bits' number whill be read. attention: bits should be <= 32.
//Out: none
//return: the value be read.
unsigned int attf_nextBits(int bits)	//bits <=32
{
	unsigned int ret;
	if (avs_inbuffer.databitlength < bits)//100 bytes. can use other values.
	{
		attf_waitfordata(bits);	//need enough words for analysis
	}

	if (((avs_inbuffer.bitoffset + bits - 1) / 32) == avs_inbuffer.bitoffset / 32) //in a same INT
	{
		// (XXXXXXXX XXXXXXXX -------- ----XXXX) //this word
		//   leftemptybits   |            |rightemptybits
		int leftemptybits = (avs_inbuffer.bitoffset % 32);
		int rightemptybits = 31 - ((avs_inbuffer.bitoffset + bits - 1) % 32);
		unsigned thisword = avs_inbuffer.inbuffer[avs_inbuffer.bitoffset / 32];

		thisword = (thisword << 24) | ((thisword >> 24) | (thisword & 0x0000FF00) << 8) | ((thisword & 0x00FF0000) >> 8);		//change the order from bslbf to bsmbf

		ret = thisword << leftemptybits >> (leftemptybits + rightemptybits);
	}
	else{		//in different word
		//               leftword								rightword
		// (XXXXXXXX XXXXXXXX -------- --------) (-------- ----XXXX XXXXXXXX XXXXXXXX)
		//   leftemptybits   |                   |rightbits   | rightemptybits
		int leftemptybits = avs_inbuffer.bitoffset % 32;
		int rightemptybits = 31 - (avs_inbuffer.bitoffset + bits - 1) % 32;
		int rightbit = (avs_inbuffer.bitoffset + bits) % 32;
		unsigned leftword = avs_inbuffer.inbuffer[avs_inbuffer.bitoffset / 32];
		unsigned rightword = avs_inbuffer.inbuffer[(avs_inbuffer.bitoffset / 32 + 1) % ATTF_BUFFER_LENGTH];

		leftword = (leftword << 24) | ((leftword >> 24) | (leftword & 0x0000FF00) << 8) | ((leftword & 0x00FF0000) >> 8);	//change the order from bslbf to bsmbf
		rightword = (rightword << 24) | ((rightword >> 24) | (rightword & 0x0000FF00) << 8) | ((rightword & 0x00FF0000) >> 8);	//change the order from bslbf to bsmbf
		ret = (leftword << leftemptybits >> (leftemptybits - rightbit)) | (rightword >> rightemptybits);
	}
	avs_inbuffer.bitoffset += bits;
	avs_inbuffer.bitoffset %= (ATTF_BUFFER_LENGTH * 32);

	avs_inbuffer.databitlength -= bits;
	avs_inbuffer.emptybitlength += bits;
	return ret;
}
//-------------------------  attf bit stream tools -----------------------------

//-------------------------  encode stream functions -----------------------------
//in: nono
//out: *bitoffset: the bitoffset of stream in buffer:"avs_inbuffer.inbuffer"
//     *length: the length of stream (in BYTE) 
void ga_raw_data_block(int *bitoffset, int *length)	//table 2: ga_raw_data_block()
{
	memset(&opb, 0, sizeof(avs2audiopack_buffer));
	attf_forward(100 * 32); attf_backward(100 * 32);	//be sure there is stream data in avs_buffer.inbuffer. 
	attf_copydata(100 * 4, avs_inbuffer.bitoffset, opbbuffer);
	attf_avs2audiopack_init(opbbuffer, 100 * 4, 0, avs_inbuffer.bitoffset % 32);

	attf_forward(16);	*length = avs2audiopack_read(&opb, 16);		//data's length in BYTE
	*bitoffset = attf_getbitoffset(); 
	attf_forward((*length) * 8); 	//move forward to next section (next frame or next object)
}

//in: none
//out: *bitoffset: the bitoffset of stream in buffer:"avs_inbuffer.inbuffer"
//     *length: the length of stream (in BYTE) 
void ll_raw_data_block(int *bitoffset, int *length)	//table 1: ll_raw_data_block()
{
	memset(&opb, 0, sizeof(avs2audiopack_buffer));
	attf_forward(100 * 32); attf_backward(100 * 32);
	attf_copydata(100 * 4, avs_inbuffer.bitoffset, opbbuffer);
	attf_avs2audiopack_init(opbbuffer, 100 , 0, avs_inbuffer.bitoffset % 32);

	*bitoffset = attf_getbitoffset();
	attf_forward(16); *length = avs2audiopack_read(&opb, 16);
	attf_forward((*length) * 8);				//move forward, 
}

//-------------------------  bed stream functions -----------------------------
//--------------------------------- object metadata define & functions ------------------------------------------
static void ob_raw_data_block()				//table 78
{
	int i, ele;

	attf_forward(7); avs_3Dinfo.ObjectNum = avs2audiopack_read(&opb, 7);
	for (i = 0; i < avs_3Dinfo.ObjectNum; i++)
	{
		threeD_ObjectInfo(i);	
	}
	attf_bytealignment();
	attf_forward(100 * 32); attf_backward(100 * 32);
	attf_copydata(100, avs_inbuffer.bitoffset, opbbuffer);
	attf_avs2audiopack_init(opbbuffer, 100, 0, avs_inbuffer.bitoffset % 32);

	if (avs_3Dinfo.ObjectNum > 0)
	{
		attf_forward(7); avs_3Dinfo.threeD_ObjectDataGroupNum = avs2audiopack_read(&opb, 7);

		if (avs_3Dinfo.threeD_ObjectDataGroupNum != avs_3Dinfo.ObjectNum)
		{
			printf("threeD_ObjectDataGroupNum(%d) !=  ObjectNum(%d)\n", avs_3Dinfo.threeD_ObjectDataGroupNum, avs_3Dinfo.ObjectNum);
			longjmp(attf_jump_buffer, 0);
		}
		for (ele = 0; ele < avs_3Dinfo.threeD_ObjectDataGroupNum; ele++)
			threeD_ObjectData(ele);
	}
}
	
//最多一个对象的objectinfo占用29个字节
static void threeD_ObjectInfo(int object_index)	//table 79
{
	int sb;
	int PanInfoFlag;
	int pId[4] = { 0 }, px[4] = { 0 }, py[4] = { 0 }, pz[4] = { 0 };
	
	attf_forward(7); avs_3Dinfo.object_ID[object_index] = avs2audiopack_read(&opb, 7);
	
	for (sb = 0; sb < 4; sb++)	//NumPanSubBlocks = 4
	{
		if (sb == 0)
			PanInfoFlag = 1;
		else
		{
			attf_forward(1); PanInfoFlag = avs2audiopack_read(&opb, 1);
		}

		if (PanInfoFlag == 1)
		{
			attf_forward(3); pId[sb] = avs2audiopack_read(&opb, 3);
			if (sb == 0)
			{
				attf_forward(10); px[sb] = avs2audiopack_read(&opb, 10); 
				attf_forward(10); py[sb] = avs2audiopack_read(&opb, 10); 
				attf_forward(10); pz[sb] = avs2audiopack_read(&opb, 10); 
			}
			else{
				attf_forward(4); px[sb] = avs2audiopack_read(&opb, 4);		
				if (px[sb] == 15)
				{
					attf_forward(8); px[sb] = avs2audiopack_read(&opb, 8);
					if (px[sb] == 255)
					{
						attf_forward(12); px[sb] = avs2audiopack_read(&opb, 12);
					}
				}
				attf_forward(4); py[sb] = avs2audiopack_read(&opb, 4);			
				if (py[sb] == 15)
				{
					attf_forward(8); py[sb] = avs2audiopack_read(&opb, 8);
					if (py[sb] == 255)
					{
						attf_forward(12); py[sb] = avs2audiopack_read(&opb, 12);
					}
				}
				attf_forward(4); pz[sb] = avs2audiopack_read(&opb, 4);			 
				if (pz[sb] == 15)
				{
					attf_forward(8); pz[sb] = avs2audiopack_read(&opb, 8);
					if (pz[sb] == 255)
					{
						attf_forward(12); pz[sb] = avs2audiopack_read(&opb, 12);
					}
				}
			}
			
			attf_forward(1); avs_3Dinfo.ObjectAreaControl[object_index][sb] = avs2audiopack_read(&opb, 1); 
			if (avs_3Dinfo.ObjectAreaControl[object_index][sb] == 1)
			{
				attf_forward(4); avs_3Dinfo.ObjectAreaGamma[object_index][sb] = avs2audiopack_read(&opb, 4);
			}

			attf_forward(3); avs_3Dinfo.ObjectType[object_index][sb] = avs2audiopack_read(&opb, 3);
			if (avs_3Dinfo.ObjectType[object_index][sb] > 0)
			{
				attf_forward(7); avs_3Dinfo.ObjectEnvelope[object_index][sb] = avs2audiopack_read(&opb, 7); 
			}
		}
		attf_forward(4); avs2audiopack_read(&opb, 4); //4 bits reserved 
	}
	attf_forward(16); avs2audiopack_read(&opb, 16);  //16 bits reserved

	for (sb = 0; sb < 4; sb++)
	{
		switch (pId[sb])
		{
		case 0:
			px[sb] = px[sb];			py[sb] = py[sb];			pz[sb] = pz[sb];
			break;
		case 1:
			px[sb] = px[sb];			py[sb] = py[sb];			pz[sb] = (-1)*pz[sb];
			break;
		case 2:
			px[sb] = px[sb];			py[sb] = (-1)*py[sb];		pz[sb] = pz[sb];
			break;
		case 3:
			px[sb] = px[sb];			py[sb] = (-1)*py[sb];		pz[sb] = (-1)*pz[sb];
			break;
		case 4:
			px[sb] = (-1)*px[sb];		py[sb] = py[sb];			pz[sb] = pz[sb];
			break;
		case 5:
			px[sb] = (-1)*px[sb];		py[sb] = py[sb];			pz[sb] = (-1)*pz[sb];
			break;
		case 6:
			px[sb] = (-1)*px[sb];		py[sb] = (-1)*py[sb];		pz[sb] = pz[sb];
			break;
		case 7:
			px[sb] = (-1)*px[sb];		py[sb] = (-1)*py[sb];		pz[sb] = (-1)*pz[sb];
			break;
		}
	}

	avs_3Dinfo.objpx[object_index][0] = (float)px[0] / 1000;
	avs_3Dinfo.objpy[object_index][0] = (float)py[0] / 1000;
	avs_3Dinfo.objpz[object_index][0] = (float)pz[0] / 1000;

	for (sb = 1; sb<4; sb++)
	{
		px[sb] = px[sb] + px[sb - 1];
		avs_3Dinfo.objpx[object_index][sb] = (float)px[sb] / 1000;

		py[sb] = py[sb] + py[sb - 1];
		avs_3Dinfo.objpy[object_index][sb] = (float)py[sb] / 1000;

		pz[sb] = pz[sb] + pz[sb - 1];
		avs_3Dinfo.objpz[object_index][sb] = (float)pz[sb] / 1000;
	}
}

static void threeD_ObjectData(int object_index)	//table 80
{
	attf_forward(200 * 32); attf_backward(200 * 32);
	attf_copydata(200, avs_inbuffer.bitoffset, opbbuffer);
	attf_avs2audiopack_init(opbbuffer, 200, 0, avs_inbuffer.bitoffset % 32);	

	attf_forward(2); avs_3Dinfo.threeD_Object_codec_id[object_index] = avs2audiopack_read(&opb, 2);
	if (avs_3Dinfo.threeD_Object_codec_id[object_index] != 0)
	{
		printf("only general encoder is supported\n");
		longjmp(attf_jump_buffer, 0);
	}

	if (avs_3Dinfo.threeD_Object_codec_id[object_index] == 0)
	{
		attf_forward(3); avs_3Dinfo.channel_number_index[object_index] = avs2audiopack_read(&opb, 3);
		switch (avs_3Dinfo.channel_number_index[object_index]) {	//Table A.7 channel_number table
		case 0:  //mono
			avs_3Dinfo.channel_number[object_index] = 1;
			break;
		case 1:  //stereo
			avs_3Dinfo.channel_number[object_index] = 2;
			break;
		case 2:  //5.1
			avs_3Dinfo.channel_number[object_index] = 6;
			break;
		case 3:  //7.1
			avs_3Dinfo.channel_number[object_index] = 8;
			break;
		case 4: //10.2
			avs_3Dinfo.channel_number[object_index] = 12;
			break;
		case 5: //22.2
			avs_3Dinfo.channel_number[object_index] = 24;
			break;
		default:
			printf("Error object channel_number_index %d\n", avs_3Dinfo.channel_number[object_index]);
			longjmp(attf_jump_buffer, 0);
//			exit(1);
			break;
		}
	}

	if (avs_3Dinfo.threeD_Object_codec_id[object_index] == 1)
	{
		attf_forward(4); avs_3Dinfo.channel_number[object_index] = avs2audiopack_read(&opb, 4);
		if (avs_3Dinfo.channel_number[object_index] == 0xf)			//if > 15, 8 bits for channel_number
		{
			attf_forward(4); avs_3Dinfo.channel_number[object_index] = 15 + avs2audiopack_read(&opb, 4);
		}
	}

	attf_forward(2); avs_3Dinfo.resolution[object_index] = (avs2audiopack_read(&opb, 2) + 1) * 8;			//table A.2: resolution
	if (avs_3Dinfo.threeD_Object_codec_id[object_index] == 0)
	{
		attf_forward(6); avs_3Dinfo.bitrate_index[object_index] = avs2audiopack_read(&opb, 6);
		attf_forward(1); avs_3Dinfo.bitstream_type[object_index] = avs2audiopack_read(&opb, 1);
	}

	attf_bytealignment();
	
	if (avs_3Dinfo.threeD_Object_codec_id[object_index] == 0)		//general encoder data block
	{	
		ga_raw_data_block(&avs_3Dinfo.bitoffset[object_index], &avs_3Dinfo.datalength[object_index]);	//object audio's postion and length.记录对象音频数据的位置和长度
	}
	if (avs_3Dinfo.threeD_Object_codec_id[object_index] == 1)		//lossless encoder data block
	{	
		//ll_raw_data_block(&avs_3Dinfo.bitoffset[object_index], &avs_3Dinfo.datalength[object_index]);
		printf("Can not arrive at here, lossless is not supported here!\n");
		longjmp(attf_jump_buffer, 0);
		exit(1);
	}
}
//---------------------------------End: object metadata define & functions ------------------------------------------

//------------------------- main process function -----------------------------
int aatf_frame()		//table A.4:  attf_frame()
{
	//avs_attfheadinfo.syncword = attf_nextBits(12);	//syncword: 12bits	
	avs_attfheadinfo.audio_codec_id = attf_nextBits(2);						 //0: general audio encoder data

	if (avs_attfheadinfo.audio_codec_id != 0)
	{
		printf("only general encoder is supported !\n");
		longjmp(attf_jump_buffer, 0);
	}

	if (avs_attfheadinfo.audio_codec_id < 2)
	{
		//----- table A.5:  aatf_frame_head() ----------
		avs_attfheadinfo.coding_profile = attf_nextBits(2);					//0: general encoder framework
		if (avs_attfheadinfo.coding_profile == 2 || avs_attfheadinfo.coding_profile == 3)
		{
			printf("only basic framework and metadata is supported\n");
			longjmp(attf_jump_buffer, 0);
		}

		avs_attfheadinfo.sampling_frequency_index = attf_nextBits(4);		
		if ((avs_attfheadinfo.audio_codec_id == 1) && avs_attfheadinfo.sampling_frequency_index == 0x0f)
		{
			avs_attfheadinfo.sampling_frequency = attf_nextBits(24);
		}
		switch (avs_attfheadinfo.sampling_frequency_index) {		//table A.9: sampling rate table
		case 0:
			avs_attfheadinfo.sampling_frequency = 192000;
			break;
		case 1:
			avs_attfheadinfo.sampling_frequency = 96000;
			break;
		case 2:
			avs_attfheadinfo.sampling_frequency = 48000;
			break;
		case 3:
			avs_attfheadinfo.sampling_frequency = 44100;
			break;
		case 4:
			avs_attfheadinfo.sampling_frequency = 32000;;
			break;
		case 5:
			avs_attfheadinfo.sampling_frequency = 24000;
			break;
		case 6:
			avs_attfheadinfo.sampling_frequency = 22050;
			break;
		case 7:
			avs_attfheadinfo.sampling_frequency = 16000;
			break;
		case 8:
			avs_attfheadinfo.sampling_frequency = 8000;
			break;
		default:
			printf("Wrong sampling_frequency_index %d\n", avs_attfheadinfo.sampling_frequency_index);
			longjmp(attf_jump_buffer, 0);
			break;
		}

		if (avs_attfheadinfo.audio_codec_id == 0)
		{
			avs_attfheadinfo.channel_number_index = attf_nextBits(3);
			//avs_attfheadinfo.channel_number_index = 0xf; //for test the setjmp and longjmp
			switch (avs_attfheadinfo.channel_number_index) {	//Table A.7 channel_number table
			case 0:  //mono
				avs_attfheadinfo.channel_number = 1;
				break;
			case 1:  //stereo
				avs_attfheadinfo.channel_number = 2;
				break;
			case 2:  //5.1
				avs_attfheadinfo.channel_number = 6;
				break;
			case 3:  //7.1
				avs_attfheadinfo.channel_number = 8;
				break;
			case 4: //10.2
				avs_attfheadinfo.channel_number = 12;
				break;
			case 5: //22.2
				avs_attfheadinfo.channel_number = 24;
				break;
			default:
				printf("Error channel_number_index %d!\n", avs_attfheadinfo.channel_number_index);
				longjmp(attf_jump_buffer, 0);
				break;
			}
		}
		
		if (avs_attfheadinfo.audio_codec_id == 1)
		{
			avs_attfheadinfo.channel_number = attf_nextBits(4);
			if (avs_attfheadinfo.channel_number == 0xf)
			{
				avs_attfheadinfo.channel_number = 15 + attf_nextBits(4);
			}
		}

		// resolution
		avs_attfheadinfo.resolution = (attf_nextBits(2) + 1) * 8;	//table A.2: resolution
		if (avs_attfheadinfo.resolution == 32)
		{
			printf("32 bits/sample is not supported!\n");
			longjmp(attf_jump_buffer, 0);	//32 bit per sample is not supported.
		}

		if (avs_attfheadinfo.audio_codec_id == 0)
		{
			avs_attfheadinfo.bitrate_index = attf_nextBits(6);
			avs_attfheadinfo.bitstream_type = attf_nextBits(1);
		}
		//----- end table A.5:  aatf_frame_head() ----------

		attf_bytealignment();
		
		avs_attfheadinfo.crc_check = attf_nextBits(16);	//crc_check
		
		if (avs_attfheadinfo.audio_codec_id == 0)		//general encoder data block
		{
			ga_raw_data_block(&avs_attfheadinfo.bitoffset, &avs_attfheadinfo.datalength);	//bed audio's postion and length.记录音床音频数据的位置和长度
		}
		if (avs_attfheadinfo.audio_codec_id == 1)		//lossless encoder data block
		{
			//ll_raw_data_block();		//TBD
			printf("Can not arrive at here, lossless is not supported here!\n");
			longjmp(attf_jump_buffer, 0);
		}
		if (avs_attfheadinfo.coding_profile == 1)		//3D meta
		{
			attf_forward(29 * 40 * 32); attf_backward(29 * 40 * 32);		//29 bytes per object's info(maxium), 
			attf_copydata(29 * 40 * 4, avs_inbuffer.bitoffset, opbbuffer);

			attf_avs2audiopack_init(opbbuffer, 29 * 40 * 4, 0, avs_inbuffer.bitoffset % 32);
			attf_forward(4); avs_attfheadinfo.threeDVersion = avs2audiopack_read(&opb, 4);
			ob_raw_data_block();
		}
	}
	return 1;
}

//----------  printf  tools for debug -----------
void print_avsinbuffer()
{
	printf("\n\nreadbitbegin = 0x%x, wirtebitbegin = 0x%x\n", avs_inbuffer.readbitbegin, avs_inbuffer.writebitbegin);
	printf("databitlength = 0x%x,emptybitlength = 0x%x\n ", avs_inbuffer.databitlength, avs_inbuffer.emptybitlength);
	printf("bitoffset = 0x%x\n", avs_inbuffer.bitoffset);
}

void print_attfheadinfo()
{
	printf("-------------- Bed --------------\n");
	printf("audio_codec_id = %d\n", avs_attfheadinfo.audio_codec_id);
	printf("coding_profile = %d\n", avs_attfheadinfo.coding_profile);
	printf("sampling_frequency_index = %d  -> ", avs_attfheadinfo.sampling_frequency_index);
	printf("sampling_frequency = %d\n", avs_attfheadinfo.sampling_frequency);
	printf("channel_number_index = %d  -> ", avs_attfheadinfo.channel_number_index);
	printf("channel_number = %d\n", avs_attfheadinfo.channel_number);
	printf("resolution = %d\n", avs_attfheadinfo.resolution);
	printf("bitrate_index = %d = 0x%x\n", avs_attfheadinfo.bitrate_index, avs_attfheadinfo.bitrate_index);
	printf("bitstream_type = %d\n", avs_attfheadinfo.bitstream_type);
	printf("threeDVersion = %d\n", avs_attfheadinfo.threeDVersion);
	printf("crc_check = %d  = 0x%x\n", avs_attfheadinfo.crc_check, avs_attfheadinfo.crc_check);
	printf("\ndatalength = %d = 0x%x\n", avs_attfheadinfo.datalength, avs_attfheadinfo.datalength);
	printf("bitoffset = %d = 0x%x\n\n", avs_attfheadinfo.bitoffset, avs_attfheadinfo.bitoffset);
}
	
void print_avs_3Dinfo()
{
	int i;
	printf("-------------- Object --------------\n");
	printf("ObjectNum = %d  ", avs_3Dinfo.ObjectNum);
	if (avs_3Dinfo.ObjectNum > 0)
		printf("threeD_ObjectDataGroupNum = %d\n\n", avs_3Dinfo.threeD_ObjectDataGroupNum);

	for (i = 0; i < avs_3Dinfo.ObjectNum;i++)
	{
		printf("............ Object_ID = %d ............\n", avs_3Dinfo.object_ID[i]);
		printf("objx = %f, objy = %f,objz = %f\n", avs_3Dinfo.objpx[i][0], avs_3Dinfo.objpy[i][0], avs_3Dinfo.objpz[i][0]);
		printf("objx = %f, objy = %f,objz = %f\n", avs_3Dinfo.objpx[i][1], avs_3Dinfo.objpy[i][1], avs_3Dinfo.objpz[i][1]);
		printf("objx = %f, objy = %f,objz = %f\n", avs_3Dinfo.objpx[i][2], avs_3Dinfo.objpy[i][2], avs_3Dinfo.objpz[i][2]);
		printf("objx = %f, objy = %f,objz = %f\n", avs_3Dinfo.objpx[i][3], avs_3Dinfo.objpy[i][3], avs_3Dinfo.objpz[i][3]);

		printf("ObjectAreaControl = %d, ObjectAreaGamma = %d,ObjectType = %d\n\n", avs_3Dinfo.ObjectAreaControl[i][3], avs_3Dinfo.ObjectAreaGamma[i][3], avs_3Dinfo.ObjectType[i][3]);
			
		printf("threeD_Object_codec_id = %d\n", avs_3Dinfo.threeD_Object_codec_id[i]);
		printf("channel_number_index = %d -> ", avs_3Dinfo.channel_number_index[i]);
		printf("channel_number = %d\n", avs_3Dinfo.channel_number[i]);
		printf("resolution = %d\n", avs_3Dinfo.resolution[i]);
		printf("bitrate_index = %d = 0x%x\n", avs_3Dinfo.bitrate_index[i], avs_3Dinfo.bitrate_index[i]);
		printf("bitstream_type = %d\n", avs_3Dinfo.bitstream_type[i]);
		printf("\ndatalength = %d = 0x%x\n", avs_3Dinfo.datalength[i], avs_3Dinfo.datalength[i]);
		printf("bitoffset = %d = 0x%x\n\n", avs_3Dinfo.bitoffset[i], avs_3Dinfo.bitoffset[i]);
	}
}
//--------------------------- end printf tools ---------------------------------


//description: copy the ga raw data into buffer
//In: datyalength: the raw data's length(in byte, read from stream)
//	bitoffset: the raw data's offset in inbuffer.
//Out: buffer: the raw stream buffer
//return: buffer
//attention: take care that the datalength can not be larger than the length of buffer!!!!!!!!
int *attf_copydata(int datalength, int bitoffset, int *buffer)
{
	int i;

	for (i = 0; i < (datalength + 7) / 4; i++)
	{
		buffer[i] = avs_inbuffer.inbuffer[(bitoffset / 32 + i)%ATTF_BUFFER_LENGTH];
	}
	return buffer;
}

//description: Check whether the CRC is OK or not! if not OK, wrong sync word or wrong frame.
//return : 1: CRC OK!;  otherwise jump to find sync word 
int attf_crccheck()
{
	unsigned int temp[14];	//12 bytes for CRC-Check: head(4 bytes), length(2 bytes), rawdata(6 bytes)
	unsigned int CRC_check;
	unsigned int CRC_temp;


	temp[0] = attf_nextBits(8);	//0xFF
	temp[1] = attf_nextBits(8);	//0xFX
	temp[2] = attf_nextBits(8);	//
	temp[3] = attf_nextBits(8);	//

	CRC_check = attf_nextBits(16);			//16 bits CRC-check bits
	CRC_check = ((CRC_check >> 8) | (CRC_check << 8)) & 0xFFFF;

	temp[4] = attf_nextBits(8);	//datalength(Hi)
	temp[5] = attf_nextBits(8);	//datalength(Low)
	temp[6] = attf_nextBits(8);	//data1
	temp[7] = attf_nextBits(8);	//data2
	temp[8] = attf_nextBits(8);	//data3
	temp[9] = attf_nextBits(8);	//data4
	temp[10] = attf_nextBits(8);//data5
	temp[11] = attf_nextBits(8);//data6
	temp[12] = attf_nextBits(8);//data5
	temp[13] = attf_nextBits(8);//data6

	//temp[0] = 0xFF;// attf_nextBits(8);	//0xFF
	//temp[1] = 0xF1;// attf_nextBits(8);	//0xFX
	//temp[2] = 0x25;// attf_nextBits(8);	//
	//temp[3] = 0x42;// attf_nextBits(8);	//
	//CRC_check = 0x9F43;// attf_nextBits(16);			//16 bits CRC-check bits
	//temp[4] = 0x97;// attf_nextBits(8);	//datalength(Hi)
	//temp[5] = 0x00;// attf_nextBits(8);	//datalength(Low)
	//temp[6] = 0x08;// attf_nextBits(8);	//data1
	//temp[7] = 0x04;// attf_nextBits(8);	//data2
	//temp[8] = 0x00;// attf_nextBits(8);	//data3
	//temp[9] = 0x00;// attf_nextBits(8);	//data4
	//temp[10] = 0x00;// attf_nextBits(8);//data5
	//temp[11] = 0x00;// attf_nextBits(8);//data6
	//temp[12] = 0x00;// attf_nextBits(8);//data7
	//temp[13] = 0x00;// attf_nextBits(8);//data8

	CRC_temp = CRC16(temp, 14);		
	if (CRC_temp == CRC_check)
	{
		//CRC check is right;
		attf_backward(16 * 8 - 8 - 4);	//backward to 0xFFF.
		return 1;
	}
	else{
		printf("CRC Check is worng!\n");
		longjmp(attf_jump_buffer, 0);	//wrong avs_frame, re find the sync words.
		return 0;
	}
}


FILE *fin;
//if buffer has empty space, read data into buffer.
void attf_waitfordata(int bits)
{
	int i, length;
	int tempbuffer[1024 * 4];
	if (avs_inbuffer.emptybitlength > 1024 * 32)		//if have space in buffer, read stream data into inbuffer.
	{
		length = fread(tempbuffer, sizeof(int), bits / 32 + 1, fin);		//如果文件最后的数不满32bit，那么最后的数据会不读，丢掉

		if (length * 32 < bits)
		{
			attf_overflag = 1;	//data is out.
		}
		avs_inbuffer.databitlength += length * 32;		//bit
		avs_inbuffer.emptybitlength -= length * 32;
		for (i = 0; i < length; i++)
		{
			avs_inbuffer.inbuffer[avs_inbuffer.writebitbegin / 32] = tempbuffer[i];
			avs_inbuffer.writebitbegin += 32;			//byte
			avs_inbuffer.writebitbegin %= (ATTF_BUFFER_LENGTH * 32);	//byte
		}
	}
}
//--------------------------  main file -------------------------------------------
void main_1()
{
	ChanInfo inputInfo;
	ChanInfo objinputInfo;
	int sampleRateCore;
	HANDLE_STAvs2Dec phstAvs2Dec = {0};
	HANDLE_STAvs2Dec objphstAvs2Dec[128] = {0};
	int codectype;
	int fill_element_num;
	int useBWE;
	float AllChannelTimeDataFloat[4 * SAMPLES_PER_FRAME*MAX_ALLCHANNEL];
	int outputLen;
	long dataSizeDec = 0;
	char output_filename[100] = "test.wav";

	FILE *f_sound_out;
	char ptrobj[128][100];
	FILE* objInfo[128];
	FILE* objcontain = fopen("object.txt", "rb");

	int i,j=0,cnt;


	cnt = 0;
	while (!feof(objcontain))
	{
		fscanf(objcontain, "%s", ptrobj[cnt]);

		printf("%s\n", ptrobj[cnt]);
		cnt++;
	}

	//FILE *fin = fopen("test.avs", "rb");
	fin = fopen("test.avs", "rb");

	


	attf_init_inbuffer();
	attf_overflag = 0;

	//fseek(fin, 0x1588, SEEK_SET);
	fseek(fin, 0, SEEK_SET);

	while (1)
	{
		if (setjmp(attf_jump_buffer) == 0)
		{
			attf_bytealignment();
			if (attf_nextBits(8) == 0xFF)
			{
				if (attf_nextBits(4) == 0xF)
				{
					attf_backward(8 + 4);	//back to the beginning of the frame
					attf_crccheck();		//CRC check
					aatf_frame();			//CRC check right! analysize the stream

					if (attf_nextBits(8) == 0xFF && attf_nextBits(4) == 0xF)	//next frame's head. the final frame may be lost because of this judgement.
					{
						unsigned int tempbuffer[1500];
						j++;
						printf("\r\r**********************  j = %d  correct avs frame **********************  \r", j);
						attf_backward(8 + 4);
						//print_attfheadinfo();
						//print_avs_3Dinfo();
						if (avs_attfheadinfo.datalength > 1500 * 4 - 8)
						{
							printf("bed datalength is larger than tempbuffer\n");
							exit(1);
						}

						/*initialize struct inputInfo*/
						{
							int tmp;
							inputInfo.nChannels = avs_attfheadinfo.channel_number;
							tmp = avs_attfheadinfo.bitrate_index;
							if(inputInfo.nChannels == 1) i = 0;
							else if(inputInfo.nChannels == 2) i = 1;
							else if(inputInfo.nChannels == 6) i = 2;
							else if(inputInfo.nChannels == 8) i = 3;
							tmp -= i * 16;
							inputInfo.bitRate = brate_mapping[i][tmp] * 1000;
							inputInfo.bitsPerSample = avs_attfheadinfo.resolution;
							inputInfo.sampleRate = avs_attfheadinfo.sampling_frequency;
							sampleRateCore = inputInfo.sampleRate / 2;
						}


						attf_copydata(avs_attfheadinfo.datalength, avs_attfheadinfo.bitoffset, tempbuffer);		//bed stream


						//decoder function is put here!!
						
						{                                                                                       //chenhan
							attf_avs2audiopack_init(tempbuffer, 1, 0, avs_attfheadinfo.bitoffset % 32);

							if (inputInfo.nChannels > 1)
							{
								codectype = avs2audiopack_read(&opb, 1);  //codectype
								if (codectype == 1)
									avs2audiopack_read(&opb, 4);
							}

							useBWE = avs2audiopack_read(&opb, 1);
							fill_element_num = avs2audiopack_read(&opb, 1);

						}

						/*initialize phstAvs2Dec*/
						if (!phstAvs2Dec)
						{
							init_avs2_general_decoder_frame(sampleRateCore, inputInfo, &phstAvs2Dec, useBWE, fill_element_num);
							if ((f_sound_out = Wave_fopen(output_filename)) == NULL)
							{
								fprintf(stderr, "Error opening output wav file %s.\n", output_filename);
								exit(0);
							}
						}
						road = 1;
						general_decoder_frame(phstAvs2Dec, inputInfo, tempbuffer, avs_attfheadinfo.datalength, AllChannelTimeDataFloat, &outputLen);
						write_data(AllChannelTimeDataFloat, outputLen * inputInfo.nChannels, inputInfo.bitsPerSample, f_sound_out);



						for (i = 0; i < avs_3Dinfo.ObjectNum; i++)
						{
							if (avs_3Dinfo.datalength[i] > 1500 * 4 - 8)
							{
								printf("object datalength is larger than tempbuffer\n");
								exit(1);
							}
							attf_copydata(avs_3Dinfo.datalength[i], avs_3Dinfo.bitoffset[i], tempbuffer);		//object stream
							//decoder function is put here!!

							/*initialize struct objinputInfo*/
							{
								objinputInfo.nChannels = avs_3Dinfo.channel_number[i];

								switch (avs_3Dinfo.bitrate_index[i])
								{
									case 0: objinputInfo.bitRate = 16000; break;
									case 1: objinputInfo.bitRate = 32000; break;
									case 2: objinputInfo.bitRate = 44000; break;
									case 3: objinputInfo.bitRate = 56000; break;
									case 4: objinputInfo.bitRate = 64000; break;
									case 5: objinputInfo.bitRate = 72000; break;
									case 6: objinputInfo.bitRate = 80000; break;
									case 7: objinputInfo.bitRate = 96000; break;
									case 8: objinputInfo.bitRate = 128000; break;
									case 9: objinputInfo.bitRate = 144000; break;
									case 10: objinputInfo.bitRate = 164000; break;
									case 11: objinputInfo.bitRate = 192000; break;

									case 16: objinputInfo.bitRate = 24000; break;
									case 17: objinputInfo.bitRate = 32000; break;
									case 18: objinputInfo.bitRate = 48000; break;
									case 19: objinputInfo.bitRate = 64000; break;
									case 20: objinputInfo.bitRate = 80000; break;
									case 21: objinputInfo.bitRate = 96000; break;
									case 22: objinputInfo.bitRate = 128000; break;
									case 23: objinputInfo.bitRate = 144000; break;
									case 24: objinputInfo.bitRate = 196000; break;
									case 25: objinputInfo.bitRate = 256000; break;
									case 26: objinputInfo.bitRate = 320000; break;

									case 32: objinputInfo.bitRate = 128000; break;
									case 33: objinputInfo.bitRate = 192000; break;
									case 34: objinputInfo.bitRate = 256000; break;
									case 35: objinputInfo.bitRate = 320000; break;
									case 36: objinputInfo.bitRate = 384000; break;
									case 37: objinputInfo.bitRate = 448000; break;
									case 38: objinputInfo.bitRate = 512000; break;
									case 39: objinputInfo.bitRate = 640000; break;
									case 40: objinputInfo.bitRate = 720000; break;
								}
								objinputInfo.bitsPerSample = avs_3Dinfo.resolution[i];
								objinputInfo.sampleRate = inputInfo.sampleRate;
							}

							road = 2;
							index_obj = avs_3Dinfo.object_ID[i];

							if (!objphstAvs2Dec[i])
							{
									objInfo[i] = Wave_fopen(ptrobj[i]);
									objinit_avs2_general_decoder_frame(sampleRateCore, objinputInfo, &objphstAvs2Dec[i], useBWE, fill_element_num);
							}
							objgeneral_decoder_frame(objphstAvs2Dec[avs_3Dinfo.object_ID[i]], objinputInfo, tempbuffer, avs_3Dinfo.datalength[i], AllChannelTimeDataFloat, &outputLen);
							write_data(AllChannelTimeDataFloat, outputLen * objinputInfo.nChannels, objinputInfo.bitsPerSample, objInfo[avs_3Dinfo.object_ID[i]]);


						}
					}
				}
				else
				{
					attf_backward(4);		//not a sync, check next word for sync word again.
					//avs_inbuffer.readbitbegin += 4;
					//avs_inbuffer.readbitbegin %= (ATTF_BUFFER_LENGTH * 32);

					avs_inbuffer.databitlength -= 4;
					avs_inbuffer.emptybitlength += 4;
				}
			}
		}
		if (attf_overflag == 1 && avs_inbuffer.databitlength < 10 * 8)
			break;
	}
	{
		int txt;
		Wave_fclose(f_sound_out, inputInfo.nChannels, inputInfo.sampleRate, inputInfo.bitsPerSample);
		if (avs_attfheadinfo.coding_profile == 1)
		{
			for (txt = 0; txt < cnt; txt++)
				Wave_fclose(objInfo[txt], objinputInfo.nChannels, inputInfo.sampleRate, inputInfo.bitsPerSample);
		}
	}
	fclose(fin);
}

