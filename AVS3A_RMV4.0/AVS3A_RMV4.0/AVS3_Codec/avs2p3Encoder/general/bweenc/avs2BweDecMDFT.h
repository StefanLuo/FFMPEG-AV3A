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

#ifndef AVS2BWEDECODERHHH
#define AVS2BWEDECODERHHH
#include <string.h>
#include "mdftmultiblockanalysis_dec.h"
#define FRAME_SIZE 1024

typedef struct
{
	int dec_preSeqmode[2];//={-1,-1};
	int dec_preGroupmode[2];//={-1,-1};


    float mdft4096block_decdata[2][4096];//={0};
	//---------------------------------------

	int Groupmode;
	int Seqmode;

	int lf_winseq_pre[20];//={1,4,4};
	float Srstereo[2][4096+2048];
	float BWESrstereo[2][4096+2048];

} StAvs2BweDecMDFT;


typedef struct
{
  float preOrigMdftData[2048*3];

  float bweMdftSpectrum[FRAME_SIZE * 4+2048];

  int Groupmode;
  int Seqmode;

} StAvs2BweDecCommon;

int  Avs2BweDecMDFTOpen(unsigned int *st_in,unsigned int *st_common);
int  Avs2BweDecMDFTClose(unsigned int *st_in,unsigned int *st_common);
int  Avs2BweDecMDFTana(unsigned int *st_decin, unsigned int *st_deccommon,float *decdata,int ch);
int  Avs2BweDecMDFTsyn(unsigned int *st_decin, unsigned int *st_deccommon,float *ptimedata,int ch);
int Avs2DecMDFTupdate(unsigned int *st_decin, unsigned int *st_deccommon,int ch,int *lf_winseq_dec);

int Avs2LFDecMDFTsyn(unsigned int *st_decin, float *ptimedata,float  *Mdftout,int useBWE,int *lf_winseq_dec);



int bulidcossintable4096();
int invbitN4096();
int invbitN128();
int bulidcossintable128();
int invbitN512();
int bulidcossintable512();
int invbitN2048();
int bulidcossintable2048();
int	bulidcossintable1024();
int	invbitN1024();
int invbitN256();
int bulidcossintable256();
int Avs2DecMDFTfunOpen();

 int mdftmultiblock_decodranalysis(// char WindowSequence,            /*ch通道当前帧的块类型 */
								  StAvs2BweDecMDFT *pstBweMDFT,
                            unsigned int ch,             /*通道 */
						   float * ptimedata,  /*输入，ch通道当前帧的时域重构数据*/
						   float * pmdftdata  /*输出，ch通道前一帧的MDFT数据
										
						                      */
                            );
int mdftmultiblock_decodranalysisUpdate(StAvs2BweDecMDFT *pstBweMDFT,
										int Seqmode,            /*当前帧的块类型 */
										int Groupmode, 						
                                 unsigned int ch);



int imdft_lowpassframe4096block_multi(float *Soutput, int *Swinseq,float *Mdftout,int ch,int startpos,float *Sr);

#endif