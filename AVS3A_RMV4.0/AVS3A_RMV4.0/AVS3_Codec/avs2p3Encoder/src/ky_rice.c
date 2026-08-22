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

#include <stdlib.h>
#include "ky_rice.h"

/* Golomb-rice encoding */
unsigned int unAdaptiveRiceParam(unsigned int ulRiceSum,
								 unsigned int ulRiceParam)
{
	unsigned int ulRiceVal;
	ulRiceVal   = (1 << ulRiceParam)*RICE_NUM_MUL ;

	if(ulRiceSum > ulRiceVal){
		ulRiceVal	 = ulRiceVal << 1;
		while(ulRiceSum > ulRiceVal){
			ulRiceParam++            ;
			ulRiceVal= ulRiceVal << 1;
		}
	}else{
		while ((ulRiceParam > 0) 
			&& (ulRiceSum < ulRiceVal)){
			ulRiceParam--            ;
			ulRiceVal= ulRiceVal >> 1;
		}
	}
	return ulRiceParam;
}

void vGolombRiceEncode(const int* pnSrc,
					   const int  nSamples,
					   const int  nRes,
					   const int  nLpcOrd,
					   unsigned int ulRiceSum,
					   unsigned int ulRiceParam,
					   int bufno,
					   int blksizebit)
{
	int i, j, nMExt, maxLeadZero;

	int blocksize  = (1 << blksizebit);
	int blocks     = nSamples >> blksizebit;
	int remainder  = nSamples - (blocks << blksizebit);
	if(nRes < 8){
		nMExt      = 3;
	}else if(nRes < 16){
		nMExt      = 4;
	}else{
		nMExt      = 5;
	};

	maxLeadZero    = MAX_LEAD_ZEROS;
	for (i = 0; i < blocks; i++)
	{
		unsigned int blockSum = 0;
		for(j = 0; j < blocksize; j++)
		{
			unsigned int ulVal;
			unsigned int ulLeadZero;
			unsigned int ulRemain;

			int k     = ulRiceParam;
			int nData = *pnSrc++;

			if ((i * blocksize + j) < nLpcOrd){
				ulVal = nData;
			}else{
				if(nData < 0){
					ulVal = -(nData<<1)-1;
				}else{
					ulVal = nData<<1;
				}
			}
			blockSum  += ulVal;

			ulLeadZero = (ulVal>>k);
			ulRemain = ulVal & ((1<<k)-1);

			if(ulLeadZero < (maxLeadZero>>1)){
				EncodeWaveletPutBits(0,ulLeadZero, bufno);
				EncodeWaveletPutBits(1,        1 , bufno);
				EncodeWaveletPutBits(ulRemain, k , bufno);
			}else{
				while((ulVal>>k) >= maxLeadZero){
					k++;
				}
				ulLeadZero = (ulVal>>k);
				EncodeWaveletPutBits(0,32,            bufno);
				if(ulLeadZero > (maxLeadZero>>1)){
					EncodeWaveletPutBits(0,ulLeadZero-32, bufno);
				}
				EncodeWaveletPutBits(1,        1 , bufno);
				EncodeWaveletPutBits(k-ulRiceParam, nMExt, bufno);
				ulRemain	= ulVal & ((1<<k)-1);
				ulRiceParam = k;
				EncodeWaveletPutBits(ulRemain, k , bufno);
			}
		}
		ulRiceSum  += 
			blockSum - blocksize*(ulRiceSum/RICE_NUM_MUL);
		/* Adapative Golomb-rice parameter */
		ulRiceParam = 
			unAdaptiveRiceParam(ulRiceSum, ulRiceParam);
	}

	/* Process remainder samples */
	for (i = 0; i < remainder; i++)
	{
		unsigned int ulVal;
		unsigned int ulLeadZero;
		unsigned int ulRemain;

		int k     = ulRiceParam;
		int nData = *pnSrc++;

		if(nData < 0){
			ulVal = -(nData<<1)-1;
		}else{
			ulVal = nData<<1;
		}
		ulLeadZero= (ulVal>>k);
		ulRemain  = ulVal & ((1<<k)-1);

		if(ulLeadZero < (maxLeadZero>>1)){
			EncodeWaveletPutBits(0,ulLeadZero, bufno);
			EncodeWaveletPutBits(1,        1 , bufno);
			EncodeWaveletPutBits(ulRemain, k , bufno);
		}else{
			while((ulVal>>k) >= maxLeadZero){
				k++;
			}
			ulLeadZero = (ulVal>>k);
			EncodeWaveletPutBits(0,32,            bufno);
			if(ulLeadZero > (maxLeadZero>>1)){	
				EncodeWaveletPutBits(0,ulLeadZero-32, bufno);
			}	
			EncodeWaveletPutBits(1,        1 , bufno);
			EncodeWaveletPutBits(k-ulRiceParam, nMExt, bufno);
			ulRemain	= ulVal & ((1<<k)-1);
			ulRiceParam = k;
			EncodeWaveletPutBits(ulRemain, k , bufno);
		}
	}
}



