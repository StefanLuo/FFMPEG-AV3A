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

#ifndef _AV3ENC_H
#define _AV3ENC_H

//#include <libtsp.h>
//#include <libtsp/AFpar.h>

#include "i2r_encoder.h"

/* AVS ID's */
#define AVS1 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef float psyfloat;

#define MAX_CHANNELS 8
#define MAX_BANDS	128

#define SINE_WINDOW 0
#define NSFB_LONG  51
#define NSFB_SHORT 15
#define MAX_SHORT_WINDOWS 8
#define BLOCK_LEN_LONG 1024
#define BLOCK_LEN_SHORT 128
#define MAX_SCFAC_BANDS ((NSFB_SHORT+1)*MAX_SHORT_WINDOWS)

#define FLP_MAX_ORDER			    20
#define FLP_GAIN_THRESH				2.7
#define MAX_SAMPLING_RATES		    12

#define MAX_FILL_BITS				2160
#define FILL_DFT					0
////////////////////////////////////////////////

enum WINDOW_TYPE {
    ONLY_LONG_WINDOW,
    LONG_SHORT_WINDOW,
    ONLY_SHORT_WINDOW,
    SHORT_LONG_WINDOW
};


enum stream_format {
  RAW_STREAM = 0,
  SAVE_STREAM = 1,
  TRANSFORM_STREAM = 2,
};

typedef struct
{
   FILE *fp;
   short bps;
   int channels;
   int samplerate;
   int samples;
} pcmfile_t;

typedef struct {                           
    //int     bitrate;                   
	int     format;
    char*   outFile;                 
    char*   inFile;                      
    int     showHelp;
	/* LFE coding */
	int     msenc;
	int     maxLpcOrder;
	int     wavelet;
	int     entropy;
	int     codecId;
	//int     codingProfile;
} cmd_params;

typedef struct {
    int paired_ch;
} ChannelInfo;


typedef struct EncCfg
{
    /* config version */
    int version;
    /* copyright string */
    char *copyright;
    /* AVS version */
    unsigned int AVSVersion;
	/* samplerate of AV3 file */
	unsigned long sampleRate;
	unsigned int sampleRateIdx;
	/* number of channels in AV3 file */
    unsigned int numChannels;
    /* bitrate / channel of AV3 file */
    unsigned long bitRate;
    /* Bitstream output format (0 = Raw; 1 = ADTS) */
    unsigned int outputFormat;	
	int channel_map[8];

	int isLFE;
	int resolution;
	int msenc;
	int codecId;
	//int codingProfile;
} EncCfg, *EncCfgPtr;

///////////////////////////////////////////////
typedef struct {
    unsigned int usedBytes;
    /* frame number */
    unsigned int frameNum;
	/* Psydchoacoustics data */
    /* sample buffers of current next and next next frame*/
    double *sampleBuff[MAX_CHANNELS];
    /* Channel and Coder data for all channels */
    ChannelInfo channelInfo[MAX_CHANNELS];
    /* Configuration data */
    EncCfg config;

	/* head size (AASF) */
	int headSize;

	Encoder_t *hLosslessEnc;
} EncFrame, *EncFramePtr;

EncFramePtr EncOpen(unsigned long sampleRate,
                    unsigned int numChannels,
                    unsigned long *inputSamples,
                    unsigned long *maxOutputBytes);
int EncEncode(EncFramePtr hEncoder,
                          int *inputBuffer,
                          unsigned int samplesInput,
                          unsigned char *outputBuffer);
int EncClose(EncFramePtr hEncoder);

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

extern const unsigned char 
defChMappings[MAX_CHANNELS][MAX_CHANNELS];

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif