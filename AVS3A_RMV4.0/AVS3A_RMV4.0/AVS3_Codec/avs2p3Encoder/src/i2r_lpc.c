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

#ifdef WIN32
	typedef __int64 INT64;
#else
	#include <stdint.h>
	typedef int64_t INT64;
#endif

#include <math.h>
#include <memory.h>
#include <limits.h>
#include "av3enc.h"
#include "i2r_lpc.h"

/* autocorrelation function */
void acf(double *x, long N, long k, short norm, double *rxx)
{
	long i, n;

	for (i = 0; i <= k; i++)
	{
		rxx[i] = 0.0;
		for (n = i; n < N; n++)
			rxx[i] += x[n] * x[n-i];
	}

	if (norm)
	{
		for (i = 1; i <= k; i++)
			rxx[i] /= rxx[0];
		rxx[0] = 1.0;
	}
}

/* Hanning window */
void hanning(long *x, double *xd, long N)
{
	long n;

	for (n = 0; n < N; n++)
		xd[n] = (double)x[n] * (0.5 - 0.5 * cos(2.0*PI*n/(N-1)));
}

/* Hamming window*/
void hamming(long *x, double *xd, long N)
{
	long n;

	for (n = 0; n < N; n++)
		xd[n] = (double)x[n] * (0.54 - 0.46 * cos(2.0*PI*n/(N-1)));
}

/* Rect window*/
void rect(long *x, double *xd, long N)
{
	long n;

	for (n = 0; n < N; n++)
		xd[n] = (double)x[n];
}

/* Blackman window*/
void blackman(long *x, double *xd, long N)
{
	long n;

	for (n = 0; n < N; n++)
		xd[n] = (double)x[n] * (0.42 - 0.5 * cos(2.0*PI*n/(N-1)) + 0.08 * cos(4.0*PI*n/(N-1)));
}

/* Levinson-Durbin algorithm*/
short durbin(short ord, double *rxx, double *par
			 ,int*x, int* lpc_order,long N)
{
	short i, j;
	double evar, temp;
	double len, bps, bit;
	double dir[FRAME_LEN];
	unsigned int best_order = 1;
	double best_len = 0xFFFFFFFF;

	par--;
	evar = rxx[0];

	for (i = 1; i <= ord; i++)
	{
		par[i] = -rxx[i];
		for (j = 1; j < i; j++)
			par[i] -= dir[j] * rxx[i-j];
		par[i] /= evar;
		dir[i] = par[i];
		for (j = 1; j <= i/2; j++)
		{
			temp = dir[j] + par[i] * dir[i-j];
			dir[i-j] = dir[i-j] + par[i] * dir[j];
			dir[j] = temp;
		}
		evar *= (1.0 - par[i] * par[i]);

		if (evar == 0.0){
			*lpc_order = best_order;
			return 0;
		}
		bps = log(evar)/(2*LN2);
		len = bps*N + i *7;
		if(len<best_len){
				best_len = len;
				best_order = i;
		}
	}
	*lpc_order = best_order;

	return(0);
}

/* Calculate LPC coefficients for a block of samples */
short GetCof(long *x, long N, short P,
			 short win, double *par, int* lpc_order)
{
	double *xd, *rxx;
	xd = (double *)calloc(N,sizeof(double));
	rxx = (double *)calloc(P+1,sizeof(double));

	/* Windowing */
	if (win == 1)
		hamming(x, xd, N);
	else if (win == 2)
		rect(x, xd, N);
	else if (win == 3)
		blackman(x, xd, N);
	else
		hanning(x, xd, N);

	acf(xd, N, P, 0, rxx);
	durbin(P, rxx, par
		,x, lpc_order, N);

	free(xd); 
	free(rxx); 

	return(0);
}


/* Check if all samples are zero */
short BlockIsZero(long *x, long N)
{
	long n;
	for (n = 0; n < N; n++){
		if(x[n] != 0){
			break;
		}
	}
	return (n==N)?1:0;
}


// Calculate prediction residual for random access block (internal par -> cof conversion)
short GetResidual(long *x, long N, short P, short Q, long *par, long *cof, long *d)
{
	long n, i, m;
	long korr;
	INT64 y, temp, temp2;

	if(N < P) P = (short)N;

	korr = 1 << (Q - 1);	// Correction term

	par--;
	cof--;

	// Progressive prediction for first P values
	for (n = 0; n < P; n++)
	{
		// Initialisation with correction term
		y = korr;

		// Calculate estimate
		for (i = 1; i <= n; i++)
			y += (INT64)cof[i] * x[n-i];			// cof[i] because of cof-- (see above)

		// Subtract estimate from signal
		d[n] = x[n] + (long)(y >> Q);				// Division y / 2^Q

		m = n + 1;	// Order

		// Calculate direct form m-th order predictor coefficients
		for (i = 1; i <= m/2; i++)
		{
			temp = cof[i] + ((((INT64)par[m] * cof[m-i]) + korr) >> Q);
			if ((temp > LONG_MAX) || (temp < LONG_MIN))	// Overflow
				return(1);
			temp2 = cof[m-i] + ((((INT64)par[m] * cof[i]) + korr) >> Q);
			if ((temp2 > LONG_MAX) || (temp2 < LONG_MIN))	// Overflow
				return(1);

			cof[m-i] = (long)temp2;
			cof[i] = (long)temp;
		}
		cof[m] = par[m];
	}

	for (n = P; n < N; n++)
	{
		// Initialisation with correction term
		y = korr;

		// Calculate estimate
		for (i = 1; i <= P; i++)
			y += (INT64)cof[i] * x[n-i];

		// Subtract estimate from signal
		d[n] = x[n] + (long)(y >> Q);				// Division y / 2^Q
	}
	
	return (0);
}






