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

#ifndef NNNNNNN4096

#define NNNNNNN4096
#include <math.h>
#include <stdio.h>

/*
设定数据结构 float/double
*/
#define FLOAT float
float superOnlyLongWindowKBD_w[2048];
float superOnlyShortWindowKBD_w[];
float superOnlyShortWindowSine_w[];
/*
512-MDFT 初始化
invbitN512(),  setting the Bit-Reverse table
bulidcossintable512(), setting the cos/sin function table
*/
int invbitN512();
int bulidcossintable512();

/*
512- MDFT
*/
int mdft512(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);

/*
512- inverse MDFT
*/
int imdft512(FLOAT *Sr, FLOAT *Si,FLOAT *sout);


/*
4096-MDFT 初始化
invbitN2048(),  setting the Bit-Reverse table
bulidcossintable2048(), setting the cos/sin function table
*/
int invbitN4096();
int bulidcossintable4096();

/*
2048- MDFT
*/
int mdft4096(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);

/*
2048- inverse MDFT
*/
int imdft4096(FLOAT *Sr, FLOAT *Si,FLOAT *sout);


 
#endif
