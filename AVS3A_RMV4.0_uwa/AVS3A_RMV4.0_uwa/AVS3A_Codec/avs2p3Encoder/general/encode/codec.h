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

#ifndef _tianlai_codec_h_
#define _tianlai_codec_h_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "avs2audio.h"
#include "tns1.h"


/* tianlai_info contains all the setup information specific to the
   specific compression/decompression mode in progress (eg,
   psychoacoustic settings, channel setup, options, codebook
   etc). tianlai_info and substructures are in backends.h.
*********************************************************************/
#if 1

#ifndef PI
#define PI        3.14159265358979323846264338327950288
#endif
	
typedef struct tianlai_info{
  int version;
  int channels;
  long rate;

//  long bitrate_upper;
//  long bitrate_nominal;
 // long bitrate_lower;
 // long bitrate_window;

  void *codec_setup;
} tianlai_info;

/* tianlai_dsp_state buffers the current tianlai audio
   analysis/synthesis state.  The DSP state belongs to a specific
   logical bitstream ****************************************************/
typedef struct tianlai_dsp_state{
  int analysisp;
  tianlai_info *vi;

  float **pcm;
  float **pcmret;
  int      pcm_storage;
  int      pcm_current;
  int      pcm_returned;

  int  preextrapolate;
  int  eofflag;

  long lW;
  long W;
  long nW;
  long centerW;

  long granulepos;
  long sequence;

  long glue_bits;
  long time_bits;
  long floor_bits;
  long res_bits;

  void       *backend_state;
} tianlai_dsp_state;

/* tianlai_block is a single block of data to be processed as part of
the analysis/synthesis stream; it belongs to a specific logical
bitstream, but is independent from other tianlai_blocks belonging to
that logical bitstream. *************************************************/

struct alloc_chain{
  void *ptr;
  struct alloc_chain *next;
};

typedef struct tianlai_block{
  /* necessary stream state for linking to the framing abstraction */
//  float  **pcm;       /* this is a pointer into local storage */
  avs2audiopack_buffer opb;

  long  lW;
  long  W;
  long  nW;
  int   pcmend;
  int   mode;

  int         eofflag;
  long granulepos;
  long sequence;
  tianlai_dsp_state *vd; /* For read-only access of configuration */

  /* local storage to avoid remallocing; it's up to the mapping to
     structure it */
  void               *localstore;
  long                localtop;
  long                localalloc;
  long                totaluse;
  struct alloc_chain *reap;

  /* bitmetrics for the frame */
  long glue_bits;
  long time_bits;
  long floor_bits;
  long res_bits;

  void *internal;

  int floor_post[8][50];
  int output[1024];

  tns_info tnsInfo;
  tns_config tnsConf[4];

  //int lg_lf;

    int encLen;

  float SpectralCoefficient[FRAME_LEN_LONG];
} tianlai_block;
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

