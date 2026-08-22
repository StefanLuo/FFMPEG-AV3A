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

#ifndef GENERALH
#define GENERALH

#include <stdio.h>
#include <stdlib.h>
#include "../general/decode/lfenc.h"
//#include "lfdec.h"


typedef struct {
  ELEMENT_TYPE elType;
  int nChannelsInEl;
  int ChannelIndex[8];
 } ELEMENTENCODE_INFO;


struct MULTI_CHAN_MODE{
	int numofele; 	/* the number of element */
	ELEMENT_TYPE idType[MAX_ALLCHANNEL];	/* the type of the element */
	int ele_id[MAX_ALLCHANNEL];
};

typedef struct MULTI_CHAN_MODE MC_MODE;



struct STAvs2Dec{

	int useBWE;
	int usePS;
	int bitRateIndex;
	ChanInfo config;

	int elementnum;
	int elementnum_tianlai51;
	int	chIndex[MAX_ALLCHANNEL];
    int bandWidth[MAX_ALLCHANNEL];
	int config_idx[MAX_ALLCHANNEL];
	
	ST_RATE_CONFIG srateInfo;
    PS_DATA *psData[MAX_ALLCHANNEL/2], *psData_pre[MAX_ALLCHANNEL/2];
	PS_BWE_DATA *psBweData[MAX_ALLCHANNEL/2], *psBweData_pre[MAX_ALLCHANNEL/2];

	MCR_INFO mcrInfo;

	AVS2DECODER avs2DecoderInfo;
    BWEBITSTREAM streamBWE[MAX_ALLCHANNEL];                           /*!< pointer to bwe bitstream buffer */
    int ci_table;

	 MC_MODE *encoder_mode;
     ELEMENTENCODE_INFO elementencode_info_obj[8];
     ELEMENTENCODE_INFO elementencode_info_tianlai51[8];
     //struct AVS2_DECODER_INSTANCE Avs2DecoderInstance_objframe[6];

};


typedef struct STAvs2Dec *HANDLE_STAvs2Dec;


void init_avs2_general_decoder_frame(int sampleRateCore, ChanInfo inputInfo, HANDLE_STAvs2Dec *phstAvs2Dec, int useBWE, int fill_element_num);

int general_decoder_frame(/*int argc, char *argv[]*/HANDLE_STAvs2Dec stAvs2Dec, ChanInfo inputInfo, 
					unsigned int readBuf[], unsigned short numBytes, float AllChannelTimeDataFloat[], int *outputLen);

void close_general_decoder_frame(HANDLE_STAvs2Dec *phstAvs2Dec, int nChannels);


AVS2DECODER CAvs2DecoderOpen_objframe(//HANDLE_BIT_BUF pBs,       /*!< pointer to bitbuffer structure */
                           BWEBITSTREAM *pStreamBWE, /*!< pointer to bwe bitstream structure */
                           float *pTimeData,
						   int idx,
						    HANDLE_STAvs2Dec stAvs2Dec);


#endif