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
#include "../general/encode/iir32resample.h"
#include "../general/encode/resampler.h"
#include "../general/bweenc/avs2BweEncMDFT.h"
#include "../general/bweenc/encoder.h"
#include "../general/encode/lfenc.h"
#include "../general/encode/pca.h"
#include "../general/encode/maxcorr.h"
#include "../general/encode/mc_rom.h"
#include "enc.h"

#ifdef __unix__
	#define	bname(s)	(strrchr(s, '/')? strrchr(s, '/') + 1 : s)
	#define	C35		"\E[35m"
	#define C0		"\E[0m"
#else
	#define	bname(s)	s 
	#define	C35		""
	#define	C0		""
#endif

#define DOWN_DELAY (6)
#define MCR_DELAY (1024*2)
#define TOTAL_DELAY (CORE_FRAMESIZE*4+MCR_DELAY)

#define MAX_ALLCHANNEL 16


#define CORE_DELAY   (1600)
#define INPUT_DELAY  ((CORE_DELAY)*2+1)     /* ((1600 (core codec)*2 (multi rate) + 6*64 (sbr dec delay) - 2048 (sbr enc delay) + magic*/
#define MAX_DS_FILTER_DELAY 16                         /* the additional max resampler filter delay (source fs)*/

#define MAX_PAYLOAD_SIZE    256

static IIR21_RESAMPLER IIR21_reSampler[MAX_ALLCHANNEL]; 
static IIR21_RESAMPLER IIR21_bweSampler[MAX_ALLCHANNEL]; 

static float inputBuffer[MAX_ALLCHANNEL][AVS2ENC_BLOCKSIZE*2 + MAX_DS_FILTER_DELAY + INPUT_DELAY];
static float downmixBuffer[MAX_ALLCHANNEL][AVS2ENC_BLOCKSIZE*2 + MAX_DS_FILTER_DELAY + INPUT_DELAY];

//#define MAX_CH_ELE_DEF 6



 codec_setup_info ci_table[13];

//extern int inputchannelnum;
extern const double rate_mapping_44_multi[5];



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

	/*aditional channel configuration, shumin.xu 20210105*/
	{ 4, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, 0, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7 } },						    //9           7.1
	{ 1, { ID_PCA4, 0, 0, 0, 0, 0 }, { 0, 1, 2, 3, 0, 0 } },													    //10          4.0
	{ 2, { ID_CPE_F, ID_CPE_F, 0, 0, 0, 0 }, { 0, 1, 2, 3, 0, 0 } },											    //11
	{ 4, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, 0, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7 } },						 	//12          5.1.2
	{ 5, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, ID_CPE_H, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9} },				//13          5.1.4
	{ 5, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } },				//14          7.1.2
	{ 6, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, ID_CPE_H }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11} }, //15          7.1.4
	{ 8, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}} //16 3rd HOA, shumin.xu 20210510

};


///////////////////////////////



 static MC_MODE *encoder_mode;
static ELEMENTENCODE_INFO elementencode_info[/*8*/9];


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
static ELEMENTENCODE_INFO elementencode_info_tianlai51[8];

//static double allchannelbasesetting[MAX_ALLCHANNEL]={4,4,4,2,2,2};

static long Maxpcmvalue=(32768*256);

 static long datalencount[MAX_ALLCHANNEL]={0}; //各声道编码码流大小统计
 static long mcrlencount[MAX_ALLCHANNEL]={0}; //各声道编码码流大小统计

 static long PCAdatalencount[MAX_ALLCHANNEL]={0}; //各声道编码码流大小统计



//////////////////////////////
//float *pcm_buffer0;
//  int blocktypeB;
static int lf_winseqpre[20] = {1,4,4};

int wOffset, cOffset;
		
static int coreOffset = CORE_FRAMESIZE/2 + CORE_FRAMESIZE/8/2;

static float Mdftout[CORE_FRAMESIZE * 8] = {0};


static int codetypemode=0;//0 ： all tsinghua; 1: all tianlai; 2 auto


