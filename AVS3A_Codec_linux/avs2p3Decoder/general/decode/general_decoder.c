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
#include "../bwedec/avs2BweDecMDFT.h"
#include "../bwedec/decoder.h"
#include "../bwedec/decode_hf.h"
#include "avs2audio.h"
#include "lfdec.h"
#include "pca.h"
#include "codebook.h"
#include "maxcorr.h"
#include "mc_rom.h"
#include "lfdec.h"
#include "avs2decmain.h"
#include "crc_16.h"

HuffmanTableStruc	huffmanbook;
codebook huffmanDecodeBook;

#define SAMPLES_PER_FRAME 1024
/* IO-Buffers */
#define INPUT_BUF_SIZE (6144*2/8)                      /*!< Size of Input buffer in bytes*/
unsigned int inBuffer[INPUT_BUF_SIZE/(sizeof(int))];   /*!< Input buffer */

#define MAX_CH_ELE_DEF 6

struct MULTI_CHAN_MODE{
	int numofele; 	/* the number of element */
	ELEMENT_TYPE idType[MAX_ALLCHANNEL];	/* the type of the element */
	int ele_id[MAX_ALLCHANNEL];
};

typedef struct MULTI_CHAN_MODE MC_MODE;

// added by lumin 2014.11.21
static MC_MODE	CoupleChannelTable[8+8+2] = {

	{1,	{ID_SCE,0,0,0,0,0},{0,0,0,0,0,0}},
	{2,	{ID_SCE,ID_SCE,0,0,0,0},{0,1,0,0,0,0}},
	{1,	{ID_CPE_F,0,0,0,0,0},{0,1,0,0,0,0}},
	{1,	{ID_CPE_L,0,0,0,0,0},{0,1,0,0,0,0}},//CPE_L
	{6,	{ID_SCE,ID_SCE,ID_SCE,ID_SCE,ID_SCE,ID_SCE},{0,1,2,3,4,5}},
	{5,	{ID_SCE,ID_SCE,ID_SCE,ID_SCE,ID_CPE_F,0},{0,1,2,3,4,5}},
	{4,	{ID_CPE_F,ID_SCE,ID_SCE,ID_CPE_F,0,0},{0,1,2,3,4,5}},
	{3,	{ID_CPE_F,ID_CPE_F,ID_CPE_F,0,0,0},{0,1,2,3,4,5}},
	{3, {ID_CPE_F,ID_CPE_F,ID_CPE_F,0,0,0},{0,4,2,3,1,5}},
	{4, {ID_CPE_F,ID_CPE_F,ID_CPE_F,ID_CPE_F,0,0},{0,1,2,3,4,5,6,7}},

	{ 1, { ID_PCA4, 0, 0, 0, 0, 0 }, { 0, 1, 2, 3, 0, 0 } },													//10
	{ 2, { ID_CPE_F, ID_CPE_F, 0, 0, 0, 0 }, { 0, 1, 2, 3, 0, 0 } },											//11
	/*additional head channel configuration*/                                                                   //chenhan 20180328
	{ 4, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, 0, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7 } },						 	//12          5.1.2
	{ 5, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, ID_CPE_H, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } },				//13          5.1.4
	{ 5, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } },				//14          7.1.2
	{ 6, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, ID_CPE_H }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } }, //15          7.1.4
	{ 8, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 } }, //16          16ch 3rd HOA, shumin.xu 210421
	{ 5, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_SCE, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8 } } //9ch 2nd HOA, shumin.xu 211123
};

typedef struct {
  ELEMENT_TYPE elType;
  int nChannelsInEl;
  int ChannelIndex[8];
 } ELEMENTENCODE_INFO;


float TimeDataFloat[4*SAMPLES_PER_FRAME];              /*!< Output buffer */

float AllChannelTimeDataFloat[4*SAMPLES_PER_FRAME*MAX_ALLCHANNEL];  

 int Maxpcmvalue=(32768*256);
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
#define MAX_CH (6)

tianlai_block  vb[MAX_CH];
long frame = 0;

