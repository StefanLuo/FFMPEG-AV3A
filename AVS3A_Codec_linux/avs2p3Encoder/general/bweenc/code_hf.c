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
#include <memory.h>
#include <math.h>
#include "code_hf.h"
#include "mdftmultiblockanalysis.h"
#include "avs2BweEncMDFT.h"
#include "encoder.h"
#include "../encode/avs2audio.h"
#include "../encode/lfenc.h"

#define FRAME_LEN_LONG 1024

static const struct
{
  unsigned int    bitrateFrom ;
  unsigned int    bitrateTo ;

  unsigned int    sampleRate ;
  unsigned int    numChannels ;

  unsigned int    startFreq ;
  unsigned int    stopFreq ;

  int numNoiseBands ;
  int noiseFloorOffset ;
  int noiseMaxLevel ;
  BWE_STEREO_MODE stereoMode ;
  int sfbBands ;
  int sfbOffsets[MAX_SFB_NUM];

} tuningTable[] = {


  /*** mono ***/
  { 16000, 22000,  16000, 1,  12,  38,  1, 0, 6, BWE_MONO, 12, {1,1,1,1,2,2,2,2,3,3,4,4}}, /* nominal: 16 kbit/s */
  { 22000, 36000,  16000, 1,  21,  38,  1, 0, 6, BWE_MONO, 10, {1,1,2,2,2,2,2,2,2,2} }, /* nominal: 32 kbit/s */
  { 36000, 44001,  16000, 1,  21,  38,  1, 0, 6, BWE_MONO, 10, {1,1,2,2,2,2,2,2,5,5} }, /* nominal: 40 kbit/s */

  { 12000, 18000,  22050, 1,  10,  31, 1, 0, 6, BWE_MONO, 12, {1,1,1,1,1,1,2,2,2,3,3,3} }, /* nominal: 14 kbit/s */
  { 18000, 22000,  22050, 1,  12,  34, 2, 0, 6, BWE_MONO, 14, {1,1,1,1,1,1,1,1,2,2,2,2,2,3} }, /* nominal: 20 kbit/s */
  { 22000, 28000,  22050, 1,  16,  43, 2, 0, 6, BWE_MONO, 14, {1,1,1,1,2,2,2,2,2,2,2,3,3,3} },  /* nominal: 24 kbit/s */
  { 28000, 36000,  22050, 1,  21,  47, 2, 0, 3, BWE_MONO, 12, {1,1,2,2,2,2,2,2,3,3,3,3} }, /* nominal: 32 kbit/s */
  { 36000, 52000,  22050, 1,  21,  47, 2, 0, 3, BWE_MONO, 14, {1,1,1,1,2,2,2,2,2,2,2,2,3,3} }, /* nominal: 48 kbit/s */
  { 52000, 60000,  22050, 1,  21,  47, 3, 0, -3, BWE_SWITCH_LRC, 10, {1,2,2,2,2,3,3,3,3,3} }, /* nominal: 56 kbit/s */ //wchg 20201204

  { 12000, 18000,  24000, 1,  10,  31, 1, 0, 6, BWE_MONO, 12, {1,1,1,1,1,1,2,2,2,3,3,3} }, /* nominal: 14 kbit/s */
  { 18000, 22000,  24000, 1,  12,  34, 2, 0, 6, BWE_MONO, 14, {1,1,1,1,1,1,1,1,2,2,2,2,2,3} }, /* nominal: 20 kbit/s */
  { 22000, 28000,  24000, 1,  16,  43, 2, 0, 6, BWE_MONO, 14, {1,1,1,1,2,2,2,2,2,2,2,3,3,3} }, /* nominal: 24 kbit/s */
  { 28000, 36000,  24000, 1,  21,  47, 2, 0, 3, BWE_MONO, 12, {1,1,2,2,2,2,2,2,3,3,3,3} }, /* nominal: 32 kbit/s */
  { 36000, 52000,  24000, 1,  21,  47, 2, 0, 3, BWE_MONO, 14, {1,1,1,1,2,2,2,2,2,2,2,2,3,3} }, /* nominal: 48 kbit/s */
  { 52000, 60000,  24000, 1,  21,  47, 3, 0, -3, BWE_SWITCH_LRC, 10, {1,2,2,2,2,3,3,3,3,3} }, /* nominal: 56 kbit/s */ //wchg 20201204
  /*** stereo ***/
  { 18000, 24000,  16000, 2,  4,  2, 1, 0, -3, BWE_SWITCH_LRC, 3 }, /* nominal: 18 kbit/s */

  { 24000, 28000,  22050, 2,  5,  6, 1, 0, -3, BWE_SWITCH_LRC, 3 }, /* nominal: 24 kbit/s */
  { 28000, 36000,  22050, 2,  7,  8, 2, 0, -3, BWE_SWITCH_LRC, 2 }, /* nominal: 32 kbit/s */
  { 36000, 44000,  22050, 2, 10,  9, 2, 0, -3, BWE_SWITCH_LRC, 2 }, /* nominal: 40 kbit/s */
  { 44000, 52000,  22050, 2, 23,  47, 3, 0, -3, BWE_SWITCH_LRC, 10, {1,2,2,2,2,3,3,3,3,3} }, /* nominal: 48 kbit/s */
  { 52000, 60000,  22050, 2, 21,  47, 3, 0, -3, BWE_SWITCH_LRC, 10, {1,2,2,2,2,3,3,3,3,3} }, /* nominal: 56 kbit/s */ //wchg 20201204

  { 24000, 28000,  24000, 2,  5,  6, 1, 0, -3, BWE_SWITCH_LRC, 3 }, /* nominal: 24 kbit/s */
  { 28000, 36000,  24000, 2,  7,  8, 2, 0, -3, BWE_SWITCH_LRC, 2 }, /* nominal: 32 kbit/s */
  { 36000, 44000,  24000, 2, 10,  9, 2, 0, -3, BWE_SWITCH_LRC, 2 }, /* nominal: 40 kbit/s */
  { 44000, 52000,  24000, 2, 23,  47, 3, 0, -3, BWE_SWITCH_LRC, 10, {1,2,2,2,2,3,3,3,3,3} }, /* nominal: 48 kbit/s */
  { 52000, 60000,  24000, 2, 21,  47, 3, 0, -3, BWE_SWITCH_LRC, 10, {1,2,2,2,2,3,3,3,3,3} }, /* nominal: 56 kbit/s */ //wchg 20201204
};

typedef struct
{
	int blockNum;                /*!< Number of block                    */
	int blockLen;
	int sfbCnt;                 /*!< Number of scalefactor window bands  */
	int sfbWidth[MAX_SFB_NUM];      /*!< Width of scalefactor bands      */
	int sfbOffset[MAX_SFB_NUM+1];   /*!< Start of scalefactor bands      */
	int sfbCntNoiseFloor;
	int sfbNoiseFloorWidth[MAX_NOISE_NUM];
	int sfbNoiseFloorOffset[MAX_NOISE_NUM + 1];
}SFB_PARAM;

//16kb/s table
static const SFB_PARAM sfbParams_16[5] = {
{ //long
	31,					// blocknum
	32,					// blockLen
	12,					// sfbCnt
	{1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3},	
	{0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 18, 21},
	2,	
	{6, 15},	 
	{0, 6, 21},
},
{ //1/2 long 
	31,
	16,
	12,					// sfbCnt
	{1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3},	
	{0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 18, 21},
	2,	
	{6, 15},	 
	{0, 6, 21},
},
{ //1/4 long 
	31,
	8,
	12,					// sfbCnt
	{1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3},	
	{0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 18, 21},
	2,	
	{6, 15},	 
	{0, 6, 21},
},
{ //1/8 long 
	31,
	4,
	12,					// sfbCnt
	{1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3},	
	{0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 18, 21},
	2,	
	{6, 15},	 
	{0, 6, 21},
},
{ //1/16 long 
	31,
	2,
	12,					// sfbCnt
	{1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3},	
	{0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 18, 21},
	2,	
	{6, 15},	 
	{0, 6, 21},
},
};

//20kb/s table
static const SFB_PARAM sfbParams_20[5] = {
{ //long
	34,					// blocknum
	32,					// blockLen
	14,					// sfbCnt
	{1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3},	
	{0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 19, 22},
	3,	
	{4, 4, 14},	 
	{0, 4, 8, 22},
},
{ //1/2 long 
	34,
	16,
	14,
	{1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3},
	{0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 19, 22},
	3,	
	{4, 4, 14},	 
	{0, 4, 8, 22},
},
{ //1/4 long 
	34,
	8,
	14,
	{1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3},
	{0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 19, 22},
	3,	
	{4, 4, 14},	 
	{0, 4, 8, 22},
},
{ //1/8 long 
	34,
	4,
	14,
	{1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3},
	{0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 19, 22},
	3,	
	{4, 4, 14},	 
	{0, 4, 8, 22},
},
{ //1/16 long 
	34,
	2,
	14,
	{1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3},
	{0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 19, 22},
	3,	
	{4, 4, 14},	 
	{0, 4, 8, 22},
},
};

//32kb/s
static const SFB_PARAM sfbParams_32[5] = {
{ //long
	47,					// blocknum
	32,					// blockLen
	12,					// sfbCnt
	{1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3},	
	{0, 1, 2, 4, 6, 8, 10, 12, 14, 17, 20, 23, 26},
	2,	
	{10, 16},	 
	{0, 10, 26},
},
{ //1/2 long 
	47,
	16,
	12,
	{1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3},	
	{0, 1, 2, 4, 6, 8, 10, 12, 14, 17, 20, 23, 26},
	2,	
	{10, 16},	 
	{0, 10, 26},
},
{ //1/4 long 
	47,
	8,
	12,
	{1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3},	
	{0, 1, 2, 4, 6, 8, 10, 12, 14, 17, 20, 23, 26},
	2,	
	{10, 16},	 
	{0, 10, 26},
},
{ //1/8 long 
	47,
	4,
	12,
	{1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3},	
	{0, 1, 2, 4, 6, 8, 10, 12, 14, 17, 20, 23, 26},
	2,	
	{10, 16},	 
	{0, 10, 26},
},
{ //1/16 long 
	47,
	2,
	12,
	{1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3},	
	{0, 1, 2, 4, 6, 8, 10, 12, 14, 17, 20, 23, 26},
	2,	
	{10, 16},	 
	{0, 10, 26},
},
};


static const float smoothFilter[4]  = {0.05857864376269f, 0.2f, 0.34142135623731f, 0.4f};

