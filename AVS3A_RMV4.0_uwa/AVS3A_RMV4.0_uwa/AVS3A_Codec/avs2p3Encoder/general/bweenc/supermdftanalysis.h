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

#ifndef SUPERMDFTANALYSIS
#define SUPERMDFTANALYSIS

#include <math.h>
#include <stdio.h>
#include "avs2BweEncMDFT.h"

extern float superfftTwiddleTab[1024+1];


extern float superLongWindowSine[1024*2];


extern float superShortWindowSine[128*2];

extern float superShortWindowKBD[512];

extern float superLongWindowKBD[];
extern float Short128WindowSine[];

extern float Long1024WindowSine[];

extern float superdataframe[];
int superMdctTransform_Real(float *mdstDelayBuffer,
                   float *realOut,	
                   int windowSequence); 

int superMdstTransform_Real(float *mdstDelayBuffer,
                   float *realOut,	
                   int windowSequence); 


//int savecurrentsuperframedata(StAvs2BweMDFT*pstBweMDFT,float *curdata,int ch);


int updatesuperframeanalysis(StAvs2BweMDFT *pstAVS2BweMDFT,int ch);
int superframemdftanalysis(float *supermdftdata,int ch, int blockType);
int modeselect(float * superdataframe,int premode,int ch);
int *modeselect_0_bwelf(StAvs2BweMDFT *pstAvs2BweMDFT, float *Sinput, int sbrframeindex, int bitRate, long Maxpcmvalue);
#endif