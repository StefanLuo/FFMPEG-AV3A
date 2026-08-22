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

#ifndef _V_BITRATE_H_
#define _V_BITRATE_H_

#include "psy.h"
#include "codec.h"

//对CBR的支持 -cbr 0: abr  -cbr 1: cbr wchg 20210611
#define  CONSTANT_BITRATE_CONTROL 1

typedef struct bitrate_manager_state{
  int            managed;

  long           avg_reservoir;
  long           minmax_reservoir;
  long           avg_bitsper;
  long           min_bitsper;
  long           max_bitsper;

 
  double         avgfloat;

  tianlai_block  *vb;
  int            choice;
} bitrate_manager_state;

/**/
typedef struct bitrate_manager_info{
  long           avg_rate;
  long           min_rate;
  long           max_rate;
  long           reservoir_bits;
  double         reservoir_bias;

  double         slew_damp;

} bitrate_manager_info;

typedef void tianlai_look_floor;
typedef void tianlai_look_residue;

typedef void tianlai_info_floor;
typedef void tianlai_info_residue;
typedef void tianlai_info_mapping;


typedef struct private_state{
  int                     window[2*2];//wuchaogang 2013.8.28

  int                     modebits;
  tianlai_look_floor     **flr;
  tianlai_look_residue   **residue;
  tianlai_look_psy        *psy;
  tianlai_look_psy_global *psy_g_look;

  /* local storage, only used on the encoding side.  This way the
     application does not need to worry about freeing some packets'
     memory and not others'; packet storage is always tracked.
     Cleared next call to a _dsp_ function */
//  unsigned char *header;
//  unsigned char *header1;
//  unsigned char *header2;

  bitrate_manager_state bms;

  long sample_count;
} private_state;

 void tianlai_bitrate_init(tianlai_info *vi,bitrate_manager_state *bs,int useBWE);
 void tianlai_bitrate_clear(bitrate_manager_state *bs);
 int tianlai_bitrate_managed(tianlai_block *vb);
 int tianlai_bitrate_addblock(tianlai_block *vb, int blocknum, int n, int tnsdatasize);
int tianlai_bitrate_flushpacket(tianlai_dsp_state *vd,avs2audio_packet *op);

#endif
