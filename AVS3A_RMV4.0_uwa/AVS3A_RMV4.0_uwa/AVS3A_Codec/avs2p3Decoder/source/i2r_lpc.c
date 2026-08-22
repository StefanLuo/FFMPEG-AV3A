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


// Calculate original signal for random access block (internal par -> cof conversion)
short GetSignal(long *x, long N, short P, short Q, long *par, long *cof, long *d)
{
	long n, i, m;
	long korr;
	INT64 y, temp, temp2;
	short OverFlow = 0;

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

		// Add estimate to residual
		x[n] = d[n] - (long)(y >> Q);				// Division y / 2^Q

		m = n + 1;	// Order

		// Calculate direct form m-th order predictor coefficients
		for (i = 1; i <= m/2; i++)
		{
			temp = cof[i] + ((((INT64)par[m] * cof[m-i]) + korr) >> Q);
			if ((temp > LONG_MAX) || (temp < LONG_MIN))	// Overflow
				return (1);
			temp2 = cof[m-i] + ((((INT64)par[m] * cof[i]) + korr) >> Q);
			if ((temp2 > LONG_MAX) || (temp2 < LONG_MIN))	// Overflow
				return (1);

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

		// Add estimate to residual
		x[n] = d[n] - (long)(y >> Q);				// Division y / 2^Q
	}

	return (0);
}
