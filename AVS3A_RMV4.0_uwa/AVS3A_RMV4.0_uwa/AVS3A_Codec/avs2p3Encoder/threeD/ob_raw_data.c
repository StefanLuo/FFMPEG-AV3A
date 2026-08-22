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

#include "..\general\encode\Bitstream.h"
#include "..\general\bweenc\encoder.h"
void object_data(int ObjectNum, int Object_ID[], int*OutBytes, char*obInfo, int txt, ChanInfo *objinputInfo)
{
	int i;
	int n;
	int obj;
	int sb;
	int ObjectDataGroupNum = ObjectNum;
	int Object_codec_id = 0;
	int channel_number_index;
	int channel_number = 6;
	int resolution;
	int bitrate_index = 4;
	int bitstream_type = 1;
	int numbit = 0;
	avs2audiopack_buffer *opb;
	opb = calloc(1, sizeof(avs2audiopack_buffer));
	avs2audiopack_writeinit(opb);

	switch (objinputInfo->nChannels)
	{
	case 1: channel_number_index = 0; break;
	case 2: channel_number_index = 1; break;
	case 6: channel_number_index = 2; break;
	}

	switch (objinputInfo->bitsPerSample)
	{
	case 8: resolution = 0; break;
	case 16: resolution = 1; break;
	case 24: resolution = 2; break;
	}

	if (channel_number_index == 0)
	{
		switch (objinputInfo->bitRate)
		{
		case 16000: bitrate_index = 0; break;
		case 32000: bitrate_index = 1; break;
		case 44000: bitrate_index = 2; break;
		case 56000: bitrate_index = 3; break;
		case 64000: bitrate_index = 4; break;
		case 72000: bitrate_index = 5; break;
		case 80000: bitrate_index = 6; break;
		case 96000: bitrate_index = 7; break;
		case 128000: bitrate_index = 8; break;
		case 144000: bitrate_index = 9; break;
		case 164000: bitrate_index = 10; break;
		case 192000: bitrate_index = 11; break;
		}
	}
	if (channel_number_index == 1)
	{
		switch (objinputInfo->bitRate)
		{
		case 24000: bitrate_index = 16; break;
		case 32000: bitrate_index = 17; break;
		case 48000: bitrate_index = 18; break;
		case 64000: bitrate_index = 19; break;
		case 80000: bitrate_index = 20; break;
		case 96000: bitrate_index = 21; break;
		case 128000: bitrate_index = 22; break;
		case 144000: bitrate_index = 23; break;
		case 196000: bitrate_index = 24; break;
		case 256000: bitrate_index = 25; break;
		case 320000: bitrate_index = 26; break;
		}
	}
	if (channel_number_index == 2)
	{
		switch (objinputInfo->bitRate)
		{
		case 128000: bitrate_index = 32; break;
		case 192000: bitrate_index = 33; break;
		case 256000: bitrate_index = 34; break;
		case 320000: bitrate_index = 35; break;
		case 384000: bitrate_index = 36; break;
		case 448000: bitrate_index = 37; break;
		case 512000: bitrate_index = 38; break;
		case 640000: bitrate_index = 39; break;
		case 720000: bitrate_index = 40; break;
		}
	}

	if (txt == 0) {
		avs2audiopack_write(opb, ObjectDataGroupNum, 7);
		numbit += 7;
	}


	avs2audiopack_write(opb, Object_codec_id, 2);
	numbit += 2;
	if (Object_codec_id == 0) {
		avs2audiopack_write(opb, channel_number_index, 3);
		numbit += 3;
	}
	if (Object_codec_id == 1) {
		avs2audiopack_write(opb, channel_number, 8);
		numbit += 8;
	}
	avs2audiopack_write(opb, resolution, 2);
	numbit += 2;
	if (Object_codec_id == 0) {
		avs2audiopack_write(opb, bitrate_index, 6);
		numbit += 6;
		avs2audiopack_write(opb, bitstream_type, 1);
		numbit += 1;
	}

	if (numbit % 8 != 0)
	{
		avs2audiopack_write(opb, 0, (8 - numbit % 8));
		numbit += (8 - numbit % 8);
	}
	*OutBytes = numbit / 8;
	memcpy(obInfo, opb->buffer, opb->endbyte);
}
