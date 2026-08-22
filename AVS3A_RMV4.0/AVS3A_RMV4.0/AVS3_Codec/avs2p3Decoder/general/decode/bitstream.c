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
#include "bitstream.h"
#include "avs2audio.h"

//unsigned int readBuf[1024 * 6 * 3 / 2 / 4];


//endbyte must be set 0; other value is invalid.
//void avs2audiopack_init(avs2audiopack_buffer *b, unsigned int *inbuf, int storage, int endbyte,int endbit)
//{
//	b_packbuffer = b;
//
//	memset(b, 0, sizeof(avs2audiopack_buffer));
//	b_packbuffer->buffer = b_packbuffer->ptr = inbuf;
//	b_packbuffer->storage = storage;
//	b_packbuffer->endbit = endbit;
//	b_packbuffer->endbyte = endbyte;
//}

void avs2audiopack_adv(avs2audiopack_buffer *b_packbuffer,int bits)
{
	bits += b_packbuffer->endbit;

	b_packbuffer->ptr += bits / 32;
	b_packbuffer->endbyte += bits / 32;
	b_packbuffer->endbit = bits & 31;
	return;
#if 0
overflow:
	//b_packbuffer->ptr = NULL;
	b_packbuffer->endbyte = b_packbuffer->storage;
	b_packbuffer->endbit = 1;
#endif
}

/* Read in bits without advancing the bitptr; bits <= 32 */
long avs2audiopack_look(avs2audiopack_buffer *b_packbuffer, int bits)
{
	unsigned long ret;
	unsigned long m;

	if (bits<0 || bits>32) return -1;
	m = mask[bits];
	bits += b_packbuffer->endbit;


	ret = b_packbuffer->ptr[0] >> b_packbuffer->endbit;
	if (bits>32 && b_packbuffer->endbit){
		ret |= b_packbuffer->ptr[1] << (32 - b_packbuffer->endbit);
	}
	return(m&ret);
}

///* bits <= 32 */
long avs2audiopack_read(avs2audiopack_buffer *b, int bits)
{
	long ret;
	unsigned long m;

	if (bits<0 || bits>32) goto err;
	m = mask[bits];
	bits += b->endbit;

	if (b->endbyte >= b->storage - 4)
	{
		/* not the main path */
		if (b->endbyte > b->storage - ((bits + 31) >> 5)) goto overflow;
		/* special case to avoid reading b->ptr[0], which might be past the end of  the buffer; also skips some useless accounting */
		else if (!bits)
			return(0L);
	}

	ret = b->ptr[0] >> b->endbit;
	if (bits>32 && b->endbit)
		ret |= b->ptr[1] << (32 - b->endbit);

	ret &= m;
	b->ptr += bits / 32;
	b->endbyte += bits / 32;
	b->endbit = bits & 31;
	return ret;

overflow:
err :
	b->ptr = NULL;
	b->endbyte = b->storage;
	b->endbit = 1;

	printf("(In function : avs2audiopack_read )ERROR: overflow");
	return -1L;
}
