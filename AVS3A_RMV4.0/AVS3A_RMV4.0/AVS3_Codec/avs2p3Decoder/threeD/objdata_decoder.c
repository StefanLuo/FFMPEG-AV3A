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

#include "..\general\decode\Bitstream.h"

void objdata_decoder(avs2audiopack_buffer *opb, int*count, int*Object_codec_id, int* channel_number, int* objbitrate, int* objbitpersample)
{
	int buffer[100];
	int n = 0;
	int*p1 = buffer;
	int sb;
	int channel_number_index;
	int resolution;
	int bitrate_index;

	*p1 = avs2audiopack_read(opb, 2);
	*Object_codec_id = *p1;
	*count += 2;
	p1 += 1;

	if (*Object_codec_id == 0) {
		*p1 = avs2audiopack_read(opb, 3);
		channel_number_index = *p1;
		*count += 3;
		p1 += 1;
	}
	switch (channel_number_index)
	{
	case 0: *channel_number = 1; break;
	case 1: *channel_number = 2; break;
	case 2: *channel_number = 6; break;
	}


	if (*Object_codec_id == 1) {
		*p1 = avs2audiopack_read(opb, 8);
		*count += 8;
		p1 += 1;
	}

	*p1 = avs2audiopack_read(opb, 2);
	resolution = *p1;
	*count += 2;
	p1 += 1;
	switch (resolution)
	{
	case 0: *objbitpersample = 8; break;
	case 1: *objbitpersample = 16; break;
	case 2: *objbitpersample = 24; break;
	}

	if (*Object_codec_id == 0) {
		*p1 = avs2audiopack_read(opb, 6);
		bitrate_index = *p1;
		*count += 6;
		p1 += 1;
		switch (bitrate_index)
		{
		case 0: *objbitrate = 16000; break;
		case 1: *objbitrate = 32000; break;
		case 2: *objbitrate = 44000; break;
		case 3: *objbitrate = 56000; break;
		case 4: *objbitrate = 64000; break;
		case 5: *objbitrate = 72000; break;
		case 6: *objbitrate = 80000; break;
		case 7: *objbitrate = 96000; break;
		case 8: *objbitrate = 128000; break;
		case 9: *objbitrate = 144000; break;
		case 10: *objbitrate = 164000; break;
		case 11: *objbitrate = 192000; break;

		case 16: *objbitrate = 24000; break;
		case 17: *objbitrate = 32000; break;
		case 18: *objbitrate = 48000; break;
		case 19: *objbitrate = 64000; break;
		case 20: *objbitrate = 80000; break;
		case 21: *objbitrate = 96000; break;
		case 22: *objbitrate = 128000; break;
		case 23: *objbitrate = 144000; break;
		case 24: *objbitrate = 196000; break;
		case 25: *objbitrate = 256000; break;
		case 26: *objbitrate = 320000; break;

		case 32: *objbitrate = 128000; break;
		case 33: *objbitrate = 192000; break;
		case 34: *objbitrate = 256000; break;
		case 35: *objbitrate = 320000; break;
		case 36: *objbitrate = 384000; break;
		case 37: *objbitrate = 448000; break;
		case 38: *objbitrate = 512000; break;
		case 39: *objbitrate = 640000; break;
		case 40: *objbitrate = 720000; break;
		}

		if (*Object_codec_id == 0) {
			*p1 = avs2audiopack_read(opb, 1);
			*count += 1;
			p1 += 1;
		}
	}
}
