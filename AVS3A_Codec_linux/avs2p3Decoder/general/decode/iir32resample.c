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
#include <assert.h>
#include <stdio.h>
#include <string.h>  
#include "FloatFR.h"
#include "iir32resample.h"

#define IIR_UPSAMPLE_FAC  2
#define IIR_32_ORDER  8

static const float coeffNum[IIR_32_ORDER] = 
  {2.129719423e-03f*IIR_UPSAMPLE_FAC, -9.219380130e-04f*IIR_UPSAMPLE_FAC, 3.859704087e-03f*IIR_UPSAMPLE_FAC, 1.218339222e-03f*IIR_UPSAMPLE_FAC, 
   1.218339222e-03f*IIR_UPSAMPLE_FAC, 3.859704087e-03f*IIR_UPSAMPLE_FAC, -9.219380130e-04f*IIR_UPSAMPLE_FAC, 2.129719423e-03f*IIR_UPSAMPLE_FAC};
static const float coeffDen[IIR_32_ORDER] = 
  {-1.000000000e+00f, 4.917738074e+00f, -1.129019179e+01f, 1.541498076e+01f, 
   -1.342576947e+01f, 7.432055685e+00f, -2.419025499e+00f, 3.576405931e-01f};

#define IIR_CHANNELS 2
#define IIR_DOWNSAMPLE_FAC  3
#define IIR_INTERNAL_BUFSIZE (IIR_UPSAMPLE_FAC*IIR_DOWNSAMPLE_FAC + IIR_32_ORDER)

static float statesIIR[IIR_32_ORDER * IIR_CHANNELS];

int IIR32Resample( float *inbuf,
               float *outbuf,
               int    inSamples,
               int    outSamples,
               int    stride)
{
  int i, k, s, ch, r;
  double accu;
  float  scratch[IIR_INTERNAL_BUFSIZE];
  int nProcessRuns  = outSamples  >> 1;

  for (ch=0; ch<stride; ch++) {
    int idxIn  = ch;
    int idxOut = ch;

    for (s=0; s<IIR_32_ORDER; s++) {

      scratch[s] = statesIIR[s*stride+ch];
    }

    for (r=0; r<nProcessRuns; r++) {


      s=IIR_32_ORDER;

      for (i=0; i<IIR_DOWNSAMPLE_FAC; i++) {

        accu = inbuf[idxIn];

        for (k=1; k<IIR_32_ORDER; k++) {
          accu += coeffDen[k] * scratch[s-k];
        }

        scratch[s] = (float) accu;

        s++;

        accu = 0.0;

        for (k=1; k<IIR_32_ORDER; k++) {

          accu += coeffDen[k] * scratch[s-k];
        }

        scratch[s] = (float) accu;

        s++;

        idxIn += stride;

      }

      s = IIR_32_ORDER;

      for (i=0; i<IIR_UPSAMPLE_FAC; i++) {

        accu = coeffNum[0] * scratch[s];

        for (k=1; k<IIR_32_ORDER; k++) {

          accu += coeffNum[k] * scratch[s-k];
        }

        outbuf[idxOut] = (float) accu;


        s += IIR_DOWNSAMPLE_FAC;

        idxOut += stride;

      }

      memmove( &scratch[0], &scratch[IIR_UPSAMPLE_FAC*IIR_DOWNSAMPLE_FAC], IIR_32_ORDER*sizeof(float));

    }

    for (s=0; s<IIR_32_ORDER; s++) {

      statesIIR[s*stride+ch] = scratch[s];
    }


  } /* ch */

  
  return outSamples * stride;
}

int
IIR32GetResamplerFeed( int blockSizeOut)
{
  int size;

  size  = blockSizeOut * 3;

  size /= 2;


  return size;
}



void
IIR32Init( void)
{
  unsigned int s;


  for (s=0; s<sizeof(statesIIR)/sizeof(float); s++) {

    statesIIR[s] = 0.0f;
  }

}
