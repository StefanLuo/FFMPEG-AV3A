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

#ifndef _V_CODEBOOK_H_
#define _V_CODEBOOK_H_

#include "lfenc.h"

extern void tianlai_staticbook_destroy(static_codebook *b);
extern int tianlai_book_init_encode(codebook *dest,const static_codebook *source);
extern int tianlai_book_init_decode(codebook *dest,const static_codebook *source);
extern void tianlai_book_clear(codebook *b);

extern float *_book_unquantize(const static_codebook *b,int n,int *map);
extern float *_book_logdist(const static_codebook *b,float *vals);
extern float _float32_unpack(long val);
extern long   _float32_pack(float val);
extern int  _best(codebook *book, float *a, int step);
extern int _ilog(unsigned int v);
extern long _book_maptype1_quantvals(const static_codebook *b);

extern int tianlai_book_besterror(codebook *book,float *a,int step,int addmul);
extern long tianlai_book_codeword(codebook *book,int entry);
extern long tianlai_book_codelen(codebook *book,int entry);

extern int tianlai_book_encode(codebook *book, int a, avs2audiopack_buffer *b);

extern long tianlai_book_decode(codebook *book, avs2audiopack_buffer *b);
extern long tianlai_book_decodevs_add(codebook *book, float *a,
                                     avs2audiopack_buffer *b,int n);
extern long tianlai_book_decodev_set(codebook *book, float *a,
                                    avs2audiopack_buffer *b,int n);
extern long tianlai_book_decodev_add(codebook *book, float *a,
                                    avs2audiopack_buffer *b,int n);
extern long tianlai_book_decodevv_add(codebook *book, float **a,
                                     long off,int ch,
                                    avs2audiopack_buffer *b,int n);

int tianlai_huffmantable_init_decode(codebook *c, int codelist[], int lengthlist[], int entries, int dim);

#endif
