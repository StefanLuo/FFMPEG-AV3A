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

#ifndef _I2R_ENCODER_H
#define _I2R_ENCODER_H

#include <stdio.h>
#include "lwt.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_CH 8
#define FRAME_LEN 1024
#define LPC_Q 20 			// LPC Quantizer value
#define MAXODR 127
#define RA_LEN 16

typedef unsigned char BYTE;
typedef unsigned long ULONG;

typedef struct 
{
	short P;						// (Max.) Predictor order
	long  Chan;						// Number of channels
	short Res;						// Actual resolution (in bits)
	int	  pcm_scale;				//
	long Freq;						// Sampling frequency
	long fid;						// Current frame
	long msenc;						// channel correlation process command
	long wavelet;					// wavelet process command
	BYTE ucEntropy;					// The entropy encoding flag

	//unsigned char *bbuf, *buffer;
	long *x[MAX_CH], *xp[MAX_CH], *d, *cof;

	long*			residual[MAX_CH];
	long*			parQ[MAX_CH];
	short			optPredOrd[MAX_CH];

	double			*par;

	int				ll_bits;
	int*			wBuf[2];
	cacdStruct  cacd;
} Encoder_t;

Encoder_t *i2r_EncoderInit(int ChanNum, int SampFreq, int Res, int maxLpcOrder);
void i2r_EncoderRelease(Encoder_t *pDec); 
void i2r_EncodeFrame(Encoder_t *pEnc, int nCh, int nChanNum);
void EncodeBlockAnalysis(Encoder_t *pEnc, long Channel, long *d
		,long length); 
void EncodeBlockCoding(Encoder_t *pEnc, long Channel, long *x
		, long length, int bufno);

typedef void (*pFuncEntropy)(Encoder_t *,
							 int,
							 int*,
							 int,
							 int,
							 int);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
