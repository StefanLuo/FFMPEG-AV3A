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

#include "lwt.h"

#ifdef WIN32
typedef __int64 INT64;
#else
#include <stdint.h>
typedef int64_t INT64;
#endif
#define Q 14

INT64 liftFILT0[] = {-2731,-2731};
INT64 liftFILT1[] = {-9216, -9216};
INT64 liftFILT2[] = {-21845, -21845};
INT64 liftFILT3[] ={ 126, -938, 3372, 3372, -938, 126}; 

short lwt(int *x, cacdStruct *rx, unsigned int lenx, int res)
{
	int i;
	INT64 tmp, max_val, min_val;

// KY_BUGFIX // PKU fix 2011/06/23
	max_val =  (1<<(res-1))-1;
	min_val = -(1<<(res-1))  ;

	// ---- Splitting ---- //
	rx->lenca = (lenx>>1) + lenx%2;
	rx->lencd = lenx>>1;

	for (i=0; i!=rx->lenca; i++)
	{
		rx->ca[i] = x[i<<1];
	}
	for (i=0; i!=rx->lencd; i++)
	{
		rx->cd[i] = x[(i<<1) + 1];
	}

	// ---- Lifting ---- //
	// d -> p -> d -> p  //
	//first pass
	for(i=0; i!=rx->lenca-1; i++)
	{
		tmp = rx->cd[i] + ((liftFILT0[0] * rx->ca[i+1] + liftFILT0[1] * rx->ca[i])>>Q);
		if (tmp >max_val || tmp < min_val)
			return(1);
		rx->cd[i] =  tmp;
	}
	if (!(lenx & 0x01))
	{
		tmp =  rx->cd[i] + ((liftFILT0[1] * rx->ca[i]) >> Q);
		if (tmp >max_val || tmp < min_val)
			return(1);
		rx->cd[i] = tmp;
	}


	//second pass
	tmp = rx->ca[0] + ((liftFILT1[0] * rx->cd[0])>>Q);
	if (tmp >max_val || tmp < min_val)
		return(1);
	rx->ca[0] = tmp;
	for(i=1; i!=rx->lencd; i++)
	{
		tmp = rx->ca[i] + ((liftFILT1[1] * rx->cd[i-1] + liftFILT1[0] * rx->cd[i])>>Q);
		if (tmp >max_val || tmp < min_val)
			return(1);
		rx->ca[i] = tmp;
	}


	//third pass
	for(i=0; i!=rx->lenca-1; i++)
	{
		tmp = rx->cd[i] + ((liftFILT2[0] * rx->ca[i+1] + liftFILT2[1] * rx->ca[i])>>Q);
		if (tmp >max_val || tmp < min_val)
			return(1);
		rx->cd[i] = tmp;
	}
	if (!(lenx&0x01))
	{
		tmp = rx->cd[i] + ((liftFILT2[1] * rx->ca[i])>>Q);
		if (tmp >max_val || tmp < min_val)
			return(1);
		rx->cd[i] = tmp;
	}


	//fourth pass
	tmp = rx->ca[0] + ((liftFILT3[0] * rx->cd[2] + liftFILT3[1] * rx->cd[1] + liftFILT3[2] * rx->cd[0])>>Q);
	if (tmp >max_val || tmp < min_val)
		return(1);
	rx->ca[0] = tmp;
	tmp = rx->ca[1] + ((liftFILT3[0] * rx->cd[3] + liftFILT3[1] * rx->cd[2] + liftFILT3[2] * rx->cd[1] + liftFILT3[3] * rx->cd[0])>>Q);
	if (tmp >max_val || tmp < min_val)
		return(1);
	rx->ca[1] = tmp;
	tmp = rx->ca[2] + ((liftFILT3[0] * rx->cd[4] + liftFILT3[1] * rx->cd[3] + liftFILT3[2] * rx->cd[2] + liftFILT3[3] * rx->cd[1] + liftFILT3[4] * rx->cd[0])>>Q);
	if (tmp >max_val || tmp < min_val)
		return(1);
	rx->ca[2] = tmp;
	for(i=3; i!=rx->lencd-2; i++)
	{
		tmp = rx->ca[i] + (( liftFILT3[0] * rx->cd[i+2] + liftFILT3[1] * rx->cd[i+1] + liftFILT3[2] * rx->cd[i] + liftFILT3[3] * rx->cd[i-1] + liftFILT3[4] * rx->cd[i-2] + liftFILT3[5] * rx->cd[i-3])>>Q);
		if (tmp >max_val || tmp < min_val)
			return(1);
		rx->ca[i] = tmp;
	}
	tmp  = rx->ca[rx->lencd-2] + ((liftFILT3[1] * rx->cd[rx->lencd-1] + liftFILT3[2] * rx->cd[rx->lencd-2] + liftFILT3[3] * rx->cd[rx->lencd-3] + liftFILT3[4] * rx->cd[rx->lencd-4] + liftFILT3[5] * rx->cd[rx->lencd-5])>>Q);
	if (tmp >max_val || tmp < min_val)
		return(1);
	rx->ca[rx->lencd-2] = tmp;
	tmp = rx->ca[rx->lencd-1] + ((liftFILT3[2] * rx->cd[rx->lencd-1] + liftFILT3[3] * rx->cd[rx->lencd-2] + liftFILT3[4] * rx->cd[rx->lencd-3] + liftFILT3[5] * rx->cd[rx->lencd-4])>>Q);
	if (tmp >max_val || tmp < min_val)
		return(1);
	rx->ca[rx->lencd-1] = tmp;
	
	return(0);
}