const int book_envelop_time_code[46] = {0x37FBE, 0x37FBF, 0x37FEA, 0x1BFDC, 0x1BFF4, 0x1BFF6,
0x1BFF7, 0xDFEC, 0xDFFE, 0x6FFC, 0x1BFC, 0x0FFE, 0x1BE, 0x01FE, 0x00FE, 0x001A, 0x001E, 0x000E,
0x0002, 0x0000, 0x0002, 0x0006, 0x000E, 0x001E, 0x000C, 0x003E, 0x001F, 0x007E, 0x0036, 0x006E,
0x00DE, 0x03FE, 0x07FE, 0x037E, 0x06FE, 0x1FFE, 0x3FFE, 0x3FFF, 0x1BFE, 0x37FA, 0x6FFE, 0xDFFF,
0xDFED, 0x1BFDD, 0x1BFDE, 0x37FEB
};

const int book_envelop_time_length[46] = {0x13, 0x13, 0x13, 0x12, 0x12, 0x12, 0x12, 0x11, 0x11, 
0x10, 0x0E, 0x0C, 0x0A, 0x09, 0x08, 0x06, 0x06, 0x05, 0x03, 0x02, 0x02, 0x03, 0x04, 0x05, 0x05,
0x06, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0B, 0x0C, 0x0D, 0x0E, 0x0E, 0x0E, 0x0F, 0x10,
0x11, 0x11, 0x12, 0x12, 0x13
};

const int book_envelop_freq_code[31] = {0x1C00C, 0x1C00E, 0x7002, 0x7006, 0x3800, 0x1C02, 0x0E02,
0x0702, 0x0382, 0x01C2, 0x00E2, 0x0072, 0x003A, 0x001E, 0x0006, 0x0000, 0x0002, 0x0001, 0x001F,
0x003B, 0x0073, 0x00E3, 0x01C3, 0x0383, 0x0703, 0x0E03, 0x1C03, 0x3802, 0x7007, 0x1C00F, 0x1C00D
};

const int book_envelop_freq_length[31] = {0x11, 0x11, 0x0F, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
0x09, 0x08, 0x07, 0x06, 0x05, 0x03, 0x02, 0x02, 0x02, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
0x0C, 0x0D, 0x0E, 0x0F, 0x11, 0x11
};

const int book_noisefloor_time_code[25] = {0x7E7FC, 0xFCFFA, 0xFCFFB, 0x1F9FE, 0x1F9FC, 0x3F3E,
0x0FCE, 0x03F2, 0x01FA, 0x00FE, 0x003E, 0x000E, 0x0006, 0x0000, 0x0002, 0x001E, 0x00FF, 0x01FB,
0x01F8, 0x07E6, 0x1F9E, 0x7E7E, 0x1F9FD, 0x7E7FE, 0x7E7FF
};

const int book_noisefloor_time_length[25] = {0x13, 0x14, 0x14, 0x11, 0x11, 0x0E, 0x0C, 0x0A, 
0x09, 0x08, 0x06, 0x04, 0x03, 0x01, 0x02, 0x05, 0x08, 0x09, 0x09, 0x0B, 0x0D, 0x0F, 0x11, 
0x14, 0x14
};

const int book_noisefloor_freq_code[25] = {0x3E3E, 0x0F8E, 0x07C6, 0x01F0, 0x01F2, 0x00FA,
0x007E, 0x007E, 0x001E, 0x000E, 0x0006, 0x000E, 0x0002, 0x0000, 0x0002, 0x0006, 0x001E, 0x003E, 
0x007F, 0x007F, 0x00FB, 0x01F3, 0x03E2, 0x1F1E, 0x3E3F
};

const int book_noisefloor_freq_length[25] = {0x0F, 0x0D, 0x0C, 0x0A, 0x0A, 0x09, 0x08, 0x07,
0x06, 0x05, 0x04, 0x04, 0x03, 0x02, 0x02, 0x03, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0E, 
0x0F
};

/*******************************************************************************
Functionname:  CalculatePSD
*******************************************************************************
\brief   calculate the PSDs.

\return  
*******************************************************************************/
static void CalculatePSD(float *orig_coef, 
				 float *bwe_coef, 
				 float *orig_psd, 
				 float *bwe_psd,
				 int lg)
{
	int i;

	for (i = 0; i < lg / 2; i++)
	{
		orig_psd[i] = orig_coef[2*i] * orig_coef[2*i] + orig_coef[2*i+1] * orig_coef[2*i+1];
		bwe_psd[i] = bwe_coef[2*i] * bwe_coef[2*i] + bwe_coef[2*i+1] * bwe_coef[2*i+1];
	}
}

static void bub_sort(float p[], int index[], int n)
{
	int m,k,j,i;
	float d;
	int e;
    k=0; 
	m=n-1;
	
	for(i = 0; i < n; i++)
		index[i] = i;

    while (k<m)
    { 
		j=m-1; m=0;
		
        for (i=k; i<=j; i++)
		{
			if (p[i]>p[i+1])
            {
				d=p[i];
				p[i]=p[i+1];
				p[i+1]=d; 
				m=i;
				
				e = index[i];
				index[i] = index[i + 1];
				index[i + 1] = e;
			}
		}
		
		j=k+1; 
		k=0;
		
		for (i=m; i>=j; i--)
		{
			if (p[i-1]>p[i])
			{
				d=p[i];
				p[i]=p[i-1];
				p[i-1]=d;
				k=i;
				
				e = index[i];
				index[i] = index[i - 1];
				index[i - 1] = e;
			}
		}
	}
	
    return;
}

/*******************************************************************************
Functionname:  PreProcessingForPsd
*******************************************************************************
\brief   preprocessing the PSDs of band extension coefficients.
		This module is used to avoid too many dips.

\return  preprocessed PSDs.
*******************************************************************************/
static void PreProcessingForPsd(float* pPSD, float* pPsdDb, int lg, 
							 SFB_PARAM sfbParam,
							 int blockLen,
							 int blockNum)
{
	int i, b;
	float tmpPSD[FRAME_LEN_LONG*2];
	float tmpPSD1[MAX_BLOCK_LEN];
	int tmpIndex[MAX_BLOCK_LEN];
	int processMinNum = (int)(blockLen / 4);

	for (i = 0; i < lg; i++)
	{
		tmpPSD[i] = pPSD[i];
		tmpPSD[i] = (float)(10 * log10(tmpPSD[i] + 0.0000001));
	}

	for (b = 0; b < blockNum; b++)
	{
		for (i = b * blockLen; i < (b + 1) * blockLen; i++)
		{
 			tmpIndex[i - b * blockLen] = i - b * blockLen;
			tmpPSD1[i - b * blockLen] = tmpPSD[i];
		}

		bub_sort(tmpPSD1, tmpIndex, blockLen);

		for (i = b * blockLen; i < (b + 1) * blockLen; i++)
		{
			tmpIndex[i - b * blockLen] = b * blockLen + tmpIndex[i - b * blockLen];
		}

		for (i = 0; i < processMinNum; i++)
		{
			tmpPSD[tmpIndex[i]] = tmpPSD[tmpIndex[processMinNum]];
		}
	}


	for (i = 0; i < lg; i++)
	{
		pPsdDb[i] = tmpPSD[i];
	}
}

/*******************************************************************************
Functionname:  CalculateTNR
*******************************************************************************
\brief   Calculate the Tonal-Noise-Ratios, it's called by CalculateTonalNoiseRatio
	     routine. This module should match the equivalent part in the decoder.

\return  The Tonal-Noise-Ratios.
*******************************************************************************/
static void CalculateTNR(float *psd_db, 
						 float *tnr,
						 int lg, 
						 SFB_PARAM sfbParam,
						 int blockLen,
						 int blockNum)
{
	int i, b;
	float noiseLevel;
	float noiseLevelFloor;
	float decayFactorForPeak = 3;
	float decayFactorForDip = 0.5;	
	float peakPSD[FRAME_LEN_LONG*2];
	float dipPSD[FRAME_LEN_LONG*2];
	float peakSum;
	float dipSum;
	float origSum;


	peakPSD[0] = psd_db[0];
	dipPSD[0] = psd_db[0];

	for (i = 1; i < lg; i++)
	{
		peakPSD[i] = max(peakPSD[i-1] - decayFactorForPeak, psd_db[i]);
		dipPSD[i] = max(min(dipPSD[i-1] + decayFactorForDip, psd_db[i]),0.0);
	}
	
	for (b = 0; b < blockNum; b++)
	{
		peakSum = 0.0;
		dipSum = 0.0;
		origSum = 0.0;

		for (i = b * blockLen; i < (b + 1) * blockLen; i++)
		{
			peakSum += pow(10.0, 0.1*peakPSD[i]);
			dipSum += pow(10.0, 0.1*dipPSD[i]);
			origSum += pow(10.0, 0.1*psd_db[i]);
		}
		
		/* small energy		*/
		if (origSum < 1.0f)
		{
			peakSum = 0.0f;
			dipSum = 0.0f;
			origSum = 0.0f;
		}
		
		noiseLevelFloor = dipSum / (peakSum + 1.0);
		noiseLevel = origSum * (noiseLevelFloor / (1 + noiseLevelFloor));
		
		tnr[b] = (float)((origSum - noiseLevel) / (noiseLevel + 0.000001));
	}
}

/*******************************************************************************
Functionname:  CalculateTonalNoiseRatio
*******************************************************************************
\brief   calculate the Tonal-Noise-Ratio.

\return  
*******************************************************************************/
void CalculateOrigTonalNoiseRatio(float *orig_psd,
							 float *orig_tnr,
							 int lg,
							 SFB_PARAM sfbParam,
							 int blockLen,
							 int blockNum
							 )
{
	float tmpOrigPsd[FRAME_LEN_LONG*2];

	PreProcessingForPsd(orig_psd, tmpOrigPsd, lg, sfbParam, blockLen, blockNum);

	
	CalculateTNR(tmpOrigPsd, 
				 orig_tnr, 
				 lg,
				 sfbParam, blockLen, blockNum);
}

/*******************************************************************************
Functionname:  CalculateBweTonalNoiseRatio
*******************************************************************************
\brief   calculate the Tonal-Noise-Ratio.

\return  
*******************************************************************************/
void CalculateBweTonalNoiseRatio(float *bwe_psd,
							 float *bwe_tnr,
							 int lg,
							 SFB_PARAM sfbParam,
							 int blockLen,
							 int blockNum
							 )
{
	float tmpBwePsd[FRAME_LEN_LONG];

	//calculate TNR

	PreProcessingForPsd(bwe_psd, tmpBwePsd, lg, sfbParam, blockLen, blockNum);
	
	CalculateTNR(tmpBwePsd, 
				 bwe_tnr, 
				 lg,
				 sfbParam, blockLen, blockNum);
}

