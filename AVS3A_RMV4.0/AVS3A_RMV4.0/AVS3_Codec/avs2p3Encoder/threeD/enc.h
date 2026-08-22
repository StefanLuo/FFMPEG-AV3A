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

#ifndef _enc_h_
#define _enc_h_

#include "..\general\encode\maxcorr.h"
#include "..\general\encode\resampler.h"

enum ENCSTATUS {
  ENC_NOERROR = 0
  ,ENC_FINISHED = 1
};
struct MULTI_CHAN_MODE{
	int numofele; 	/* the number of element */
	ELEMENT_TYPE idType[MAX_ALLCHANNEL];	/* the type of the element */
	int ele_id[MAX_ALLCHANNEL];
};

typedef struct {
  ELEMENT_TYPE elType;
  int nChannelsInEl;
  int ChannelIndex[MAX_ALLCHANNEL];
 } ELEMENTENCODE_INFO;


typedef struct MULTI_CHAN_MODE MC_MODE;

struct STAvs2Enc{

	int useBWE;
	int useSuperMode;

	int brIndex, bitRateIndex;
	int bDoIIR2Downsample;
	int bDoUpsample;
	int bEncodeMono;

	int elementnum;
	int elementnum_tianlai51;
	double basesetting;
	int config_idx;

	int writeOffset;
	int coreReadOffset;
	int envReadOffset;
	int envWriteOffset;
	int upsampleReadOffset;

	ENC_CONFIG config;
	ST_RATE_CONFIG srateInfo;
	MCR_INFO mcr_info;

	PS_DATA	 *psData[MAX_ALLCHANNEL/2];
	PS_DATA	 *psData_pre[MAX_ALLCHANNEL/2];
	PS_BWE_DATA	 *psBweData[MAX_ALLCHANNEL/2], *psBweData_pre[MAX_ALLCHANNEL/2];

	int PCAcorebitpershort;

	struct AVS2_ENCODER *lfEncset[MAX_ALLCHANNEL];



	IIR21_RESAMPLER IIR21_reSampler[MAX_ALLCHANNEL]; 
	IIR21_RESAMPLER IIR21_bweSampler[MAX_ALLCHANNEL];
    float inputBuffer[MAX_ALLCHANNEL][AVS2ENC_BLOCKSIZE*2 + MAX_DS_FILTER_DELAY + INPUT_DELAY];
    MC_MODE *encoder_mode;
    ELEMENTENCODE_INFO elementencode_info[8];

   float	 time_dmix[MAX_ALLCHANNEL][((1024*4+1024*2)*2+6)*2]; //ÏÂ»ìÉùµÀ

   float downmixBuffer[MAX_ALLCHANNEL][AVS2ENC_BLOCKSIZE*2 + MAX_DS_FILTER_DELAY + INPUT_DELAY];

} ;

typedef struct STAvs2Enc *HANDLE_STAvs2Enc;


void init_avs2_general_encoder(ChanInfo *inputInfo, HANDLE_STAvs2Enc *phstAvs2Init, int *inLen, int*cpe_config, int*PCAGroupmode, int*useSuperMode, int fill_element_num);

int general_encoder_frame(HANDLE_STAvs2Enc stAvs2Init, char *TimeDataPcmBuffer, int inSamples, ChanInfo *inputInfo, int *OutBytes, unsigned char sampleData[], int fill_element_num);

void close_avs2_general_encoder(HANDLE_STAvs2Enc *phstAvs2Init, int numOfChannels);



#endif