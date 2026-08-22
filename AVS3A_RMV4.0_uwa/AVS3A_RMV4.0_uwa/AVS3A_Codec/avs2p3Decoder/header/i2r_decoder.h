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

#ifndef _I2R_DECODER_H
#define _I2R_DECODER_H

#include <stdio.h>
#include "lwt.h"

#ifdef __cplusplus
extern "C" {
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
	short P;				/* max. prediction order*/

	long  Chan;				/* number of channels */
	short Res;				/* original resolution (in bits/sample) */
	int pcm_scale;
	long Freq;				/* Sampling frequency */

	unsigned char CoefTable;// Table for entropy coding of coefficients

	long *x[MAX_CH], *d, *cofQ;

	long*		residual[MAX_CH];
	long*		parRec[MAX_CH];
	short		optPredOrd[MAX_CH];

	cacdStruct cacd;
} Decoder_t;

void DecodeBlockParameter(Decoder_t *pDec, long Channel, long length);
short DecodeBlockReconstruct(Decoder_t *pDec, long Channel, long *x, long length);
Decoder_t *i2r_DecoderInit(int ChanNum, int SampFreq, int Res);
void i2r_DecoderRelease(Decoder_t *pDec); 
void i2r_DecodeFrame(Decoder_t *pDec,  int Chan,int ChanNum, long output_pcm[][FRAME_LEN]);	

#ifdef __cplusplus
}
#endif // __cplusplus
#endif

