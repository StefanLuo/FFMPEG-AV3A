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

#ifndef _V_SCALES_H_
#define _V_SCALES_H_

#include <math.h>

#ifdef _MSC_VER
/* MS Visual Studio doesn't have C99 inline keyword. */
#define inline __inline
#endif

/* 20log10(x) */
#define tianlai_IEEE_FLOAT32 1
#ifdef tianlai_IEEE_FLOAT32

static inline float unitnorm(float x){
  union {
    unsigned int i;
    float f;
  } ix;
  ix.f = x;
  ix.i = (ix.i & 0x80000000U) | (0x3f800000U);
  return ix.f;
}

/* Segher was off (too high) by ~ .3 decibel.  Center the conversion correctly. */
static inline float todB(const float *x){
  union {
    unsigned int i;
    float f;
  } ix;
  ix.f = *x;
  ix.i = ix.i&0x7fffffff;
  return (float)(ix.i * 7.17711438e-7f -764.6161886f);
}

#define todB_nn(x) todB(x)

#else

static float unitnorm(float x){
  if(x<0)return(-1.f);
  return(1.f);
}

#define todB(x)   (*(x)==0?-400.f:log(*(x)**(x))*4.34294480f)
#define todB_nn(x)   (*(x)==0.f?-400.f:log(*(x))*8.6858896f)

#endif

#define fromdB(x) (exp((x)*.11512925f))

/* The bark scale equations are approximations, since the original
   table was somewhat hand rolled.  The below are chosen to have the
   best possible fit to the rolled tables, thus their somewhat odd
   appearance (these are more accurate and over a longer range than
   the oft-quoted bark equations found in the texts I have).  The
   approximations are valid from 0 - 30kHz (nyquist) or so.

   all f in Hz, z in Bark */

#define toBARK(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))
#define fromBARK(z) (102.f*(z)-2.f*pow(z,2.f)+.4f*pow(z,3.f)+pow(1.46f,z)-1.f)
#define toMEL(n)    (log(1.f+(n)*.001f)*1442.695f)
#define fromMEL(m)  (1000.f*exp((m)/1442.695f)-1000.f)

/* Frequency to octave.  We arbitrarily declare 63.5 Hz to be octave
   0.0 */

#define toOC(n)     (log(n)*1.442695f-5.965784f)
#define fromOC(o)   (exp(((o)+5.965784f)*.693147f))

#endif
