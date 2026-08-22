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

#ifndef AVS2BWEENCODERH
#define AVS2BWEENCODERH

#include <string.h>
#include "mdftmultiblockanalysis.h"

#define SUPERBLOCK_SWITCHING_OFFSET		   ((1*1024+3*128+64+128)*2)

typedef struct
{
	 float superdataframe[(SUPERBLOCK_SWITCHING_OFFSET+2048*2)];

	 int onsetpos;
	 int preoffset;
	 int pre_flag;
	 int cur_flag;
	 float globalengall;

	 int pre_blocktypeB;
	
	 int rightnext;
	 int leftnext;
	 int rightnow;
	 int leftnow;
	 int onsetflag[18]; 
	 int seqnext;
	 int seqnow;
	 int pre_Swinseq0;

	 float energyhighpass0[20];
	 float energyhighpass2[20];
	 float energyhighpass3[20];

	 float corrflag[16];
	 int firstframe;
	 float pre_corrflag[16];
} StAvs2BweMDFT;

typedef struct
{
	
	float mdft4096block_complex[2][4096+2048];
	float mdft4096block_2048complex[2][4096+2048];
	int nGroupmode, nSeqmode;
	int nOnsetFlag, nOnsetPos;
	 ///---------------------------
	 int preoffset;
	 int pre_flag;
	 int cur_flag;

} StAvs2BweCommon;


int  Avs2BweMDFTOpen(unsigned int *ppst_in, unsigned int *ppst_common);
int  Avs2BweMDFTTransform( int *st_in,  int *st_common,int nChannels, int bitRate,int *blocktypeBout);
int  Avs2LFMDFTupdate(unsigned int *st_in);
int  Avs2BweMDFTClose(unsigned int *st_in, unsigned int *st_common);

int Avs2LFmodeselect( int *st_in,float *curdata, int bitRate,int **lf_winseq,int numSamplesRead,long Maxpcmvalue);
int Avs2modeselect( int *st_in,float *curdata,int nChannels, int lfEnc,int **lf_winseq,int numSamplesRead);
int Avs2modeselect0( int *st_in, float *curdata, int **lf_winseq, int numSamplesRead,long Maxpcmvalue);

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

int Avs2EncMDFTfunOpen();

int savecurrentsuperframedata(StAvs2BweMDFT*pstBweMDFT,float *curdata,int ch,int numSamplesRead);
int savecurrentsuperframedata0(StAvs2BweMDFT*pstBweMDFT,float *curdata,int numSamplesRead);
int modeselet_frame(StAvs2BweMDFT *pstAvs2BweMDFT, StAvs2BweCommon *pstBweCommon, int *Groupmode,int *Seqmode, int bitRate);
int mdftframeblock_multi_frame(StAvs2BweMDFT *pstAvs2BweMDFT,int ch,int *Swinseq,float *Mdftout,int startpos);
int updatesuperframeanalysis(StAvs2BweMDFT *pstAVS2BweMDFT,int ch);

int mdft_lowpassframeblock_multi(float *Sinput, int *Swinseq,float *Mdftout,int startpos);
int mdftframeblock_multi(float *Sinput, int *Swinseq,float *Mdftout,int startpos);
int blocktypeB;

int LF2modeselet_frame(StAvs2BweMDFT *pstAvs2BweMDFT, int *Groupmode, int *Seqmode, int lfEnc,int **lf_winseq,long Maxpcmvalue);
int Avs2MDFTupdate(unsigned int *st_in);

int ZeroLFEHighFreq_BWE(int *st_common);
#endif