/*******************************************************************************
Functionname:  CalculateDetectorInput
*******************************************************************************
\brief   calculate the ratio of TNRs between original spectrum and extended spectrum.

\return  
*******************************************************************************/
static void CalculateDetectorInput(float orig_tnr[][MAX_BLOCK_NUM],
								   float bwe_tnr[][MAX_BLOCK_NUM],
								   float diff_tnr_sfb[][MAX_SFB_NUM],
								   SFB_PARAM sfbParam)



{
	int est;
	int i, ll, lu, k;
	float maxValOrig, maxValBwe;

	for(est = 2; est < 4; est++)
	{
		for (i = 0; i < sfbParam.sfbCnt; i++)
		{
			ll = sfbParam.sfbOffset[i];
			lu = sfbParam.sfbOffset[i + 1];

			maxValOrig = 0;
			maxValBwe = 0;

			for (k = ll; k < lu; k++)
			{
				if (orig_tnr[est][k] > maxValOrig)
				{
					maxValOrig = orig_tnr[est][k];
				}

				if (bwe_tnr[est][k] > maxValBwe)
				{
					maxValBwe = bwe_tnr[est][k];
				}
			}
		
			if (maxValBwe >= 1)
			{
				diff_tnr_sfb[est][i] = maxValOrig / maxValBwe;
			}
			else
			{
				diff_tnr_sfb[est][i] = maxValOrig;
			}
		}
	}


}

/*******************************************************************************
Functionname:  CalculateFlatnessMeasure
*******************************************************************************
\brief   calculate the flatness of the input spectrum.

\return  
*******************************************************************************/
static void CalculateFlatnessMeasure(float orig_tnr[][MAX_BLOCK_NUM],
									 float bwe_tnr[][MAX_BLOCK_NUM], 
									 float orig_sfm_sfb[][MAX_SFB_NUM],
									 float bwe_sfm_sfb[][MAX_SFB_NUM],
									 SFB_PARAM sfbParam)
{
	int est;
	int i, j;
	int ll, lu;
	
	for(est = 2; est < 4; est++)
	{
		for (i = 0; i < sfbParam.sfbCnt; i++)
		{
			ll = sfbParam.sfbOffset[i];
			lu = sfbParam.sfbOffset[i + 1];

			orig_sfm_sfb[est][i] = 1;
			bwe_sfm_sfb[est][i] = 1;

			if (lu - ll > 1)
			{
				float amOrig, amBwe, gmOrig, gmBwe, sfmOrig, sfmBwe;

				amOrig = amBwe = 0;
				gmOrig = gmBwe = 1;

				for (j = ll; j < lu; j++)
				{
					sfmOrig = orig_tnr[est][j];
					sfmBwe = bwe_tnr[est][j];

					amOrig += sfmOrig;
					gmOrig *= sfmOrig;
					amBwe += sfmBwe;
					gmBwe *= sfmBwe;
				}

				amOrig /= (lu - ll);
				amBwe /= (lu - ll);
				gmOrig = (float)pow(gmOrig, 1.0/(lu-ll));
				gmBwe = (float)pow(gmBwe, 1.0/(lu-ll));

				if (amOrig)
				{
					orig_sfm_sfb[est][i] = gmOrig / amOrig;
				}

				if (amBwe)
				{
					bwe_sfm_sfb[est][i] = gmBwe / amBwe;
				}
			}
		}		
	}

}

#define THR_DIFF            15.0//25.0f//15.0f
#define THR_DIFF_GUIDE      2.5f
#define THR_TONE_GUIDE      1.26f//15.0f
#define THR_TONE            15.0f
#define THR_SFM_BWE         0.3f//0.1f
#define THR_SFM_ORIG        0.1f//0.29f
#define DECAY_GUIDE_ORIG    0.3f
#define DECAY_GUIDE_DIFF    0.2f
#define I_THR_TONE         (1.0f/15.0f)


/*******************************************************************************
Functionname:  Detector
*******************************************************************************
\brief   detect if sine should be added.

\return  
*******************************************************************************/
static void Detector(float *orig_tnr,
					 float *orig_sfm_sfb,
					 float *bwe_sfm_sfb,
					 float *diff_tnr_sfb,
					 int *addHarmonic,
					 int prevAddHarmonic[],
					 GUIDE_VECTORS guideVectors[],
					 GUIDE_VECTORS *newGuideVectors,
					 SFB_PARAM sfbParam
					)
{
	int i, j;
	float thresTemp, thresOrig;
	int ll, lu;
	
	memset(addHarmonic, 0, MAX_SFB_NUM * sizeof(int));

	for (i = 0; i < sfbParam.sfbCnt; i++)
	{
		if(guideVectors->pGuideVectorDiff[i])
		{
			thresTemp = max(DECAY_GUIDE_DIFF * guideVectors->pGuideVectorDiff[i], THR_DIFF_GUIDE);			
		}
		else
			thresTemp = THR_DIFF;

		thresTemp = min(thresTemp, THR_DIFF);

		if (diff_tnr_sfb[i] > thresTemp)
		{
			addHarmonic[i] = 1;
			newGuideVectors->pGuideVectorDiff[i] = diff_tnr_sfb[i];
		}
		else
		{
			if (guideVectors->pGuideVectorDiff[i])
			{
				guideVectors->pGuideVectorOrig[i] = THR_TONE_GUIDE;
			}
		}
	}

	for(i = 0; i < sfbParam.sfbCnt; i++)
	{
		ll = sfbParam.sfbOffset[i];
		lu = sfbParam.sfbOffset[i + 1];

		thresOrig = max(guideVectors->pGuideVectorOrig[i]*DECAY_GUIDE_ORIG, THR_TONE_GUIDE );

		thresOrig = min(thresOrig, THR_TONE);

		if(guideVectors->pGuideVectorOrig[i])
		{
			for(j = ll; j < lu; j++)
			{
				if(orig_tnr[j] > thresOrig)
				{
					addHarmonic[i] = 1;
					newGuideVectors->pGuideVectorOrig[i] = orig_tnr[j];
				}
			}			
		}
	}

	thresOrig = THR_TONE;
	
	for(i = 0; i <sfbParam.sfbCnt; i++)
	{
		ll = sfbParam.sfbOffset[i];
		lu = sfbParam.sfbOffset[i + 1];

		if(lu - ll > 1)
		{
			for(j = ll; j < lu; j++)
			{
				if(orig_tnr[j] > thresOrig && (bwe_sfm_sfb[i] > THR_SFM_BWE && orig_sfm_sfb[i] < THR_SFM_ORIG) && !addHarmonic[i-1] && !addHarmonic[i+1])
				{
					addHarmonic[i] = 1;
					newGuideVectors->pGuideVectorOrig[i] = orig_tnr[j];
				}
			}
		}
		else
		{
			if(i < sfbParam.sfbCnt - 1)
			{
				if(i > 0)
				{
					if(orig_tnr[ll] >THR_TONE && (diff_tnr_sfb[i] < I_THR_TONE /*||*/&& diff_tnr_sfb[i-1] < I_THR_TONE))
					{
						addHarmonic[i] = 1;
						newGuideVectors->pGuideVectorOrig[i] = orig_tnr[ll];						
					}
				}
				else
				{
					if(orig_tnr[ll] >THR_TONE && diff_tnr_sfb[i + 1] < I_THR_TONE)
					{
						addHarmonic[i] = 1;
						newGuideVectors->pGuideVectorOrig[i] = orig_tnr[ll];						
					}					
				}
			}
		}		
	}

}

