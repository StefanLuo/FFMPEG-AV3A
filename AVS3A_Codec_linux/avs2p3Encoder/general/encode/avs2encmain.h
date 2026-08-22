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

#ifndef AVS2ENCMAINH
#define AVS2ENCMAINH

#include "../bweenc/avs2BweEncMDFT.h"
#include "../bweenc/encoder.h"
#include "lfenc.h"

#define CORE_DELAY   (1600)
#define INPUT_DELAY  ((CORE_DELAY)*2+1)     /* ((1600 (core codec)*2 (multi rate) + 6*64 (sbr dec delay) - 2048 (sbr enc delay) + magic*/
#define MAX_DS_FILTER_DELAY 16                         /* the additional max resampler filter delay (source fs)*/

#define MAX_PAYLOAD_SIZE    256  //512
#define MASK      0x0001

void int2bin(int value, int no_of_bits, short *bitstream);

void WriteBitstreamPlus(short length, short offset, short * serial, /*FILE * f_serial*/unsigned char* headbuffer);

int write_avs2file_header(int samplingRate, 
						   int bitrate_type,
						   ChanInfo * conf, int useSuperMode,
						   int cpe_config, int PCAGroupmodeHeader, /*FILE *f_output*/unsigned char* headbuffer);

void write_avs2AATF_header(int samplingRate, 
						   int bitstream_type, 
						   ChanInfo * conf, 
						   /*FILE *f_output*/unsigned char* headbuffer);

void Bitstream_fclose(FILE *f_output, int len, int total_len);

//int Avs2EncMain(int argc, char *argv[]);

//not used, shumin.xu 20200105
//int codectypeselect(struct AVS2_ENCODER *lfEncset[],int nchannels,float mdftSpectrum[][FRAME_LEN_LONG * 2],unsigned int *PCAGroupmode );

#endif