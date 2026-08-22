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

#include <stdlib.h>
#include <memory.h>
#include "avs2BweEncMDFT.h"
#include "../encode/lfenc.h"

int LFmodeselet_frame(StAvs2BweMDFT *pstAvs2BweMDFT, int *Groupmode, int *Seqmode, int bitRate,int **lf_winseq,long Maxpcmvalue);
int updatesuperframeanalysis2(StAvs2BweMDFT *pstAvs2BweMDFT,int ch);


int Avs2EncMDFTfunOpen()
{
	//变换所用表数据 
	bulidcossintable4096();
	invbitN4096();
	bulidcossintable512();
	invbitN512();
	bulidcossintable2048();
	invbitN2048();
	bulidcossintable1024();
	invbitN1024();
	bulidcossintable256();
	invbitN256();
	bulidcossintable128();
	invbitN128();

	return 0;
}

/*
函数初始化
*/
int Avs2BweMDFTOpen(unsigned int *ppst_in, unsigned int *ppst_common)
{
	StAvs2BweMDFT *pstBweMDFT; 
	StAvs2BweCommon *pstBweCommon; 

	int i;
	//变换
	*ppst_in = (unsigned int) (malloc(sizeof(StAvs2BweMDFT)));

	pstBweMDFT = (*ppst_in );

	memset((void*)*ppst_in,0,sizeof(StAvs2BweMDFT));


	//公有部分
	*ppst_common = (unsigned int)(malloc(sizeof(StAvs2BweCommon)));
	pstBweCommon = (*ppst_common);
	memset((void*)*ppst_common,0,sizeof(StAvs2BweCommon));

	pstBweMDFT->onsetpos =0;
	pstBweMDFT->preoffset=0;
	pstBweMDFT->pre_flag =0;
	pstBweMDFT->cur_flag =0;
	pstBweMDFT->globalengall=0;

	pstBweMDFT->pre_blocktypeB=1;
	
	pstBweMDFT->rightnext=4;pstBweMDFT->leftnext=4;pstBweMDFT->rightnow=4;pstBweMDFT->leftnow=4;
	for(i=0;i<18;i++)
		pstBweMDFT->onsetflag[i]=0; 
	pstBweMDFT->seqnext=4;pstBweMDFT->seqnow=4;
	pstBweMDFT->pre_Swinseq0=1;

	/////////////////////
	pstBweCommon->preoffset=0;
	pstBweCommon->pre_flag=0;
	pstBweCommon->cur_flag=0;

	return 0;
}


int Avs2BweMDFTTransform(int *st_in,  int *st_common,int nChannels, int bitRate, int *blocktypeBout)
{  
	int (*WinseqTabletmp)[20];
	int ch=0;	//只对0通道做分析和变换
	int Groupmode, Seqmode;


	StAvs2BweMDFT *pstBweMDFT; 
	StAvs2BweCommon *pstBweCommon; 

	pstBweMDFT= (StAvs2BweMDFT*)(st_in);
	pstBweCommon= (StAvs2BweCommon *)(st_common);

//	for(i=0;i<nChannels;i++)
//		savecurrentsuperframedata(pstBweMDFT,curdata+i,i);
	

	if(nChannels==0)
	{
		Groupmode = 4;
		Seqmode = 9;
		modeselet_frame(pstBweMDFT, pstBweCommon,&Groupmode, &Seqmode, bitRate);
  
		pstBweCommon->nGroupmode = Groupmode;
		pstBweCommon->nSeqmode = Seqmode;
	}
	else
	{
		Groupmode = pstBweCommon->nGroupmode;
		Seqmode = pstBweCommon->nSeqmode;
	}
	WinseqTabletmp =(int (*)[20])getWinseqTable(Groupmode);
	mdftframeblock_multi_frame(pstBweMDFT,ch,(int*)((WinseqTabletmp[Seqmode]+3)),pstBweCommon->mdft4096block_complex[ch],WinseqTabletmp[Seqmode][2]+4096/4);


	{
		int onsetpos=0;
		int onsetflag=0;
		
		onsetflag =Onsetdetector_multi(&onsetpos, (int*)((WinseqTabletmp[Seqmode]+3)),pstBweCommon->mdft4096block_complex[ch], ch, WinseqTabletmp[Seqmode][2]+4096/4,pstBweCommon->nGroupmode,pstBweCommon->nSeqmode);
		
		//lijing added
		if(Groupmode == 0)
			onsetflag = 0;
		//20211118
		onsetflag = 1;
		 pstBweCommon->nOnsetFlag = onsetflag;
		 pstBweCommon->nOnsetPos = onsetpos;
		 	
	}


	mdftframeblock_multi_frame(pstBweMDFT,ch, (int*)((WinseqTable_00[5]+3)), pstBweCommon->mdft4096block_2048complex[ch], WinseqTable_00[5][2]+4096/4);

//	*blocktypeBout =blocktypeB;
	return 0;
}