void init_avs2_general_encoder(ChanInfo *inputInfo, HANDLE_STAvs2Enc *phstAvs2Enc, int *inLen,int*cpe_config,int*PCAGroupmodeHeader,int*useSuperMode,int fill_element_num)
{
	HANDLE_STAvs2Enc stAvs2Enc=NULL;
	int 		i,k;
	int			pmode = 0;

	int			chIndex[MAX_ALLCHANNEL];

//	float		time_dmix[MAX_ALLCHANNEL][(TOTAL_DELAY*2+DOWN_DELAY)*2]; //下混声道

	int writeOffset  = MCR_DELAY/2;

	short numOfChannels, bitsPerSample;

	int useParametricStereo = 0;
	//int bDoIIR32Resample = 0;
//	int bDoUpsample = 0;
	int bDingleRate = 0;

    int coreReadOffset = 0;
    int envWriteOffset = 0;
    int envReadOffset = 0;
    int upsampleReadOffset = 0;

	int error/*, inSamples*/;
//	int bDoIIR2Downsample = 0;

	int nChannelsCore = 1;
	int sampleRateCore;
	int bEncodeMono = 0;
	int useBWE = 0;
	int usePS = 1;

//	PS_DATA	 *psData[MAX_ALLCHANNEL/2];
//	PS_DATA	 *psData_pre[MAX_ALLCHANNEL/2];

	int inSamples;
	
//	struct AVS2_ENCODER *lfEncset[MAX_ALLCHANNEL];

//	ENC_CONFIG config;

	int cid = 0;
	int elementnum=1;
	int elementnum_tianlai51=1;
//	double basesetting, channelbasesetting;

	int eid;
//	int brIndex, bitRateIndex;
	int coreBitrate;
	double lowpass_kHz;

//	ST_RATE_CONFIG srateInfo;
//	MCR_INFO mcr_info;

	int PCAcorebitpershort=150;
    int PCAGroupmode = 0;
     *PCAGroupmodeHeader=0xFF;

//	int config_idx=0;
	*cpe_config = 0;

     useSuperMode = 0;
	*phstAvs2Enc = NULL;
	stAvs2Enc = calloc( 1, sizeof(struct STAvs2Enc));


	numOfChannels = inputInfo->nChannels;

	//shumin.xu 20210105
	switch (inputInfo->nChannels) {
	case 1:  //mono
		inputInfo->channel_number_index = 0;
		break;
	case 2:  //stereo
		inputInfo->channel_number_index = 1;
		break;
	case 6:  //5.1
		inputInfo->channel_number_index = 2;
		break;
	case 8:  //7.1
		inputInfo->channel_number_index = 3;
		break;
	case 10:
		break;
	case 12:  //10.2
		inputInfo->channel_number_index = 4;
		break;
	case 24:  //22.2
		inputInfo->channel_number_index = 5;
		break;
	case 4:
		inputInfo->channel_number_index = 6;
		break;
	case 16: //16ch 3rd HOA, shumin.xu 20210510
		inputInfo->channel_number_index = 11;
		break;
	default:
		fprintf(stderr, "(%s):  (%d) encoding has not been supported yet!", __FUNCTION__, inputInfo->nChannels);
		return -1;
	}

	//head
	if (inputInfo->headflag == 2)  //chenhan 20180328
	{
		if (inputInfo->nChannels == 8)
			inputInfo->channel_number_index = 7;
		if (inputInfo->nChannels == 10)
			inputInfo->channel_number_index = 9;
	}
	else if (inputInfo->headflag == 4)
	{
		if (inputInfo->nChannels == 10)
			inputInfo->channel_number_index = 8;
		if (inputInfo->nChannels == 12)
			inputInfo->channel_number_index = 10;
	}

	if(inputInfo->nChannels >= 5)
	{
		for(i =0; i< 5; i++)
			if(inputInfo->bitRate >= rate_mapping_44_multi[i])
				stAvs2Enc->bitRateIndex = i;
	}

	//	inputchannelnum = numOfChannels;

	
	//decide ps and core bitrates
	if(numOfChannels == 1)
	{
		usePS = 0;
	}
	else if(numOfChannels >= 2)
	{
		//this version do not support BWE now
		if(bitrate_init(&stAvs2Enc->srateInfo, inputInfo->bitRate/1000, inputInfo->channel_number_index))
		{
			usePS = 1;
			mcr_init(&stAvs2Enc->mcr_info, stAvs2Enc->srateInfo.mcr_brate[0]);
		}
		else
		{
			fprintf(stderr, "Error support the bitrate.\n");
			exit(EXIT_FAILURE);	
		}

	}

	// added by lumin 2014.12.18
	if(numOfChannels == 1) //mono
		*cpe_config = 0;
	else if(numOfChannels >= 2)  //stereo
		*cpe_config = stAvs2Enc->srateInfo.couple_config;

	//if((usePS==1)&&(stAvs2Enc->srateInfo.mcr_brate[0]<12))
	//{
		//useSuperMode = 1;
	//}
	////////////////////////
    /* set up basic parameters for avs codec */
	InitDefaultConfig(&stAvs2Enc->config);

	nChannelsCore = 1;

	if( ((inputInfo->nChannels == 1) && (inputInfo->bitRate <= 48000/*32000*/)) 
		|| ((inputInfo->nChannels >= 2) && (inputInfo->bitRate <= 48000/*64000*/)) 
		|| ((inputInfo->nChannels >= 5) && (inputInfo->bitRate <= 144000/*128000*/))
		|| ((inputInfo->nChannels >= 7) && (inputInfo->bitRate <= 192000))
		|| ((inputInfo->nChannels >= 9) && (inputInfo->bitRate <= 216000/*256000*/))
		|| ((inputInfo->nChannels >= 11) && (inputInfo->bitRate <= 240000))
		|| ((inputInfo->nChannels >= 13) && (inputInfo->bitRate <= 320000)))
	{
		useBWE = 1;
	}
	if((*PCAGroupmodeHeader!=0xFF)&&(inputInfo->nChannels >2)&&((inputInfo->bitRate <= 192000)))
		useBWE = 1;



	stAvs2Enc->bDoUpsample = 0;
	if (inputInfo->sampleRate == 16000) 
	{
		stAvs2Enc->bDoUpsample = 1;
		inputInfo->sampleRate = 32000;
		bDingleRate = 1;
	}

	sampleRateCore = inputInfo->sampleRate;


	/* set IIR 2:1 downsampling */
	stAvs2Enc->bDoIIR2Downsample = 0; 
	if(useBWE)
		stAvs2Enc->bDoIIR2Downsample = (stAvs2Enc->bDoUpsample) ? 0 : 1;

	/* set up 1:2 upsampling */
    if (stAvs2Enc->bDoUpsample) 
	{
		if (inputInfo->nChannels>1) 
		{
		  fprintf( stderr, "\n Stereo @ 16kHz input sample rate is not supported\n");
		  return -1;
		}
		for(cid = 0; cid < MAX_ALLCHANNEL; cid++)
		{
			InitIIR21_Resampler(&(IIR21_reSampler[cid]));
			InitIIR21_Resampler(&(IIR21_bweSampler[cid/2]));
		}

		if(useParametricStereo)
		{
		  writeOffset   += AVS2ENC_BLOCKSIZE;

		  upsampleReadOffset  = writeOffset;
		  envWriteOffset  = envReadOffset;
		}
		else
		{
		  writeOffset        += AVS2ENC_BLOCKSIZE;

		  coreReadOffset      = writeOffset;

		  upsampleReadOffset  = writeOffset - (((INPUT_DELAY-IIR21_reSampler[0].delay) >> 1));

		  envWriteOffset      = ((INPUT_DELAY-IIR21_reSampler[0].delay) &  0x1);

		  envReadOffset       = 0;
		}	
	}
	else
	{
		/* set up 2:1 downsampling */
		if (stAvs2Enc->bDoIIR2Downsample)
		{
			for(cid = 0; cid < MAX_ALLCHANNEL; cid++)
			{
				InitIIR21_Resampler(&(IIR21_reSampler[cid]));
				InitIIR21_Resampler(&(IIR21_bweSampler[cid/2]));

				writeOffset += IIR21_reSampler[cid].delay;
			}
	    }
	}



	
	/* set up encoder */
	if (sampleRateCore < 32000)
	{
	    return 0;
	}


	sampleRateCore = sampleRateCore/2;

	stAvs2Enc->config.sampleRate = sampleRateCore;
	
	//maximum pcm value
	bitsPerSample = inputInfo->bitsPerSample;
	Maxpcmvalue = 1<<(inputInfo->bitsPerSample-1);

	//channel information
	if(inputInfo->nChannels==1)
	{
		encoder_mode = &CoupleChannelTable[0];
	}
	// added by lumin 2014.11.21
	else if(inputInfo->nChannels>=2)
	{
		encoder_mode = &CoupleChannelTable[stAvs2Enc->srateInfo.couple_config];
	}
	memcpy(chIndex, encoder_mode->ele_id, MAX_ALLCHANNEL * sizeof(int));
	elementnum =  encoder_mode->numofele;
	stAvs2Enc->elementnum = elementnum;
	stAvs2Enc->config_idx = 0;

	cid = 0;
	for(i = 0; i < elementnum; i++)
	{
		  elementencode_info[i].elType = encoder_mode->idType[i];

		  switch(elementencode_info[i].elType){

			case ID_SCE:      /* single channel */
				elementencode_info[i].nChannelsInEl = 1;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid++;				
			break;

			case ID_CPE_F:      /* channel pair */
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 2;				
			break;
			
			case ID_CPE_L:      /* channel pair */
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 2;				
			break;
			
			//shumin.xu 20210105
			case ID_CPE_H:      /* channel pair */
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 2;
			break;

			case ID_LFE:     /*LFE channel */
				elementencode_info[i].nChannelsInEl = 1;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid++;
			break;
			
			case ID_PCA2:     /*PCA2 channel */
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 2;
			break;

			case ID_PCA4:     /*PCA4 channel */
				elementencode_info[i].nChannelsInEl = 4;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 4;
			break;

			case ID_PCA6:     /*PCA6 channel */
				elementencode_info[i].nChannelsInEl = 6;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 6;
			break;
		  }

	}
	for (i = elementnum; i < elementnum + fill_element_num; i++)
	{
		elementencode_info[i].elType = ID_FIL;
	}

	for(i = 0; i < elementnum; i++)
	{
		stAvs2Enc->psData[i] = (PS_DATA *)malloc(sizeof(PS_DATA));
		memset(stAvs2Enc->psData[i], 0, sizeof(PS_DATA));

		stAvs2Enc->psBweData[i] = (PS_BWE_DATA *)malloc(sizeof(PS_BWE_DATA));
		memset(stAvs2Enc->psBweData[i], 0, sizeof(PS_BWE_DATA));
		stAvs2Enc->psBweData[i]->bandwidth = AVS2ENC_BLOCKSIZE;
		
		stAvs2Enc->psData_pre[i] = (PS_DATA *)malloc(sizeof(PS_DATA));
		memset(stAvs2Enc->psData_pre[i], 0, sizeof(PS_DATA));

		stAvs2Enc->psBweData_pre[i] = (PS_BWE_DATA *)malloc(sizeof(PS_BWE_DATA));
		memset(stAvs2Enc->psBweData_pre[i], 0, sizeof(PS_BWE_DATA));
        stAvs2Enc->psBweData_pre[i]->bandwidth = AVS2ENC_BLOCKSIZE;
	}

   // tianlai channel inf
	//encoder_mode_tianlai51 =&MC51ModeTianlai[numOfChannels-1];
	if(numOfChannels<=2)
		encoder_mode_tianlai51 =&PCAGroupmodeHeaderTable[numOfChannels-1];
	else if(numOfChannels==6)
			encoder_mode_tianlai51 =&PCAGroupmodeHeaderTable[2];
	else if(numOfChannels==8)
			encoder_mode_tianlai51 =&PCAGroupmodeHeaderTable[16];
	//shumin.xu 20210510
	else if (numOfChannels == 4)
		encoder_mode_tianlai51 = NULL;
	else if (numOfChannels <= 16)
		encoder_mode_tianlai51 = NULL;
	else
	{
		printf("not support %d channel",numOfChannels);
		return 1;
	}

	if (encoder_mode_tianlai51 != NULL)  //shumin.xu 20210105
	{
		elementnum_tianlai51 = encoder_mode_tianlai51->numofele;
		stAvs2Enc->elementnum_tianlai51 = elementnum_tianlai51;

		cid = 0;
		for (i = 0; i < elementnum_tianlai51; i++)
		{
			elementencode_info_tianlai51[i].elType = encoder_mode_tianlai51->idType[i];

			switch (elementencode_info_tianlai51[i].elType) {

			case ID_SCE:      /* single channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 1;

				cid++;
				break;

			case ID_CPE_F:      /* channel pair */
				elementencode_info_tianlai51[i].nChannelsInEl = 2;

				cid++;
				cid++;
				break;

			case ID_LFE:     /*LFE channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 1;

				cid++;
				break;

			case ID_PCA2:     /*PCA2 channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 2;

				cid++;
				cid++;
				break;

			case ID_PCA4:     /*PCA2 channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 4;

				cid += 4;
				break;

			case ID_PCA6:     /*PCA2 channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 6;

				cid += 6;
				break;
			}

		}
		for (i = elementnum_tianlai51; i < elementnum_tianlai51 + fill_element_num; i++)
		{
			elementencode_info_tianlai51[i].elType = ID_FIL;
		}

	}


	//0~13各级ci对应的码表的设置
	//ci_settable_init(ci_table, config);
	ci_settable_init0(ci_table, stAvs2Enc->config, stAvs2Enc->bDoIIR2Downsample);

	//initialize every encoder (lfEncset[])
	k = 0;
	for(eid = 0; eid < elementnum; eid++)
	{
		int iid;
		for(iid = 0; iid < elementencode_info[eid].nChannelsInEl; iid++)
		{
			if((numOfChannels==1)||(numOfChannels==2&&bEncodeMono))
				coreBitrate = inputInfo->bitRate;
			else
			{
				coreBitrate = stAvs2Enc->srateInfo.core_brate[k]*1000;
			}
			if(coreBitrate == 0) 
			{
				if (useBWE)
					stAvs2Enc->config.bitRate = stAvs2Enc->srateInfo.core_brate[k-1]*1000;
				else
					stAvs2Enc->config.bitRate = 48000;
			}
			else
				stAvs2Enc->config.bitRate = coreBitrate;

			stAvs2Enc->config.nChannelsIn = 1;
			stAvs2Enc->config.nChannelsOut = 1;

			error = LFEncOpen(&stAvs2Enc->lfEncset[k], &stAvs2Enc->config, useBWE);

			tianlai_info_init(&(stAvs2Enc->lfEncset[k]->vi));

			stAvs2Enc->lfEncset[k]->vi.channels = stAvs2Enc->config.nChannelsIn;
			stAvs2Enc->lfEncset[k]->vi.rate = stAvs2Enc->config.sampleRate;

			stAvs2Enc->lfEncset[k]->elInfo.elType = elementencode_info[eid].elType;
			stAvs2Enc->lfEncset[k]->elInfo.nChannelsInEl = elementencode_info[eid].nChannelsInEl;

			gettableindex(1, stAvs2Enc->config.sampleRate*2, -1, stAvs2Enc->config.bitRate, -1, &stAvs2Enc->basesetting);

			ci_set(&(stAvs2Enc->lfEncset[k]->vi), stAvs2Enc->basesetting);
			tianlai_encode_setup_setting(&(stAvs2Enc->lfEncset[k]->vi), stAvs2Enc->lfEncset[k]->vi.channels, stAvs2Enc->config.sampleRate*2);

			lowpass_kHz = freqbeginend_setting(&(stAvs2Enc->lfEncset[k]->vi), elementencode_info[eid].nChannelsInEl, stAvs2Enc->bitRateIndex, 0);
			reset_bandWidth(stAvs2Enc->config.sampleRate, /*basesetting*/stAvs2Enc->config_idx, lowpass_kHz, &(stAvs2Enc->lfEncset[k]->config.bandWidth),useBWE);

			tianlai_analysis_init(&(stAvs2Enc->lfEncset[k]->vd), &(stAvs2Enc->lfEncset[k]->vi), useBWE);

			_vds_flr_res_set(&(stAvs2Enc->lfEncset[k]->vd), &(stAvs2Enc->lfEncset[k]->vi), min(12,stAvs2Enc->basesetting+3));



			tianlai_block_init(&(stAvs2Enc->lfEncset[k]->vd), &(stAvs2Enc->lfEncset[k]->vb));

			stAvs2Enc->lfEncset[k]->vb.lW = 1;
			stAvs2Enc->lfEncset[k]->vb.W = 1 ;
			stAvs2Enc->lfEncset[k]->vb.nW = 1 ;

			Avs2EncMDFTfunOpen();
			Avs2BweMDFTOpen((unsigned int*)&(stAvs2Enc->lfEncset[k]->st1_in), (unsigned int*)&(stAvs2Enc->lfEncset[k]->st_common)); 

			if(useBWE)
			{
				Avs2BweEncoderOpen((unsigned int*)&(stAvs2Enc->lfEncset[k]->st2_in), stAvs2Enc->config.bitRate, sampleRateCore, nChannelsCore, &(stAvs2Enc->config.bandWidth),&stAvs2Enc->config_idx);
			}
			
	
			stAvs2Enc->lfEncset[k]->elInfo.elType  = elementencode_info[eid].elType;

			///wu add 2015.2.6
			lowpass_kHz = freqbeginend_setting(&(stAvs2Enc->lfEncset[k]->vi), elementencode_info[eid].nChannelsInEl, stAvs2Enc->bitRateIndex, 0);
			reset_bandWidth(stAvs2Enc->config.sampleRate, /*basesetting*/stAvs2Enc->config_idx, lowpass_kHz, &(stAvs2Enc->lfEncset[k]->config.bandWidth),useBWE);

			if(stAvs2Enc->config.sampleRate > 32000)
			init_tns_configuration(stAvs2Enc->config.bitRate, stAvs2Enc->config.sampleRate, stAvs2Enc->config.nChannelsIn, stAvs2Enc->lfEncset[k]->config.bandWidth, &(stAvs2Enc->lfEncset[k]->vb.tnsConf[0]), 1);

			if(coreBitrate > 0)
				reset_bitrate_copy0(&(stAvs2Enc->lfEncset[k]->vd), coreBitrate);

	        //memset(stAvs2Enc->time_dmix[k],  0, MAX_ALLCHANNEL*(TOTAL_DELAY*2+DOWN_DELAY)*sizeof(float)*2);

			k++;
		}

	}
	

	//getbitpershort
	stAvs2Enc->PCAcorebitpershort = PCAcorebitpershort;
	stAvs2Enc->PCAcorebitpershort = getbitpershort(&(stAvs2Enc->lfEncset[0]->vd));
	//重新配置目标码率


	lf_winseqpre[19]=0;
	/* set up input samples block size feed */
	
		if(useBWE)
			inSamples = AVS2ENC_BLOCKSIZE * inputInfo->nChannels * 2;
		else
			inSamples = AVS2ENC_BLOCKSIZE * inputInfo->nChannels;
		
		/*if(useSuperMode)
			inSamples *= 2;*/

        if (stAvs2Enc->bDoUpsample) 
		{
           inSamples =  inSamples>>1;
		}


	//添加声道对中核心编码器的配置和初始化
	//InitDefaultConfig(&config);
	stAvs2Enc->config.nChannelsIn = nChannelsCore;
	stAvs2Enc->config.nChannelsOut = nChannelsCore;
	stAvs2Enc->config.sampleRate = sampleRateCore;


	stAvs2Enc->useSuperMode = useSuperMode;
	stAvs2Enc->useBWE = useBWE;
	stAvs2Enc->bEncodeMono = bEncodeMono;

	stAvs2Enc->writeOffset = writeOffset;
	stAvs2Enc->coreReadOffset = coreReadOffset;
	stAvs2Enc->envReadOffset =envReadOffset;
	stAvs2Enc->envWriteOffset = envWriteOffset;
	stAvs2Enc->upsampleReadOffset = upsampleReadOffset;

	*inLen = inSamples;

	*phstAvs2Enc = stAvs2Enc;


	return 0;	
}