/*******************************************************************************
Functionname:  MissingHarmonicsDetector
*******************************************************************************
\brief   Judge if Sine is missed in the extended band, if yes, add it.
         Currently, harmonic(the structure of sines) had not been considered
		 and realized, it should be studied later.

\return  
*******************************************************************************/
void MissingHarmonicsDetector(int onsetFlag, int onsetPos, int newDetectionAllowed,
							  StBweData *BWEDetData, 
							  float orig_tnr[][MAX_BLOCK_NUM], float bwe_tnr[][MAX_BLOCK_NUM], 
							  int addHarmonicVec[], 
							 SFB_PARAM sfbParam)
{
	int i, est;
	float diff_tnr_sfb[4][MAX_SFB_NUM];
	float orig_sfm_sfb[4][MAX_SFB_NUM];
	float bwe_sfm_sfb[4][MAX_SFB_NUM];
    int detectionVectors[4][MAX_SFB_NUM];
	int start;

	memset(addHarmonicVec, 0, MAX_SFB_NUM * sizeof(float));

	if(newDetectionAllowed)
	{
		start = 2;

		memcpy(BWEDetData->guideVectors[2].pGuideVectorDiff, BWEDetData->guideVectors[0].pGuideVectorDiff, MAX_SFB_NUM * sizeof(float));
		memcpy(BWEDetData->guideVectors[2].pGuideVectorOrig, BWEDetData->guideVectors[0].pGuideVectorOrig, MAX_SFB_NUM * sizeof(float));
	}
	else
	{
		start = 0;
	}
	
	for(i = 0; i < 2; i++)
	{
		memcpy(&orig_tnr[i], BWEDetData->prevOrig_tnr[i], MAX_BLOCK_NUM * sizeof(float));
		memcpy(&bwe_tnr[i], BWEDetData->prevBwe_tnr[i], MAX_BLOCK_NUM * sizeof(float));
		memcpy(&diff_tnr_sfb[i], BWEDetData->prevDiff_tnr[i], MAX_SFB_NUM * sizeof(float));
		memcpy(&orig_sfm_sfb[i], BWEDetData->prevOrig_sfm[i], MAX_SFB_NUM * sizeof(float));
		memcpy(&bwe_sfm_sfb[i], BWEDetData->prevBwe_sfm[i], MAX_SFB_NUM * sizeof(float));
	}

	CalculateDetectorInput(orig_tnr, bwe_tnr, diff_tnr_sfb, sfbParam);

	CalculateFlatnessMeasure(orig_tnr, bwe_tnr, orig_sfm_sfb, bwe_sfm_sfb, sfbParam);

	for(i = 0; i < 2; i++)
	{
		memcpy(BWEDetData->prevOrig_tnr[i], &orig_tnr[i+2], MAX_BLOCK_NUM * sizeof(float));
		memcpy(BWEDetData->prevBwe_tnr[i], &bwe_tnr[i+2], MAX_BLOCK_NUM * sizeof(float));
		memcpy(BWEDetData->prevDiff_tnr[i], &diff_tnr_sfb[i+2], MAX_SFB_NUM * sizeof(float));
		memcpy(BWEDetData->prevOrig_sfm[i], &orig_sfm_sfb[i+2], MAX_SFB_NUM * sizeof(float));
		memcpy(BWEDetData->prevBwe_sfm[i], &bwe_sfm_sfb[i+2], MAX_SFB_NUM * sizeof(float));
	}

	for(est = start; est < 4; est++)
	{
		memset(detectionVectors[est], 0, MAX_SFB_NUM * sizeof(int));

		if(est < 3)
		{
			memset(&BWEDetData->guideVectors[est+1].pGuideVectorDiff, 0, MAX_SFB_NUM * sizeof(float));
			memset(&BWEDetData->guideVectors[est+1].pGuideVectorOrig, 0, MAX_SFB_NUM * sizeof(float));

			Detector(
				orig_tnr[est], 
				orig_sfm_sfb[est], 
				bwe_sfm_sfb[est], 
				diff_tnr_sfb[est], 
				detectionVectors[est], 
				BWEDetData->prevAddHarmonicVec,
				&BWEDetData->guideVectors[est], 
				&BWEDetData->guideVectors[est+1], 
				sfbParam);			
		}
		else
		{
			memset(&BWEDetData->guideVectors[est].pGuideVectorDiff, 0, MAX_SFB_NUM * sizeof(float));
			memset(&BWEDetData->guideVectors[est].pGuideVectorOrig, 0, MAX_SFB_NUM * sizeof(float));

			Detector(
				orig_tnr[est], 
				orig_sfm_sfb[est], 
				bwe_sfm_sfb[est], 
				diff_tnr_sfb[est], 
				detectionVectors[est], 
				BWEDetData->prevAddHarmonicVec,
				&BWEDetData->guideVectors[est], 
				&BWEDetData->guideVectors[est], 
				sfbParam);			
		}		
	}

	for(i = 0; i < sfbParam.sfbCnt; i++)
	{
		for(est = start; est < 4; est++)
			addHarmonicVec[i] = addHarmonicVec[i] || detectionVectors[est][i];
	}

	if(!newDetectionAllowed)
	{
 		for(i=0; i<sfbParam.sfbCnt; i++)
		{
			if(addHarmonicVec[i] - BWEDetData->prevAddHarmonicVec[i] > 0)
				addHarmonicVec[i] = 0;
		}
	}
	
#if 0
	addHarmonicVec[0] = 0;//first band
#endif
	//Calculates a compensation vector
	
	memcpy(BWEDetData->guideVectors[0].pGuideVectorDiff, BWEDetData->guideVectors[2].pGuideVectorDiff, MAX_SFB_NUM * sizeof(float));
	memcpy(BWEDetData->guideVectors[0].pGuideVectorOrig, BWEDetData->guideVectors[2].pGuideVectorOrig, MAX_SFB_NUM * sizeof(float));

	for(i = 0; i < sfbParam.sfbCnt; i++)
	{
		if( (BWEDetData->guideVectors[0].pGuideVectorDiff[i] || BWEDetData->guideVectors[0].pGuideVectorOrig[i] ) && !addHarmonicVec[i] )
		{
			BWEDetData->guideVectors[0].pGuideVectorDiff[i] = 0.0;
			BWEDetData->guideVectors[0].pGuideVectorOrig[i] = 0.0;			
		}
	}


}

/*******************************************************************************
Functionname:  CalculateHfEnvelop
*******************************************************************************
\brief   Calculate the envelopes of HF bands. 

\return  
*******************************************************************************/
void CalculateHfEnvelop(int bitRate,
						int nLen,
						int addHarmonicFlag, int addHarmonicVec[], 
						float mdftSpectrum[],      /*mdft spectrum */
						float envelop[], int mod)
{
	int lg_lf, lg_hf, lg;
	int copy_start1, copy_start2, copy_start3;
	int copy_len1, copy_len2, copy_len3;
	float orig_buffer[FRAME_LEN_LONG*4+128];//wchg 内存保护
	float orig_psd[FRAME_LEN_LONG*2];

	int sfb, i, k, missingHarmonicFlag;
	float energy, energy_tmp;
	int li, ui;
	int count;
	
	SFB_PARAM sfbParam;
	int blockLen;
	int blockNum;


	lg = nLen;

	if(bitRate < 20000)
	{
		sfbParam = sfbParams_16[mod];
	    blockLen = sfbParam.blockLen;
	    blockNum = sfbParam.blockNum-10;

		lg_lf = lg * 5 / 32;
		copy_start1 = lg / 32;
		copy_len1 = lg_lf - copy_start1;
		copy_start2 = copy_start1;
		copy_len2 = copy_start1 * 3 + copy_start1 / 2;
		copy_start3 = copy_start1 + copy_start1 / 2;
		copy_len3 = copy_start1 * 3;
	}	
	else if(bitRate>=20000 && bitRate<28000)
	{	
/*		sfbParam = sfbParams_20[mod];
	    blockLen = sfbParam.blockLen;
	    blockNum = sfbParam.blockNum-12;

		lg_lf = lg * 3 / 16;
		copy_start1 = lg / 32;
		copy_len1 = lg_lf - copy_start1;
		copy_start2 = copy_start1;
		copy_len2 = copy_start1 * 4 + copy_start1 / 2;
		copy_start3 = copy_len2;
		copy_len3 = copy_start1 + copy_start1 / 2;	*/	
		sfbParam = sfbParams_16[mod];
		blockLen = sfbParam.blockLen;
		blockNum = sfbParam.blockNum - 10;

		lg_lf = lg * 5 / 32;
		copy_start1 = lg / 32;
		copy_len1 = lg_lf - copy_start1;
		copy_start2 = copy_start1;
		copy_len2 = copy_start1 * 3 + copy_start1 / 2;
		copy_start3 = copy_start1 + copy_start1 / 2;
		copy_len3 = copy_start1 * 3;
	}
	else if(bitRate >= 28000)
	{
		sfbParam = sfbParams_32[mod];
	    blockLen = sfbParam.blockLen;
	    blockNum = sfbParam.blockNum-21;

		lg_lf = lg * 21 / 64;
		copy_start1 = lg / 64;
		copy_len1 = lg_lf - copy_start1;
		copy_start2 = lg * 15 / 64;
		copy_len2 = copy_start1 * 6;
		copy_start3 = 0;
		copy_len3 = 0;			
	}
	
	lg_hf = copy_len1 + copy_len2 + copy_len3;
	
	memcpy(orig_buffer, mdftSpectrum, lg * 2 * sizeof(float));
	
	for (i = 0; i < lg_hf; i++)
		orig_psd[i] = orig_buffer[lg_lf*2+2*i]*orig_buffer[lg_lf*2+2*i] + orig_buffer[lg_lf*2+2*i+1]*orig_buffer[lg_lf*2+2*i+1];
	
	
	for (sfb = 0; sfb < sfbParam.sfbCnt; sfb++)
	{
		
		li = sfbParam.sfbOffset[sfb];
		ui = sfbParam.sfbOffset[sfb + 1];		
		
		missingHarmonicFlag = 0;		
		if(addHarmonicFlag)
		{
			if(addHarmonicVec[sfb])
				missingHarmonicFlag = 1;
		}

		if(missingHarmonicFlag)
		{

			count = blockLen;
			energy = 0.0f;
			for (i = li * blockLen; i < (li+1) * blockLen; i++)
			{
				energy += orig_psd[i];
			}	
			
			for(k=li+1; k<ui; k++)
			{
				energy_tmp = 0.0;
				for(i = k * blockLen; i < (k+1) * blockLen; i++)
				{
					energy_tmp += orig_psd[i];
				}
				
				if(energy_tmp > energy)
					energy = energy_tmp;
			}
		}
		else
		{
			count = (ui - li) * blockLen;
			energy = 0.0f;

			for (i = li * blockLen; i < ui * blockLen; i++)
			{
				energy += orig_psd[i];
			}			

		}
		energy /= (float)count;		

		/*! for quanting	*/
		envelop[sfb] = energy;
		
	}

	return;
}


