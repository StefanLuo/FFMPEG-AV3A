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

#ifndef MDFTMULTIBLOCKANA
#define MDFTMULTIBLOCKANA
#include "avs2BweDecMDFT.h"

#include "../bweenc/mdftimdft4096.h"

int WinseqTable[71][10];
int BlockseqTable[71][10];

int Seqstart1[28][12];
int Seqstart2[25][11];
int Seqstart3[14][11];
int Seqstart4[4][11];

static int WinseqTable_00[9][20];

static int GroupseqTable_00[9][10];
static int BlockseqTable_00[9][16];
static int WinseqTable_01[64][20];
static int GroupseqTable_01[64][10];

static int BlockseqTable_01[64][16];
static int WinseqTable_02[140][20];

static int GroupseqTable_02[140][10];

static int BlockseqTable_02[140][16];
static int WinseqTable_03[256][20];

static int GroupseqTable_03[256][10];
static int BlockseqTable_03[256][16];

static int WinseqTable_04[1024][20];
static int BlockseqTable_04[1024][16];
static int GroupseqTable_04[1024][10];

static int LL[5];

int mdftframeblock(float *Sinput, int *Swinseq,float *Mdftout);


int modeselect(float * superdataframe,int premode,int ch);

int **getWinseqTable(int Groupmode);
int  ** getBlockseqTable(int Groupmode);
int  *getGroupseqTable(int Groupmode);

int mdftframeblock_multi(float *Sinput, int *Swinseq,float *Mdftout,int startpos);
int Onsetdetector_multi(int *onsetpos, int *Swinseq,float *Mdftout,int ch,int startpos,int nGroupmode,int nSeqmode);
int upsamplemdftframeblock(float *mdft4096block,int *Blockseq);
int imdft_frame4096block_multi(float *Soutput, int *Swinseq,float *Mdftout,int startpos,float *Sr);

/*
4096-2048-1024-512-256-128 MDFT
*/
int mdft4096(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);
int imdft4096(FLOAT *Sr, FLOAT *Si,FLOAT *sout);

int mdft2048(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);
int imdft2048(FLOAT *Sr, FLOAT *Si,FLOAT *sout);

int mdft1024(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);
int imdft1024(FLOAT *Sr, FLOAT *Si,FLOAT *sout);

int mdft512(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);
int imdft512(FLOAT *Sr, FLOAT *Si,FLOAT *sout);

int mdft256(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);
int imdft256(FLOAT *Sr, FLOAT *Si,FLOAT *sout);

int mdft128(FLOAT *sinput, FLOAT *Sr, FLOAT *Si);
int imdft128(FLOAT *Sr, FLOAT *Si,FLOAT *sout);

#endif