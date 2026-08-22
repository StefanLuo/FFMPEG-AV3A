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

#ifndef __MAXCORR_H__
#define __MAXCORR_H__

#include "tns1.h"
#include "avs2encmain.h"

#ifdef __cplusplus
extern "C" {
#endif
 
#ifndef USE_MDCT
	#define USE_MDCT
#endif
#define MAX_ALLCHANNEL  16//(8)

#ifdef	USE_MDCT
#define BLKSCALE 		(4) 
#else
#define BLKSCALE 		(1)
#endif

#define RT0  1
	
	int VQ;
	int VQ_DIM;
	int	BIT_PER_VEC;
	int CH;

#define LOG2            (0.69314718056f)
#define ILOG2           (1.442695041f)

#define M_PI            (3.1415926f)
#define M_PI_2          (M_PI/2)	
#define FRAMESIZE 		(1024)
#define BLOCKSIZE		(FRAMESIZE*2)

#define NSFBAND			(64)
#define MAX_NSF			(12*8)

#define CORE_FRAMESIZE	(AVS2ENC_BLOCKSIZE)

#define MAX(a,b)		((a)>(b)?(a):(b))
#define MIN(a,b)		((a)<(b)?(a):(b))

typedef struct
{
	int brate;
	int couple_config;
	int nchannel;
	int core_brate[MAX_ALLCHANNEL];
	int mcr_brate[MAX_ALLCHANNEL];
}ST_RATE_CONFIG;

typedef struct
{
	short index;
	short sfb_num[4];  // number of subbands with different mdct length (128, 256, 512, 1024)
	short sfb_offset[4][NSFBAND+1];

	short vq_dim[4];
	short vq_size[4];
	short bit_per_vec[4];
	float *pt_cbook[4];
	float *pt_mag_cbook[4];
	float *pt_ang_cbook[4];

	short group_id;
	float group_thr[2];

}MCR_INFO;

typedef struct
{
    float theta; 
	float theta_q;
	int	  theta_pos;
} MAXCORR;

typedef struct
{
	int winseq[20];
	int bandwidth;

	float left_data[BLOCKSIZE];
	float right_data[BLOCKSIZE];
	float sum_data[BLOCKSIZE];
	float dif_data[BLOCKSIZE];

	MAXCORR mcr_data[MAX_NSF*2];

	int sflag;
} PS_DATA;

typedef struct
{
	int Groupmode;
	int Seqmode;
	int bandwidth;

	float left_data[BLOCKSIZE*2+2048];
	float right_data[BLOCKSIZE*2+2048];
	float sum_data[BLOCKSIZE*2+2048];
	float dif_data[BLOCKSIZE*2+2048];

	float timeBuffer[BLOCKSIZE*3];

	MAXCORR mcr_data[MAX_NSF*2];
	MAXCORR ang_data[MAX_NSF*2];

	int sflag;
} PS_BWE_DATA;

int mcr_encode(PS_DATA	*ps_data,
			   MCR_INFO	mcr_info,
			   int		len,
			   int		pos,
			   int		mcrpos,
			   int		flag);

int MCR_Encoder(avs2audiopack_buffer	*data_mcr,
				PS_DATA					*ps_data,
				MCR_INFO				mcr_info,
				PS_DATA					*ps_data_pre);


int bitrate_init(ST_RATE_CONFIG *srate_info, int bitrate, int channel_number_index);

int MCR_BWE_Encoder(int bitRate,
					float					*timeOutput,
					avs2audiopack_buffer	*data_mcr,
					PS_BWE_DATA				*psBweData,
					MCR_INFO				mcrInfo,
					PS_BWE_DATA				*psBweData_pre);

#ifdef __cplusplus
}
#endif

#endif // __MAXCORR_H__
