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
#include "..\general\bwedec\avs2BweDecMDFT.h"
#include "..\general\bwedec\decoder.h"
#include "..\general\bwedec\decode_hf.h"
#include "..\general\decode\avs2audio.h"
#include "..\general\decode\lfdec.h"
#include "..\general\decode\pca.h"
#include "..\general\decode\codebook.h"
#include "..\general\decode\maxcorr.h"
#include "..\general\decode\mc_rom.h"
#include "..\general\decode\lfdec.h"
#include "..\general\decode\avs2decmain.h"
#include "general_decoder_frame.h"
#include "attf_head.h"

HuffmanTableStruc	huffmanbook;
codebook huffmanDecodeBook;

#define SAMPLES_PER_FRAME 1024
/* IO-Buffers */
#define INPUT_BUF_SIZE (6144*2/8)                      /*!< Size of Input buffer in bytes*/
unsigned int inBuffer[INPUT_BUF_SIZE/(sizeof(int))];   /*!< Input buffer */

#define MAX_CH_ELE_DEF 6

typedef struct MULTI_CHAN_MODE MC_MODE;

// added by lumin 2014.11.21
static MC_MODE	CoupleChannelTable[8+8+1] = {

	{1,	{ID_SCE,0,0,0,0,0},{0,0,0,0,0,0}},
	{2,	{ID_SCE,ID_SCE,0,0,0,0},{0,1,0,0,0,0}},
	{1,	{ID_CPE_F,0,0,0,0,0},{0,1,0,0,0,0}},
	{1,	{ID_CPE_L,0,0,0,0,0},{0,1,0,0,0,0}},//CPE_L
	{6,	{ID_SCE,ID_SCE,ID_SCE,ID_SCE,ID_SCE,ID_SCE},{0,1,2,3,4,5}},
	{5,	{ID_SCE,ID_SCE,ID_SCE,ID_SCE,ID_CPE_F,0},{0,1,2,3,4,5}},
	{4,	{ID_CPE_F,ID_SCE,ID_SCE,ID_CPE_F,0,0},{0,1,2,3,4,5}},
	{3,	{ID_CPE_F,ID_CPE_F,ID_CPE_F,0,0,0},{0,1,2,3,4,5}},
	{3, {ID_CPE_F,ID_CPE_F,ID_CPE_F,0,0,0},{0,4,2,3,1,5}},
	{4, {ID_CPE_F,ID_CPE_F,ID_CPE_F,ID_CPE_F,0,0}, {0,1,2,3,4,5,6,7}},

	{ 1, { ID_PCA4, 0, 0, 0, 0, 0 }, { 0, 1, 2, 3, 0, 0 } },													//10
	{ 2, { ID_CPE_F, ID_CPE_F, 0, 0, 0, 0 }, { 0, 1, 2, 3, 0, 0 } },											//11
	/*additional head channel configuration*/                                                                   //chenhan 20180328
	{ 4, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, 0, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7 } },						 	//12          5.1.2
	{ 5, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, ID_CPE_H, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } },				//13          5.1.4
	{ 5, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } },				//14          7.1.2
	{ 6, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, ID_CPE_H }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } }, //15          7.1.4
	{ 8, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 } } //16          16ch 3rd HOA, shumin.xu 210421
};

float TimeDataFloat[4*SAMPLES_PER_FRAME];              /*!< Output buffer */

//float AllChannelTimeDataFloat[4*SAMPLES_PER_FRAME*MAX_ALLCHANNEL];  

//extern Maxpcmvalue; 
//////////////////////////////
codec_setup_info ci_table[13];

extern const double rate_mapping_44_multi[5];

 MC_MODE *encoder_mode;
ELEMENTENCODE_INFO elementencode_info[8];

static MC_MODE PCAGroupmodeHeaderTable[36]={
/* mono */
	{1,	{ID_SCE, 0,0,0,0}},//0
	/* stereo */
	{1,	{ID_PCA2, 0,0,0,0}},//1

	/* 5.1*/
	{2,	{ID_PCA4,ID_PCA2, 0}}, //0x02
	/* 5.1*/
	{3,	{ID_PCA2,ID_PCA2,ID_PCA2}}, //0x03
	/* 5.1*/
	{3,	{ID_PCA2,ID_PCA2,ID_PCA2}}, //0x04
	/* 5.1*/
	{2,	{ID_PCA5,ID_LFE, 0}}, //0x05
	/* 5.1*/
	{3,	{ID_PCA4,ID_SCE_C, ID_LFE}}, //0x06
	/* 5.1*/
	{1,	{ID_PCA6, 0}}, //0x07
	/* reserved */
	{0, {0,0,0,0,0}},//0x08
	/* reserved */
	{0, {0,0,0,0,0}},//0x09
	/* reserved */
	{0, {0,0,0,0,0}},//0x0a
	/* reserved */
	{0, {0,0,0,0,0}},//0x0b
	/* reserved */
	{0, {0,0,0,0,0}},//0x0c
	/* reserved */
	{0, {0,0,0,0,0}},//0x0d
	/* reserved */
	{0, {0,0,0,0,0}},//0x0e
	/* reserved */
	{0, {0,0,0,0,0}},//0x0f
	/* 7.1 */
	{3,	{ID_PCA4,ID_PCA2,ID_PCA2, 0}} //0x10
};

static MC_MODE *encoder_mode_tianlai51 =&PCAGroupmodeHeaderTable[2];
ELEMENTENCODE_INFO elementencode_info_tianlai51[8];
struct AVS2_DECODER_INSTANCE Avs2DecoderInstance_frame[MAX_ALLCHANNEL];