int Avs2LFMDFTupdate(unsigned int *st_in)
{
	StAvs2BweMDFT *pstBweMDFT; 

	pstBweMDFT= (StAvs2BweMDFT*)(st_in);

	  
	updatesuperframeanalysis(pstBweMDFT,0);
	//updatesuperframeanalysis(pstBweMDFT,1);

	return 0;
}
int Avs2MDFTupdate(unsigned int *st_in)
{
	StAvs2BweMDFT *pstBweMDFT; 

	pstBweMDFT = (StAvs2BweMDFT*)(st_in);

	  
	updatesuperframeanalysis2(pstBweMDFT,0);
	//updatesuperframeanalysis2(pstBweMDFT,1);

	return 0;
}

int Avs2BweMDFTClose(unsigned int *st_in, unsigned int *st_common)
{

	//变换
	if(*st_in != 0)
		free((void*)*st_in);
	
	//公有部分
	if(*st_common != 0)
		free((void*)*st_common);

	return 0;	
}


int Avs2LFmodeselect( int *st_in,float *curdata,int bitRate,int **lf_winseq,int numSamplesRead ,long Maxpcmvalue)
{  
	int Groupmode, Seqmode;
	int i;

	StAvs2BweMDFT *pstBweMDFT; 

	pstBweMDFT = (StAvs2BweMDFT*)(st_in);

	savecurrentsuperframedata0(pstBweMDFT, curdata, numSamplesRead);

	Groupmode = 4;
	Seqmode = 9;

	LFmodeselet_frame(pstBweMDFT, &Groupmode, &Seqmode, bitRate,lf_winseq,Maxpcmvalue);

	return 0;
}


int Avs2modeselect0( int *st_in, float *curdata, int **lf_winseq, int numSamplesRead,long Maxpcmvalue)
{  
	int Groupmode, Seqmode;
	int i;

	StAvs2BweMDFT *pstBweMDFT; 

	pstBweMDFT = (StAvs2BweMDFT*)(st_in);

	savecurrentsuperframedata0(pstBweMDFT, curdata, numSamplesRead);
	Groupmode = 4;
	Seqmode = 9;

	LF2modeselet_frame0(pstBweMDFT, &Groupmode, &Seqmode, lf_winseq,Maxpcmvalue);

	return 0;
}


int ZeroLFEHighFreq_BWE(int *st_common)
{
	int(*WinseqTabletmp)[20];
	int Groupmode, Seqmode;
	StAvs2BweCommon *pstBweCommon = (StAvs2BweCommon *)(st_common);

	Groupmode = pstBweCommon->nGroupmode;
	Seqmode = pstBweCommon->nSeqmode;

	WinseqTabletmp = (int(*)[20])getWinseqTable(Groupmode);
	int LL[5] = { 4096 / 8 / 2, 4096 / 4 / 2, 4096 / 2 / 2, 4096 / 2, 4096 / 16 / 2 };
	int *lf_winseq = (int*)((WinseqTabletmp[Seqmode] + 3));
	int mdftoffset = 0;
	for (int index = 1; index<(lf_winseq[0] + 1); index++)
	{
		int lowbandoffset;
		int ll = max(LL[lf_winseq[index] - 1], LL[lf_winseq[index + 1] - 1]) / 2;

		switch (ll)
		{
		case 4096 / 8 / 2 / 2:			lowbandoffset = 2; break;
		case 4096 / 8 / 2:				lowbandoffset = 4; break;
		case 4096 / 4 / 2:				lowbandoffset = 8; break;
		case 4096 / 2 / 2:				lowbandoffset = 8; break;
		case 4096 / 16 / 2 / 2:			lowbandoffset = 2; break;
		default:		lowbandoffset = 8; break;
		}

		for (int kk = lowbandoffset * 2; kk < ll * 2; kk++)
		{
			pstBweCommon->mdft4096block_complex[0][mdftoffset + kk * 2] = 0;
			pstBweCommon->mdft4096block_complex[0][mdftoffset + kk * 2 + 1] = 0;
		}
		for (int kk = 0; kk < ll * 2; kk++)
		{
			pstBweCommon->mdft4096block_2048complex[0][mdftoffset + kk * 2] = 0;
			pstBweCommon->mdft4096block_2048complex[0][mdftoffset + kk * 2 + 1] = 0;
		}
		mdftoffset += (ll * 4);
	}
	return 0;
}