int general_decoder(int argc, char *argv[])
{
	
	char *input_filename;
	char *output_filename;
	ChanInfo inputInfo;
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

	ST_RATE_CONFIG srateInfo;
	MCR_INFO mcrInfo;

	PS_DATA *psData[MAX_ALLCHANNEL/2], *psData_pre[MAX_ALLCHANNEL/2];
	PS_BWE_DATA *psBweData[MAX_ALLCHANNEL/2], *psBweData_pre[MAX_ALLCHANNEL/2];
  
	unsigned char readBuf[4096];

	int i, j, k;
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
	float MdctSpectrum[MAX_ALLCHANNEL][FRAME_LEN_LONG*4+2048];
	int outlen;
	int bitRateIndex = 0;
	int brk = 0;

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
	unsigned short crc_bits;
	int fill_element_num = 1;

	input_filename = NULL;
	output_filename = NULL;
	
	copyright();
	parsecmdline(argc, argv, &input_filename, &output_filename, NULL);
	
	/* Open input wave file */
	if ((f_input = fopen(input_filename, "rb")) == NULL)
	{
		fprintf(stderr, "Error opening the input bitstream file %s.\n",
			input_filename);
		exit(0);
	}

	fread(header_tag, 4, 1, f_input);
	
	if (memcmp(header_tag, "AASF", 4) == 0)
		isAASF = 1;
	else if (header_tag[0] == (char)0x7f
		&& ((header_tag[1] & 0xf0) == 0xe0))
		isAASF = 2;
	else if (header_tag[0] == (char)0xff
		&& ((header_tag[1] & 0xf0) == 0xf0))
		isAASF = 2;
	else
		isAASF = -1;

	fseek(f_input, -sizeof(char) * 4, 1);
	
	inputInfo.headChannels = 0;

	if(isAASF == 1)
		read_avs2file_header(&dataSizeDec, &sampleRateCore, &inputInfo, &useSuperMode, &cpe_config, &PCAGroupmodeHeader, f_input);

    if(isAASF == 2)
	{
		brk = read_avs2AATF_header(&dataSizeDec, &sampleRateCore, &inputInfo, f_input);
	}

	nchannels = inputInfo.nChannels;
	headchannels = inputInfo.headChannels;
	bitrate = inputInfo.bitRate;

	if(nchannels == 1)
	{
		usePS = 0;
		if(bitrate < 40000)
			useBWE = 1;
	}
	else if(nchannels == 2)
	{
		//this version do not support BWE now
		if(bitrate >= 128000)
		{
			usePS = 2;
			useBWE = 0;
		}
		else if(bitrate > 64000/*48000*/)
		{
			usePS = 1;
			useBWE = 0;
		}
		else
		{
			usePS = 1;
			useBWE = 1;
		}
		bitrate_init(&srateInfo, bitrate/1000, nchannels, 0);
		mcr_init(&mcrInfo, srateInfo.mcr_brate[0], useBWE);

	}
	else if(nchannels >= 4)
	{
		usePS = 1;
		useBWE = 0;
			
		bitrate_init(&srateInfo, bitrate/1000, nchannels, headchannels);
		mcr_init(&mcrInfo, srateInfo.mcr_brate[0]);
	}
	/*else if(nchannels == 8)
	{
		usePS = 1;
		useBWE = 0;
			
		bitrate_init(&srateInfo, bitrate/1000, nchannels);
		mcr_init(&mcrInfo, srateInfo.mcr_brate[0]);
	}*/
	Maxpcmvalue = 1<<(inputInfo.bitsPerSample-1);

	if(inputInfo.nChannels >=5)
	{
		for(i =0; i< 5; i++)
			if(inputInfo.bitRate >= rate_mapping_44_multi[i])
				bitRateIndex = i;
	}

	{   //chenhan
		char tempbuf[10];
		avs2audiopack_buffer *opb;
		fseek(f_input, 2, 1);
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
		fseek(f_input, -12, 1);
	}

	/* Open output wav file */
	if ((f_sound_out = Wave_fopen(output_filename)) == NULL)
	{
		fprintf(stderr, "Error opening output wav file %s.\n", output_filename);
		exit(0);
	}

	nChannelsCore = nChannelsBWE =1;

	///setting encoder_mode
	encoder_mode =  &CoupleChannelTable[0];
	// added by lumin 2014.11.21
	if(inputInfo.nChannels>=2)
	{
		encoder_mode = &CoupleChannelTable[srateInfo.couple_config];
	}
	memcpy(chIndex, encoder_mode->ele_id, MAX_ALLCHANNEL*sizeof(int));

	elementnum =  encoder_mode->numofele;
	cid = 0;
	for(i = 0; i < elementnum; i++)
	{
		psData[i] = (PS_DATA *)malloc(sizeof(PS_DATA));
		memset(psData[i], 0, sizeof(PS_DATA));
		psData[i]->bandwidth = CORE_FRAMESIZE;
		psData_pre[i] = (PS_DATA *)malloc(sizeof(PS_DATA));
		memset(psData_pre[i], 0, sizeof(PS_DATA));
		psData_pre[i]->bandwidth = CORE_FRAMESIZE;

		psBweData[i] = (PS_BWE_DATA *)malloc(sizeof(PS_BWE_DATA));
		memset(psBweData[i], 0, sizeof(PS_BWE_DATA));
		psBweData[i]->bandwidth = CORE_FRAMESIZE;
		psBweData_pre[i] = (PS_BWE_DATA *)malloc(sizeof(PS_BWE_DATA));
		memset(psBweData_pre[i], 0, sizeof(PS_BWE_DATA));
		psBweData_pre[i]->bandwidth = CORE_FRAMESIZE;
	}
	for( i=0;i<elementnum;i++)
	{
		  elementencode_info[i].elType = encoder_mode->idType[i];

		  switch (elementencode_info[i].elType) {

			case ID_SCE:      /* single channel */
				elementencode_info[i].nChannelsInEl=1;
				elementencode_info[i].ChannelIndex[0] = cid++;
				
			break;
			case ID_CPE_F:      /* channel pair */
				elementencode_info[i].nChannelsInEl=2;
				elementencode_info[i].ChannelIndex[0] = cid++;
				elementencode_info[i].ChannelIndex[1] = cid++;
				
			break;
			case ID_CPE_L:      /* channel pair */
				elementencode_info[i].nChannelsInEl=2;
				elementencode_info[i].ChannelIndex[0] = cid++;
				elementencode_info[i].ChannelIndex[1] = cid++;
				
			break;			
			
			case ID_LFE:     /*LFE channel */
				elementencode_info[i].nChannelsInEl=1;
				elementencode_info[i].ChannelIndex[0] = cid++;
			break;

			case ID_CPE_H:                                                                 //chenhan 20180402
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
	for (i = elementnum; i < elementnum + fill_element_num; i++)
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
		elementnum_tianlai51 = encoder_mode_tianlai51->numofele;
		cid = 0;
		for (i = 0; i < elementnum_tianlai51; i++)
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

		for (i = elementnum; i < elementnum + fill_element_num; i++)
		{
			elementencode_info_tianlai51[i].elType = ID_FIL;
			elementencode_info_tianlai51[i].nChannelsInEl = 1;
		} //shumin.xu 210513
	}
	
	useParametricStereo = 0;
	config = inputInfo;
	
	// initialize time data buffer 
	memset(TimeDataFloat, 0, SAMPLES_PER_FRAME * 4 * sizeof(float));
	memset(TimeDataOut, 0, SAMPLES_PER_FRAME * 2 * sizeof(float));

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
			core_bitrate = srateInfo.core_brate[i]*1000;

		if(core_bitrate == 0) 
		{
			if (useBWE)
				config.bitRate = srateInfo.core_brate[i-1]*1000;
			else
				config.bitRate = 48000;
		}
		else
			config.bitRate = core_bitrate;

		avs2DecoderInfo = CAvs2DecoderOpen(&streamBWE[i], TimeDataFloat, i);

		Avs2BweDecMDFTOpen((unsigned int*)&(avs2DecoderInfo->st1_decin), (unsigned int*)&(avs2DecoderInfo->st_deccommon));

		if(useBWE)
		{
			Avs2BweDecoderOpen((unsigned int*)&(avs2DecoderInfo->st2_decin), config.bitRate, sampleRateCore, nChannelsCore, &bandWidth[i], &config_idx[i]);
		}		

		ErrorStatus = CAvs2DecoderInit(avs2DecoderInfo, sampleRateCore, config.bitRate, useBWE, &bandWidth[i]);

		_make_decode_ready(&(avs2DecoderInfo->vf), sampleRateCore, config.bitRate);
	}

	if(useBWE)
		outlen = frameSize * 2;		
	else
		outlen = frameSize;
	// the frame loop
	fprintf(stderr, "\n --- Running ---\n");
	
	while (1)
	{
		int readcodectypeflag = 0;

		if (isAASF == 2 && frame > 0)
		{
			memset(header_tag, 0, 4);

			fread(header_tag, 4, 1, f_input);

			//read syncword
			if (!(header_tag[0] == (char)0x7f && ((header_tag[1] & 0xe0) == 0xe0)))
			{
				if (!(header_tag[0] == (char)0xff && ((header_tag[1] & 0xf0) == 0xf0)))
					break;
			}
			fseek(f_input, -sizeof(char) * 4, 1);

			brk = read_avs2AATF_header(&dataSizeDec, &sampleRateCore, &inputInfo, f_input);
		}

		/*CRC 校验*/
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
				dataSizeDec -= 9;
			}
			else if (isAASF == 1) brk = -1;
		}

		if (inputInfo.nChannels > 2)
		{
			int i;
			for (i = 0; i < 3; i++)
				fread(&codectype, sizeof(char), 1, f_input);

			fseek(f_input, -sizeof(char) * 3, 1);

			PCAGroupmode = codectype & 0x1F;
			PCAGroupmode = PCAGroupmode >> 1;
			codectype = codectype & 0x01;
		}
		else if (inputInfo.nChannels == 2)
		{
			codectype = 0;
		}
		else if (inputInfo.nChannels == 1)
		{
			codectype = 1;
		}

		if (codectype == 0)
		{
			int iid;
			int cid = 0;
			int stoploop = 0;
#if 0
			int mcr_buff[MAX_CH][200] = { 0 };
			int lf_winseq[20] = { 1,4,4 };
#endif			
			int rnum;

			tianlai_block *vb = &(Avs2DecoderInstance[0].vf.vb);

			rnum = fread(&numBytes, sizeof(unsigned short), 1, f_input);
			dataSizeDec -= 2;

			if (rnum < 1)
			{
				stoploop = 1;
				break;
			}

			fread(readBuf, sizeof(unsigned char), numBytes, f_input);
			dataSizeDec -= numBytes;

			memset(&vb->opb, 0, sizeof(avs2audiopack_buffer));
			vb->opb.buffer = vb->opb.ptr = readBuf;
			vb->opb.storage = numBytes;

			if (readcodectypeflag == 0)
			{
				avs2audiopack_read(&vb->opb, 1);  //codectype	
			}

			useBWE = avs2audiopack_read(&vb->opb, 1);
			fill_element_num = avs2audiopack_read(&vb->opb, 1);
			readcodectypeflag = 0;

			for (k = 0; k < elementnum + fill_element_num; k++)
			{
				int rnum, num_mcr;
				int super_flag = 0;
				int type;
				int coreNum;

				float freq_mono[FRAMESIZE * 4] = { 0 };
				float freq_left[FRAMESIZE * 4] = { 0 };
				float freq_right[FRAMESIZE * 4] = { 0 };

				type = avs2audiopack_read(&vb->opb, 4);
				Avs2DecoderInstance[cid].type = type;
				if (Avs2DecoderInstance[cid].type == ID_CPE_F 
					|| Avs2DecoderInstance[cid].type == ID_CPE_S
					|| Avs2DecoderInstance[cid].type == ID_CPE_L
					|| Avs2DecoderInstance[cid].type == ID_CPE_H)
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

						streamBWE[iid].NrElements = 0;

						memcpy(lf_winseq, lf_winseq_table[Avs2DecoderInstance[iid].lf_winseq_index], 20 * sizeof(int));
#else
						//tianlai_block *vb;
						int count;

						// read one frame of encoded audio to file 
						if (srateInfo.core_brate[iid] == 0)
						//if (iid-cid>coreNum)
						{
							memset(MdctSpectrum[iid], 0, (FRAME_SIZE * 4 + 2048) * sizeof(float));
							break;
						}

						streamBWE[iid].NrElements = 0;
						//memcpy(lf_winseq, lf_winseq_table[Avs2DecoderInstance[iid].lf_winseq_index], 20 * sizeof(int));

						config.bitRate = srateInfo.core_brate[cid] * 1000;
						//printf("config.bitRate = %d\n",config.bitRate);
#endif					
						// decode lf frame using a new lf decoder 
						Avs2DecoderInstance[iid].type = type;
						ErrorStatus = Avs2LFDecoder(useBWE, &bandWidth[iid], bitRateIndex, config.bitRate, inputInfo.bitsPerSample,
							&Avs2DecoderInstance[iid], readBuf, numBytes,
							&sampleRateCore, 1, MdctSpectrum[iid],
							readcodectypeflag, vb, config_idx[iid]);

						readcodectypeflag++;
#if 0
						if (useBWE)
						{ //added in 2015.09.10 (taking the orig spectrum from the encoder, to ignore the influence of the core mono coding)
							int j;
							FILE *fp2 = fopen("MdftSamples.txt", "a");

							for (j = 0; j < 1024 * 4; j++)
							{
								fprintf(fp2, "%f,", MdctSpectrum[iid][j]);
							}

							fprintf(fp2, "\n");
							fclose(fp2);

						}
						{ //added in 2015.09.09 (taking the orig spectrum from the encoder, to ignore the influence of the core mono coding)
							int j;
							float origSpectrum[4096] = { 0.0 };

							if (useBWE && (frame >= 2))
							{
								for (j = 0; j < 1024 * 4; j++)
								{
									//fscanf(fp1,"%f,", &origSpectrum[j]);
									//MdctSpectrum[iid][j] = origSpectrum[j] / 2;
								}
							}
							//if(useBWE==0)
							{
								for (j = 0; j < 1024; j++)
								{
									//fscanf(fp0[iid],"%f,", &origSpectrum[j]);
									//MdctSpectrum[iid][j] = origSpectrum[j] / 8;
								}
							}

						}
#endif						
					}

					//if (stoploop == 1) break;

					// judging next frame contains new mcr bits
					if (Avs2DecoderInstance[cid].type == ID_CPE_F
						|| Avs2DecoderInstance[cid].type == ID_CPE_S
						|| Avs2DecoderInstance[cid].type == ID_CPE_L
						|| Avs2DecoderInstance[cid].type == ID_CPE_H)
					{
						if (useBWE)
						{
							if (Avs2DecoderInstance[cid].type == ID_CPE_L)
							{
							StAvs2BweDecode *pstBweData;
							StAvs2BweDecMDFT *pstBweMDFT;
							StAvs2BweDecCommon *pstBweCommon;

							pstBweData = (StAvs2BweDecode *)(Avs2DecoderInstance[cid].st2_decin);
							pstBweMDFT = (StAvs2BweDecMDFT *)(Avs2DecoderInstance[cid].st1_decin);
							pstBweCommon = (StAvs2BweDecCommon *)(Avs2DecoderInstance[cid].st_deccommon);
							psBweData[k]->Seqmode = pstBweData->savedHfParam[0].seqMode;
							psBweData[k]->Groupmode = pstBweData->savedHfParam[0].groupMode;
							psBweData[k]->bandwidth = CORE_FRAMESIZE;

							psBweData_pre[k]->Seqmode = pstBweMDFT->Seqmode;
							psBweData_pre[k]->Groupmode = pstBweMDFT->Groupmode;
							pstBweMDFT = (StAvs2BweDecCommon *)(Avs2DecoderInstance[cid + 1].st1_decin);
							pstBweMDFT->Seqmode = psBweData_pre[k]->Seqmode;
							pstBweMDFT->Groupmode = psBweData_pre[k]->Groupmode;
							Avs2DecoderInstance[cid + 1].lf_winseq_index = Avs2DecoderInstance[cid].lf_winseq_index;

							memcpy(psBweData_pre[k]->sum_data, MdctSpectrum[cid], (CORE_FRAMESIZE * 4 + 2048) * sizeof(float));
							if (coreNum > 0)
								memcpy(psBweData_pre[k]->dif_data, MdctSpectrum[cid + 1], (CORE_FRAMESIZE * 4 + 2048) * sizeof(float));

							if (Avs2DecoderInstance[cid].type == ID_CPE_S)
							{
								super_flag = 1;
							}
							else
							{
								super_flag = 0;
							}
							num_mcr = MCR_BWE_Decoder(inputInfo.bitRate, psBweData[k], mcrInfo, psBweData_pre[k], super_flag, &(vb->opb));

							memcpy(MdctSpectrum[cid], psBweData_pre[k]->left_data, (CORE_FRAMESIZE * 4 + 2048) * sizeof(float));
							memcpy(MdctSpectrum[cid + 1], psBweData_pre[k]->right_data, (CORE_FRAMESIZE * 4 + 2048) * sizeof(float));
						}
						}
						else
						{
							psData[k]->bandwidth = bandWidth[cid];
							memcpy(psData[k]->winseq, lf_winseq_table[Avs2DecoderInstance[cid].lf_winseq_index], 20 * sizeof(int));
							memcpy(psData[k]->sum_data, MdctSpectrum[cid], CORE_FRAMESIZE * sizeof(float));
							memcpy(psData[k]->dif_data, MdctSpectrum[cid + 1], CORE_FRAMESIZE * sizeof(float));

							Avs2DecoderInstance[cid + 1].lf_winseq_index = Avs2DecoderInstance[cid].lf_winseq_index;

							if (Avs2DecoderInstance[cid].type == ID_CPE_S)
							{
								super_flag = 1;
							}
							else
							{
								super_flag = 0;
							}
							num_mcr = MCR_Decoder(psData[k], mcrInfo, psData_pre[k], /*f_input,*/ super_flag, &(vb->opb));

							memcpy(MdctSpectrum[cid], psData[k]->left_data, CORE_FRAMESIZE * sizeof(float));
							memcpy(MdctSpectrum[cid + 1], psData[k]->right_data, CORE_FRAMESIZE * sizeof(float));
						}
					}

					cid += elementencode_info[k].nChannelsInEl;
				}
				else if (Avs2DecoderInstance[cid].type == ID_SCE)
				{
					{
						int indexinelement = 0, elementindex = 0;
						//int startchannelindex;

						//startchannelindex = 0; elementencode_info[k].ChannelIndex[0];//elementencode_info_tianlai51[elementindex].ChannelIndex[0];
						//2014.11.13 wchg 
						Avs2DecoderInstance[cid].usePCAitemnum = elementencode_info[k].nChannelsInEl;//elementencode_info_tianlai51[elementindex].nChannelsInEl;

						//if ((elementindex == 0) && (indexinelement == 0)/* && (inputInfo.nChannels > 1)*/)
						//{
						//	unsigned int codectype;
						//	codectype = avs2audiopack_read(&vb->opb, 1);
						//	if (codectype == 1)
						//	{
						//		avs2audiopack_read(&vb->opb, 4);
						//	}
						//}
						//if (elementindex == 0)
						//{
						//	useBWE = avs2audiopack_read(&vb->opb, 1);
						//	fill_element_num = avs2audiopack_read(&vb->opb, 1);
						//}
						//{
						//	int type = avs2audiopack_read(&vb->opb, 4);
						//}
						{
							int usePCAitemnum;
							int lf_winseq_index;
							char ElementInstanceTag;

							/*if ((elementencode_info_tianlai51[elementindex].nChannelsInEl > 1) && (indexinelement == 0))
							usePCAitemnum = avs2audiopack_read(&vb->opb, 3);
							else*/
							usePCAitemnum = 1;
							ElementInstanceTag = avs2audiopack_read(&vb->opb, 4);
							lf_winseq_index = avs2audiopack_read(&vb->opb, 6);
							for (indexinelement = 0; indexinelement < elementencode_info[k].nChannelsInEl/*elementencode_info_tianlai51[elementindex].nChannelsInEl*/; indexinelement++)
							{
								channelindex = cid; //elementencode_info_tianlai51[elementindex].ChannelIndex[indexinelement];

								Avs2DecoderInstance[channelindex].lf_winseq_index = lf_winseq_index;
								Avs2DecoderInstance[channelindex].usePCAitemnum = usePCAitemnum;
								Avs2DecoderInstance[channelindex].ElementInstanceTag = ElementInstanceTag;

							}
						}

						for (indexinelement = 0; indexinelement < elementencode_info[k].nChannelsInEl/*elementencode_info_tianlai51[elementindex].nChannelsInEl*/; indexinelement++)
						{
							channelindex = cid; // elementencode_info_tianlai51[elementindex].ChannelIndex[indexinelement];

							streamBWE[channelindex].NrElements = 0;

							/* decode lf frame using a new lf decoder */
							// decode the first usePCAitemnum principal components,  PCA Matrix
							if (Avs2DecoderInstance[channelindex].usePCAitemnum > indexinelement)
								ErrorStatus = Avs2LFDecoder_PCA(useBWE,
								/*inputInfo.nChannels,*/1,
								bitRateIndex,
								config.bitRate,
								inputInfo.bitsPerSample,
								0,
								&Avs2DecoderInstance[channelindex],//avs2DecoderInfo,
								NULL,
								readBuf,
								numBytes,
								/*&(Avs2DecoderInstance[channelindex].vf.vb)*/vb,//&(Avs2DecoderInstance[channelindex].vf.vb),
								&(Avs2DecoderInstance[channelindex].vf.TnsData),
								&sampleRateCore,
								1,
								MdctSpectrum[channelindex],
								indexinelement,
								/*elementindex,*/0,
								/*elementencode_info_tianlai51[elementindex].nChannelsInEl,*/elementencode_info[k].nChannelsInEl,
								config_idx[channelindex]);
							else
							{
								memset(MdctSpectrum[channelindex], 0, 1024 * 4);

								Avs2DecoderInstance[channelindex].lf_winseq_index = Avs2DecoderInstance[cid].lf_winseq_index;
							}

							/*{
							FILE *fp = fopen("bitnum.txt","a");
							fprintf(fp, "[core][%d]\n", vb->opb.endbyte);
							fclose(fp);
							}*/

						}	//for(indexinelement=0;indexinelement<elementencode_info[elementindex].nChannelsInEl;indexinelement++)

						//avs2audiopack_read(&vb->opb, (8 - vb->opb.endbit));

						//if (elementencode_info_tianlai51[elementindex].nChannelsInEl > 1) {
						//	int lf_winseq_dec[20];
						//	memcpy(lf_winseq_dec, lf_winseq_table[Avs2DecoderInstance[cid].lf_winseq_index], 20 * 4);
						//	//PCA synthesis, using the first usePCAitemnum principal components and PCA Matrix
						//	multichannelMDCT_PCA_syn(&MdctSpectrum[cid], lf_winseq_dec, elementencode_info_tianlai51[elementindex].nChannelsInEl, &MdctSpectrum[startchannelindex], elementindex, Avs2DecoderInstance[startchannelindex].usePCAitemnum);
						//}
					}
					cid += elementencode_info[k].nChannelsInEl;
				}
				else if (Avs2DecoderInstance[cid].type == ID_FIL)
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

			cid = 0;
			for (k = 0; k < elementnum; k++)
			{
				int super_flag = 0;
				for (iid = cid; iid < cid + elementencode_info[k].nChannelsInEl; iid++)
				{
					config.bitRate = srateInfo.core_brate[cid] * 1000;

					Avs2Decoder_syn(&Avs2DecoderInstance[iid], useBWE, MdctSpectrum[iid], TimeDataOut, config.bitRate,inputInfo.bitsPerSample);
					
					for (j = 0; j < outlen; j++)
					{
						AllChannelTimeDataFloat[inputInfo.nChannels*j + chIndex[iid]] = TimeDataOut[j];
					}
				}
				cid += elementencode_info[k].nChannelsInEl;
			}
			write_data(AllChannelTimeDataFloat, outlen * inputInfo.nChannels, inputInfo.bitsPerSample, f_sound_out);

			if (isAASF == 2)
			{
				char tmp = 0;
				fread(&tmp, sizeof(char), 1, f_input);
				dataSizeDec -= 1;
			}

			frame++;

			if (brk < 0 && (dataSizeDec <= 0) || (stoploop == 1))
				break;
		}
		else
		{
			tianlai_block *vb;
			channelindex = 0;
			int rnum = 0, stoploop = 0;

			/* read one frame of encoded audio to file */
			//	indexinelement=0;
			//	if(Avs2DecoderInstance[startchannelindex].usePCAitemnum>indexinelement)
			{
				fread(&numBytes, sizeof(short), 1, f_input);
				rnum = fread(readBuf, sizeof(unsigned char), numBytes, f_input);

				dataSizeDec -= 2;
				dataSizeDec -= numBytes;

				if (rnum < 1)
				{
					stoploop = 1;
					break;
				}
			}

			vb = &(Avs2DecoderInstance[0].vf.vb);
			memset(&vb->opb, 0, sizeof(avs2audiopack_buffer));
			vb->opb.buffer = vb->opb.ptr = readBuf;
			vb->opb.storage = numBytes;



			//重新配置PCA组合 //PCAGroupmode
			if (inputInfo.nChannels == 8) {
				encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[0x10 + PCAGroupmode];
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

			if (inputInfo.nChannels == 6) {
				int tmp = 0x02;
				//if (frame > 1000)
				//	tmp = 0x07; //shumin.xu, 210529 for test
				encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[tmp + PCAGroupmode];
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

			for (elementindex = 0; elementindex < elementnum_tianlai51 /*+ fill_element_num*/; elementindex++)
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
				Avs2DecoderInstance[startchannelindex].usePCAitemnum = elementencode_info_tianlai51[elementindex].nChannelsInEl;


				indexinelement = 0;
				if ((elementindex == 0) && (indexinelement == 0) && (inputInfo.nChannels > 1))
				{
					unsigned int codectype;
					codectype = avs2audiopack_read(&vb->opb, 1);
					if (codectype == 1)
					{
						avs2audiopack_read(&vb->opb, 4);
					}
				}
				if (elementindex == 0)
				{
					useBWE = avs2audiopack_read(&vb->opb, 1);
					fill_element_num = avs2audiopack_read(&vb->opb, 1);
				}
				{
					int type = avs2audiopack_read(&vb->opb, 4);
				}
				{
					int usePCAitemnum;
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

						Avs2DecoderInstance[channelindex].lf_winseq_index = lf_winseq_index;
						Avs2DecoderInstance[channelindex].usePCAitemnum = usePCAitemnum;
						Avs2DecoderInstance[channelindex].ElementInstanceTag = ElementInstanceTag;

					}
				}

				for (indexinelement = 0; indexinelement < elementencode_info_tianlai51[elementindex].nChannelsInEl; indexinelement++)
				{

					channelindex = elementencode_info_tianlai51[elementindex].ChannelIndex[indexinelement];


					streamBWE[channelindex].NrElements = 0;


					/* decode lf frame using a new lf decoder */
					// decode the first usePCAitemnum principal components,  PCA Matrix
					if (Avs2DecoderInstance[startchannelindex].usePCAitemnum > indexinelement)
						ErrorStatus = Avs2LFDecoder_PCA(useBWE,
							inputInfo.nChannels,
							bitRateIndex,
							config.bitRate,
							inputInfo.bitsPerSample,
							0,
							&Avs2DecoderInstance[channelindex],//avs2DecoderInfo,
							NULL,
							readBuf,
							numBytes,
							&(Avs2DecoderInstance[0].vf.vb),//&(Avs2DecoderInstance[channelindex].vf.vb),
							&(Avs2DecoderInstance[channelindex].vf.TnsData),
							&sampleRateCore,
							1,
							MdctSpectrum[channelindex],
							indexinelement,
							elementindex,
							elementencode_info_tianlai51[elementindex].nChannelsInEl,
							config_idx[channelindex]);
					else
					{
						memset(MdctSpectrum[channelindex], 0, 1024 * 4);

						Avs2DecoderInstance[channelindex].lf_winseq_index = Avs2DecoderInstance[startchannelindex].lf_winseq_index;
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


					memcpy(lf_winseq_dec, lf_winseq_table[Avs2DecoderInstance[startchannelindex].lf_winseq_index], 20 * 4);
					//PCA synthesis, using the first usePCAitemnum principal components and PCA Matrix
					multichannelMDCT_PCA_syn(&MdctSpectrum[startchannelindex], lf_winseq_dec, elementencode_info_tianlai51[elementindex].nChannelsInEl, &MdctSpectrum[startchannelindex], elementindex, Avs2DecoderInstance[startchannelindex].usePCAitemnum);


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
						fill_byte = avs2audiopack_read(&vb->opb, 8/*16*/);
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
				if (useBWE && elementencode_info_tianlai51[elementindex].elType != ID_FIL)
				{
					for (channelindex = 0; channelindex < inputInfo.nChannels; channelindex++)
					{
						tianlai_block *vb = &Avs2DecoderInstance[0].vf.vb;
						int count;

						if (channelindex == 3)
							continue;


						streamBWE[channelindex].NrElements = 0;


						count = Avs2DecoderInstance[channelindex].pStreamBWE->bweElement[Avs2DecoderInstance[channelindex].pStreamBWE->NrElements].Payload = avs2audiopack_read(&vb->opb, 8);

						for (i = 0; i < count / 4; i++)
						{
							Avs2DecoderInstance[channelindex].pStreamBWE->bweElement[Avs2DecoderInstance[channelindex].pStreamBWE->NrElements].Data[i] = (unsigned int)avs2audiopack_read(&vb->opb, 32);
						}
						if (count % 4 != 0)
						{
							Avs2DecoderInstance[channelindex].pStreamBWE->bweElement[Avs2DecoderInstance[channelindex].pStreamBWE->NrElements].Data[i] = (unsigned int)avs2audiopack_read(&vb->opb, 8 * (count % 4));
						}

						Avs2DecoderInstance[channelindex].pStreamBWE->NrElements += 1;


					}
				}
				else
				{
					for (channelindex = 0; channelindex < inputInfo.nChannels; channelindex++)
					{

						streamBWE[channelindex].NrElements = 0;
					}
				}
			} //for(indexinelement=0;indexinelement<elementencode_info[elementindex].nChannelsInEl;indexinelement++)

			for (elementindex = 0; elementindex < elementnum_tianlai51; elementindex++)
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
					Avs2DecoderInstance[startchannelindex].usePCAitemnum = elementencode_info_tianlai51[elementindex].nChannelsInEl;

					for (indexinelement = 0; indexinelement < elementencode_info_tianlai51[elementindex].nChannelsInEl; indexinelement++)
					{

						channelindex = elementencode_info_tianlai51[elementindex].ChannelIndex[indexinelement];


						//IMDCT
						Avs2Decoder_syn(&Avs2DecoderInstance[channelindex],
							useBWE,
							MdctSpectrum[channelindex],
							TimeDataOut,
							config.bitRate,
							inputInfo.bitsPerSample);

						if (useBWE)
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

			/* clip time samples */
			if (useBWE)
				write_data(AllChannelTimeDataFloat, outlen * inputInfo.nChannels, inputInfo.bitsPerSample, f_sound_out);
			else
				write_data(AllChannelTimeDataFloat, outlen * inputInfo.nChannels, inputInfo.bitsPerSample, f_sound_out);

			char tmp = 0;
			fread(&tmp, sizeof(char), 1, f_input);
			dataSizeDec -= 1;

			if (brk < 0 && (dataSizeDec <= 0) || (stoploop == 1))
				break;

			frame++;
		}


		fprintf(stderr, "[%d]\r", frame);
	}

	/* close encoder */	
	for( cid=0;cid<inputInfo.nChannels;cid++)
	{
		Avs2BweDecMDFTClose((unsigned int*)&(Avs2DecoderInstance[cid].st1_decin), (unsigned int*)&(Avs2DecoderInstance[cid].st_deccommon));
		if(useBWE)
		{
			Avs2BweDecoderClose((unsigned int*)&(Avs2DecoderInstance[cid].st2_decin));
		}
	}
	
	Wave_fclose(f_sound_out, inputInfo.nChannels, inputInfo.sampleRate, inputInfo.bitsPerSample);

	return 0;
}