extern const int lf_winseq_table[40][20];

 /////////
#ifdef __unix__
	#define	bname(s)	(strrchr(s, '/')? strrchr(s, '/') + 1 : s)
	#define	C35		"\E[35m"
	#define C0		"\E[0m"
#else
	#define	bname(s)	s                 
	#define	C35		""
	#define	C0		""
#endif

#define CORE_DELAY (4801-FRAMESIZE)
#define MCR_DELAY  (2048)
#define CORE_FRAMESIZE (1024)
//#define MAX_CH (6)

tianlai_block  vb[MAX_ALLCHANNEL];
void init_avs2_general_decoder_frame(int sampleRateCore, ChanInfo inputInfo, HANDLE_STAvs2Dec *phstAvs2Dec,int useBWE,int fill_element_num)
{
	HANDLE_STAvs2Dec stAvs2Dec=NULL;
	
	int	nchannels, headchannels = 0;
	int core_bitrate,bitrate;
	int usePS = 1;
	int i;
	int nChannelsCore, nChannelsBWE;

	int ErrorStatus = 0;

//	int elementnum;
//	int elementnum_tianlai51;
//	int	chIndex[MAX_ALLCHANNEL] = {0};
    int channelindex =0, cid;
	int PCAGroupmodeHeader=0xFF;
	int huffmanbooklist[3][121];

    *phstAvs2Dec = NULL;
	stAvs2Dec = calloc( 1, sizeof(struct STAvs2Dec));

	nchannels = inputInfo.nChannels;
	headchannels = inputInfo.headChannels;
	bitrate = inputInfo.bitRate;

	if(nchannels == 1)
	{
		usePS = 0;
		//if(bitrate < 40000)
		//	useBWE = 1;
	}
	else if(nchannels == 2)
	{
		//this version do not support BWE now
		if(bitrate >= 128000)
		{
			usePS = 2;
//			useBWE = 0;
		}
		else if(bitrate > 48000)
		{
			usePS = 1;
//			useBWE = 0;
		}
		else
		{
			usePS = 1;
//			useBWE = 1;
		}
			
		bitrate_init(&stAvs2Dec->srateInfo, bitrate/1000, nchannels, 0);
		mcr_init(&stAvs2Dec->mcrInfo, stAvs2Dec->srateInfo.mcr_brate[0]);

	}
	else if(nchannels >= 4)
	{
		usePS = 1;
//		useBWE = 0;
			
		bitrate_init(&stAvs2Dec->srateInfo, bitrate/1000, nchannels, headchannels);
		mcr_init(&stAvs2Dec->mcrInfo, stAvs2Dec->srateInfo.mcr_brate[0]);
	}
	/*else if (nchannels == 8)
	{
		usePS = 1;
		useBWE = 0;

		bitrate_init(&stAvs2Dec->srateInfo, bitrate / 1000, nchannels);
		mcr_init(&stAvs2Dec->mcrInfo, stAvs2Dec->srateInfo.mcr_brate[0]);
	}*/
	//Maxpcmvalue = 1<<(inputInfo.bitsPerSample-1);

	if(inputInfo.nChannels >=5)
	{
		for(i =0; i< 5; i++)
			if(inputInfo.bitRate >= rate_mapping_44_multi[i])
				stAvs2Dec->bitRateIndex = i;
	}

	nChannelsCore = nChannelsBWE =1;

	///setting encoder_mode
	encoder_mode =  &CoupleChannelTable[0];
	// added by lumin 2014.11.21
	if(inputInfo.nChannels>=2)
	{
		encoder_mode = &CoupleChannelTable[stAvs2Dec->srateInfo.couple_config];
	}
	memcpy(stAvs2Dec->chIndex, encoder_mode->ele_id, MAX_ALLCHANNEL*sizeof(int));

	stAvs2Dec->elementnum =  encoder_mode->numofele;
	cid = 0;
	for(i = 0; i < stAvs2Dec->elementnum; i++)
	{
		stAvs2Dec->psData[i] = (PS_DATA *)malloc(sizeof(PS_DATA));
		memset(stAvs2Dec->psData[i], 0, sizeof(PS_DATA));
	  stAvs2Dec->psData[i]->bandwidth = CORE_FRAMESIZE;
		stAvs2Dec->psData_pre[i] = (PS_DATA *)malloc(sizeof(PS_DATA));
		memset(stAvs2Dec->psData_pre[i], 0, sizeof(PS_DATA));
		stAvs2Dec->psData_pre[i]->bandwidth = CORE_FRAMESIZE;
		
		stAvs2Dec->psBweData[i] = (PS_BWE_DATA *)malloc(sizeof(PS_BWE_DATA));
		memset(stAvs2Dec->psBweData[i], 0, sizeof(PS_BWE_DATA));
		stAvs2Dec->psBweData[i]->bandwidth = CORE_FRAMESIZE;
		stAvs2Dec->psBweData_pre[i] = (PS_BWE_DATA *)malloc(sizeof(PS_BWE_DATA));
		memset(stAvs2Dec->psBweData_pre[i], 0, sizeof(PS_BWE_DATA));
		stAvs2Dec->psBweData_pre[i]->bandwidth = CORE_FRAMESIZE;		
	}
	for( i=0;i<stAvs2Dec->elementnum;i++)
	{
		  elementencode_info[i].elType = encoder_mode->idType[i];

		  switch (elementencode_info[i].elType) {

			case ID_SCE:      /* single channel */
				elementencode_info[i].nChannelsInEl=1;
				elementencode_info[i].ChannelIndex[0] = cid++;
				
			break;
			case ID_CPE_F:      /* channel pair */
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid++;
				elementencode_info[i].ChannelIndex[1] = cid++;
				
			break;
			case ID_CPE_L:      /* channel pair */
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid++;
				elementencode_info[i].ChannelIndex[1] = cid++;
				
			break;			
			
			case ID_LFE:     /*LFE channel */
				elementencode_info[i].nChannelsInEl=1;
				elementencode_info[i].ChannelIndex[0] = cid++;
			break;

			case ID_CPE_H:     //chenhan 20180402
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid++;
				elementencode_info[i].ChannelIndex[1] = cid++;
				break;

			case ID_PCA2:     /*PCA2 channel */
				elementencode_info[i].nChannelsInEl=2;
				elementencode_info[i].ChannelIndex[0] = cid++;
				elementencode_info[i].ChannelIndex[1] = cid++;
			break;

			case ID_PCA4:     /*PCA4 channel */
				elementencode_info[i].nChannelsInEl=4;
				elementencode_info[i].ChannelIndex[0] = cid++;
				elementencode_info[i].ChannelIndex[1] = cid++;
				elementencode_info[i].ChannelIndex[2] = cid++;
				elementencode_info[i].ChannelIndex[3] = cid++;
			break;

			case ID_PCA6:     /*PCA6 channel */
				elementencode_info[i].nChannelsInEl=6;
				elementencode_info[i].ChannelIndex[0] = cid++;
				elementencode_info[i].ChannelIndex[1] = cid++;
				elementencode_info[i].ChannelIndex[2] = cid++;
				elementencode_info[i].ChannelIndex[3] = cid++;
				elementencode_info[i].ChannelIndex[4] = cid++;
				elementencode_info[i].ChannelIndex[5] = cid++;
			break;
		  }

	}
	for (i = stAvs2Dec->elementnum; i < stAvs2Dec->elementnum + fill_element_num; i++)
	{
		elementencode_info[i].elType = ID_FIL;
		elementencode_info[i].nChannelsInEl = 1;
	}

//	encoder_mode_tianlai51 =&MC51ModeTianlai[inputInfo.nChannels-1];
	if (inputInfo.nChannels <= 2)
		encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[inputInfo.nChannels - 1];
	else if (inputInfo.nChannels == 6)
		encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[2];
	else if (inputInfo.nChannels == 8)
		encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[16];
	else if (inputInfo.nChannels == 4)
		encoder_mode_tianlai51 = NULL;
	else if (inputInfo.nChannels <= 16)
		encoder_mode_tianlai51 = NULL;
	else
	{
		printf("not support %d channel", inputInfo.nChannels);
		return -1;
	}

	if (encoder_mode_tianlai51 != NULL)
	{
		stAvs2Dec->elementnum_tianlai51 = encoder_mode_tianlai51->numofele;
		cid = 0;
		for (i = 0; i < stAvs2Dec->elementnum_tianlai51; i++)
		{
			elementencode_info_tianlai51[i].elType = encoder_mode_tianlai51->idType[i];

			switch (elementencode_info_tianlai51[i].elType) {

			case ID_SCE:      /* single channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 1;
				elementencode_info_tianlai51[i].ChannelIndex[0] = cid++;

				break;
			case ID_CPE_F:      /* channel pair */
				elementencode_info_tianlai51[i].nChannelsInEl = 2;
				elementencode_info_tianlai51[i].ChannelIndex[0] = cid++;
				elementencode_info_tianlai51[i].ChannelIndex[1] = cid++;

				break;


			case ID_LFE:     /*LFE channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 1;
				elementencode_info_tianlai51[i].ChannelIndex[0] = cid++;
				break;

			case ID_PCA2:     /*PCA2 channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 2;
				elementencode_info_tianlai51[i].ChannelIndex[0] = cid++;
				elementencode_info_tianlai51[i].ChannelIndex[1] = cid++;
				break;

			case ID_PCA4:     /*PCA4 channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 4;
				elementencode_info_tianlai51[i].ChannelIndex[0] = cid++;
				elementencode_info_tianlai51[i].ChannelIndex[1] = cid++;
				elementencode_info_tianlai51[i].ChannelIndex[2] = cid++;
				elementencode_info_tianlai51[i].ChannelIndex[3] = cid++;
				break;

			case ID_PCA6:     /*PCA6 channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 6;
				elementencode_info_tianlai51[i].ChannelIndex[0] = cid++;
				elementencode_info_tianlai51[i].ChannelIndex[1] = cid++;
				elementencode_info_tianlai51[i].ChannelIndex[2] = cid++;
				elementencode_info_tianlai51[i].ChannelIndex[3] = cid++;
				elementencode_info_tianlai51[i].ChannelIndex[4] = cid++;
				elementencode_info_tianlai51[i].ChannelIndex[5] = cid++;
				break;
			}

		}
		for (i = stAvs2Dec->elementnum_tianlai51; i < stAvs2Dec->elementnum_tianlai51 + fill_element_num; i++)
		{
			elementencode_info_tianlai51[i].elType = ID_FIL;
			elementencode_info[i].nChannelsInEl = 1;
		}
	}
//	useParametricStereo = 0;


	//if( ((nChannelsCore == 1) && (inputInfo.bitRate <= 32000)) || ((nChannelsCore == 2) && (inputInfo.bitRate <= 48000/*32000*/)) || ((inputInfo.nChannels >= 5) && (inputInfo.bitRate <=128000)))
	//{
	//	useBWE = 1;
	//}

	//if((PCAGroupmodeHeader!=0xFF)&&(inputInfo.nChannels>2)&&((inputInfo.bitRate <= 192000)))
	//	useBWE = 1;
	
	stAvs2Dec->config = inputInfo;
	
	// initialize time data buffer 

	memset(TimeDataFloat, 0, SAMPLES_PER_FRAME * 4 * sizeof(float));
//	memset(TimeDataOut, 0, SAMPLES_PER_FRAME * 2 * sizeof(float));


	//initialize ci_table
	ci_settable_init(ci_table);

    inithuffmantable(&huffmanbook, huffmanbooklist, huffmantablescl, 1, 60, 1);

	tianlai_huffmantable_init_decode(&huffmanDecodeBook, huffmanbooklist[2], huffmanbooklist[1], 121, 1);

	Avs2DecMDFTfunOpen();

	//setting avs2DecoderInfo[],initialize every avs2Decoder
	for(i = 0; i < inputInfo.nChannels; i++)
	{
		if(nchannels==1)
			core_bitrate = bitrate;
		else 
			core_bitrate = stAvs2Dec->srateInfo.core_brate[i]*1000;

		if(core_bitrate == 0) 
		{
			if (useBWE)
				stAvs2Dec->config.bitRate = stAvs2Dec->srateInfo.core_brate[i-1]*1000;
			else
				stAvs2Dec->config.bitRate = 48000;
		}
		else
			stAvs2Dec->config.bitRate = core_bitrate;

		stAvs2Dec->avs2DecoderInfo = CAvs2DecoderOpen_frame(&stAvs2Dec->streamBWE[i], TimeDataFloat, i);

		Avs2BweDecMDFTOpen((unsigned int*)&(stAvs2Dec->avs2DecoderInfo->st1_decin), (unsigned int*)&(stAvs2Dec->avs2DecoderInfo->st_deccommon));

		if(useBWE)
		{
			Avs2BweDecoderOpen((unsigned int*)&(stAvs2Dec->avs2DecoderInfo->st2_decin), stAvs2Dec->config.bitRate, sampleRateCore, nChannelsCore, &stAvs2Dec->bandWidth[i], &stAvs2Dec->config_idx[i]);
		}		

		ErrorStatus = CAvs2DecoderInit(stAvs2Dec->avs2DecoderInfo, sampleRateCore, stAvs2Dec->config.bitRate, useBWE, &stAvs2Dec->bandWidth[i]);

		_make_decode_ready(&(stAvs2Dec->avs2DecoderInfo->vf), sampleRateCore, stAvs2Dec->config.bitRate);
	}

	stAvs2Dec->useBWE = useBWE;
	stAvs2Dec->usePS = usePS;

	*phstAvs2Dec = stAvs2Dec;


	return;
}

