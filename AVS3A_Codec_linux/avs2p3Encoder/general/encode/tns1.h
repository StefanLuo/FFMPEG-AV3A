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

/*
  Temporal Noise Shaping
*/
#ifndef _TNS1_H
#define _TNS1_H

#define AVS2ENC_BLOCKSIZE  1024

#define FRAME_LEN_LONG    AVS2ENC_BLOCKSIZE
#define TRANS_FAC         8
#define FRAME_LEN_SHORT   (FRAME_LEN_LONG/TRANS_FAC)

#define MAX_SFB_SHORT   15
#define MAX_SFB_LONG    51
#define MAX_SFB         (MAX_SFB_SHORT > MAX_SFB_LONG ? MAX_SFB_SHORT : MAX_SFB_LONG)   /* = MAX_SFB_LONG */


#define TNS_MAX_ORDER       12
#define TNS_MAX_ORDER_SHORT 5
#define TNS_MAX_ORDER_SHORT_LONG 7
#define TNS_MAX_ORDER_LONG_SHORT 10
#define FILTER_DIRECTION    0

#define RATIO_MULT        0.25f

#define TNS_MODIFY_BEGIN           2600 /* Hz */
#define RATIO_PATCH_LOWER_BORDER   380  /* Hz */
#define TNS_PARCOR_THRESH          0.1f


typedef struct{
  float   threshOn;
  int     lpcStartFreq;
  int     lpcStopFreq;
  float   tnsTimeResolution;
}tns_config_tabulated;

typedef struct {
  char tnsActive;
  int tnsMaxSfb;

  int maxOrder;
  int tnsStartFreq;
  int coefRes;

  tns_config_tabulated confTab;

  float acfWindow[TNS_MAX_ORDER+1];
  int tnsStartBand;
  int tnsStartLine;

  int tnsStopBand;
  int tnsStopLine;

  int lpcStartBand;
  int lpcStartLine;

  int lpcStopBand;
  int lpcStopLine;

  int tnsRatioPatchLowestCb;
  int tnsModifyBeginCb;

  float threshold;

  int sfbCnt;

  int sfbOffset[MAX_SFB+1];

  int sfbActive;

  int lowpassline;

}tns_config;

typedef struct {
  char  tnsActive;
  float parcor[TNS_MAX_ORDER];
  float predictionGain;
} tns_subblock_info;

typedef struct{
  char tnsActive[TRANS_FAC];
  char coefRes[TRANS_FAC];
  int length[TRANS_FAC];
  int order[TRANS_FAC];
  int coef[TRANS_FAC*TNS_MAX_ORDER_SHORT];

  tns_subblock_info subblockInfo[TRANS_FAC];
  
}tns_info;

#endif /* _TNS1_H */