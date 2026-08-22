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

#include "../general/bwedec/avs2BweDecMDFT.h"
#include "../general/bwedec/decoder.h"
#include "../general/decode/avs2audio.h"
#include "../general/decode/lfdec.h"
#include "../general/decode/pca.h"
#include "../general/decode/codebook.h"
#include "../general/decode/maxcorr.h"
#include "../general/decode/mc_rom.h"
#include "../general/decode/lfdec.h"
#include "../general/decode/avs2decmain.h"
#include "general_decoder_frame.h"
#include "../general/decode/crc_16.h"

//HuffmanTableStruc	huffmanbook;
//codebook huffmanDecodeBook;


#define SAMPLES_PER_FRAME 1024
/* 
   IO-Buffers 
*/
#define INPUT_BUF_SIZE (6144*2/8)                      /*!< Size of Input buffer in bytes*/
unsigned int inBuffer[INPUT_BUF_SIZE/(sizeof(int))];   /*!< Input buffer */

#define MAX_CH_ELE_DEF 6

//typedef struct MULTI_CHAN_MODE MC_MODE;

// added by lumin 2014.11.21

//float TimeDataFloat[4*SAMPLES_PER_FRAME];              /*!< Output buffer */
HANDLE_STAvs2Dec objphstAvs2Dec[128];
struct AVS2_DECODER_INSTANCE Avs2DecoderInstance_objframe[128][MAX_ALLCHANNEL];
extern int index_obj;
float AllChannelTimeDataFloat[4*SAMPLES_PER_FRAME*MAX_ALLCHANNEL];  
extern int road;
//////////////////////////////
//codec_setup_info ci_table[13];

extern const double rate_mapping_44_multi[5];

 //MC_MODE *encoder_mode;
//ELEMENTENCODE_INFO elementencode_info[8];

ELEMENTENCODE_INFO elementencode_info_tianlai51[8];

struct AVS2_DECODER_INSTANCE Avs2DecoderInstance[MAX_ALLCHANNEL];


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