void CalculateHfEnvelop_M3(int bitRate,
						int nLen,
						int addHarmonicFlag, int addHarmonicVec[], 
						float mdftSpectrum[],      /*mdft spectrum */
						float envelop[], int mod)
{
	int lg_lf, lg_hf, lg;
	int copy_start1, copy_start2, copy_start3;
	int copy_len1, copy_len2, copy_len3;
	float orig_buffer[FRAME_LEN_LONG*4+128];//wchg 内存保护
	float orig_psd[FRAME_LEN_LONG*2];

	int sfb, i, k, missingHarmonicFlag;
	float energy, energy_tmp;
	int li, ui;
	int count;
	
	SFB_PARAM sfbParam;
	int blockLen;
	int blockNum;
	int crossflag = 0;

	if(mod==0)
	{
		 BLOCKLEN32_WIN16=16;
	}else if(mod==1)
	{
		 BLOCKLEN32_WIN16=8;
	}else if(mod==2)
	{
		 BLOCKLEN32_WIN16=4;
	}else if(mod==3)
	{
		 BLOCKLEN32_WIN16=2;
	}else 
		BLOCKLEN32_WIN16=1;


	lg = nLen;

    if(bitRate < 20000 )
	{
		sfbParam = sfbParams_16[mod];
	    blockLen = sfbParam.blockLen;
	    blockNum = sfbParam.blockNum-10;

		lg_lf = lg * 5 / 32;
		copy_start1 = lg / 32;
		copy_len1 = lg_lf - copy_start1;
		copy_start2 = copy_start1;
		copy_len2 = copy_start1 * 3 + copy_start1 / 2;
		copy_start3 = copy_start1 + copy_start1 / 2;
		copy_len3 = copy_start1 * 3;
	}
	else if(bitRate >= 20000 && bitRate < 28000)
	{
	/*	sfbParam = sfbParams_20[mod];
	    blockLen = sfbParam.blockLen;
	    blockNum = sfbParam.blockNum-12;

		lg_lf = lg * 3 / 16;
		copy_start1 = lg / 32;
		copy_len1 = lg_lf - copy_start1;
		copy_start2 = copy_start1;
		copy_len2 = copy_start1 * 4 + copy_start1 / 2;
		copy_start3 = copy_len2;
		copy_len3 = copy_start1 + copy_start1 / 2;		*/
		sfbParam = sfbParams_16[mod];
		blockLen = sfbParam.blockLen;
		blockNum = sfbParam.blockNum - 10;

		lg_lf = lg * 5 / 32;
		copy_start1 = lg / 32;
		copy_len1 = lg_lf - copy_start1;
		copy_start2 = copy_start1;
		copy_len2 = copy_start1 * 3 + copy_start1 / 2;
		copy_start3 = copy_start1 + copy_start1 / 2;
		copy_len3 = copy_start1 * 3;
	}
	else if(bitRate>= 28000)
	{
		sfbParam = sfbParams_32[mod];
	    blockLen = sfbParam.blockLen;
	    blockNum = sfbParam.blockNum-21;

		lg_lf = lg * 21 / 64;
		copy_start1 = lg / 64;
		copy_len1 = lg_lf - copy_start1;
		copy_start2 = lg * 15 / 64;
		copy_len2 = copy_start1 * 6;
		copy_start3 = 0;
		copy_len3 = 0;
	}
	
	lg_hf = copy_len1 + copy_len2 + copy_len3;
	
	memcpy(orig_buffer, mdftSpectrum, lg * 2 * sizeof(float));
	
	for (i = 0; i < lg_hf; i++)
		orig_psd[i] = (orig_buffer[lg_lf * 2 + 2 * i] * orig_buffer[lg_lf * 2 + 2 * i] + orig_buffer[lg_lf * 2 + 2 * i + 1] * orig_buffer[lg_lf * 2 + 2 * i + 1]) / 8 / 8;
	
	
	for (sfb = 0; sfb < sfbParam.sfbCnt; sfb++)
	{
		li = sfbParam.sfbOffset[sfb];
		ui = sfbParam.sfbOffset[sfb + 1];		
		
		missingHarmonicFlag = 0;		
		if(addHarmonicFlag)
		{
			if(addHarmonicVec[sfb])
				missingHarmonicFlag = 1;
		}

		if(missingHarmonicFlag)
		{
			count = blockLen;
			energy = 0.0f;
			if (crossflag == 0)
			{
				for (i = li * blockLen; i < (li + 1) * blockLen; i++)
				{
					energy += orig_psd[i];
				}
			}
			
			for(k=li+1; k<ui; k++)
			{
				energy_tmp = 0.0;
				for(i = k * blockLen; i < (k+1) * blockLen; i++)
				{
					energy_tmp += orig_psd[i];
				}
				
				if(energy_tmp > energy)
					energy = energy_tmp;

				energy_tmp = 0.0;
				for (i = k * blockLen - blockLen / 2; i < (k + 1) * blockLen - blockLen / 2; i++)
				{
					energy_tmp += orig_psd[i];
				}

				if (energy_tmp > energy)
					energy = energy_tmp;
			}
			//if (crossflag == 0)
			{	float energy_tmp2 = 0;
				k = ui;
				energy_tmp = 0.0;
				for (i = k * blockLen - blockLen / 2; i < (k + 1) * blockLen - blockLen / 2; i++)
				{
					energy_tmp += orig_psd[i];
				}
				energy_tmp2 = 0.0;
				for (i = k * blockLen; i < (k + 1) * blockLen; i++)
				{
					energy_tmp2 += orig_psd[i];
				}
				if ((energy_tmp > energy) && (energy_tmp>energy_tmp2))
				{
					energy = energy_tmp;
					crossflag = 1;
				}
				else
					crossflag = 0;
			}
		}
		else
		{
			crossflag = 0;
			count = (ui - li) * blockLen;
			energy = 0.0f;

			for (i = li * blockLen; i < ui * blockLen; i++)
			{
				energy += orig_psd[i];
			}	
			///////////////////////////////////////////////
			{
				energy = 0.0f;

				if(sfb>0)
				for (i = li * blockLen-BLOCKLEN32_WIN16; i < li * blockLen+0; i++)
				{
					energy += orig_psd[i]*sin(3.1415926/4.0*( BLOCKLEN32_WIN16-li * blockLen+i+0.5)/BLOCKLEN32_WIN16)*sin(3.1415926/4.0*( BLOCKLEN32_WIN16-li * blockLen+i+0.5)/BLOCKLEN32_WIN16);//(pow(2,i - (li * blockLen-0.5))*0.5);
					energy += orig_psd[ li * blockLen*2-1-i]*cos(3.1415926/4.0*( BLOCKLEN32_WIN16-li * blockLen+i+0.5)/BLOCKLEN32_WIN16)*cos(3.1415926/4.0*( BLOCKLEN32_WIN16-li * blockLen+i+0.5)/BLOCKLEN32_WIN16);//*(1-pow(2,i - (li * blockLen-0.5))*0.5);
				}else
					for (i = li * blockLen; i < li * blockLen+BLOCKLEN32_WIN16; i++)
				{
					energy += orig_psd[i];
					
				}
				
				
				if(li * blockLen+BLOCKLEN32_WIN16< ui * blockLen-BLOCKLEN32_WIN16)
				for (i = li * blockLen+BLOCKLEN32_WIN16; i < ui * blockLen-BLOCKLEN32_WIN16; i++)
				{
					energy += orig_psd[i];
				}

				if(sfb<sfbParam.sfbCnt-1)
				for (i = ui * blockLen-BLOCKLEN32_WIN16; i < ui * blockLen+0; i++)
				{
					energy += orig_psd[i]*sin(3.1415926/4.0*( BLOCKLEN32_WIN16+ui * blockLen-i-0.5)/BLOCKLEN32_WIN16)*sin(3.1415926/4.0*(BLOCKLEN32_WIN16+ ui * blockLen-i-0.5)/BLOCKLEN32_WIN16);//(1-pow(2,i - (ui * blockLen-0.5))*0.5);
					energy += orig_psd[ ui * blockLen*2-1-i]*cos(3.1415926/4.0*(BLOCKLEN32_WIN16+ ui * blockLen-i-0.5)/BLOCKLEN32_WIN16)*cos(3.1415926/4.0*(BLOCKLEN32_WIN16+ ui * blockLen-i-0.5)/BLOCKLEN32_WIN16);//*(pow(2,i - (ui * blockLen-0.5))*0.5);
			
				}
				else
				for (i = ui * blockLen-BLOCKLEN32_WIN16; i < ui * blockLen; i++)
				{
					energy += orig_psd[i];
				}

			}
			////////////////////////////////////////////////

		}

		energy /= (float)count;		

		/*! for quanting	*/
		envelop[sfb] = energy;
	
	}

	return;
}

/*******************************************************************************
Functionname:  NoiseFloorDetection
*******************************************************************************
\brief   calculate the Noise-Floor of the noise band.

\return  
*******************************************************************************/
static void NoiseFloorDetection(float orig_tnr[][MAX_BLOCK_NUM],
								float bwe_tnr[][MAX_BLOCK_NUM],
								int startIndex,
								int stopIndex,
								float *noiseFloor,
								int addHarmonicFlag, 
								int startSfb,
								int stopSfb)
{
	int l, k;
	float meanOrig = 0, meanBwe = 0;
	float tonalityOrig, tonalityBwe;
	float diff;

	if( addHarmonicFlag )   /*! if sine addition is done	*/
	{
		for (l = startSfb; l < stopSfb; l++)
		{
			tonalityOrig = 0;
			tonalityBwe = 0;
			
			for(k = startIndex; k < stopIndex; k++)
			{
				tonalityOrig += orig_tnr[k][l];
				tonalityBwe += bwe_tnr[k][l];
			}

			tonalityOrig /= (stopIndex - startIndex);
			tonalityBwe /= (stopIndex - startIndex);

			if (tonalityOrig > meanOrig)		
				meanOrig = tonalityOrig;	
			
			if (tonalityBwe > meanBwe)	
				meanBwe = tonalityBwe;
		}
	}	
	else
	{
		for (l = startSfb; l < stopSfb; l++)
		{
			tonalityOrig = 0;
			tonalityBwe = 0;
			
			for(k = startIndex; k < stopIndex; k++)
			{
				tonalityOrig += orig_tnr[k][l];
				tonalityBwe += bwe_tnr[k][l];
			}

   		    meanOrig += tonalityOrig;
			meanBwe += tonalityBwe;
		}

		meanOrig = tonalityOrig / (stopSfb - startSfb);
		meanBwe = tonalityBwe / (stopSfb - startSfb);
	}
	

	/*! small fix to avoid noise during silent passages		*/
	if (meanOrig < 0.000976562 || meanBwe < 0.000976562) /*! -30dB	*/
	{
		meanOrig = 101.5936673f;	/*! 20dB	*/
		meanBwe = 101.5936673f;
	}

	if (meanOrig < 1)	meanOrig = 1;
	if (meanBwe < 1)	meanBwe = 1;


	if(addHarmonicFlag)
	{
		diff = 1.0;
	}
	else
	{
		diff = max(1.0, meanBwe/meanOrig);
	}

	*noiseFloor = diff / meanOrig;
	
	*noiseFloor = min(*noiseFloor, 2.0);
}

/*******************************************************************************
Functionname:  NoiseFloorEstimate
*******************************************************************************
\brief   Estimate the Noise-Floor, and decide if the noise addition should be done.
         This module is totally based on the estimation of TNR. 

\return  
*******************************************************************************/
void NoiseFloorEstimate(int nGroupNum, int addHarmonicFlag, float orig_tnr[][MAX_BLOCK_NUM], float bwe_tnr[][MAX_BLOCK_NUM], 
						float noiseFloor[][MAX_NOISE_NUM], float prevNoiseFloor[][MAX_NOISE_NUM], SFB_PARAM sfbParam)
{
	int sfb;
	int i, j;
	int nNoiseFloorNum, startPos[2], stopPos[2];

	if(nGroupNum == 1)
	{
		nNoiseFloorNum = 1;

		startPos[0] = 2;//0;
		stopPos[0] = 4;//2;
	}
	else
	{
		nNoiseFloorNum = 2;

		startPos[0] = 2;//0;
		stopPos[0] = 3;//1;
		startPos[1] = 3;//1;
		stopPos[1] = 4;//2;
	}

	for(i = 0; i < nNoiseFloorNum; i++)
	{
		for (sfb = 0; sfb < sfbParam.sfbCntNoiseFloor; sfb++)
		{
			NoiseFloorDetection(orig_tnr, 
				bwe_tnr,
				startPos[i],
				stopPos[i],
				&noiseFloor[i][sfb],
				addHarmonicFlag,
				sfbParam.sfbNoiseFloorOffset[sfb],
				sfbParam.sfbNoiseFloorOffset[sfb + 1]
				);

		}

	}

	//smooth noise floor using the previous noise floor
	for(i = 0; i < nNoiseFloorNum; i++)
	{
		for(j = 1; j < 4; j++)
			memcpy(prevNoiseFloor[j-1], prevNoiseFloor[j], sfbParam.sfbCntNoiseFloor * sizeof(float));
		memcpy(prevNoiseFloor[3], noiseFloor[i], sfbParam.sfbCntNoiseFloor * sizeof(float));

		for(sfb = 0; sfb < sfbParam.sfbCntNoiseFloor; sfb++)
		{
			noiseFloor[i][sfb] = 0.0;
			for(j = 0; j < 4; j++)
				noiseFloor[i][sfb] += smoothFilter[j] * prevNoiseFloor[j][sfb]; 
		}
	}
}

