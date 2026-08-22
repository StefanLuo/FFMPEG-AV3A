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

#include <math.h>
#include <string.h>
#include "ky_midside.h"

#ifndef LL_OPTIMIZATION
double Thres[MAX_CHANNELS]={
	ThresMax, ThresMax, ThresMax, ThresMax, 
	ThresMax, ThresMax, ThresMax, ThresMax
};

long CorreAnalysis(long *in0[2], long *in1[2], double *Thres)
{
	int i, j;
	double r[2];
    double escape = 0.000001;

	for (j = 0; j < 2; j++)
	{
		double divssum   = 0;
		double ex0 = 0, ex1 = 0;
		double mx0 = 0, mx1 = 0;

		/* Calculating the mean value */
		for (i = 0; i < FRAME_LEN; i++){
			mx0    = mx0 + in0[j][i];
			mx1    = mx1 + in1[j][i];
		}
		mx0        = mx0 / FRAME_LEN;
		mx1        = mx1 / FRAME_LEN;

		/* Calculating the square mean value */
		for (i = 0; i < FRAME_LEN; i++)
		{
			divssum= divssum + (double)(in0[j][i] - mx0)*(in1[j][i] - mx1);
			ex0    = ex0     + (double)(in0[j][i] - mx0)*(in0[j][i] - mx0);
			ex1    = ex1     + (double)(in1[j][i] - mx1)*(in1[j][i] - mx1);
		}
		ex0        = sqrt(ex0);
		ex1        = sqrt(ex1);

		/* Calculating the correlation coefficient */
		r[j]       = divssum/(ex0*ex1 + escape);
	}

	/* Correlation judgement */
	if ((r[0] > *Thres) && (r[0] > r[1])){
		memcpy(in0[0], in0[1], FRAME_LEN * sizeof(long));
		memcpy(in1[0], in1[1], FRAME_LEN * sizeof(long));
		return 1;
	}
	return 0;
}

long MidSide2Channel(long *in0, long *in1, long *out0, long *out1, long nCh)
{
	int i;
	long *in0Tmp[2];
	long *in1Tmp[2];

	in0Tmp[0]		= in0;
	in1Tmp[0]		= in1;
	in0Tmp[1]		= out0;
	in1Tmp[1]		= out1;

	for(i = 0; i < FRAME_LEN; i++)
	{
		long left, right;
		
		left		= in0[i];
		right		= in1[i];

		out0[i]		= (left + right)>>1;
		out1[i]		= (left - right);
	}

    /* Channel correlation analysis */
	return CorreAnalysis(in0Tmp, in1Tmp, &Thres[nCh]);
}

/* The threshold adaptive */
void UpdateThres(long corr, double *Thres)
{
	if(corr){
		*Thres = *Thres - ThresStep;
		if (*Thres < ThresMin){
			*Thres = ThresMin;
		}
	}else{
		*Thres = *Thres + ThresStep;
		if (*Thres > ThresMax){
			*Thres = ThresMax;
		}
	}
}
#endif