void ilwt(int *x, cacdStruct *rx)
{
	int lenx, i;
	lenx = rx->lenca + rx->lencd;

	//first pass
	rx->ca[0] = rx->ca[0] - ((liftFILT3[0] * rx->cd[2] + liftFILT3[1] * rx->cd[1] + liftFILT3[2] * rx->cd[0])>>Q);
	rx->ca[1] = rx->ca[1] - ((liftFILT3[0] * rx->cd[3] + liftFILT3[1] * rx->cd[2] + liftFILT3[2] * rx->cd[1] + liftFILT3[3] * rx->cd[0])>>Q);
	rx->ca[2] = rx->ca[2] - ((liftFILT3[0] * rx->cd[4] + liftFILT3[1] * rx->cd[3] + liftFILT3[2] * rx->cd[2] + liftFILT3[3] * rx->cd[1] + liftFILT3[4] * rx->cd[0])>>Q);
	for(i=3; i!=rx->lencd-2; i++)
		rx->ca[i] = rx->ca[i] - (( liftFILT3[0] * rx->cd[i+2] + liftFILT3[1] * rx->cd[i+1] + liftFILT3[2] * rx->cd[i] + liftFILT3[3] * rx->cd[i-1] + liftFILT3[4] * rx->cd[i-2] + liftFILT3[5] * rx->cd[i-3])>>Q);
	rx->ca[rx->lencd-2] = rx->ca[rx->lencd-2] - ((liftFILT3[1] * rx->cd[rx->lencd-1] + liftFILT3[2] * rx->cd[rx->lencd-2] + liftFILT3[3] * rx->cd[rx->lencd-3] + liftFILT3[4] * rx->cd[rx->lencd-4] + liftFILT3[5] * rx->cd[rx->lencd-5])>>Q);
	rx->ca[rx->lencd-1] = rx->ca[rx->lencd-1] - ((liftFILT3[2] * rx->cd[rx->lencd-1] + liftFILT3[3] * rx->cd[rx->lencd-2] + liftFILT3[4] * rx->cd[rx->lencd-3] + liftFILT3[5] * rx->cd[rx->lencd-4])>>Q);

	// second pass
	for(i=0; i!=rx->lenca-1; i++)
		rx->cd[i] =  rx->cd[i] - ((liftFILT2[0] * rx->ca[i+1] + liftFILT2[1] * rx->ca[i])>>Q);
	if (!(lenx&0x01))
		rx->cd[i] = rx->cd[i] - ((liftFILT2[1] * rx->ca[i])>>Q);


	// third pass
	rx->ca[0] = rx->ca[0] - ((liftFILT1[0] * rx->cd[0])>>Q);
	for(i=1; i!=rx->lencd; i++)
		rx->ca[i] =rx->ca[i] - ((liftFILT1[1] * rx->cd[i-1] + liftFILT1[0] * rx->cd[i])>>Q);


	// fourth pass
	for(i=0; i!=rx->lenca-1; i++)
		rx->cd[i] =  rx->cd[i] - ((liftFILT0[0] * rx->ca[i+1] + liftFILT0[1] * rx->ca[i])>>Q);
	if (!(lenx&0x01))
		rx->cd[i] = rx->cd[i] - (( liftFILT0[1] * rx->ca[i])>>Q);


	for (i=0; i!=lenx; i++)
	{
		if (i%2)
			x[i] = rx->cd[i>>1];
		else
			x[i] = rx->ca[i>>1];
	}
}