/*******************************************************************************
Functionname:  QuantBweInfo
*******************************************************************************
\brief   Quantize the information of Band Width Extension. 

  The quantized formula:
    ILOG2 = 1/Log2
                                      energy
	quantized_energy = INT[max(0, Log(------ + 1e-12) * ILOG2) + 0.5]
									   64.0
     
    quantized_noise_floor = INT[NOISE_FLOOR_OFFSET -Log(noise_floor) * ILOG2 +0.5]
\return  
*******************************************************************************/
int QuantEnvelop(float envelop[], 
				  StCoderHFPrmData *prm, 
				  SFB_PARAM sfbParam,
				  int bitsPerSample)
{	
	int hfBits = 0;
	int sfb;
	float energy;
	int quanted_energy;	
	int *p_prm_env;

	p_prm_env = prm->hf_env;

	/*! quantize the envelope information	*/
	for (sfb = 0; sfb < sfbParam.sfbCnt; sfb++)
	{
		energy = envelop[sfb] * 0.5;
		
		energy = (float)(log(energy/(64.0) + EPS) * ILOG2 -2*(bitsPerSample-16));
		energy = energy < 0 ? 0 : energy;
		
		quanted_energy = (int)(energy /*+ 0.5*/);
		if(quanted_energy > 31)
			quanted_energy = 31; //limited the maximum

		*p_prm_env++ = quanted_energy;
	}

	return hfBits;
}
int QuantNoiseFloor(float noiseFloor[], 
				  StCoderHFPrmData *prm, 
				  SFB_PARAM sfbParam)
{
	int hfBits = 0;
	int sfb;
	float noise_floor;
	int quanted_noise_floor;	
	int *p_prm_noise_floor;

	p_prm_noise_floor = prm->noise_floor;

	/*! quantized the noise_floor information */
	for (sfb = 0; sfb < sfbParam.sfbCntNoiseFloor; sfb++)
	{
		noise_floor = noiseFloor[sfb];
		quanted_noise_floor = (int)(NOISE_FLOOR_OFFSET - log(noise_floor)*ILOG2 /*+ 0.5*/);

//		if(quanted_noise_floor > 36 )
//			quanted_noise_floor = 36;

		if(quanted_noise_floor > 20)
			quanted_noise_floor = 20;
		if(quanted_noise_floor < 5)
			quanted_noise_floor = 5;

		*p_prm_noise_floor++ = quanted_noise_floor;
	}

	return hfBits;
}

void ExtractTonalPara(int bitRate,
					  int nGroupNum,
					  int onsetFlag,
					int onsetPos,
			  float mdftSpectrum[],      /*mdft spectrum */
			  float mdftSpectrum_4096[],
			  int *addHarmonicFlag,
			  int addHarmonicVec[],
			  float noiseFloor[][MAX_NOISE_NUM],
			  StBweData *BweDetData,
			  StAvs2BweCommon_li *pstBweCommon)
{
	int i, k;
	int lg_lf, lg_hf, lg;
	float orig_buffer[FRAME_LEN_LONG*4+128];//wchg 内存保护
	float orig_psd[FRAME_LEN_LONG*2], bwe_psd[FRAME_LEN_LONG*2];
	float orig_tnr[4][MAX_BLOCK_NUM], bwe_tnr[4][MAX_BLOCK_NUM];
	int copy_start1, copy_start2, copy_start3;
	int copy_len1, copy_len2, copy_len3;
	float orig_magn[2048], copy_magn[2048];
	int sfbMode = 1;
	int transientFrame, newDetectionAllowed;
	SFB_PARAM sfbParam;
	int blockLen;         /*block length */
	int blockNum;


	//check if a transient frame 
	transientFrame = 0;
	if( pstBweCommon->nOnsetFlag )
	{
		transientFrame = 1;
	}
	else
	{
		if(BweDetData->prevTransientFrame && !BweDetData->prevTransientFlag)
			transientFrame = 1;
	}
	newDetectionAllowed = 0;

	if(transientFrame)
		newDetectionAllowed = 1;
	
	for(k = 0; k < 2; k++)
	{
		lg = 1024;
		memcpy(orig_buffer, &mdftSpectrum[k*2*lg], lg * 2 * sizeof(float));

		sfbMode = 1;
		
		if(bitRate < 20000)
		{
			sfbParam = sfbParams_16[sfbMode];
			blockLen = sfbParam.blockLen;
			blockNum = sfbParam.blockNum-10;

			lg_lf = lg * 5 / 32;
			copy_start1 = lg / 32;
			copy_len1 = lg_lf - copy_start1;
			copy_start2 = copy_start1;
			copy_len2 = copy_start1 * 3 + copy_start1 / 2;
			copy_start3 = copy_start1 + copy_start1 / 2;
			copy_len3 = copy_start1 * 3;
		}
		else if(bitRate >= 20000 && bitRate < 28000)
		{
	/*	    sfbParam = sfbParams_20[sfbMode];
	        blockLen = sfbParam.blockLen;
	        blockNum = sfbParam.blockNum-12;

			lg_lf = lg * 3 / 16;
			copy_start1 = lg / 32;
			copy_len1 = lg_lf - copy_start1;
			copy_start2 = copy_start1;
			copy_len2 = copy_start1 * 4 + copy_start1 / 2;
			copy_start3 = copy_len2;
			copy_len3 = copy_start1 + copy_start1 / 2;*/
			sfbParam = sfbParams_16[sfbMode];
			blockLen = sfbParam.blockLen;
			blockNum = sfbParam.blockNum - 10;

			lg_lf = lg * 5 / 32;
			copy_start1 = lg / 32;
			copy_len1 = lg_lf - copy_start1;
			copy_start2 = copy_start1;
			copy_len2 = copy_start1 * 3 + copy_start1 / 2;
			copy_start3 = copy_start1 + copy_start1 / 2;
			copy_len3 = copy_start1 * 3;
		}
		else if(bitRate >= 28000)
		{
			sfbParam = sfbParams_32[sfbMode];
			blockLen = sfbParam.blockLen;
			blockNum = sfbParam.blockNum-21;

			lg_lf = lg * 21 / 64;
			copy_start1 = lg / 64;
			copy_len1 = lg_lf - copy_start1;
			copy_start2 = lg * 15 / 64;
			copy_len2 = copy_start1 * 6;
			copy_start3 = 0;
			copy_len3 = 0;	
		}
		
		lg_hf = copy_len1 + copy_len2 + copy_len3;
		
		for(i = 0; i < lg; i++)
			orig_magn[i] = (float)sqrt(orig_buffer[2*i]*orig_buffer[2*i] + orig_buffer[2*i+1]*orig_buffer[2*i+1]);
		for (i = 0; i < lg; i++)
			orig_psd[i] = orig_buffer[2*i] * orig_buffer[2*i] + orig_buffer[2*i+1] * orig_buffer[2*i+1];

		//copy
		memcpy(copy_magn, &orig_magn[copy_start1], copy_len1 * sizeof(float));
		memcpy(&copy_magn[copy_len1], &orig_magn[copy_start2], copy_len2 * sizeof(float));
		memcpy(&copy_magn[copy_len1 + copy_len2], &orig_magn[copy_start3], copy_len3 * sizeof(float));

		//calculate power of copied mdft
		for(i = 0; i < lg_hf; i++)
			bwe_psd[i] = copy_magn[i] * copy_magn[i];		

		//calculate original and bwe TNR of every block
//		CalculateOrigTonalNoiseRatio(orig_psd, tmp_tnr, (lg_lf+lg_hf), sfbMode);
		CalculateBweTonalNoiseRatio(bwe_psd, bwe_tnr[k+2], lg_hf, sfbParam, blockLen, blockNum);
		
		CalculateOrigTonalNoiseRatio(&orig_psd[lg_lf], orig_tnr[k+2], lg_hf, sfbParam, blockLen, blockNum);

	}

	//detect the missing sine
	MissingHarmonicsDetector(onsetFlag, onsetPos, newDetectionAllowed, BweDetData, 
		orig_tnr, bwe_tnr, addHarmonicVec, sfbParam);

	BweDetData->prevTransientFrame = transientFrame;
	BweDetData->prevTransientFlag = pstBweCommon->nOnsetFlag;


	//determine if adding harmonic
	*addHarmonicFlag = 0;
	for (i = 0; i < sfbParam.sfbCnt; i++)
	{
		if (addHarmonicVec[i])
		{
			*addHarmonicFlag = 1;
			break;
		}
	}

	memcpy(BweDetData[0].prevAddHarmonicVec, addHarmonicVec, MAX_SFB_NUM * sizeof(float));

	//estimate noise floor
	if(nGroupNum > 1)
		NoiseFloorEstimate(nGroupNum, *addHarmonicFlag, orig_tnr, bwe_tnr, noiseFloor, 
			BweDetData->prevNoiseFloor, sfbParam);
	else
	{
		int sfb;

		lg = 2048;
		memcpy(orig_buffer, mdftSpectrum_4096, lg * 2 * sizeof(float));

		sfbMode = 0;

		if(bitRate < 20000)
		{
			sfbParam = sfbParams_16[0];
			blockLen = sfbParam.blockLen;
			blockNum = sfbParam.blockNum-10;

			lg_lf = lg * 5 / 32;
			copy_start1 = lg / 32;
			copy_len1 = lg_lf - copy_start1;
			copy_start2 = copy_start1;
			copy_len2 = copy_start1 * 3 + copy_start1 / 2;
			copy_start3 = copy_start1 + copy_start1 / 2;
			copy_len3 = copy_start1 * 3;
		}
		else if(bitRate >= 20000 && bitRate < 28000)
		{
		/*    sfbParam = sfbParams_20[0];
	        blockLen = sfbParam.blockLen;
	        blockNum = sfbParam.blockNum-12;

			lg_lf = lg * 3 / 16;
			copy_start1 = lg / 32;
			copy_len1 = lg_lf - copy_start1;
			copy_start2 = copy_start1;
			copy_len2 = copy_start1 * 4 + copy_start1 / 2;
			copy_start3 = copy_len2;
			copy_len3 = copy_start1 + copy_start1 / 2;		*/
			sfbParam = sfbParams_16[0];
			blockLen = sfbParam.blockLen;
			blockNum = sfbParam.blockNum - 10;

			lg_lf = lg * 5 / 32;
			copy_start1 = lg / 32;
			copy_len1 = lg_lf - copy_start1;
			copy_start2 = copy_start1;
			copy_len2 = copy_start1 * 3 + copy_start1 / 2;
			copy_start3 = copy_start1 + copy_start1 / 2;
			copy_len3 = copy_start1 * 3;
		}
		else if(bitRate >= 28000)
		{
			sfbParam = sfbParams_32[0];
			blockLen = sfbParam.blockLen;
			blockNum = sfbParam.blockNum-21;

			lg_lf = lg * 21 / 64;
			copy_start1 = lg / 64;
			copy_len1 = lg_lf - copy_start1;
			copy_start2 = lg * 15 / 64;
			copy_len2 = copy_start1 * 6;
			copy_start3 = 0;
			copy_len3 = 0;	
		}

		lg_hf = copy_len1 + copy_len2 + copy_len3;
		
		for(i = 0; i < lg; i++)
			orig_magn[i] = (float)sqrt(orig_buffer[2*i]*orig_buffer[2*i] + orig_buffer[2*i+1]*orig_buffer[2*i+1]);
		for (i = 0; i < lg; i++)
			orig_psd[i] = orig_buffer[2*i] * orig_buffer[2*i] + orig_buffer[2*i+1] * orig_buffer[2*i+1];

		//copy
		memcpy(copy_magn, &orig_magn[copy_start1], copy_len1 * sizeof(float));
		memcpy(&copy_magn[copy_len1], &orig_magn[copy_start2], copy_len2 * sizeof(float));
		memcpy(&copy_magn[copy_len1 + copy_len2], &orig_magn[copy_start3], copy_len3 * sizeof(float));

		//calculate power of copied mdft
		for(i = 0; i < lg_hf; i++)
			bwe_psd[i] = copy_magn[i] * copy_magn[i];		

		//calculate original and bwe TNR of every block
		CalculateOrigTonalNoiseRatio(&orig_psd[lg_lf], orig_tnr[0], lg_hf, sfbParam, blockLen, blockNum);
		CalculateBweTonalNoiseRatio(bwe_psd, bwe_tnr[0], lg_hf, sfbParam, blockLen, blockNum);


		for (sfb = 0; sfb < sfbParam.sfbCntNoiseFloor; sfb++)
		{
			NoiseFloorDetection(orig_tnr, 
				bwe_tnr,
				0,
				1,
				&noiseFloor[0][sfb],
				*addHarmonicFlag,
				sfbParam.sfbNoiseFloorOffset[sfb],
				sfbParam.sfbNoiseFloorOffset[sfb + 1]
				);

		}
	}

	return;
}

