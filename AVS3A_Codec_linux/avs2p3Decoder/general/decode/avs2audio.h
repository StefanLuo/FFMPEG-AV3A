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

#ifndef _avs2audio_H2
#define _avs2audio_H2

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "os_types.h"

typedef struct {
  long endbyte;
  int  endbit;

  unsigned int *buffer;
  unsigned int *ptr;
  long storage;
} avs2audiopack_buffer;


/* avs2audio_packet is used to encapsulate the data and metadata belonging
   to a single raw avs2audio/tianlai packet *************************************/

typedef struct {
  unsigned char *packet;
  long  bytes;
  long  b_o_s;
  long  e_o_s;

  avs2audio_int64_t  granulepos;

  avs2audio_int64_t  packetno;     /* sequence number for decode; the framing
                                knows where there's a hole in the data,
                                but we need coupling so that the codec
                                (which is in a separate abstraction
                                layer) also knows about the gap */
} avs2audio_packet;


/* avs2audio BITSTREAM PRIMITIVES: bitstream ************************/

// void  avs2audiopack_writeinit(avs2audiopack_buffer *b);

 void  avs2audiopack_writetrunc(avs2audiopack_buffer *b,long bits);
 //void avs2audiopack_write(avs2audiopack_buffer *b,unsigned long value,int bits);

 void  avs2audiopack_reset(avs2audiopack_buffer *b);
 void  avs2audiopack_writeclear(avs2audiopack_buffer *b);
 void  avs2audiopack_readinit(avs2audiopack_buffer *b,unsigned char *buf,int bytes);

 long  avs2audiopack_look(avs2audiopack_buffer *b,int bits);

 void  avs2audiopack_adv(avs2audiopack_buffer *b,int bits);

 long  avs2audiopack_read(avs2audiopack_buffer *b,int bits);

 long  avs2audiopack_bytes(avs2audiopack_buffer *b);

 unsigned char *avs2audiopack_get_buffer(avs2audiopack_buffer *b);

#ifdef __cplusplus
}
#endif

#endif  /* _avs2audio_H */