int decoder_3D(int argc, char *argv[])
{
	
	char *input_filename;
	char *output_filename;
	char *filepath = NULL;
	ChanInfo inputInfo;
	ChanInfo objinputInfo;
	ChanInfo config;

	int nChannelsCore, nChannelsBWE;
	int sampleRateCore;
	int bEncodeMono = 0;
	int useParametricStereo = 0;
	int useBWE = 0;
	int usePS = 1;

    AVS2DECODER avs2DecoderInfo = 0;

	FILE *f_input;
	FILE *f_sound_out;
	int dataSizeDec = 0;
	int objdataSizeDec = 0;
	int Object_ID[128];

	ST_RATE_CONFIG srateInfo;
	MCR_INFO mcrInfo;

	PS_DATA *psData[MAX_ALLCHANNEL/2], *psData_pre[MAX_ALLCHANNEL/2];
	
	unsigned char readBuf[4096];

	int i, j,k;
    BWEBITSTREAM streamBWE[MAX_ALLCHANNEL];                           /*!< pointer to bwe bitstream buffer */
	int ErrorStatus = 0;
	int frameSize = FRAME_SIZE;


	char channelMode = 0;
	float TimeDataOut[FRAME_LEN_LONG * 2];

	int bandWidth[MAX_ALLCHANNEL] = {0};
	
	int elementindex;
	int channelindex =0, cid;
	int elementnum;
	int elementnum_tianlai51;
	float MdctSpectrum[MAX_ALLCHANNEL][FRAME_LEN_LONG*2];
	int outlen;
	int bitRateIndex;

	int core_bitrate,bitrate;
	int useSuperMode = 0;
	int	cpe_config = 0;
	int	chIndex[MAX_ALLCHANNEL] = {0};

    int huffmanbooklist[3][121];

	char codectype=0;
	unsigned short numBytes = 0;

	int	nchannels, headchannels = 0;

	unsigned int PCAGroupmode;
	int PCAGroupmodeHeader=0xFF;
	int config_idx[MAX_ALLCHANNEL] = {0};

	char  header_tag[4];
	int   isAASF = 1;
	int   frameStart = 0;
    
	int rnum;
	int stoploop = 0;
	HANDLE_STAvs2Dec phstAvs2Dec;
  
	//unsigned char readBuf[4096];
	//unsigned short numBytes;
	int outputLen;
    long frame=0;
	int ini=0;
	int channel_number;
	int objbitrate;
	int objbitpersample;

	float objpx[128][4];
    float objpy[128][4];
    float objpz[128][4];
    int objarea[128][4];
    float posbuffer[16];

	char obInfo[1024];
    char ptrpos[128][100];
	int cnt=0;
	int txtNum;
	int ObjectNum;
	int ObjectDataGroupNum;
	int count;
    int length;
	avs2audiopack_buffer opb;
    int Object_codec_id;

	float a[128][16];
	int txt;
	input_filename = NULL;
	output_filename = NULL;
	copyright();

	parsecmdline(argc, argv, &input_filename, &output_filename, &filepath);

	 char ptrobj[128][100];
	 FILE* objInfo[128];
	 char tmp1[128];
	 if (filepath != NULL)
		 sprintf(tmp1, "%s%s", filepath, "\\object_dec.txt");
	 else
		 sprintf(tmp1, "%s", "object_dec.txt");
	 FILE* objcontain=fopen(tmp1,"rb");

	 int fill_element_num;

  
	 cnt=0;
	 while(!feof(objcontain))
	 {
	      fscanf(objcontain, "%s", ptrobj[cnt]);
	        
			printf("%s\n",ptrobj[cnt]);
			cnt++;
	 }
	 txtNum = cnt;

	
	/* Open input wave file */
	if ((f_input = fopen(input_filename, "rb")) == NULL)
	{
		fprintf(stderr, "Error opening the input bitstream file %s.\n",
			input_filename);
		exit(0);
	}

	fread(header_tag, 4, 1, f_input);
	
	if(memcmp(header_tag, "AASF", 4) == 0)
		isAASF = 1;
	else if (header_tag[0] == (char)0x7f
		&& ((header_tag[1] & 0xf0) == 0xe0))
		isAASF = 2;
	else if(header_tag[0] == (char)0xff 
		&& ((header_tag[1]&0xf0) == 0xf0))
		isAASF = 2;
	else
		isAASF = -1;

	fseek(f_input, -sizeof(char) * 4, SEEK_CUR);
	inputInfo.headChannels = 0;
	if(isAASF == 1)
		read_avs2file_header(&dataSizeDec, &sampleRateCore, &inputInfo, &useSuperMode, &cpe_config, &PCAGroupmodeHeader, f_input);

	if (isAASF == 2)
	{
		stoploop = read_avs2AATF_header(&dataSizeDec, &sampleRateCore, &inputInfo, f_input);
	}
	nchannels = inputInfo.nChannels;
	headchannels = inputInfo.headChannels;
	bitrate = inputInfo.bitRate;


	{   //chenhan
		char tempbuf[10];
		avs2audiopack_buffer *opb;
		fseek(f_input, 2, SEEK_CUR);
		fread(tempbuf, 1, 10, f_input);
		opb = calloc(1, sizeof(avs2audiopack_buffer));
		opb->buffer = opb->ptr = tempbuf;
		opb->storage = 10;
		if (inputInfo.nChannels > 1)
		{
			codectype = avs2audiopack_read(opb, 1);  //codectype
			if (codectype == 1)
				avs2audiopack_read(opb, 4);
		}
		useBWE = avs2audiopack_read(opb, 1);
		fill_element_num = avs2audiopack_read(opb, 1);

		free(opb);
		fseek(f_input, -12, SEEK_CUR);
	}

	/* Open output wav file */
	if ((f_sound_out = Wave_fopen(output_filename)) == NULL)
	{
		fprintf(stderr, "Error opening output wav file %s.\n", output_filename);
		exit(0);
	}


	init_avs2_general_decoder_frame(sampleRateCore, inputInfo, &phstAvs2Dec,useBWE, fill_element_num);

	// the frame loop
	fprintf(stderr, "\n --- Running ---\n");
	
	while (1)
	{
		unsigned char crc_value = 0;
		int readcodectypeflag = 0;

		if (isAASF == 2 && frame > 0)
		{
			memset(header_tag, 0, 4);

			fread(header_tag, 4, 1, f_input);

			//read syncword
			if (!(header_tag[0] == (char)0xff && ((header_tag[1] & 0xf0) == 0xf0)))
			{
				if (!(header_tag[0] == (char)0x7f && ((header_tag[1] & 0xe0) == 0xe0)))
					break;
			}
			fseek(f_input, -sizeof(char) * 4, 1);

			stoploop = read_avs2AATF_header(&dataSizeDec, &sampleRateCore, &inputInfo, f_input);
			if (stoploop < 0) break;
		}

#if 1
		/*CRC ะฃั้*/
		{
			if (isAASF == 2)
			{
				fseek(f_input, -sizeof(char) * 9, 1);

				char crcValue;
				unsigned short nCRCBits;
				unsigned char crcbuf[65536];
				
				fread(crcbuf, sizeof(char), dataSizeDec - 1, f_input);

				nCRCBits = CRC16(&crcbuf, dataSizeDec - 1);
				char crcB = nCRCBits ^ (nCRCBits >> 8);
				fread(&crcValue, sizeof(char), 1, f_input);
				if (crcB != crcValue)
					printf("crc check is fault!\n");

				fseek(f_input, -sizeof(char) * (dataSizeDec - 9), 1);
			}
			dataSizeDec -= 9;
		}
#endif
		fread(&numBytes, sizeof(unsigned short), 1, f_input);
		dataSizeDec -= 2;
		fread(readBuf, sizeof(unsigned char), numBytes, f_input);
		dataSizeDec -= numBytes;

		road = 1;
		general_decoder_frame(phstAvs2Dec, inputInfo, readBuf, numBytes, AllChannelTimeDataFloat, &outputLen);

		write_data(AllChannelTimeDataFloat, outputLen * inputInfo.nChannels, inputInfo.bitsPerSample, f_sound_out);

		//write object info
		for (txt = 0; txt < txtNum; txt++)
		{
			for (cnt = 0; cnt < 4; cnt++) {
				objpx[txt][cnt] = 2;
				objpy[txt][cnt] = 2;
				objpz[txt][cnt] = 2;
				objarea[txt][cnt] = 2;
			}
		}

		txt = fread(readBuf, sizeof(unsigned char), 1024, f_input);
		fseek(f_input, -sizeof(unsigned char) *txt, 1);
		count = 0;

		memset(&opb, 0, sizeof(avs2audiopack_buffer));
		opb.buffer = opb.ptr = readBuf;
		opb.storage = 1024;
		avs2audiopack_read(&opb, 4);   //20181122 shumin.xu
		count += 4;
		ObjectNum = avs2audiopack_read(&opb, 7);

		count += 7;

		for (txt = 0; txt < ObjectNum; txt++)
		{
			objInfo_decoder(&opb, objpx, objpy, objpz, objarea, &count, Object_ID, txt);
		}
		if (count % 8 == 0)
			length = count / 8;
		else
		{
			count = count + 8 - count % 8;
			length = count / 8;
		}

		fseek(f_input, sizeof(char)*length, 1);
		dataSizeDec -= length;

		for (txt = 0; txt < txtNum; txt++)
		{

			for (cnt = 0; cnt < 4; cnt++) {
				posbuffer[4 * cnt + 0] = objpx[txt][cnt];
				posbuffer[4 * cnt + 1] = objpy[txt][cnt];
				posbuffer[4 * cnt + 2] = objpz[txt][cnt];
				posbuffer[4 * cnt + 3] = (float)objarea[txt][cnt];
			}
		}

		txt = fread(readBuf, sizeof(unsigned char), 8, f_input);
		fseek(f_input, -sizeof(unsigned char) *txt, 1);
		count = 0;

		memset(&opb, 0, sizeof(avs2audiopack_buffer));
		opb.buffer = opb.ptr = readBuf;
		opb.storage = 1024;
		ObjectDataGroupNum = avs2audiopack_read(&opb, 7);
		count += 7;

		for (txt = 0; txt < ObjectNum; txt++)
		{
			if (txt != 0)
			{
				fread(readBuf, sizeof(unsigned char), 8, f_input);
				fseek(f_input, -sizeof(unsigned char) * 8, 1);
				count = 0;

				memset(&opb, 0, sizeof(avs2audiopack_buffer));
				opb.buffer = opb.ptr = readBuf;
				opb.storage = 1024;
			}

			objdata_decoder(&opb, &count, &Object_codec_id, &channel_number, &objbitrate, &objbitpersample);
			if (count % 8 == 0)
				length = count / 8;
			else
			{
				count = count + 8 - count % 8;
				length = count / 8;
			}
			fseek(f_input, sizeof(char)*length, 1);
			dataSizeDec -= length;

			if (ini == 0)
			{
				objinputInfo.nChannels = channel_number;
				objinputInfo.sampleRate = inputInfo.sampleRate;
				objinputInfo.bitsPerSample = objbitpersample;
				objinputInfo.bitRate = objbitrate;

				for (ini = 0; ini < txtNum; ini++)
					objInfo[ini] = Wave_fopen(ptrobj[ini]);


				for (ini = 0; ini < txtNum; ini++)
				{
					index_obj = ini;
					objinit_avs2_general_decoder_frame(sampleRateCore, objinputInfo, &objphstAvs2Dec[ini], useBWE, fill_element_num);
				}

			}

			rnum = fread(&numBytes, sizeof(unsigned short), 1, f_input);
			dataSizeDec -= 2;
			fread(readBuf, sizeof(unsigned char), numBytes, f_input);
			dataSizeDec -= numBytes;
			road = 2;
			index_obj = Object_ID[txt];

			objgeneral_decoder_frame(objphstAvs2Dec[Object_ID[txt]], objinputInfo, readBuf, numBytes, AllChannelTimeDataFloat, &outputLen);
			write_data(AllChannelTimeDataFloat, outputLen * objinputInfo.nChannels, objinputInfo.bitsPerSample, objInfo[Object_ID[txt]]);
		}

		fread(&crc_value, 1, 1, f_input);
		dataSizeDec -= 1;

		frame++;
		printf("%d\r", frame);


		//if (isAASF == 1 && (dataSizeDec <= 0) || (stoploop == 1))
		if (stoploop < 0)
			break;

	}

	/* close encoder */	
	for( channelindex=0;channelindex<inputInfo.nChannels;channelindex++)
	{
		Avs2BweDecMDFTClose((unsigned int*)&(Avs2DecoderInstance[channelindex].st1_decin), (unsigned int*)&(Avs2DecoderInstance[channelindex].st_deccommon));
		if(useBWE)
		{
			Avs2BweDecoderClose((unsigned int*)&(Avs2DecoderInstance[channelindex].st2_decin));
		}
	}
	
	Wave_fclose(f_sound_out, inputInfo.nChannels, inputInfo.sampleRate, inputInfo.bitsPerSample);
	for(txt=0;txt<txtNum;txt++)
		Wave_fclose(objInfo[txt], objinputInfo.nChannels, inputInfo.sampleRate, inputInfo.bitsPerSample);

	return 0;
}