//grouping of short blocks
void HFPrmGrouping(int li,
				   int ld,
				   StHFPrm *prm,
				   StCoderHFPrmData *prm_data,
				   SFB_PARAM sfbParam,
				   int bitsPerSample)
{
	float envelop[MAX_SFB_NUM];
	float noiseFloor[MAX_NOISE_NUM];
	int i, j;
	float temp;

	for(j = 0; j < sfbParam.sfbCnt; j++)
	{
		temp = 0.0;
		for(i = li; i < ld; i++)
		{
			temp += prm[i].envelop[j];
		}

		envelop[j] = temp / (float)(ld - li);
	}

	for(j = 0; j < sfbParam.sfbCntNoiseFloor; j++)
	{
		temp = 0.0;
		for(i = li; i < ld; i++)
		{
			temp += prm[i].noiseFloor[j];
		}
		noiseFloor[j] = temp / (float)(ld - li);
	}

	//quantize grouped parameters
	QuantEnvelop(envelop, prm_data, sfbParam,bitsPerSample);

	return;
}

unsigned int shift_fun(unsigned int data, unsigned char len)  
{  
    unsigned char i;  
    unsigned int tmp=0x0000;  
  
    for(i=0;i<len;i++)  
    {  
        tmp=((data>>i)&0x0001)|tmp;  
        if(i<len-1)  
            tmp=tmp<<1;  
    }  
  
    //printf("  after shift fun1 data=%x \n",tmp);  
  
    return tmp;  
      
}  

