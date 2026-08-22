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

#ifndef DECODER_H
#define DECODER_H

#include <stdio.h>
#include "dec.h"

typedef struct
{
	short FileFormat;            /* File format */
	int bitRate;
	short bitsPerSample;
	long sampleRate;
	short nChannels;

	int ref_frame_interval;
	int use_mono_encode;		/* force to encode in mono */	
} ChanInfo;

FILE *Wave_fopen(char *Filename, char *Mode, short *NumOfChannels, long *SamplingRate, short *BitsPerSample,
				 long *DataSize);

void Wave_fclose(FILE *FilePtr, short BitsPerSample);


void write_data(
			   float data[],  /* input : data              */
			   int   size,    /* input : number of samples */
			   	int  bitsPerSample,    /* input : bitsPerSample */
			   FILE  *fp      /* output: file pointer      */
			   );

extern int Avs2BweDecoderOpen(unsigned int *st_in, int bitrate, int sampleRateCore, int numChannels, int *bandWidth,int *config_idx);
extern int Avs2BweDecoder(int bitRate, BWEBITSTREAM* pStreamBwe, unsigned int *pst_in, unsigned int *pst_common,int bitsPerSample);

extern int Avs2BweDecoderClose(unsigned int *st_in);

#endif