int general_decoder_frame(/*int argc, char *argv[]*/HANDLE_STAvs2Dec stAvs2Dec, ChanInfo inputInfo,
	unsigned int readBuf[], unsigned short numBytes, float AllChannelTimeDataFloat[], int *outputLen)
{


	int sampleRateCore;
	int bEncodeMono = 0;


	int i, j, k;
	//     BWEBITSTREAM streamBWE[10];                           /*!< pointer to bwe bitstream buffer */
	int ErrorStatus = 0;
	int frameSize = FRAME_SIZE;


	char channelMode = 0;
	float TimeDataOut[FRAME_LEN_LONG * 2];

	// 	int bandWidth[MAX_ALLCHANNEL] = {0};

	int elementindex;
	int channelindex = 0, cid;
	//	int elementnum;
	int elementnum_tianlai51;
	float MdctSpectrum[MAX_ALLCHANNEL][FRAME_LEN_LONG * 4 + 2048];
	int outlen;
	//	int bitRateIndex;

	//	int core_bitrate,bitrate;
	int useSuperMode = 0;
	int	cpe_config = 0;
	//	int	chIndex[MAX_ALLCHANNEL] = {0};
	int fill_element_num = 0;
	//    int huffmanbooklist[3][121];
	char codectype = 0;
	//	unsigned short numBytes = 0;

	//	int			nchannels;

	unsigned int PCAGroupmode = 0;



	tianlai_block *vb = &(Avs2DecoderInstance_frame[0].vf.vb);

	memset(&vb->opb, 0, sizeof(avs2audiopack_buffer));
	vb->opb.buffer = vb->opb.ptr = readBuf;
	vb->opb.storage = numBytes;
	vb->opb.endbit = avs_attfheadinfo.bitoffset % 32;
	vb->opb.endbyte = 0;


	if (inputInfo.nChannels > 1)
	{
		codectype = avs2audiopack_read(&vb->opb, 1);
		if (codectype == 1)
		{
			avs2audiopack_read(&vb->opb, 4);
		}
	}
	stAvs2Dec->useBWE = avs2audiopack_read(&vb->opb, 1);
	fill_element_num = avs2audiopack_read(&vb->opb, 1);


	// initialize time data buffer 
	memset(TimeDataOut, 0, SAMPLES_PER_FRAME * 2 * sizeof(float));

	sampleRateCore = inputInfo.sampleRate / 2;

	Avs2DecoderInstance_frame[0].vf.vb.Maxpcmvalue = 1 << (inputInfo.bitsPerSample - 1);

	if (stAvs2Dec->useBWE)
		outlen = frameSize * 2;
	else
		outlen = frameSize;

	{
		int readcodectypeflag = 0;

		/*		if(inputInfo.nChannels>2)
				{
					int i;

					codectype = readBuf[0];

					PCAGroupmode = codectype & 0x1F;
					PCAGroupmode = PCAGroupmode >> 1;
					codectype = codectype & 0x01;
				}
				else if(inputInfo.nChannels==2)
				{
					codectype = 0;
				}*/
		if (inputInfo.nChannels == 1)
		{
			codectype = 1;
		}


		if (codectype == 0)
		{
			int iid;
			int cid = 0;
			//int stoploop = 0;
			int mcr_buff[MAX_ALLCHANNEL][200] = { 0 };
			int lf_winseq[20] = { 1,4,4 };
			int rnum;

			readcodectypeflag = 0;

			for (k = 0; k < stAvs2Dec->elementnum + fill_element_num; k++)
			{
				int rnum, num_mcr;
				int super_flag = 0;
				int type;
				int coreNum;

				/*float freq_mono[FRAMESIZE * 4] = { 0 };
				float freq_left[FRAMESIZE * 4] = { 0 };
				float freq_right[FRAMESIZE * 4] = { 0 };*/

				type = avs2audiopack_read(&vb->opb, 4);
				Avs2DecoderInstance_frame[cid].type = type;
				if (Avs2DecoderInstance_frame[cid].type == ID_CPE_F
					|| Avs2DecoderInstance_frame[cid].type == ID_CPE_S
					|| Avs2DecoderInstance_frame[cid].type == ID_CPE_L
					|| Avs2DecoderInstance_frame[cid].type == ID_CPE_H)
				{
					coreNum = avs2audiopack_read(&vb->opb, 1);

					for (iid = cid; iid < cid + elementencode_info[k].nChannelsInEl; iid++)
					{
#if 0
						// read one frame of encoded audio to file 
						if (iid - cid > coreNum)
						{
							memset(MdctSpectrum[iid], 0, FRAME_SIZE * 2 * sizeof(float));
							break;
						}

						stAvs2Dec->streamBWE[iid].NrElements = 0;

						memcpy(lf_winseq, lf_winseq_table[Avs2DecoderInstance_frame[iid].lf_winseq_index], 20 * sizeof(int));
#else
						// read one frame of encoded audio to file 
						if (stAvs2Dec->srateInfo.core_brate[iid] == 0)
						{
							memset(MdctSpectrum[iid], 0, (FRAME_SIZE * 4 + 2048) * sizeof(float));
							break;
						}

						stAvs2Dec->streamBWE[iid].NrElements = 0;
						//memcpy(lf_winseq, lf_winseq_table[Avs2DecoderInstance_frame[iid].lf_winseq_index], 20 * sizeof(int));

						stAvs2Dec->config.bitRate = stAvs2Dec->srateInfo.core_brate[cid] * 1000;

#endif
						// decode lf frame using a new lf decoder 
						Avs2DecoderInstance_frame[iid].type = type;
						ErrorStatus = Avs2LFDecoder(stAvs2Dec->useBWE, &stAvs2Dec->bandWidth[iid], stAvs2Dec->bitRateIndex, stAvs2Dec->config.bitRate, inputInfo.bitsPerSample,
							&Avs2DecoderInstance_frame[iid], readBuf, numBytes,
							&sampleRateCore, 1, MdctSpectrum[iid],
							readcodectypeflag,
							vb, stAvs2Dec->config_idx[iid]);

						readcodectypeflag++;

		}

					//if (stoploop == 1) break;

					// judging next frame contains new mcr bits
					if (Avs2DecoderInstance_frame[cid].type == ID_CPE_F
						|| Avs2DecoderInstance_frame[cid].type == ID_CPE_S
						|| Avs2DecoderInstance_frame[cid].type == ID_CPE_L
						|| Avs2DecoderInstance_frame[cid].type == ID_CPE_H)
					{
						if (stAvs2Dec->useBWE)
						{
							StAvs2BweDecode *pstBweData;
							StAvs2BweDecMDFT *pstBweMDFT;
							StAvs2BweDecCommon *pstBweCommon;

							pstBweData = (StAvs2BweDecode *)(Avs2DecoderInstance_frame[cid].st2_decin);
							pstBweMDFT = (StAvs2BweDecMDFT *)(Avs2DecoderInstance_frame[cid].st1_decin);
							pstBweCommon = (StAvs2BweDecCommon *)(Avs2DecoderInstance_frame[cid].st_deccommon);
							stAvs2Dec->psBweData[k]->Seqmode = pstBweData->savedHfParam[0].seqMode;
							stAvs2Dec->psBweData[k]->Groupmode = pstBweData->savedHfParam[0].groupMode;
							stAvs2Dec->psBweData[k]->bandwidth = CORE_FRAMESIZE;

							stAvs2Dec->psBweData_pre[k]->Seqmode = pstBweMDFT->Seqmode;
							stAvs2Dec->psBweData_pre[k]->Groupmode = pstBweMDFT->Groupmode;
							pstBweMDFT = (StAvs2BweDecCommon *)(Avs2DecoderInstance_frame[cid + 1].st1_decin);
							pstBweMDFT->Seqmode = stAvs2Dec->psBweData_pre[k]->Seqmode;
							pstBweMDFT->Groupmode = stAvs2Dec->psBweData_pre[k]->Groupmode;
							Avs2DecoderInstance_frame[cid + 1].lf_winseq_index = Avs2DecoderInstance_frame[cid].lf_winseq_index;

							memcpy(stAvs2Dec->psBweData_pre[k]->sum_data, MdctSpectrum[cid], (CORE_FRAMESIZE * 4 + 2048) * sizeof(float));
							if (coreNum > 0)
								memcpy(stAvs2Dec->psBweData_pre[k]->dif_data, MdctSpectrum[cid + 1], (CORE_FRAMESIZE * 4 + 2048) * sizeof(float));


							if (Avs2DecoderInstance_frame[cid].type == ID_CPE_S)
							{
								super_flag = 1;
							}
							else
							{
								super_flag = 0;
							}
							num_mcr = MCR_BWE_Decoder(inputInfo.bitRate, stAvs2Dec->psBweData[k], stAvs2Dec->mcrInfo, stAvs2Dec->psBweData_pre[k], super_flag, &(vb->opb));

							memcpy(MdctSpectrum[cid], stAvs2Dec->psBweData_pre[k]->left_data, (CORE_FRAMESIZE * 4 + 2048) * sizeof(float));
							memcpy(MdctSpectrum[cid + 1], stAvs2Dec->psBweData_pre[k]->right_data, (CORE_FRAMESIZE * 4 + 2048) * sizeof(float));

						}
						else
						{
							stAvs2Dec->psData[k]->bandwidth = stAvs2Dec->bandWidth[cid];
							memcpy(stAvs2Dec->psData[k]->winseq, lf_winseq_table[Avs2DecoderInstance_frame[cid].lf_winseq_index], 20 * sizeof(int));
							memcpy(stAvs2Dec->psData[k]->sum_data, MdctSpectrum[cid], CORE_FRAMESIZE * sizeof(float));
							memcpy(stAvs2Dec->psData[k]->dif_data, MdctSpectrum[cid + 1], CORE_FRAMESIZE * sizeof(float));

							Avs2DecoderInstance_frame[cid + 1].lf_winseq_index = Avs2DecoderInstance_frame[cid].lf_winseq_index;

							if (Avs2DecoderInstance_frame[cid].type == ID_CPE_S)
							{
								super_flag = 1;
							}
							else
							{
								super_flag = 0;
							}
							num_mcr = MCR_Decoder(stAvs2Dec->psData[k], stAvs2Dec->mcrInfo, stAvs2Dec->psData_pre[k], super_flag, &(vb->opb));

							memcpy(MdctSpectrum[cid], stAvs2Dec->psData[k]->left_data, CORE_FRAMESIZE * sizeof(float));
							memcpy(MdctSpectrum[cid + 1], stAvs2Dec->psData[k]->right_data, CORE_FRAMESIZE * sizeof(float));
						}

					}
					cid += elementencode_info[k].nChannelsInEl;
				}
				else if (type == ID_FIL)
				{
					int fill_type = 0;
					int fill_byte = 0;
					fill_type = avs2audiopack_read(&vb->opb, 4);


					switch (fill_type)
					{
					case 0:
					{
						fill_byte = avs2audiopack_read(&vb->opb, 16);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_read(&vb->opb, 1);
						}
					}
						break;
					case 1:
					{
						fill_byte = avs2audiopack_read(&vb->opb, 15);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_read(&vb->opb, 2);
						}
					}
						break;
					case 2:
					{
						fill_byte = avs2audiopack_read(&vb->opb, 14);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_read(&vb->opb, 4);
						}
					}
						break;
					case 3:
					{
						fill_byte = avs2audiopack_read(&vb->opb, 13);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_read(&vb->opb, 8);
						}
					}
					break;
					default:
					{
						printf("error fill type !\n");
					}
					break;
					}

				}
			}

			cid=0;

			for (k = 0; k < stAvs2Dec->elementnum; k++)
			{
				int rnum, num_mcr;
				int super_flag = 0;

				/*float freq_mono[FRAMESIZE * 4] = { 0 };
				float freq_left[FRAMESIZE * 4] = { 0 };
				float freq_right[FRAMESIZE * 4] = { 0 };*/


				for (iid = cid; iid < cid + elementencode_info[k].nChannelsInEl; iid++)
				{
					stAvs2Dec->config.bitRate = stAvs2Dec->srateInfo.core_brate[cid] * 1000;
					Avs2Decoder_syn(&Avs2DecoderInstance_frame[iid], stAvs2Dec->useBWE, MdctSpectrum[iid], TimeDataOut, stAvs2Dec->config.bitRate, inputInfo.bitsPerSample);

					for (j = 0; j < outlen; j++)
					{
						AllChannelTimeDataFloat[inputInfo.nChannels*j + stAvs2Dec->chIndex[iid]] = TimeDataOut[j];
					}
				}
				cid += elementencode_info[k].nChannelsInEl;
			}

		}

		else
		{
			//tianlai_block *vb;
			channelindex = 0;

			//vb = &(Avs2DecoderInstance_frame[0].vf.vb);
			//memset(&vb->opb,0,sizeof(avs2audiopack_buffer));
			//vb->opb.buffer = vb->opb.ptr = readBuf;
			//vb->opb.storage = numBytes;

			//重新配置PCA组合 //PCAGroupmode
			if (inputInfo.nChannels == 6) {
				encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[0x02 + PCAGroupmode];
				elementnum_tianlai51 = encoder_mode_tianlai51->numofele;
				channelindex = 0;
				for (i = 0; i < elementnum_tianlai51; i++)
				{
					elementencode_info_tianlai51[i].elType = encoder_mode_tianlai51->idType[i];

					switch (elementencode_info_tianlai51[i].elType) {

					case ID_SCE:      /* single channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 1;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;

						break;
					case ID_CPE_F:      /* channel pair */
						elementencode_info_tianlai51[i].nChannelsInEl = 2;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[1] = channelindex++;

						break;


					case ID_LFE:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 1;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;
						break;

					case ID_PCA2:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 2;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[1] = channelindex++;
						break;

					case ID_PCA4:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 4;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[1] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[2] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[3] = channelindex++;
						break;

					case ID_PCA6:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 6;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[1] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[2] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[3] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[4] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[5] = channelindex++;
						break;
					}

				}
				for (i = elementnum_tianlai51; i < elementnum_tianlai51 + fill_element_num; i++)
				{
					elementencode_info_tianlai51[i].elType = ID_FIL;
				}

			}


			if (inputInfo.nChannels == 2) {
				encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[0x01];
				elementnum_tianlai51 = encoder_mode_tianlai51->numofele;
				channelindex = 0;
				for (i = 0; i < elementnum_tianlai51; i++)
				{
					elementencode_info_tianlai51[i].elType = encoder_mode_tianlai51->idType[i];

					switch (elementencode_info_tianlai51[i].elType) {

					case ID_SCE:      /* single channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 1;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;

						break;
					case ID_CPE_F:      /* channel pair */
						elementencode_info_tianlai51[i].nChannelsInEl = 2;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[1] = channelindex++;

						break;


					case ID_LFE:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 1;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;
						break;

					case ID_PCA2:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 2;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[1] = channelindex++;
						break;

					case ID_PCA4:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 4;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[1] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[2] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[3] = channelindex++;
						break;

					case ID_PCA6:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 6;
						elementencode_info_tianlai51[i].ChannelIndex[0] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[1] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[2] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[3] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[4] = channelindex++;
						elementencode_info_tianlai51[i].ChannelIndex[5] = channelindex++;
						break;
					}

				}
				for (i = elementnum_tianlai51; i < elementnum_tianlai51 + fill_element_num; i++)
				{
					elementencode_info_tianlai51[i].elType = ID_FIL;
				}
			}

			for (elementindex = 0; elementindex < stAvs2Dec->elementnum_tianlai51 + fill_element_num; elementindex++)
			{
				switch (elementencode_info_tianlai51[elementindex].elType) {

				case ID_SCE:      /* single channel */
				case ID_PCA2:      /* PCAx channel */
				case ID_PCA4:
				case ID_PCA6:
				{int indexinelement;
				int startchannelindex;



				startchannelindex = elementencode_info_tianlai51[elementindex].ChannelIndex[0];
				//2014.11.13 wchg 
				Avs2DecoderInstance_frame[startchannelindex].usePCAitemnum = elementencode_info_tianlai51[elementindex].nChannelsInEl;


				indexinelement = 0;



				{
					int type = avs2audiopack_read(&vb->opb, 4);
				}
				{int usePCAitemnum;
				int lf_winseq_index;
				char ElementInstanceTag;

				if ((elementencode_info_tianlai51[elementindex].nChannelsInEl > 1) && (indexinelement == 0))
					usePCAitemnum = avs2audiopack_read(&vb->opb, 3);
				else
					usePCAitemnum = 1;
				ElementInstanceTag = avs2audiopack_read(&vb->opb, 4);
				lf_winseq_index = avs2audiopack_read(&vb->opb, 6);
				for (indexinelement = 0; indexinelement < elementencode_info_tianlai51[elementindex].nChannelsInEl; indexinelement++)
				{
					channelindex = elementencode_info_tianlai51[elementindex].ChannelIndex[indexinelement];

					Avs2DecoderInstance_frame[channelindex].lf_winseq_index = lf_winseq_index;
					Avs2DecoderInstance_frame[channelindex].usePCAitemnum = usePCAitemnum;
					Avs2DecoderInstance_frame[channelindex].ElementInstanceTag = ElementInstanceTag;

				}
				}

				for (indexinelement = 0; indexinelement < elementencode_info_tianlai51[elementindex].nChannelsInEl; indexinelement++)
				{

					channelindex = elementencode_info_tianlai51[elementindex].ChannelIndex[indexinelement];


					stAvs2Dec->streamBWE[channelindex].NrElements = 0;


					/* decode lf frame using a new lf decoder */
					// decode the first usePCAitemnum principal components,  PCA Matrix
					if (Avs2DecoderInstance_frame[startchannelindex].usePCAitemnum > indexinelement)
						ErrorStatus = Avs2LFDecoder_PCA(stAvs2Dec->useBWE,

							inputInfo.nChannels,
							stAvs2Dec->bitRateIndex,
							stAvs2Dec->config.bitRate,
							inputInfo.bitsPerSample,
							0,
							&Avs2DecoderInstance_frame[channelindex],//avs2DecoderInfo,
							NULL,
							readBuf,
							numBytes,
							&(Avs2DecoderInstance_frame[0].vf.vb),//&(Avs2DecoderInstance_frame[channelindex].vf.vb),
							&(Avs2DecoderInstance_frame[channelindex].vf.TnsData),
							&sampleRateCore,
							1,
							MdctSpectrum[channelindex],
							indexinelement,
							elementindex,
							elementencode_info_tianlai51[elementindex].nChannelsInEl,
							stAvs2Dec->config_idx[channelindex]);
					else
					{
						memset(MdctSpectrum[channelindex], 0, 1024 * 4);

						Avs2DecoderInstance_frame[channelindex].lf_winseq_index = Avs2DecoderInstance_frame[startchannelindex].lf_winseq_index;
					}

					/*{
						FILE *fp = fopen("bitnum.txt","a");
						fprintf(fp, "[core][%d]\n", vb->opb.endbyte);
						fclose(fp);
					}*/


				}	//for(indexinelement=0;indexinelement<elementencode_info[elementindex].nChannelsInEl;indexinelement++)


				//avs2audiopack_read(&vb->opb, (8 - vb->opb.endbit));


				if (elementencode_info_tianlai51[elementindex].nChannelsInEl > 1) {
					int lf_winseq_dec[20];


					memcpy(lf_winseq_dec, lf_winseq_table[Avs2DecoderInstance_frame[startchannelindex].lf_winseq_index], 20 * 4);
					//PCA synthesis, using the first usePCAitemnum principal components and PCA Matrix
					multichannelMDCT_PCA_syn(&MdctSpectrum[startchannelindex], lf_winseq_dec, elementencode_info_tianlai51[elementindex].nChannelsInEl, &MdctSpectrum[startchannelindex], elementindex, Avs2DecoderInstance_frame[startchannelindex].usePCAitemnum);


				}

				}
				break;
				case ID_FIL:
				{
					int fill_type = 0;
					int fill_byte = 0;
					unsigned int codectype;
					int type;

					type = avs2audiopack_read(&vb->opb, 4);


					fill_type = avs2audiopack_read(&vb->opb, 4);

					switch (fill_type)
					{
					case 0:
					{
						fill_byte = avs2audiopack_read(&vb->opb, 16);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_read(&vb->opb, 1);
						}
					}
					break;
					case 1:
					{
						fill_byte = avs2audiopack_read(&vb->opb, 15);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_read(&vb->opb, 2);
						}
					}
					break;
					case 2:
					{
						fill_byte = avs2audiopack_read(&vb->opb, 14);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_read(&vb->opb, 4);
						}
					}
					break;
					case 3:
					{
						fill_byte = avs2audiopack_read(&vb->opb, 13);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_read(&vb->opb, 8);
						}
					}
					break;
					}
				}
				break;
				}//switch (elementencode_info[elementindex].elType)
				if (stAvs2Dec->useBWE && elementencode_info[elementindex].elType != ID_FIL)
				{
					//for( channelindex=0;channelindex<inputInfo.nChannels;channelindex++)
					//{tianlai_block *vb =&Avs2DecoderInstance_frame[0].vf.vb;
					//	int count;

					//	if(channelindex==3)
					//		continue;
					//

					//	stAvs2Dec->streamBWE[channelindex].NrElements = 0;

					//	 count = Avs2DecoderInstance_frame[channelindex].pStreamBWE->bweElement[Avs2DecoderInstance_frame[channelindex].pStreamBWE->NrElements].Payload = avs2audiopack_read(&vb->opb, 8);

					//	for (i=0; i<count; i++)
					//	{
					//	Avs2DecoderInstance_frame[channelindex].pStreamBWE->bweElement [Avs2DecoderInstance_frame[channelindex].pStreamBWE->NrElements].Data[i] = (unsigned char) avs2audiopack_read(&vb->opb,8);
					//	}

					//	Avs2DecoderInstance_frame[channelindex].pStreamBWE->NrElements += 1;


					//}
				}
				else
				{
					//for( channelindex=0;channelindex<inputInfo.nChannels;channelindex++)
					//{

					//	stAvs2Dec->streamBWE[channelindex].NrElements = 0;
					//}
				}
			}	//for(indexinelement=0;indexinelement<elementencode_info[elementindex].nChannelsInEl;indexinelement++)





			for (elementindex = 0; elementindex < stAvs2Dec->elementnum_tianlai51; elementindex++)
			{
				switch (elementencode_info_tianlai51[elementindex].elType) {
				case ID_SCE:
				case ID_PCA2:      /* PCAx channel */
				case ID_PCA4:
				case ID_PCA6:
				{
					int indexinelement;
					int startchannelindex;



					startchannelindex = elementencode_info_tianlai51[elementindex].ChannelIndex[0];
					//2014.11.13 wchg 
					Avs2DecoderInstance_frame[startchannelindex].usePCAitemnum = elementencode_info_tianlai51[elementindex].nChannelsInEl;


					for (indexinelement = 0; indexinelement < elementencode_info_tianlai51[elementindex].nChannelsInEl; indexinelement++)
					{

						channelindex = elementencode_info_tianlai51[elementindex].ChannelIndex[indexinelement];


						//IMDCT
						Avs2Decoder_syn(&Avs2DecoderInstance_frame[channelindex],
							stAvs2Dec->useBWE,
							MdctSpectrum[channelindex],
							TimeDataOut,
							stAvs2Dec->config.bitRate,
							inputInfo.bitsPerSample);

						if (stAvs2Dec->useBWE)
							outlen = frameSize * 2;
						else
							outlen = frameSize;

						for (j = 0; j < outlen; j++)
							AllChannelTimeDataFloat[inputInfo.nChannels*j + channelindex] = TimeDataOut[j];



					}	//for(indexinelement=0;indexinelement<elementencode_info[elementindex].nChannelsInEl;indexinelement++)
				}
				break;
				}//switch (elementencode_info[elementindex].elType)

			}//		

		}

	}

	*outputLen = outlen;

	
	return 0;
}

/* close decoder */	
void close_general_decoder_frame(HANDLE_STAvs2Dec *phstAvs2Dec, int nChannels)
{
	int channelindex;

	HANDLE_STAvs2Dec stAvs2Dec = *phstAvs2Dec;

	*phstAvs2Dec = NULL;


	for( channelindex=0;channelindex<nChannels;channelindex++)
	{
		Avs2BweDecMDFTClose((unsigned int*)&(Avs2DecoderInstance_frame[channelindex].st1_decin), (unsigned int*)&(Avs2DecoderInstance_frame[channelindex].st_deccommon));
		if(stAvs2Dec->useBWE)
		{
			Avs2BweDecoderClose((unsigned int*)&(Avs2DecoderInstance_frame[channelindex].st2_decin));
		}
	}

	return;
}