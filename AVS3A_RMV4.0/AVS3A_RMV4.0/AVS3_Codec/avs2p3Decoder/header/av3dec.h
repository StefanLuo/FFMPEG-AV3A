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

#ifndef AV3DEC_H
#define AV3DEC_H

#include <stdio.h>
#include <stdlib.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////
///      macro definition                  //
/////////////////////////////////////////////

#define AVS_LOSSLESS_AUDIO_VERSION "2.0"

/* object types for av3 */
#define AV3_MAIN_LEVEL  1
#define AV3_BASIC_LEVEL 2
#define AVS_LOSSLESS	3

/* av3 header type definition*/
#define AV3_HEAD_DEFAULT 0

/* library output formats */
#define AV3_PCM_8BIT   0
#define AV3_PCM_16BIT  1
#define AV3_PCM_24BIT  2
#define AV3_PCM_32BIT  3
#define AV3_PCM_FLOAT  4

/* at least AV3DEC_MIN_IBUFSIZE bytes per decoded channel should be available */
#define AV3DEC_MIN_IBUFSIZE 768

#define MAX_CHANNELS        8
#define FRAME_LEN 1024
#define FRAMESIZE  1024


///////////////////////////////////////////////
typedef struct{
	/* aatf_decoding_header */
	int sampling_frequency_index;
	int	channel_config;
	int resolution;			
}AATFHeader;

typedef struct AV3DecConfiguration
{
    unsigned char av3ObjectType;
    unsigned char header_type;
    unsigned long defSampleRate;				// default sample rate
    unsigned char outputFormat;  
	unsigned char pcmFormat;					/* the output pcm data format */		
} AV3DecCfg, *AV3DecCfgPtr;

typedef struct
{
    uint8_t object_type;

    uint16_t frameLength;

    uint32_t frame;
 
    /* output data buffer */
    void *sample_buffer;

    /* Configuration data */
    AV3DecCfg config;

} AV3DecFrameInfo, *AV3DecFrameInfoPtr;

/* av3 file buffer */
typedef struct {
    long bytes_into_buffer;
    long bytes_consumed;
    long file_offset;
    unsigned char *buffer;
    int at_eof;
    FILE *infile;
} av3_buffer;

#define LEN_SAMP_IDX 4

static int
samp_rate_info[(1<<LEN_SAMP_IDX)] = 
    {96000, 88200, 64000, 48000, 44100, 32000, 
	 24000, 22050, 16000, 12000, 11025, 8000,
	 192000,176400,128000,0};

enum CH_POS{
	FL = 0,
	FR,
	FC,
	LF,
	LS,
	RS,
	BL,
	BR
};

enum FS_INDEX/* FS index enum */
{
	FSIDX_96=0,
	FSIDX_88,
	FSIDX_64,
	FSIDX_48,
	FSIDX_44,
	FSIDX_32,
	FSIDX_24,
	FSIDX_22,
	FSIDX_16,
	FSIDX_12,
	FSIDX_11,
	FSIDX_8,
	FSIDX_192,
	FSIDX_176,
	FSIDX_128,
	FSIDX_EXT
};

extern int 
lossless_pcm[MAX_CHANNELS][FRAME_LEN];
extern const unsigned char 
defChMappings[MAX_CHANNELS][MAX_CHANNELS];

#ifdef __cplusplus
}
#endif
#endif