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

#ifndef NNNNNN2048

#define NNNNNN2048
#include <math.h>
#include <stdio.h>
#include "mdftimdft4096.h"
/*
设定数据结构 float/double
*/
//#define FLOAT float

int invbitN128();
int bulidcossintable128();

/*
256- MDFT
*/
int mdft128(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);

/*
256- inverse MDFT
*/
int imdft128(FLOAT *Sr, FLOAT *Si,FLOAT *sout);
/*
256-MDFT 初始化
invbitN256(),  setting the Bit-Reverse table
bulidcossintable256(), setting the cos/sin function table
*/
int invbitN256();
int bulidcossintable256();

/*
256- MDFT
*/
int mdft256(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);

/*
256- inverse MDFT
*/
int imdft256(FLOAT *Sr, FLOAT *Si,FLOAT *sout);

/*
2048-MDFT 初始化
invbitN2048(),  setting the Bit-Reverse table
bulidcossintable2048(), setting the cos/sin function table
*/
int invbitN2048();
int bulidcossintable2048();

/*
2048- MDFT
*/
int mdft2048(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);

/*
2048- inverse MDFT
*/
int imdft2048(FLOAT *Sr, FLOAT *Si,FLOAT *sout);
#endif