void close_avs2_general_encoder(HANDLE_STAvs2Enc *phstAvs2Enc, int numOfChannels)
{
	int cid;

	HANDLE_STAvs2Enc stAvs2Enc = *phstAvs2Enc;

	*phstAvs2Enc = NULL;

	for(cid=0;cid<numOfChannels;cid++)
	{
		Avs2BweMDFTClose((unsigned int*)&(stAvs2Enc->lfEncset[cid]->st1_in), (unsigned int*)&(stAvs2Enc->lfEncset[cid]->st_common));
		if(stAvs2Enc->useBWE)
		{	
			Avs2BweEncoderClose((unsigned int*)&(stAvs2Enc->lfEncset[cid]->st2_in));
		}

	}


}


int general_encoder_frame(/*int argc, char *argv[]*/HANDLE_STAvs2Enc stAvs2Enc,char *TimeDataPcmBuffer, int inSamples, ChanInfo *inputInfo, int *OutBytes, unsigned char sampleData[],int fill_element_num)
{
	int i,k;
	int frameOffset = AVS2ENC_BLOCKSIZE/2 + AVS2ENC_BLOCKSIZE/16;
	avs2audiopack_buffer *data_mcr[MAX_ALLCHANNEL/2];

	short /*numOfChannels, */bitsPerSample;
    int coreReadOffset = 0;
    int TimeDataPcm[AVS2ENC_BLOCKSIZE*2*MAX_ALLCHANNEL];
    int nSamplesPerChannel;
	int numSamplesRead;
	unsigned char ancDataBytes[MAX_ALLCHANNEL][MAX_PAYLOAD_SIZE];
	unsigned int numAncDataBytes[MAX_ALLCHANNEL];
	int cid = 0;
	double /*basesetting, */channelbasesetting;
//
	int eid;
//	int brIndex, bitRateIndex;
	int coreBitrate;
	double lowpass_kHz;
	float mdftSpectrum[MAX_ALLCHANNEL][FRAME_LEN_LONG * 2];
	float Mdftout[4096*2];
	char codectype=0;// Tsinghua or Tianlai
	unsigned int PCAGroupmode =0;

	//maximum pcm value
	bitsPerSample = inputInfo->bitsPerSample;  
	Maxpcmvalue = 1<<(inputInfo->bitsPerSample-1);
	

	memset(sampleData, 0, (6144/8));

	////////////////////////////////////////////////////
	{	int n;
	    int i,  outSamples, numOutBytes = 0;
		int numMcrBytes = 0;
	
		/* File input read, resample and downmix */
		//////////////////////   File input read, resample and downmix start//////////////////////////
	
		{
			int pcmindex;

			/* no resampling prior to encoding required */
			/* read from file */
			numSamplesRead = inSamples;

			for(pcmindex=0; pcmindex<inSamples; pcmindex++)
			{
				int pcmtmp;
				short pcmshorttmp;
				char *ptchar = &pcmtmp;

				if(inputInfo->bitsPerSample==24)
				{
					memcpy(ptchar,TimeDataPcmBuffer +pcmindex*(inputInfo->bitsPerSample/8),3);
					pcmtmp =(pcmtmp<<8);
					pcmtmp =(pcmtmp/256);
				}else if(inputInfo->bitsPerSample==16)
				{
					ptchar = &pcmshorttmp;
					memcpy(ptchar,TimeDataPcmBuffer +pcmindex*(inputInfo->bitsPerSample/8),2);
					pcmtmp = pcmshorttmp; 
				}else if(inputInfo->bitsPerSample==32)
				{
					memcpy(ptchar,TimeDataPcmBuffer +pcmindex*(inputInfo->bitsPerSample/8),4);
					pcmtmp =(pcmtmp);
				}
				TimeDataPcm[pcmindex] =pcmtmp; 
			}

			switch (inputInfo->nChannels) 
			{
				case 1:
				  nSamplesPerChannel = numSamplesRead;
				  break;
				case 2:
				  nSamplesPerChannel = numSamplesRead >> 1;
				  break;
				default:
				  nSamplesPerChannel = numSamplesRead / inputInfo->nChannels;
		    }

	
			for(k = 0; k < inputInfo->nChannels; k++)
			{
				int i;
				for(i = 0; i < numSamplesRead/inputInfo->nChannels; i++)
					inputBuffer[k][stAvs2Enc->writeOffset+i] = (float) TimeDataPcm[i*inputInfo->nChannels+k];

				/* copy from short to float input buffer, downmix stereo input signal to mono, reordering necessary since the encoder takes interleaved data */
				if((inputInfo->nChannels==2) && stAvs2Enc->bEncodeMono)
				{
					int i;
					for(i = 0; i < numSamplesRead/2; i++)	
					   inputBuffer[k][stAvs2Enc->writeOffset+i] = ((float)TimeDataPcm[2*i] + (float)TimeDataPcm[2*i+1])*0.5f;   
				}
			}

		}	//end (bDoIIR32Resample) end
		//////////////////////   File input read, resample and downmix end//////////////////////////

		
		//////////////////////    mode select, mdct and BWE_encoding (start)//////////////////////////
		cid = 0;
        k = 0;
		for(eid = 0; eid < stAvs2Enc->elementnum; eid++)
		{
			switch(elementencode_info[eid].elType) 
			{

			case ID_SCE:      /* single channel */			
				{
				int ch;
				struct AVS2_ENCODER *lfEnc = stAvs2Enc->lfEncset[cid];

				numAncDataBytes[cid] = 0;
				/* 2:1 downsampling for core */
				if (stAvs2Enc->bDoIIR2Downsample) 
				{
					//select the Low frequency's MDCT/MDFT mode lf_winseq
					Avs2LFmodeselect(lfEnc->st1_in, inputBuffer[cid]+0+stAvs2Enc->writeOffset, lfEnc->config.bitRate,&(lfEnc->lf_winseq_ptr),nSamplesPerChannel,Maxpcmvalue);
					memcpy(lfEnc->lf_winseq,lfEnc->lf_winseq_ptr,20*4);
				
					if(stAvs2Enc->useBWE)
					{
						//MDFT,high frequency 
						Avs2BweMDFTTransform(lfEnc->st1_in, lfEnc->st_common,  0/*lfEnc->config.nChannelsIn*/, lfEnc->config.bitRate,&(lfEnc->blocktypeB));						
					}

					//update the mdft status
					Avs2LFMDFTupdate(lfEnc->st1_in);
					
					IIR21_Downsample(&(IIR21_reSampler[cid]), inputBuffer[cid]+stAvs2Enc->writeOffset, nSamplesPerChannel, 1, inputBuffer[cid], &outSamples, 1);

				}
				else
				{
					//select MDCT/MDFT mode lf_winseq
					Avs2modeselect0(lfEnc->st1_in, inputBuffer[cid]+coreReadOffset, &(lfEnc->lf_winseq_ptr), nSamplesPerChannel,Maxpcmvalue);
					memcpy(lfEnc->lf_winseq,lfEnc->lf_winseq_ptr,20*4);
					
					//update the mdft status
					Avs2MDFTupdate(lfEnc->st1_in);
				}


				/* encode one hf BWE frame */
				if(stAvs2Enc->useBWE)
				{
					Avs2BweEncoder(ancDataBytes[cid], &numAncDataBytes[cid], lfEnc->st2_in, lfEnc->st_common, lfEnc->config.bitRate,bitsPerSample);
				}
					
				for(i = 0; i < CORE_FRAMESIZE; i++)
				    lfEnc->pcm_buffer0[CORE_FRAMESIZE+i] = *(inputBuffer[cid]+coreReadOffset+i)/Maxpcmvalue;

			    mdft_lowpassframeblock_multi(lfEnc->pcm_buffer0-frameOffset, lfEnc->lf_winseq, Mdftout, CORE_FRAMESIZE/2);

			    for(i = 0; i < CORE_FRAMESIZE; i++)
				   mdftSpectrum[cid][i] = Mdftout[i*2] * Maxpcmvalue;
			
			}			
			cid++;				
			break;
			
			
		case ID_PCA2:      /* PCAx channel */			
		case ID_PCA4:
		case ID_PCA6:
		case ID_CPE_F:
        case ID_CPE_L:
		case ID_CPE_H:
			{
				int iid;
				int sid = cid;
				for(iid = 0; iid < elementencode_info[eid].nChannelsInEl; iid++)
				{
					int ch;
					struct AVS2_ENCODER *lfEnc = stAvs2Enc->lfEncset[cid];
					numAncDataBytes[cid + iid] = 0;
					/* 2:1 downsampling for core */
					if(stAvs2Enc->bDoIIR2Downsample) 
					{
						//select the Low frequency's MDCT/MDFT mode lf_winseq
						Avs2LFmodeselect(lfEnc->st1_in, inputBuffer[cid]+stAvs2Enc->writeOffset, lfEnc->config.bitRate,&(lfEnc->lf_winseq_ptr),nSamplesPerChannel,Maxpcmvalue);
						
						memcpy(lfEnc->lf_winseq,lfEnc->lf_winseq_ptr,20*4);

						if(iid != 0)
						{
							StAvs2BweCommon *pstBweCommon = (StAvs2BweCommon *)lfEnc->st_common; 
							StAvs2BweCommon *pstBweCommon0 = (StAvs2BweCommon *)stAvs2Enc->lfEncset[sid]->st_common; 
							
							memcpy(lfEnc->lf_winseq,stAvs2Enc->lfEncset[sid]->lf_winseq,20*4);
							
							pstBweCommon->nSeqmode = pstBweCommon0->nSeqmode;
							pstBweCommon->nGroupmode = pstBweCommon0->nGroupmode;
						}
              				
						if(stAvs2Enc->useBWE)
						{
							//MDFT,high frequency 
							Avs2BweMDFTTransform(lfEnc->st1_in, lfEnc->st_common,  iid, lfEnc->config.bitRate, &(lfEnc->blocktypeB));
						}

						if (cid == 3 && inputInfo->channel_number_index != 6)
						{
							ZeroLFEHighFreq_BWE(lfEnc->st_common);
						}
						//update the mdft status
						Avs2LFMDFTupdate(lfEnc->st1_in);
						
						IIR21_Downsample(&(IIR21_reSampler[cid]), inputBuffer[cid]+stAvs2Enc->writeOffset, nSamplesPerChannel, 1, inputBuffer[cid], &outSamples, 1);

					}
					else
					{
						//select  MDCT/MDFT mode lf_winseq
						Avs2modeselect0(lfEnc->st1_in, inputBuffer[cid]+coreReadOffset, &(lfEnc->lf_winseq_ptr), nSamplesPerChannel,Maxpcmvalue);

					
						memcpy(lfEnc->lf_winseq, lfEnc->lf_winseq_ptr, 20*4);
						
						if(iid != 0)
							memcpy(lfEnc->lf_winseq, stAvs2Enc->lfEncset[sid]->lf_winseq, 20*4);

					

						//update the mdft status
						Avs2MDFTupdate(lfEnc->st1_in);
					}
					
					// encode one hf BWE frame
					if(stAvs2Enc->useBWE)
					{
						//Avs2BweEncoder(ancDataBytes[cid], &numAncDataBytes[cid], lfEnc->st2_in, lfEnc->st_common, lfEnc->config.bitRate,bitsPerSample);
					}
				
					for(i = 0; i < CORE_FRAMESIZE; i++)
						lfEnc->pcm_buffer0[CORE_FRAMESIZE+i] = *(inputBuffer[cid]+stAvs2Enc->coreReadOffset+i)/Maxpcmvalue;

					mdft_lowpassframeblock_multi(lfEnc->pcm_buffer0-frameOffset, lfEnc->lf_winseq, Mdftout, CORE_FRAMESIZE/2);

					if (cid == 3 && inputInfo->channel_number_index != 6)
					{
						ZeroLFEHighFreq(lfEnc, &Mdftout[0]);
					}

					for(i = 0; i < CORE_FRAMESIZE; i++)
						mdftSpectrum[cid][i] = Mdftout[i*2] * Maxpcmvalue;
	
					cid++;	
				}//for(iid=0;iid<elementencode_info[eid].nChannelsInEl;iid++)
				
				if(stAvs2Enc->useBWE)
				{
					StAvs2BweCommon *pstBweCommon; 

					pstBweCommon = (StAvs2BweCommon *)(stAvs2Enc->lfEncset[sid]->st_common);
					stAvs2Enc->psBweData[k]->Seqmode = pstBweCommon->nSeqmode;
					stAvs2Enc->psBweData[k]->Groupmode = pstBweCommon->nGroupmode;

					memcpy(stAvs2Enc->psBweData[k]->left_data, pstBweCommon->mdft4096block_complex[0], (CORE_FRAMESIZE*4+2048)*sizeof(float));

					pstBweCommon = (StAvs2BweCommon *)(stAvs2Enc->lfEncset[sid+1]->st_common); 
					memcpy(stAvs2Enc->psBweData[k]->right_data, pstBweCommon->mdft4096block_complex[0], (CORE_FRAMESIZE*4+2048)*sizeof(float));

					data_mcr[eid] = (avs2audiopack_buffer *)calloc(1, sizeof(avs2audiopack_buffer));
					numMcrBytes = MCR_BWE_Encoder(inputInfo->bitRate, downmixBuffer[sid] + stAvs2Enc->writeOffset, data_mcr[eid], stAvs2Enc->psBweData[k], stAvs2Enc->mcr_info, stAvs2Enc->psBweData_pre[k]);

					{
						float temp[CORE_FRAMESIZE * 4 + 2048];
						pstBweCommon = (StAvs2BweCommon *)(stAvs2Enc->lfEncset[sid]->st_common);
						memcpy(pstBweCommon->mdft4096block_complex[0], stAvs2Enc->psBweData[k]->sum_data, (CORE_FRAMESIZE * 4 + 2048)*sizeof(float));
						memcpy(&temp[0], pstBweCommon->mdft4096block_2048complex[0], (CORE_FRAMESIZE * 4 + 2048)*sizeof(float));
						//				    memcpy(pstBweCommon->mdft4096block_2048complex[0], stAvs2Enc->psBweData[k]->sum_data, (CORE_FRAMESIZE * 4+2048) * sizeof(float));
						pstBweCommon = (StAvs2BweCommon *)(stAvs2Enc->lfEncset[sid + 1]->st_common);
						memcpy(pstBweCommon->mdft4096block_complex[0], stAvs2Enc->psBweData[k]->dif_data, (CORE_FRAMESIZE * 4 + 2048)*sizeof(float));
						//				    memcpy(pstBweCommon->mdft4096block_2048complex[0], stAvs2Enc->psBweData[k]->dif_data, (CORE_FRAMESIZE * 4+2048) * sizeof(float));
						for (i = 0; i < CORE_FRAMESIZE * 4 + 2048; i++)
						{
							temp[i] += pstBweCommon->mdft4096block_complex[0][i];
							temp[i] /= 2;
						}
						pstBweCommon = (StAvs2BweCommon *)(stAvs2Enc->lfEncset[sid]->st_common);
						memcpy(pstBweCommon->mdft4096block_2048complex[0], &temp[0], (CORE_FRAMESIZE * 4 + 2048) * sizeof(float));
					}

//					{ //added in 2015.09.10 (to ignore the influence of the core mono coding)
//						int t;		
//						FILE *fp0 = fopen("MdftSpectrum_1.txt","a");
//						for(t=0; t<CORE_FRAMESIZE*4; t++)
//							fprintf(fp0, "%f,", psBweData[k]->sum_data[t]);	
//						fprintf(fp0,"\n");
//						fclose(fp0);
//					}
				
					IIR21_Downsample(&(IIR21_bweSampler[k]), downmixBuffer[sid] + stAvs2Enc->writeOffset, nSamplesPerChannel, 1, downmixBuffer[sid], &outSamples, 1);

					Avs2BweEncoder(ancDataBytes[sid],  &numAncDataBytes[sid], stAvs2Enc->lfEncset[sid]->st2_in, stAvs2Enc->lfEncset[sid]->st_common, stAvs2Enc->lfEncset[sid]->config.bitRate,bitsPerSample);

					for(i = 0; i < CORE_FRAMESIZE; i++)
						stAvs2Enc->lfEncset[sid]->pcm_buffer0[CORE_FRAMESIZE - frameOffset + i] = *(downmixBuffer[sid] + coreReadOffset + i) / Maxpcmvalue;

					mdft_lowpassframeblock_multi(stAvs2Enc->lfEncset[sid]->pcm_buffer0-frameOffset, stAvs2Enc->lfEncset[sid]->lf_winseq, Mdftout, CORE_FRAMESIZE/2);
					
					for(i = 0; i < CORE_FRAMESIZE; i++)
						mdftSpectrum[sid][i] = Mdftout[i*2] * Maxpcmvalue;

				}
			}
			k++;
			
			break;		  

		}//switch (elementencode_info[eid].elType) {

		
		}//for( eid=0;eid<elementnum;eid++)
		//////////////////////    mode select and mdct (end)//////////////////////////


		///////////////////copy inputdata for Tsinghua superframe MCR//////////////////////////
		cid = 0;
		for(k = 0; k < stAvs2Enc->elementnum; k++)
		{
			memcpy(&stAvs2Enc->time_dmix[cid][0], &stAvs2Enc->time_dmix[cid][CORE_FRAMESIZE*2], CORE_FRAMESIZE*2*sizeof(float));

			if(elementencode_info[k].nChannelsInEl == 2)
			{
				memcpy(&stAvs2Enc->time_dmix[cid+1][0], &stAvs2Enc->time_dmix[cid+1][CORE_FRAMESIZE*2], CORE_FRAMESIZE*2*sizeof(float));
				if(stAvs2Enc->useBWE)
				{
					for(i = 0; i < (numSamplesRead/inputInfo->nChannels); i++)
					{
						stAvs2Enc->time_dmix[cid][CORE_FRAMESIZE*2-frameOffset+i] = downmixBuffer[cid][coreReadOffset+i]/Maxpcmvalue;
						stAvs2Enc->time_dmix[cid+1][CORE_FRAMESIZE*2-frameOffset+i] = downmixBuffer[cid+1][coreReadOffset+i]/Maxpcmvalue;
					}
				}
				else
				{
				  for(i = 0; i < (numSamplesRead/inputInfo->nChannels); i++)
				  {
					  stAvs2Enc->time_dmix[cid][CORE_FRAMESIZE*2+i] = (inputBuffer[cid][coreReadOffset+i]+inputBuffer[cid+1][coreReadOffset+i])/Maxpcmvalue * 0.5f;
					  stAvs2Enc->time_dmix[cid+1][CORE_FRAMESIZE*2+i] = (inputBuffer[cid][coreReadOffset+i]-inputBuffer[cid+1][coreReadOffset+i])/Maxpcmvalue * 0.5f;
				  }
			  }
				cid++;
				cid++;
			}
			else
			{
				for(i = 0; i < (numSamplesRead/inputInfo->nChannels); i++)
				{
					stAvs2Enc->time_dmix[cid][CORE_FRAMESIZE*2+i] = inputBuffer[cid][coreReadOffset+i]/Maxpcmvalue;
				}
				cid++;
			}
		}


		///////////codectype, 编码方式选择，tsinghua_MCR (0) or tianlai_PCA (1)///////////
		PCAGroupmode = 0;
		if (inputInfo->nChannels == 6)
			codectype = 0;// codectypeselect(stAvs2Enc->lfEncset, inputInfo->nChannels, mdftSpectrum, &PCAGroupmode);//1;//(codectype+1)%2;//rand()%2;
		else
			codectype = 0;
		if(inputInfo->nChannels==1)
			codectype = 1;
		if(inputInfo->nChannels==2)
			codectype = 0;

	  

		////////////////////////////////////////////////
	   if(codectype == 0)
	   {
		   avs2audiopack_buffer *opb;
		   		opb = calloc(1,sizeof(avs2audiopack_buffer));
			avs2audiopack_writeinit(opb);

			avs2audiopack_write(opb, codectype, 1);

			avs2audiopack_write(opb, stAvs2Enc->useBWE, 1);
			avs2audiopack_write(opb, fill_element_num, 1);

		   cid = 0;
		   for(k = 0; k < stAvs2Enc->elementnum + fill_element_num; k++)
		   {
			   if(elementencode_info[k].elType == ID_CPE_F
				  ||elementencode_info[k].elType == ID_CPE_L
				  ||elementencode_info[k].elType == ID_CPE_H)
			   {
			     if(stAvs2Enc->useBWE==0)
				   {
					   stAvs2Enc->psData[k]->bandwidth = stAvs2Enc->lfEncset[cid]->config.bandWidth;
					   memcpy(stAvs2Enc->psData[k]->winseq, stAvs2Enc->lfEncset[cid]->lf_winseq, 20*sizeof(int));
					   memcpy(stAvs2Enc->psData[k]->left_data, &mdftSpectrum[cid][0], CORE_FRAMESIZE*sizeof(float));
					   memcpy(stAvs2Enc->psData[k]->right_data, &mdftSpectrum[cid+1][0], CORE_FRAMESIZE*sizeof(float));

					   data_mcr[k] = (avs2audiopack_buffer *)calloc(1, sizeof(avs2audiopack_buffer));
					   numMcrBytes = MCR_Encoder(data_mcr[k], stAvs2Enc->psData[k], stAvs2Enc->mcr_info, stAvs2Enc->psData_pre[k]);

					   memcpy(&mdftSpectrum[cid][0], stAvs2Enc->psData[k]->sum_data, CORE_FRAMESIZE*sizeof(float));
					   memcpy(&mdftSpectrum[cid+1][0], stAvs2Enc->psData[k]->dif_data, CORE_FRAMESIZE*sizeof(float));

					   if(stAvs2Enc->psData[k]->sflag==1)
					   {
						   stAvs2Enc->lfEncset[cid]->elInfo.elType = ID_CPE_S;
						   stAvs2Enc->lfEncset[cid+1]->elInfo.elType = ID_CPE_S;
					   }
					   else
					   {
						   stAvs2Enc->lfEncset[cid]->elInfo.elType = ID_CPE_F;
						   stAvs2Enc->lfEncset[cid+1]->elInfo.elType = ID_CPE_F;
					   }
				   }
				   else
				   {
				   	 if(stAvs2Enc->psBweData[k]->sflag==1)
					   {
						   stAvs2Enc->lfEncset[cid]->elInfo.elType = ID_CPE_S;
						   stAvs2Enc->lfEncset[cid+1]->elInfo.elType = ID_CPE_S;
					   }
					   else
					   {
						   stAvs2Enc->lfEncset[cid]->elInfo.elType = ID_CPE_L;
						   stAvs2Enc->lfEncset[cid+1]->elInfo.elType = ID_CPE_L;
					   }
				   }

				   avs2audiopack_write(opb, stAvs2Enc->lfEncset[cid]->elInfo.elType, 4);

				   // write core channel number of each pair element
				   if(stAvs2Enc->srateInfo.core_brate[cid+1]==0)
					   avs2audiopack_write(opb, 0, 1);
				   else
					   avs2audiopack_write(opb, 1, 1);
#if CONSTANT_BITRATE_CONTROL
				   //stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
				   if (1 == 1)//(opencbr)
				   {
					   if (stAvs2Enc->useBWE)
					   {
						   if (k == 0)
							   stAvs2Enc->lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1 + OUTERLOOP_reservedBYTEforHEADER + 2;
						   else
							   stAvs2Enc->lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1;
					   }
					   else
					   {
						   if (k == 0)
							   stAvs2Enc->lfEncset[cid]->numbwebytes = OUTERLOOP_reservedBYTEforHEADER + 2;
						   else
							   stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
					   }
					   if ((fill_element_num > 0) && (k == 0))
						   stAvs2Enc->lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
					   if ((stAvs2Enc->elementnum - 1 == k))
						   stAvs2Enc->lfEncset[cid]->numbwebytes += 1;
					   stAvs2Enc->lfEncset[cid]->numbwebytes -= (stAvs2Enc->srateInfo.mcr_brate[cid] * 1000.0) * inputInfo->inSamples / (inputInfo->nChannels) / (stAvs2Enc->lfEncset[cid]->config.sampleRate * 2) / 8;
					   stAvs2Enc->lfEncset[cid]->numbwebytes += calc_mcr_bits(k, opb, cid, data_mcr[k]) / 8;
				   }
				   else
					   stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
#endif
				   stAvs2Enc->lfEncset[cid]->pcm_buffer = &stAvs2Enc->time_dmix[cid][CORE_FRAMESIZE];
				   Avs2LFEncoder(stAvs2Enc->useBWE, stAvs2Enc->lfEncset[cid], mdftSpectrum[cid],
									ancDataBytes[cid], numAncDataBytes+cid, 
									sampleData, &numOutBytes,k,codectype,opb, Maxpcmvalue);
				  
				   
				   if(stAvs2Enc->srateInfo.core_brate[cid+1]>0)
				   {
#if CONSTANT_BITRATE_CONTROL
					   if (1 == 1)//(opencbr)
					   {
						   if (k == 0)
							   stAvs2Enc->lfEncset[cid + 1]->numbwebytes = (numAncDataBytes[cid + 1] + 1)*(stAvs2Enc->useBWE) + OUTERLOOP_reservedBYTEforHEADER + 2;
						   else
							   stAvs2Enc->lfEncset[cid + 1]->numbwebytes = (numAncDataBytes[cid + 1] + 1)*(stAvs2Enc->useBWE);
						   if ((fill_element_num > 0) && (k == 0))
							   stAvs2Enc->lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
						   if ((stAvs2Enc->elementnum - 1 == k))
							   stAvs2Enc->lfEncset[cid]->numbwebytes += 1;
						   stAvs2Enc->lfEncset[cid]->numbwebytes -= (stAvs2Enc->srateInfo.mcr_brate[cid] * 1000.0) * inputInfo->inSamples / (inputInfo->nChannels) / (stAvs2Enc->lfEncset[cid]->config.sampleRate * 2) / 8;
						   stAvs2Enc->lfEncset[cid]->numbwebytes += calc_mcr_bits(k, opb, cid, data_mcr[k]) / 8;
					   }
					   else
						   stAvs2Enc->lfEncset[cid + 1]->numbwebytes = 0;
#endif

					   stAvs2Enc->lfEncset[cid+1]->pcm_buffer = &stAvs2Enc->time_dmix[cid+1][CORE_FRAMESIZE];

					   Avs2LFEncoder(stAvs2Enc->useBWE, stAvs2Enc->lfEncset[cid+1], mdftSpectrum[cid+1],
									ancDataBytes[cid+1], numAncDataBytes+cid+1, 
									sampleData, &numOutBytes,1,0,opb,Maxpcmvalue);				  
					
				   }
								 
				
					memcpy(stAvs2Enc->lfEncset[cid]->pcm_buffer0-CORE_FRAMESIZE, stAvs2Enc->lfEncset[cid]->pcm_buffer0, CORE_FRAMESIZE*2*sizeof(float));
					memcpy(stAvs2Enc->lfEncset[cid+1]->pcm_buffer0-CORE_FRAMESIZE, stAvs2Enc->lfEncset[cid+1]->pcm_buffer0, CORE_FRAMESIZE*2*sizeof(float));

				 }
			   else if (elementencode_info[k].elType == ID_FIL)
			   {
				   int fill_type = 0;
				   int fill_byte = 0xff;
				   char fill_dft_bits[2048] = { 0 };
				   int leftbytes = inputInfo->bitRate * 1024 / inputInfo->sampleRate / 8 * 2;
				   ////if (objflag)
				   leftbytes = inputInfo->bitRate * 1.0*inputInfo->inSamples / inputInfo->sampleRate / 8 / (inputInfo->nChannels) - OUTERLOOP_reservedBYTEforHEADER - 2;
				   //else
				   //	leftbytes = inputInfo->bitRate * 1.0*inputInfo->inSamples / inputInfo->sampleRate / 8 / (inputInfo->nChannels) - OUTERLOOP_reservedBYTEforHEADER - 2;
				   leftbytes -= 1 + opb->endbyte +1;
				   if (opb->endbit > 0) leftbytes--;
				   fill_byte = leftbytes;
				   if (fill_byte < 0)
				   {
					   printf("fill_byte %d < 0\n", fill_byte);
					   fill_byte = 0;
				   }

				   avs2audiopack_write(opb, 6, 4); //ID_FIL = 6;
				   avs2audiopack_write(opb, fill_type, 4);

				   switch (fill_type)
				   {

				   case 0:
				   {
					   avs2audiopack_write(opb, fill_byte, 16);
					   for (i = 0; i < 8 * fill_byte; i++)
					   {
						   avs2audiopack_write(opb, fill_dft_bits[0], 1);
					   }
				   }
					   break;
				   case 1:
				   {
					   avs2audiopack_write(opb, fill_byte, 15);
					   for (i = 0; i < 8 * fill_byte; i++)
					   {
						   avs2audiopack_write(opb, fill_dft_bits[0], 2);
					   }
				   }
					   break;
				   case 2:
				   {
					   avs2audiopack_write(opb, fill_byte, 14);
					   for (i = 0; i < 8 * fill_byte; i++)
					   {
						   avs2audiopack_write(opb, fill_dft_bits[0], 4);
					   }
				   }
					   break;
				   case 3:
				   {
					   avs2audiopack_write(opb, fill_byte, 13);
					   for (i = 0; i < 8 * fill_byte; i++)
					   {
						   avs2audiopack_write(opb, fill_dft_bits[0], 8);
					   }
				   }
					   break;
				   default:
					   printf("the value of fill_type is illegal");
					   break;
				   }
				   numMcrBytes = 0;
			   }
			   else
			   {
#if CONSTANT_BITRATE_CONTROL
				   //stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
				   if (1 == 1)//(opencbr)
				   {
					   if (stAvs2Enc->useBWE)
					   {
						   if (k == 0)
							   stAvs2Enc->lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1 + OUTERLOOP_reservedBYTEforHEADER + 2;
						   else
							   stAvs2Enc->lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1;
					   }
					   else
					   {
						   if (k == 0)
							   stAvs2Enc->lfEncset[cid]->numbwebytes = OUTERLOOP_reservedBYTEforHEADER + 2;
						   else
							   stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
					   }
					   if ((fill_element_num > 0) && (k == 0))
						   stAvs2Enc->lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
					   if ((stAvs2Enc->elementnum - 1 == k))
						   stAvs2Enc->lfEncset[cid]->numbwebytes += 1;
					   stAvs2Enc->lfEncset[cid]->numbwebytes -= (stAvs2Enc->srateInfo.mcr_brate[cid] * 1000.0) * inputInfo->inSamples / (inputInfo->nChannels) / (stAvs2Enc->lfEncset[cid]->config.sampleRate * 2) / 8;
					   stAvs2Enc->lfEncset[cid]->numbwebytes += calc_mcr_bits(k, opb, cid, data_mcr[k]) / 8;
				   }
				   else
					   stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
#endif

				   stAvs2Enc->lfEncset[cid]->pcm_buffer = &stAvs2Enc->time_dmix[cid][CORE_FRAMESIZE];

				   Avs2LFEncoder(stAvs2Enc->useBWE, stAvs2Enc->lfEncset[cid], mdftSpectrum[cid], ancDataBytes[cid],
					   numAncDataBytes + cid, sampleData, &numOutBytes, k, codectype, opb, Maxpcmvalue);

				   memcpy(stAvs2Enc->lfEncset[cid]->pcm_buffer0 - CORE_FRAMESIZE, stAvs2Enc->lfEncset[cid]->pcm_buffer0, CORE_FRAMESIZE * 2 * sizeof(float));

			   }

		   	
		     if(stAvs2Enc->useBWE && elementencode_info[k].elType != ID_FIL)
			 {
					int j;
    
  	 			    avs2audiopack_write(opb, numAncDataBytes[cid], 8);
			
				  for(j = 0; j < numAncDataBytes[cid]; j++)
					  avs2audiopack_write(opb,  ancDataBytes[cid][j], 8);				
			    
			    mcrlencount[cid+1] += numAncDataBytes[cid]+2;
			 }
			 
			   if(numMcrBytes)
			   {
				   int j,tmp;
				   if (data_mcr[k]->endbit > 0)
					   avs2audiopack_write(data_mcr[k], 0, 8 - data_mcr[k]->endbit);
				   for(j = 0; j < data_mcr[k]->endbyte; j++)
					   avs2audiopack_write(opb, data_mcr[k]->buffer[j], 8);
				 //  if (data_mcr[k]->endbit>0)
					//{
					//   tmp = data_mcr[k]->buffer[data_mcr[k]->endbyte] >> (8 - data_mcr[k]->endbit);
					//   avs2audiopack_write(opb, tmp, data_mcr[k]->endbit);
					//}

				   mcrlencount[cid] += avs2audiopack_bytes(data_mcr[k]);
			   }
					
		     cid += elementencode_info[k].nChannelsInEl;
		 }
		   
		   for(cid = 0; cid < inputInfo->nChannels; cid++)
		   {
			   if (stAvs2Enc->bDoUpsample) 
			   {
				   memmove(&inputBuffer[cid][stAvs2Enc->envReadOffset], &inputBuffer[cid][stAvs2Enc->envReadOffset+AVS2ENC_BLOCKSIZE*2], (stAvs2Enc->envWriteOffset-stAvs2Enc->envReadOffset) * sizeof(float));

				   memmove(&inputBuffer[cid][stAvs2Enc->upsampleReadOffset], &inputBuffer[cid][stAvs2Enc->upsampleReadOffset+AVS2ENC_BLOCKSIZE], (stAvs2Enc->writeOffset-stAvs2Enc->upsampleReadOffset) * sizeof(float));
			   }
			   else
			   {
				   memmove(inputBuffer[cid], inputBuffer[cid]+nSamplesPerChannel, stAvs2Enc->writeOffset * sizeof(float));
			     memmove(downmixBuffer[cid], downmixBuffer[cid]+nSamplesPerChannel, stAvs2Enc->writeOffset * sizeof(float));
			   }
		   }
		   
		   if(opb->endbit>0)
			   avs2audiopack_write(opb,0,(8 - opb->endbit));

			 /* write out the bitstream */
			 memcpy(sampleData, opb->buffer, opb->endbyte);
			 numOutBytes=avs2audiopack_bytes(opb);

		   free(opb);
	   }

	   else //tianlai codec
	   {

		   ////////////////////////tianlai_PCAcodec  start /////////////////////////
		   avs2audiopack_buffer *opb, *tmp_opb;
		   {
			   opb = calloc(1, sizeof(avs2audiopack_buffer));
			   avs2audiopack_writeinit(opb);


			   if (inputInfo->nChannels/*numOfChannels*/ > 1)
			   {
				   if (codectype == 0)
					   avs2audiopack_write(opb, 0, 1);
				   else
					   avs2audiopack_write(opb, 1, 1);

				   if (codectype == 1)
				   {
					   //PCAGroupmode
					   avs2audiopack_write(opb, PCAGroupmode, 4);
				   }


			   }
			   avs2audiopack_write(opb, stAvs2Enc->useBWE, 1);
			   avs2audiopack_write(opb, fill_element_num, 1);
		   }



		   coreBitrate = inputInfo->bitRate / (inputInfo->nChannels/*numOfChannels*/ / 2.0) - 10000;
		   //	reset_bitrate(&(lfEncset[0]->vd),128,coreBitrate);

		   ///////////////////copy inputdata for psy analysis//////////////////////////
		   cid = 0;
		   for (k = 0; k < stAvs2Enc->elementnum; k++)
		   {
			   memcpy(&stAvs2Enc->time_dmix[cid][0], &stAvs2Enc->time_dmix[cid][CORE_FRAMESIZE * 2], CORE_FRAMESIZE * 2 * sizeof(float));

			   if (elementencode_info[k].nChannelsInEl == 2)
			   {
				   for (i = 0; i < (numSamplesRead / inputInfo->nChannels); i++)
				   {
					   stAvs2Enc->time_dmix[cid][CORE_FRAMESIZE * 2 + i] = (inputBuffer[cid][coreReadOffset + i] + inputBuffer[cid + 1][coreReadOffset + i]) / Maxpcmvalue * 0.5f;
					   stAvs2Enc->time_dmix[cid + 1][CORE_FRAMESIZE * 2 + i] = 0;//(inputBuffer[cid][coreReadOffset+i]-inputBuffer[cid+1][coreReadOffset+i])/Maxpcmvalue * 0.5f;
				   }
				   cid++;
				   cid++;
			   }
			   else
			   {
				   for (i = 0; i < (numSamplesRead / inputInfo->nChannels); i++)
				   {
					   stAvs2Enc->time_dmix[cid][CORE_FRAMESIZE * 2 + i] = inputBuffer[cid][coreReadOffset + i] / Maxpcmvalue;
				   }
				   cid++;
			   }
		   }

		   //重新配置PCA组合
		   if (inputInfo->nChannels/*numOfChannels*/ == 6)
		   {
			   //	PCAGroupmode =1;
			   encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[0x02 + PCAGroupmode];
			   stAvs2Enc->elementnum_tianlai51 = encoder_mode_tianlai51->numofele;
			   cid = 0;
			   for (i = 0; i < stAvs2Enc->elementnum_tianlai51; i++)
			   {
				   elementencode_info_tianlai51[i].elType = encoder_mode_tianlai51->idType[i];

				   switch (elementencode_info_tianlai51[i].elType) {

				   case ID_SCE:      /* single channel */
					   elementencode_info_tianlai51[i].nChannelsInEl = 1;

					   cid++;
					   break;

				   case ID_CPE_F:      /* channel pair */
					   elementencode_info_tianlai51[i].nChannelsInEl = 2;

					   cid++;
					   cid++;
					   break;

				   case ID_LFE:     /*LFE channel */
					   elementencode_info_tianlai51[i].nChannelsInEl = 1;

					   cid++;
					   break;

				   case ID_PCA2:     /*PCA2 channel */
					   elementencode_info_tianlai51[i].nChannelsInEl = 2;

					   cid++;
					   cid++;
					   break;

				   case ID_PCA4:     /*PCA4 channel */
					   elementencode_info_tianlai51[i].nChannelsInEl = 4;

					   cid += 4;
					   break;

				   case ID_PCA6:     /*PCA6 channel */
					   elementencode_info_tianlai51[i].nChannelsInEl = 6;

					   cid += 6;
					   break;
				   }

			   }
			   for (i = stAvs2Enc->elementnum_tianlai51; i < stAvs2Enc->elementnum_tianlai51 + fill_element_num; i++)
			   {
				   elementencode_info_tianlai51[i].elType = ID_FIL;
			   }

		   }

		   if (inputInfo->nChannels/*numOfChannels*/ == 2)
		   {
			   //PCAGroupmode =1;
			   encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[0x01];
			   stAvs2Enc->elementnum_tianlai51 = encoder_mode_tianlai51->numofele;
			   cid = 0;
			   for (i = 0; i < stAvs2Enc->elementnum_tianlai51; i++)
			   {
				   elementencode_info_tianlai51[i].elType = encoder_mode_tianlai51->idType[i];

				   switch (elementencode_info_tianlai51[i].elType) {

				   case ID_SCE:      /* single channel */
					   elementencode_info_tianlai51[i].nChannelsInEl = 1;

					   cid++;
					   break;

				   case ID_CPE_F:      /* channel pair */
					   elementencode_info_tianlai51[i].nChannelsInEl = 2;

					   cid++;
					   cid++;
					   break;

				   case ID_LFE:     /*LFE channel */
					   elementencode_info_tianlai51[i].nChannelsInEl = 1;

					   cid++;
					   break;

				   case ID_PCA2:     /*PCA2 channel */
					   elementencode_info_tianlai51[i].nChannelsInEl = 2;

					   cid++;
					   cid++;
					   break;

				   case ID_PCA4:     /*PCA4 channel */
					   elementencode_info_tianlai51[i].nChannelsInEl = 4;

					   cid += 4;
					   break;

				   case ID_PCA6:     /*PCA6 channel */
					   elementencode_info_tianlai51[i].nChannelsInEl = 6;

					   cid += 6;
					   break;
				   }

			   }
			   for (i = stAvs2Enc->elementnum_tianlai51; i < stAvs2Enc->elementnum_tianlai51 + fill_element_num; i++)
			   {
				   elementencode_info_tianlai51[i].elType = ID_FIL;
			   }

		   }

		   cid = 0;

		   for (eid = 0; eid < stAvs2Enc->elementnum_tianlai51; eid++)
		   {
			   switch (elementencode_info_tianlai51[eid].elType) {

			   case ID_SCE:      /* single channel */
			   {

				   struct AVS2_ENCODER *lfEnc = stAvs2Enc->lfEncset[cid];

				   for (i = 0; i < 2048 / 2; i++)
					   lfEnc->pcm_buffer0[2048 / 2 + i] = *(inputBuffer[cid] + coreReadOffset + i) / Maxpcmvalue;

				   mdft_lowpassframeblock_multi(lfEnc->pcm_buffer0 - 1024 + 448, lfEnc->lf_winseq, Mdftout, (4096 / 4) / 2);

				   for (i = 0; i < 1024; i++)
					   mdftSpectrum[cid][i] = Mdftout[i * 2] * Maxpcmvalue;

			   }

			   cid++;
			   break;

			   case ID_PCA2:      /* PCAx channel */
			   case ID_PCA4:
			   case ID_PCA6:
			   {int iid;
			   int sid = cid;


			   for (iid = 0; iid < elementencode_info_tianlai51[eid].nChannelsInEl; iid++)
			   {

				   struct AVS2_ENCODER *lfEnc = stAvs2Enc->lfEncset[cid];

				   if (iid > 0)
					   memcpy(lfEnc->lf_winseq, stAvs2Enc->lfEncset[sid]->lf_winseq, 20 * sizeof(int));

				   for (i = 0; i < 2048 / 2; i++)
					   lfEnc->pcm_buffer0[2048 / 2 + i] = *(inputBuffer[cid] + coreReadOffset + i) / Maxpcmvalue;

				   mdft_lowpassframeblock_multi(lfEnc->pcm_buffer0 - 1024 + 448, lfEnc->lf_winseq, Mdftout, (4096 / 4) / 2);

				   //LFE, lowpass filter			
				   if (cid == 3)
				   {
					   int index, tt, jj, kk, ll, mdftoffset;
					   int LL[5] = { 4096 / 8 / 2, 4096 / 4 / 2, 4096 / 2 / 2, 4096 / 2,4096 / 16 / 2 };

					   tt = lfEnc->lf_winseq[1];
					   jj = lfEnc->lf_winseq[1 + 1];

					   if (LL[tt - 1] > LL[jj - 1])
						   ll = LL[tt - 1] / 2;
					   else
						   ll = LL[jj - 1] / 2;


					   mdftoffset = 0;

					   for (index = 1; index < (lfEnc->lf_winseq[0] + 1); index++)
					   {
						   int lowbandoffset;

						   tt = lfEnc->lf_winseq[index];
						   jj = lfEnc->lf_winseq[index + 1];

						   if (LL[tt - 1] > LL[jj - 1])
							   ll = LL[tt - 1] / 2;
						   else
							   ll = LL[jj - 1] / 2;

						   lowbandoffset = 8;
						   if (ll == 4096 / 8 / 2 / 2)
							   lowbandoffset = 2;
						   if (ll == 4096 / 8 / 2)
							   lowbandoffset = 4;
						   if (ll == 4096 / 4 / 2)
							   lowbandoffset = 8;
						   if (ll == 4096 / 2 / 2)
							   lowbandoffset = 8;
						   if (ll == 4096 / 16 / 2 / 2)
							   lowbandoffset = 2;
						   for (kk = lowbandoffset; kk < ll; kk++)
						   {
							   Mdftout[mdftoffset + kk * 2] = 0;
							   Mdftout[mdftoffset + kk * 2 + 1] = 0;
						   }
						   mdftoffset += (ll * 2);

					   }
				   }

				   for (i = 0; i < 1024; i++)
					   mdftSpectrum[cid][i] = Mdftout[i * 2] * Maxpcmvalue;

				   cid++;
			   }//for(iid=0;iid<elementencode_info[eid].nChannelsInEl;iid++)

			   //PCA analysis,get the usePCAitemnum principal components and PCA Matrix
			   multichannelMDCT_PCA_2(&mdftSpectrum[sid], stAvs2Enc->lfEncset[sid]->lf_winseq, elementencode_info_tianlai51[eid].nChannelsInEl, &mdftSpectrum[sid], eid);
			   }
			   break;

			   }//switch (elementencode_info[eid].elType) {

		   }//for( eid=0;eid<elementnum;eid++)

   ////////////////////////////////////////////////


   ////////////////////////////////////////////////
		   cid = 0;

		   for (eid = 0; eid < stAvs2Enc->elementnum_tianlai51 + fill_element_num; eid++)
		   {
			   switch (elementencode_info_tianlai51[eid].elType) {

			   case ID_SCE:      /* single channel */

			   //配置每个声道所用ci和编码码本
				   if (inputInfo->nChannels == 1)
					   channelbasesetting = stAvs2Enc->basesetting;
				   else
				   {
					   double averageBitrate;
					   averageBitrate = inputInfo->bitRate * 0.2;

					   channelbasesetting = stAvs2Enc->basesetting;
				   }

				   ci_set(&(stAvs2Enc->lfEncset[cid]->vi), channelbasesetting);

				   _vds_flr_res_set(&(stAvs2Enc->lfEncset[cid]->vd), &(stAvs2Enc->lfEncset[cid]->vi), min(12,channelbasesetting+3));

				   lowpass_kHz = freqbeginend_setting(&(stAvs2Enc->lfEncset[cid]->vi), inputInfo->nChannels, stAvs2Enc->bitRateIndex, 0);

				   reset_bandWidth(stAvs2Enc->config.sampleRate, stAvs2Enc->config_idx, lowpass_kHz, &(stAvs2Enc->lfEncset[cid]->config.bandWidth), stAvs2Enc->useBWE);

				   {

					   struct AVS2_ENCODER *lfEnc = stAvs2Enc->lfEncset[cid];



					   {
						   int lf_winseq_index = lfEnc->lf_winseq[19];
						   int s = stAvs2Enc->basesetting;


						   avs2audiopack_write(opb, elementencode_info_tianlai51[eid].elType, 4);
						   avs2audiopack_write(opb, s, 4);
						   avs2audiopack_write(opb, lf_winseq_index, 6);
					   }


					   //reset_bitrate(&(lfEnc->vd),PCAcorebitpershort*1.0,coreBitrate);
#if CONSTANT_BITRATE_CONTROL
					   //stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
					   if (1 == 1)//(opencbr)
					   {
						   if (stAvs2Enc->useBWE)
						   {
							   if (eid == 0)
								   stAvs2Enc->lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1 + OUTERLOOP_reservedBYTEforHEADER + 2;
							   else
								   stAvs2Enc->lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1;
						   }
						   else
						   {
							   if (eid == 0)
								   stAvs2Enc->lfEncset[cid]->numbwebytes = OUTERLOOP_reservedBYTEforHEADER + 2;
							   else
								   stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
						   }
						   if ((fill_element_num > 0) && (eid == 0))
							   stAvs2Enc->lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
						   if ((stAvs2Enc->elementnum - 1 == eid))
							   stAvs2Enc->lfEncset[cid]->numbwebytes += 1;
					   }
					   else
						   stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
#endif
						   /* encode lf frame using a  lf encoder */
					   Avs2LFEncoder_PCA(stAvs2Enc->useBWE,

						   lfEnc,
						   &(lfEnc->vb),
						   inputBuffer[cid] + coreReadOffset,
						   mdftSpectrum[cid],
						   1,
						   ancDataBytes[cid],
						   &numAncDataBytes[cid],
						   sampleData,
						   &numOutBytes,
						   0,
						   eid,
						   1,
						   1,
						   PCAGroupmode,
						   opb, Maxpcmvalue
					   );

						//update pcmbuffer
					   for (i = -2048 / 2; i < 2048 / 2; i++)
						   lfEnc->pcm_buffer0[i] = lfEnc->pcm_buffer0[2048 / 2 + i];

					   if (stAvs2Enc->bDoUpsample)
					   {
						   memmove(&inputBuffer[cid][stAvs2Enc->envReadOffset],
							   &inputBuffer[cid][stAvs2Enc->envReadOffset + AVS2ENC_BLOCKSIZE * 2],
							   (stAvs2Enc->envWriteOffset - stAvs2Enc->envReadOffset) * sizeof(float));

						   memmove(&inputBuffer[cid][stAvs2Enc->upsampleReadOffset],
							   &inputBuffer[cid][stAvs2Enc->upsampleReadOffset + AVS2ENC_BLOCKSIZE],
							   (stAvs2Enc->writeOffset - stAvs2Enc->upsampleReadOffset) * sizeof(float));
					   }
					   else
					   {
						   memmove(inputBuffer[cid], inputBuffer[cid] + nSamplesPerChannel, stAvs2Enc->writeOffset * sizeof(float));
					   }

				   }

				   cid++;

				   break;



			   case ID_PCA2:      /* PCAx channel */
			   case ID_PCA4:
			   case ID_PCA6:
			   {int iid;
			   int sid = cid;



			   //选择合适的usePCAitemnum
			   usePCAitemnum_Optimization(elementencode_info_tianlai51[eid].nChannelsInEl,
				   inputInfo->bitRate, inputInfo->nChannels, &(stAvs2Enc->lfEncset[sid]->usePCAitemnum));

			   if (inputInfo->nChannels/*numOfChannels*/ >= 2)
			   {
				   if (elementencode_info_tianlai51[eid].nChannelsInEl == 4)
				   {
					   if (stAvs2Enc->lfEncset[sid]->usePCAitemnum == 2)
					   {
						   reset_bitrate(&(stAvs2Enc->lfEncset[sid]->vd), stAvs2Enc->PCAcorebitpershort*0.9, coreBitrate);
						   reset_bitrate(&(stAvs2Enc->lfEncset[sid + 1]->vd), stAvs2Enc->PCAcorebitpershort*0.65, coreBitrate);
					   }
					   else
					   {
						   reset_bitrate(&(stAvs2Enc->lfEncset[sid]->vd), stAvs2Enc->PCAcorebitpershort*0.9, coreBitrate);
						   reset_bitrate(&(stAvs2Enc->lfEncset[sid + 1]->vd), stAvs2Enc->PCAcorebitpershort*0.65, coreBitrate);
						   reset_bitrate(&(stAvs2Enc->lfEncset[sid + 2]->vd), stAvs2Enc->PCAcorebitpershort*0.45, coreBitrate);
					   }

				   }
				   else if (elementencode_info_tianlai51[eid].nChannelsInEl == 2)
				   {
					   if (stAvs2Enc->lfEncset[sid]->usePCAitemnum == 1)
					   {
						   if (eid == 0)
							   reset_bitrate(&(stAvs2Enc->lfEncset[sid]->vd), stAvs2Enc->PCAcorebitpershort*1.1, coreBitrate);
						   else
							   reset_bitrate(&(stAvs2Enc->lfEncset[sid]->vd), stAvs2Enc->PCAcorebitpershort*0.9, coreBitrate);

					   }
					   else
					   {
						   if (eid == 0)
						   {
							   reset_bitrate(&(stAvs2Enc->lfEncset[sid + 0]->vd), stAvs2Enc->PCAcorebitpershort*0.75, coreBitrate);
							   reset_bitrate(&(stAvs2Enc->lfEncset[sid + 1]->vd), stAvs2Enc->PCAcorebitpershort*0.35, coreBitrate);
						   }
						   else if (eid == 1)
						   {
							   reset_bitrate(&(stAvs2Enc->lfEncset[sid + 0]->vd), stAvs2Enc->PCAcorebitpershort*0.95, coreBitrate);
							   reset_bitrate(&(stAvs2Enc->lfEncset[sid + 1]->vd), stAvs2Enc->PCAcorebitpershort*0.5, coreBitrate);
						   }
						   else
						   {
							   reset_bitrate(&(stAvs2Enc->lfEncset[sid + 0]->vd), stAvs2Enc->PCAcorebitpershort*0.6, coreBitrate);
							   reset_bitrate(&(stAvs2Enc->lfEncset[sid + 1]->vd), stAvs2Enc->PCAcorebitpershort*0.35, coreBitrate);
						   }

					   }
				   }
			   }

			   {	struct AVS2_ENCODER *lfEnc = stAvs2Enc->lfEncset[cid];
			   int lf_winseq_index = lfEnc->lf_winseq[19];
			   int s = stAvs2Enc->basesetting;
			   //	if((indexinelement==0)&&(nChannelsInEl>1))

			   avs2audiopack_write(opb, elementencode_info_tianlai51[eid].elType, 4);

			   {
				   if (codectype == 1)
				   {

					   avs2audiopack_write(opb, lfEnc->usePCAitemnum, 3);
				   }
			   }

			   //avs2audiopack_write(opb, lfEnc->elInfo.instanceTag, 4);
			   avs2audiopack_write(opb, s, 4);
			   avs2audiopack_write(opb, lf_winseq_index, 6);
			   }


			   for (iid = 0; iid < elementencode_info_tianlai51[eid].nChannelsInEl; iid++)
			   {

				   //配置每个声道所用ci和编码码本
				   if (inputInfo->nChannels == 1)
					   channelbasesetting = stAvs2Enc->basesetting;
				   else if ((inputInfo->nChannels == 2))
				   {
					   channelbasesetting = stAvs2Enc->basesetting;
				   }
				   else
				   {
					   double averageBitrate;
					   averageBitrate = inputInfo->bitRate * 0.2;


					   channelbasesetting = stAvs2Enc->basesetting;
				   }

				   ci_set(&(stAvs2Enc->lfEncset[cid]->vi), channelbasesetting);

				   _vds_flr_res_set(&(stAvs2Enc->lfEncset[cid]->vd), &(stAvs2Enc->lfEncset[cid]->vi), min(12,channelbasesetting+3));

				   if (!((elementencode_info_tianlai51[eid].elType == ID_PCA2) && (eid == 1)))
					   lowpass_kHz = freqbeginend_setting(&(stAvs2Enc->lfEncset[cid]->vi), elementencode_info_tianlai51[eid].nChannelsInEl, stAvs2Enc->bitRateIndex, 0);
				   else
					   lowpass_kHz = freqbeginend_setting(&(stAvs2Enc->lfEncset[cid]->vi), elementencode_info_tianlai51[eid].nChannelsInEl, /*0*/stAvs2Enc->bitRateIndex, 0);



				   reset_bandWidth(stAvs2Enc->config.sampleRate, stAvs2Enc->config_idx, lowpass_kHz, &(stAvs2Enc->lfEncset[cid]->config.bandWidth), stAvs2Enc->useBWE);

				   {
					   //				unsigned int numAncDataBytes=0;
					   struct AVS2_ENCODER *lfEnc = stAvs2Enc->lfEncset[cid];

					   //numOutBytes =0;
					   /* encode lf frame using a new lf encoder */
					   //encode the first usePCAitemnum principal components,  PCA Matrix
					   lfEnc->elInfo.elType = 0;

					   lfEnc->pcm_buffer = &stAvs2Enc->time_dmix[cid][CORE_FRAMESIZE];
#if CONSTANT_BITRATE_CONTROL
					   //stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
					   if (1 == 1)//(opencbr)
					   {
						   if (stAvs2Enc->useBWE)
						   {
							   if (eid == 0)
								   stAvs2Enc->lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1 + OUTERLOOP_reservedBYTEforHEADER + 2;
							   else
								   stAvs2Enc->lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1;
						   }
						   else
						   {
							   if (eid == 0)
								   stAvs2Enc->lfEncset[cid]->numbwebytes = OUTERLOOP_reservedBYTEforHEADER + 2;
							   else
								   stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
						   }
						   if ((fill_element_num > 0) && (eid == 0))
							   stAvs2Enc->lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
						   if ((stAvs2Enc->elementnum - 1 == eid))
							   stAvs2Enc->lfEncset[cid]->numbwebytes += 1;
						   stAvs2Enc->lfEncset[cid]->numbwebytes -= (stAvs2Enc->srateInfo.mcr_brate[cid] * 1000.0) * inputInfo->inSamples / (inputInfo->nChannels) / (stAvs2Enc->lfEncset[cid]->config.sampleRate * 2) / 8;
						   stAvs2Enc->lfEncset[cid]->numbwebytes += calc_mcr_bits(eid, opb, cid, data_mcr[eid]) / 8;
					   }
					   else
						   stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
#endif
					   if ((stAvs2Enc->lfEncset[sid]->usePCAitemnum) > iid)
						   Avs2LFEncoder_PCA(stAvs2Enc->useBWE,
							   lfEnc,
							   &(lfEnc->vb),
							   inputBuffer[cid] + coreReadOffset,
							   mdftSpectrum[cid],
							   1,
							   ancDataBytes[cid],
							   &numAncDataBytes[cid],
							   sampleData,
							   &numOutBytes,
							   iid,
							   eid,
							   elementencode_info_tianlai51[eid].nChannelsInEl,
							   1,
							   PCAGroupmode,
							   opb, Maxpcmvalue
						   );

					   //updatae pcmbuffer
					   for (i = -2048 / 2; i < 2048 / 2; i++)
						   lfEnc->pcm_buffer0[i] = lfEnc->pcm_buffer0[2048 / 2 + i];

					   if (stAvs2Enc->bDoUpsample)
					   {
						   memmove(&inputBuffer[cid][stAvs2Enc->envReadOffset],
							   &inputBuffer[cid][stAvs2Enc->envReadOffset + AVS2ENC_BLOCKSIZE * 2],
							   (stAvs2Enc->envWriteOffset - stAvs2Enc->envReadOffset) * sizeof(float));

						   memmove(&inputBuffer[cid][stAvs2Enc->upsampleReadOffset],
							   &inputBuffer[cid][stAvs2Enc->upsampleReadOffset + AVS2ENC_BLOCKSIZE],
							   (stAvs2Enc->writeOffset - stAvs2Enc->upsampleReadOffset) * sizeof(float));
					   }
					   else
					   {
						   memmove(inputBuffer[cid], inputBuffer[cid] + nSamplesPerChannel, stAvs2Enc->writeOffset * sizeof(float));
					   }

				   }




				   cid++;
			   }


			   {


				   /* write out the bitstream */
				   memcpy(sampleData, opb->buffer, opb->endbyte);
				   numOutBytes = avs2audiopack_bytes(opb);
			   }


			   }
			   break;

			   case ID_FIL:
			   {
				   int fill_type = 0;
				   int fill_byte = 0xff;
				   char fill_dft_bits[2048] = { 0 };

				   int leftbytes = inputInfo->bitRate * 1024 / inputInfo->sampleRate / 8 * 2;
				   ////if (objflag)
				   leftbytes = inputInfo->bitRate * 1.0*inputInfo->inSamples / inputInfo->sampleRate / 8 / (inputInfo->nChannels) - OUTERLOOP_reservedBYTEforHEADER - 2;
				   //else
				   //	leftbytes = inputInfo->bitRate * 1.0*inputInfo->inSamples / inputInfo->sampleRate / 8 / (inputInfo->nChannels) - OUTERLOOP_reservedBYTEforHEADER - 2;
				   leftbytes -= 1 + opb->endbyte+1;
				   if (opb->endbit > 0) leftbytes--;
				   fill_byte = leftbytes;
				   if (fill_byte < 0)
				   {
					   printf("fill_byte %d < 0\n", fill_byte);
					   fill_byte = 0;
				   }

				   avs2audiopack_write(opb, 6, 4);    //ID_FIL=6
				   avs2audiopack_write(opb, fill_type, 4);
				   switch (fill_type)
				   {
				   case 0:
				   {
					   avs2audiopack_write(opb, fill_byte, 8);
					   for (i = 0; i < 8 * fill_byte; i++)
					   {
						   avs2audiopack_write(opb, fill_dft_bits[0], 1);
					   }
				   }
				   break;
				   case 1:
				   {
					   avs2audiopack_write(opb, fill_byte, 15);
					   for (i = 0; i < 8 * fill_byte; i++)
					   {
						   avs2audiopack_write(opb, fill_dft_bits[0], 2);
					   }
				   }
				   break;
				   case 2:
				   {
					   avs2audiopack_write(opb, fill_byte, 14);
					   for (i = 0; i < 8 * fill_byte; i++)
					   {
						   avs2audiopack_write(opb, fill_dft_bits[0], 4);
					   }
				   }
				   break;
				   case 3:
				   {
					   avs2audiopack_write(opb, fill_byte, 13);
					   for (i = 0; i < 8 * fill_byte; i++)
					   {
						   avs2audiopack_write(opb, fill_dft_bits[0], 8);
					   }
				   }
				   break;
				   default:
					   printf("the value of fill_type is illegal");
					   break;
				   }
			   }
			   break;

			   }//switch
			   if (stAvs2Enc->useBWE && elementencode_info_tianlai51[eid].elType != ID_FIL)
			   {
				   int channelindex;
				   for (channelindex = 0; channelindex < inputInfo->nChannels/*numOfChannels*/; channelindex++)
				   {
					   int j;


					   if (channelindex == 3)
						   continue;


					   avs2audiopack_write(opb, numAncDataBytes[channelindex], 8);

					   for (j = 0; j < numAncDataBytes[channelindex]; j++)
						   avs2audiopack_write(opb, ancDataBytes[channelindex][j], 8);
				   }
			   }

		   }//for( eid=0;eid<elementnum;eid++)

		   if (opb->endbit > 0)
			   avs2audiopack_write(opb, 0, (8 - opb->endbit));

		   /* write out the bitstream */
		   memcpy(sampleData, opb->buffer, opb->endbyte);
		   numOutBytes = avs2audiopack_bytes(opb);
		   free(opb);


	   }//if(codectype == 0)

				

		/* write one frame of encoded audio to file */
	   if (numOutBytes)
	   {
		   //fwrite(&numOutBytes, sizeof(short), 1, fOutputFile);
		   //DataLength += 2;
		   //
		   //fwrite(sampleData, sizeof(unsigned char), numOutBytes, fOutputFile);
		   //DataLength += numOutBytes;

		   datalencount[eid] += numOutBytes;
	   }
		*OutBytes = numOutBytes;

//		frame++;
//		fprintf(stderr,"[%d]\r",frame);		
	}

//    fprintf(stderr,"\n");


	/* close encoder */	
//	for(cid=0;cid<numOfChannels;cid++)
//	{
//		Avs2BweMDFTClose((unsigned int*)&(stAvs2Init->lfEncset[cid]->st1_in), (unsigned int*)&(stAvs2Init->lfEncset[cid]->st_common));
//		if(stAvs2Init->useBWE)
//		{	
//			Avs2BweEncoderClose((unsigned int*)&(stAvs2Init->lfEncset[cid]->st2_in));
//		}
//
//	}
//	Wave_fclose(fInputFile, bitsPerSample);
//
//	if(inputInfo.outputFormat == 1)
//		Bitstream_fclose(fOutputFile, DataLength);
	
	return 0;
}

