/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2004,  Audio Video Coding Standard, Part III
*
* This software module was originally developed by
*
* Lei Miao (win.miaolei@samsung.com), Samsung AIT
* Lei Miao, CBC Multi-channel extension, 2005-09-19
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the users without any
* license fee or royalty on an "as is" basis. The AVS disclaims
* any and all warranties, whether express, implied, or statutory,
* including any implied warranties of merchantability or of fitness
* for a particular purpose. In no event shall the contributors or 
* the AVS be liable for any incidental, punitive, or consequential
* damages of any kind whatsoever arising from the use of this program.
*
* This disclaimer of warranty extends to the user of this program
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The AVS does not represent or warrant that the program furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of AVS, including shareware, may be
* subject to royalty fees to patent holders. Information regarding
* the AVS patent policy is available from the AVS Web site at
* http://www.avs.org.cn
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE AVS PATENT POLICY.
************************************************************************
*/

#ifndef _SAM_CBC_DEC_H
#define _SAM_CBC_DEC_H

#include <stdio.h>
#include "sam_cbc_dec.h"


int sam_init_cbc(int fsidx);

void sam_decodeCBC(int  target,
  int  stereo_mode,
  int  windowSequence[],
  int  num_window_groups,
  int  window_group_length[],
  int  scalefactors[][8][MAX_SCFAC_BANDS],
  int  samples[][FRAMESIZE],
  int  maxSfb[],
  int  ps_mask[],	   			
  int  ubits,
  int  frameSize,
  int  enc_top_layer,
  int  base_snf,
  int  base_band,
  int  nch,
  int  fill_enable,
  int  fill_length,
  int  *isLFE,
  // 20060116 Miao
  int  mc_present,
  int  *multi_fillLen

  );

void sam_dequantization(int target,
  int  windowSequence,
  int  scalefactors[][MAX_SCFAC_BANDS],
  int  num_window_groups,
  int  window_group_length[],
  int  samples[],
  int  maxSfb,
  int spectrums[],//Revised  by CASKY (Floating point to Fixed point)
  int  ch);

extern int siCodeMode;

#endif