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

#define NEW_COPY
#define COPY_PROCESSING

#define FFTtoQMF 0
#define BLOCKLEN32_MDFTWIN
//#define BLOCKLEN32_WIN16 16
int BLOCKLEN32_WIN16=16;
#define EPS                     1e-18
#define LOG2                    0.69314718056f
#define ILOG2                   1.442695041f
#define NOISE_FLOOR_OFFSET      6

#define MAX_BLOCK_LEN	32//8//16
#define MAX_BLOCK_NUM   96//64
#define MAX_SFB_NUM     34
#define MAX_NOISE_NUM	32
#define MAX_HF_LINES_S		MAX_BLOCK_LEN*MAX_BLOCK_NUM
#define NOT_TONAL 100

typedef enum
{
  BWE_MONO,
  BWE_LEFT_RIGHT,
  BWE_COUPLING,
  BWE_SWITCH_LRC
}
BWE_STEREO_MODE;

typedef struct{
	int hf_env[MAX_SFB_NUM];
	int noise_floor[MAX_NOISE_NUM];
	int addHarmonicVec [MAX_SFB_NUM];
	int addHarmonicFlag;
	int addNoiseVec [MAX_SFB_NUM];
	int onsetPosition[2];
	int groupMode;
	int seqMode;
	int onsetFlag;
	int onsetPos;
	
	int prevHarmFlag;
	int prevOnsetPos;
}StCoderHFPrmData;

typedef struct{
	float envelop[MAX_SFB_NUM];
	float noiseFloor[MAX_NOISE_NUM];
	int addHarmonicVec[MAX_SFB_NUM];
	int addHarmonicFlag;
}StHFPrm;

/*!< Guide vectors for every tonality estimate. */
typedef struct
{
	float pGuideVectorDiff[MAX_SFB_NUM];	/*!< guide vector indicating that a detection was made due to too high tonality difference
										by indicating the tonality difference of that detection.
										(frequency resolution of the scalefactor bands) */
	float pGuideVectorOrig[MAX_SFB_NUM];	/*!< guide vector indicating that a detection was made since the tonality of the original
										was sufficiently high, and a tone was previously detected due to too large tonality difference.
										The value in the guide vector represents the tonality of the traced tone. 
										(frequency resolution of the scalefactor bands) */
	int   pbGuideVectorDetected[MAX_SFB_NUM];	/*!< guide vector indicating that a missing harmonic was detected, disregarded the detection method.*/
}GUIDE_VECTORS;

typedef struct{
	GUIDE_VECTORS guideVectors[4];
	int prevTransientFlag;
	int prevTransientFrame;

	float prevNoiseFloor[4][MAX_NOISE_NUM];
	int prevAddHarmonicVec[MAX_SFB_NUM];
	
	float prevOrig_tnr[2][MAX_BLOCK_NUM];
	float prevBwe_tnr[2][MAX_BLOCK_NUM];
	float prevDiff_tnr[2][MAX_SFB_NUM];
	float prevOrig_sfm[2][MAX_SFB_NUM];
	float prevBwe_sfm[2][MAX_SFB_NUM];

}StBweData;

typedef struct{
	float mdft4096block_complex[2][4096+2048];
	float mdft4096block_2048complex[2][4096+2048];

	int nGroupmode, nSeqmode;
	int nOnsetFlag, nOnsetPos;
	
}StAvs2BweCommon_li;