int Avs2BweEncoder(
			  unsigned char *ancBytes,    /*!< pointer to ancillary data bytes */
			  int *numAncDataBytes,
			  int *st_in,
			  int *st_common,
			  int bitRate,
			  int bitsPerSample)
{
	int (*BlockTabletmp)[16];
	int (*GroupTabletmp)[10];
	int nGroupNum;
	
	int li, ld;
	int lstart;
	int i, j;
	int nBlockNum, nBlockLen;
	int sfbMode;
    SFB_PARAM sfbParam;

	StHFPrm prm[12];
	int addHarmonicVec[MAX_SFB_NUM];
	int addHarmonicFlag;
	float noiseFloor[2][MAX_NOISE_NUM];

	int len;
    StCoderHFPrmData prm_data[8];


	int hf_envelop[MAX_SFB_NUM];
	int hf_envelop_time_index[5][MAX_SFB_NUM], hf_envelop_freq_index[5][MAX_SFB_NUM];
	int noise_floor[MAX_NOISE_NUM];
	int noisefloor_time_index[2][MAX_NOISE_NUM], noisefloor_freq_index[2][MAX_NOISE_NUM];
	int tBits, fBits;
	int index;

	int payloadBits = 0;
	int nFillBits;
	int errflag = 0;
	StBweData *pstBweData;
	StAvs2BweCommon_li *pstBweCommon;

    avs2audiopack_buffer *opb;
	int tmp;


	pstBweData = (StBweData *)(st_in);
	pstBweCommon = (StAvs2BweCommon_li *)(st_common);

	for(i = 0; i < 12; i++)
		memset(&prm[i], 0, sizeof(StHFPrm));

	for(i = 0; i < 8; i++)
		memset(&prm_data[i], 0, sizeof(StCoderHFPrmData));

	*numAncDataBytes = 0;

	
	//extract HF parameters
	BlockTabletmp = (int(*)[16])getBlockseqTable(pstBweCommon->nGroupmode);
	nBlockNum = BlockTabletmp[pstBweCommon->nSeqmode][1];

	GroupTabletmp = (int(*)[10])getGroupseqTable(pstBweCommon->nGroupmode);
	nGroupNum = GroupTabletmp[pstBweCommon->nSeqmode][1];

	lstart  = GroupTabletmp[pstBweCommon->nSeqmode][2]; //wuchaogang 20120826 


	//extract noise floor and determine if adding harmonic
	ExtractTonalPara(bitRate, nGroupNum, pstBweCommon->nOnsetFlag, pstBweCommon->nOnsetPos, pstBweCommon->mdft4096block_2048complex[0], pstBweCommon->mdft4096block_complex[0],
		&addHarmonicFlag, addHarmonicVec, noiseFloor, pstBweData, pstBweCommon);


	//calculate envelop and quantize
	len = 0;
	for(i = 0; i < nBlockNum; i++)
	{
		nBlockLen = BlockTabletmp[pstBweCommon->nSeqmode][i+2];
		
		switch(nBlockLen) {
		case 2048:
			sfbMode = 0;
			break;
		case 1024:
			sfbMode = 1;
			break;
		case 512:
			sfbMode = 2;
			break;
		case 256:
			sfbMode = 3;
			break;
		case 128:
			sfbMode = 4;
			break;
		default:
			break;
		}

		//calculate HF envelop
		if(sfbMode<=2)
		CalculateHfEnvelop_M3(bitRate, nBlockLen, addHarmonicFlag, addHarmonicVec, 
			&pstBweCommon->mdft4096block_complex[0][len], 
			prm[i].envelop, sfbMode);
		else
		CalculateHfEnvelop(bitRate, nBlockLen, addHarmonicFlag, addHarmonicVec, 
			&pstBweCommon->mdft4096block_complex[0][len], 
			prm[i].envelop, sfbMode);
		
		len += 2 * nBlockLen;
	}

	//quantize HF parameters 
	if(bitRate < 20000)
		sfbParam = sfbParams_16[0];
	else if(bitRate>=20000&& bitRate < 28000)
		/*sfbParam = sfbParams_20[0];*/sfbParam = sfbParams_16[0];
	else if(bitRate >= 28000)
		sfbParam = sfbParams_32[0];
	
	if((pstBweCommon->nGroupmode) > 0)
	{

		for(i = 0; i < nGroupNum; i++)
		{
			li = GroupTabletmp[pstBweCommon->nSeqmode][i+2];
			ld = GroupTabletmp[pstBweCommon->nSeqmode][i+3];

			//HFPrmGrouping(li, ld, prm, &prm_data[i]);
			HFPrmGrouping(li-lstart, ld-lstart, prm, &prm_data[i], sfbParam,bitsPerSample);
		}

		for(i = 0; i < 2; i++)
			QuantNoiseFloor(noiseFloor[i], &prm_data[i], sfbParam);
	}	
	else
	{
		for(i = 0; i < nGroupNum; i++)
		{
			QuantEnvelop(prm[i].envelop, &prm_data[i], sfbParam,bitsPerSample);
			QuantNoiseFloor(noiseFloor[i], &prm_data[i], sfbParam);
		}
	}

	//encode BWE data information
	opb = calloc(1,sizeof(avs2audiopack_buffer));
	avs2audiopack_writeinit(opb);

	
	//grouping information
	if((pstBweCommon->nGroupmode) == 0)
		avs2audiopack_write(opb, 0, 1);
	else
		avs2audiopack_write(opb, 1, 1);
	
	if((pstBweCommon->nGroupmode) == 0)
	    avs2audiopack_write(opb, pstBweCommon->nSeqmode, 4);
	else
	    avs2audiopack_write(opb, pstBweCommon->nSeqmode, 10);

	//transient information
	avs2audiopack_write(opb, pstBweCommon->nOnsetFlag, 1);
	if( pstBweCommon->nOnsetFlag )
		avs2audiopack_write(opb, pstBweCommon->nOnsetPos, 4);
	
	//envelop data information
	if(nGroupNum > 1)
	{
		tBits = 0, fBits = 0;

		//time domain 
		for(j = 0; j < sfbParam.sfbCnt; j++)
		{
			hf_envelop[0] = prm_data[0].hf_env[j];
		    hf_envelop_time_index[0][j] = hf_envelop[0];
			tBits += 5;

			for(i = 1; i < nGroupNum; i++)
			{
				hf_envelop[i] = prm_data[i].hf_env[j];
				hf_envelop_time_index[i][j] = hf_envelop[i] - hf_envelop[i-1];

				//compute bits of time domain differential coding
				index = hf_envelop_time_index[i][j]+20;
				if(index < 0)
					index = 0;
				if (index > 45)
					index = 45;
					
				tBits += book_envelop_time_length[index];
			}
		}

		//frequency domain
		errflag = 0;
		for(i = 0; i < nGroupNum; i++)
		{
			hf_envelop[0] = prm_data[i].hf_env[0];
		    hf_envelop_freq_index[i][0] = hf_envelop[0];
			fBits += 5;
			
			for(j = 1; j < sfbParam.sfbCnt; j++)
			{
				hf_envelop[j] = prm_data[i].hf_env[j];
				hf_envelop_freq_index[i][j] = hf_envelop[j] - hf_envelop[j-1];

 			    //compute bits of frequency domain differential coding
				index = hf_envelop_freq_index[i][j]+16;
				if (index < 0)
				{
					index = 0;
					errflag = 1;
				}
				if (index > 30)
				{
					index = 30;
					errflag = 1;
				}
					
				fBits += book_envelop_freq_length[index];
			}
		}

		//compare 
		if ((tBits < fBits) || (errflag==1))// decide time domain differential coding
		{
			avs2audiopack_write(opb, 1, 1);
			
			for(j = 0; j < sfbParam.sfbCnt; j++)
			{
				avs2audiopack_write(opb, hf_envelop_time_index[0][j], 5);

				for(i = 1; i < nGroupNum; i++)
				{
					index = hf_envelop_time_index[i][j]+20;					
					if(index < 0)
						index = 0;
					if (index > 45)
						index = 45;

					tmp = shift_fun(book_envelop_time_code[index], book_envelop_time_length[index]);
					avs2audiopack_write(opb, tmp, book_envelop_time_length[index]);
				}
			}
		}
		else// decide frequency domain differential coding
		{
			avs2audiopack_write(opb, 0, 1);
			
			for(i = 0; i < nGroupNum; i++)
			{
				avs2audiopack_write(opb, hf_envelop_freq_index[i][0], 5);

				for(j = 1; j < sfbParam.sfbCnt; j++)
				{
					index = hf_envelop_freq_index[i][j]+16;
					if(index < 0)
						index = 0;
					if (index > 30)
						index = 30;

					tmp = shift_fun(book_envelop_freq_code[index], book_envelop_freq_length[index]);
					avs2audiopack_write(opb, tmp, book_envelop_freq_length[index]);
				}
			}
		}
	}
	else
	{
		//frequency-domain 
		hf_envelop[0] = prm_data[0].hf_env[0];
		hf_envelop_freq_index[0][0] = hf_envelop[0];

		for(j = 1; j < sfbParam.sfbCnt; j++)
		{
			hf_envelop[j] = prm_data[0].hf_env[j];
			hf_envelop_freq_index[0][j] = hf_envelop[j] - hf_envelop[j-1];
		}

		//save code of frequency differential coding
		avs2audiopack_write(opb, 0, 1);

		avs2audiopack_write(opb, hf_envelop_freq_index[0][0], 5);

		for(j = 1; j < sfbParam.sfbCnt; j++)
		{
			index = hf_envelop_freq_index[0][j]+16;
			if(index < 0)
				index = 0;
			if (index > 30)
				index = 30;

			tmp = shift_fun(book_envelop_freq_code[index], book_envelop_freq_length[index]);
			avs2audiopack_write(opb, tmp, book_envelop_freq_length[index]);
		}
	}		


	//noise floor information
	if(nGroupNum > 1)
	{
		tBits = 0;
		fBits = 0;
		
		//temporal-domain
		for(j = 0; j < sfbParam.sfbCntNoiseFloor; j++)
		{
			noise_floor[0] = prm_data[0].noise_floor[j]- NOISE_FLOOR_OFFSET + 1;
			noisefloor_time_index[0][j] = noise_floor[0];
			tBits += 4;

			for(i = 1; i < 2; i++)
			{
				noise_floor[i] = prm_data[i].noise_floor[j]- NOISE_FLOOR_OFFSET + 1;
				noisefloor_time_index[i][j] = noise_floor[i] - noise_floor[i-1];

				//compute bits of time domain differential coding
				index = noisefloor_time_index[i][j]+13;
				if(index < 0)
					index = 0;
				if (index > 24)
					index = 24;
					
				tBits += book_noisefloor_time_length[index];
			}
		}

		//frequency-domain
		for(i = 0; i < 2; i++)
		{
			noise_floor[0] = prm_data[i].noise_floor[0]- NOISE_FLOOR_OFFSET + 1;
		    noisefloor_freq_index[i][0] = noise_floor[0];
			fBits += 4;
			
			for(j = 1; j < sfbParam.sfbCntNoiseFloor; j++)
			{
				noise_floor[j] = prm_data[i].noise_floor[j]- NOISE_FLOOR_OFFSET + 1;
				noisefloor_freq_index[i][j] = noise_floor[j] - noise_floor[j-1];

				//compute bits of frequency domain differential coding
				index = noisefloor_freq_index[i][j]+14;
				if(index < 0)
					index = 0;
				if (index > 24)
					index = 24;
					
				fBits += book_noisefloor_freq_length[index];
			}			
		}
		
		//compare 
		if(tBits < fBits)// decide time domain differential coding
		{
			avs2audiopack_write(opb, 1, 1);
			
			for(j = 0; j < sfbParam.sfbCntNoiseFloor; j++)
			{
				avs2audiopack_write(opb, noisefloor_time_index[0][j], 4);

				for(i = 1; i < 2; i++)
				{
					index = noisefloor_time_index[i][j]+13;
					if(index < 0)
						index = 0;
					if (index > 24)
						index = 24;

					tmp = shift_fun(book_noisefloor_time_code[index], book_noisefloor_time_length[index]);
					avs2audiopack_write(opb, tmp, book_noisefloor_time_length[index]);					
				}
			}
		}
		else// decide frequency domain differential coding
		{
			avs2audiopack_write(opb, 0, 1);
			
			for(i = 0; i < 2; i++)
			{
				avs2audiopack_write(opb, noisefloor_freq_index[i][0], 4);

				for(j = 1; j < sfbParam.sfbCntNoiseFloor; j++)
				{
					index = noisefloor_freq_index[i][j]+14;
					if(index < 0)
				 	    index = 0;
				    if (index > 24)
					    index = 24;

					tmp = shift_fun(book_noisefloor_freq_code[index], book_noisefloor_freq_length[index]);
					avs2audiopack_write(opb, tmp, book_noisefloor_freq_length[index]);
				}
			}
		}
	}
	else
	{
		//frequency-domain
		noise_floor[0] = prm_data[0].noise_floor[0] - NOISE_FLOOR_OFFSET + 1;
		noisefloor_freq_index[0][0] = noise_floor[0];
		for(j = 1; j < sfbParam.sfbCntNoiseFloor; j++)
		{
			noise_floor[j] = prm_data[0].noise_floor[j] - NOISE_FLOOR_OFFSET + 1;
			noisefloor_freq_index[0][j] = noise_floor[j] - noise_floor[j- 1];
		}

		//save code of frequency differential coding
		avs2audiopack_write(opb, noisefloor_freq_index[0][0], 4);

		for(j = 1; j < sfbParam.sfbCntNoiseFloor; j++)
		{
			index = noisefloor_freq_index[0][j]+14;
			if(index < 0)
			    index = 0;
			if (index > 24)
				index = 24;

			tmp = shift_fun(book_noisefloor_freq_code[index], book_noisefloor_freq_length[index]);
			avs2audiopack_write(opb, tmp, book_noisefloor_freq_length[index]);
		}
	}
	

	//BWE extended information
	avs2audiopack_write(opb, addHarmonicFlag, 1);
	if(addHarmonicFlag)
	{
		for (i = 0; i < sfbParam.sfbCnt; i++)
			avs2audiopack_write(opb, addHarmonicVec[i], 1);
	}

	//fill bits
	if(opb->endbit > 0)
		avs2audiopack_write(opb, 0, (8-opb->endbit));

	*numAncDataBytes = opb->endbyte;
	memcpy(ancBytes, opb->buffer, opb->endbyte);

	avs2audiopack_writeclear(opb);
	
	return 0;
}

int Avs2BweEncoderOpen(unsigned int *st_in, int bitrate, int sampleRateCore, int numChannelsCore, int *bandWidth, int *config_idx)
{
	int idx;
	int i, paramSets = sizeof (tuningTable) / sizeof (tuningTable[0]);

	for (i = 0 ; i < paramSets ; i++)
	{
		if (numChannelsCore == tuningTable[i].numChannels)
		{
			if ((sampleRateCore == tuningTable[i].sampleRate) && (bitrate >= tuningTable[i].bitrateFrom) &&
					(bitrate < tuningTable[i].bitrateTo)) 
				idx = i;
		}
	}

	*bandWidth = tuningTable[idx].startFreq  * 32;
	*config_idx = idx;

	*st_in = (unsigned int *) (malloc(sizeof(StBweData)));

	memset((void*)*st_in, 0, sizeof(StBweData));
	
	return 0;
}

int setBWEbandWidth(int *bandWidth,int idx)
{
	*bandWidth = tuningTable[idx].startFreq  * 32;
}

int Avs2BweEncoderClose(unsigned int *st_in)
{

	//变换
	if((void*)*st_in!=NULL)
		free((void*)*st_in);

	return 0;	
}
