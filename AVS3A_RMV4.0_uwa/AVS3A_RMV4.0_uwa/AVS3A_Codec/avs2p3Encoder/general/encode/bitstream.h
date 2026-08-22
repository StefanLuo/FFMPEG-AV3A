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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "avs2audio.h"

#define BUFFER_INCREMENT 1024
static const unsigned long mask[]=
{0x00000000,0x00000001,0x00000003,0x00000007,0x0000000f,
0x0000001f,0x0000003f,0x0000007f,0x000000ff,0x000001ff,
0x000003ff,0x000007ff,0x00000fff,0x00001fff,0x00003fff,
0x00007fff,0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
0x000fffff,0x001fffff,0x003fffff,0x007fffff,0x00ffffff,
0x01ffffff,0x03ffffff,0x07ffffff,0x0fffffff,0x1fffffff,
0x3fffffff,0x7fffffff,0xffffffff };


void avs2audiopack_reset(avs2audiopack_buffer *b);

void avs2audiopack_writeinit(avs2audiopack_buffer *b);

long avs2audiopack_bytes(avs2audiopack_buffer *b);
void avs2audiopack_writetrunc(avs2audiopack_buffer *b,long bits);

/**/void avs2audiopack_writeclear(avs2audiopack_buffer *b);

/* Takes only up to 32 bits. */
void avs2audiopack_write(avs2audiopack_buffer *b,unsigned long value,int bits);

void avs2audiopack_adv(avs2audiopack_buffer *b,int bits);

/* Read in bits without advancing the bitptr; bits <= 32 */
long avs2audiopack_look(avs2audiopack_buffer *b,int bits);


void avs2audiopack_readinit(avs2audiopack_buffer *b,unsigned char *buf,int bytes);


/* bits <= 32 */
long avs2audiopack_read(avs2audiopack_buffer *b,int bits);

unsigned char *avs2audiopack_get_buffer(avs2audiopack_buffer *b);