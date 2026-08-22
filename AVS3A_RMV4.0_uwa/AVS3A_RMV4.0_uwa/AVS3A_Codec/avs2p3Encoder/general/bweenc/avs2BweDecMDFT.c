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

#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include "avs2BweDecMDFT.h"


int Avs2DecMDFTfunOpen()
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
int Avs2BweDecMDFTOpen(unsigned int *st_in, unsigned int *st_common)
{
StAvs2BweDecMDFT *pstBweMDFT; 
	//变换
	*st_in = (unsigned int*) (malloc(sizeof(StAvs2BweDecMDFT)));


	memset((void*)(*st_in),0,sizeof(StAvs2BweDecMDFT));


	//公有部分
	*st_common = (unsigned int*)(malloc(sizeof(StAvs2BweDecCommon)));

	memset((void*)(*st_common),0,sizeof(StAvs2BweDecCommon));


	pstBweMDFT = *st_in;
	pstBweMDFT->lf_winseq_pre[0]=1;//
	pstBweMDFT->lf_winseq_pre[1]=4;
	pstBweMDFT->lf_winseq_pre[2]=4;

	memset(pstBweMDFT->Srstereo[0],0,4096*sizeof(float));
	memset(pstBweMDFT->Srstereo[1],0,4096*sizeof(float));
	memset(pstBweMDFT->BWESrstereo[0],0,4096*sizeof(float));
	memset(pstBweMDFT->BWESrstereo[1],0,4096*sizeof(float));

	return 0;

}
int Avs2BweDecMDFTClose(unsigned int *st_in, unsigned int *st_common)
{

	//变换
	if(*st_in!=0)
		free((void*)(*st_in));
	
	//公有部分
	if(*st_common!=0)
		free((void*)(*st_common));
	return 0;
	
}


int Avs2BweDecMDFTana(unsigned int *st_decin, unsigned int *st_deccommon, float *decdata, int ch)
{
	int Seqmode,Groupmode;
	StAvs2BweDecMDFT *pstBweMDFT; 
	StAvs2BweDecCommon *pstBweCommon; 

	pstBweMDFT = (StAvs2BweDecMDFT*)(st_decin);
	pstBweCommon = (StAvs2BweDecCommon *)(st_deccommon);
	{	
	int (*BlockseqTabletmp)[16];
	int (*WinseqTabletmp)[20];
		
		//Seqmode 前一帧编码所用的窗型序列和块长序列所对应的序号
		Seqmode = mdftmultiblock_decodranalysis(pstBweMDFT, ch, decdata, pstBweCommon->preOrigMdftData);
	
		if(Seqmode<0)
		{
			Seqmode = 0;
			Groupmode = 0;
		}
		Groupmode = (Seqmode%8);
		Seqmode = (Seqmode>>3);

		BlockseqTabletmp =(int (*)[16])getBlockseqTable(Groupmode);
        WinseqTabletmp =(int (*)[20])getWinseqTable(Groupmode);
		upsamplemdftframeblock(pstBweCommon->preOrigMdftData,(BlockseqTabletmp[Seqmode]));
		  
	}	  	


	pstBweMDFT->Groupmode= Groupmode;
	pstBweMDFT->Seqmode = Seqmode;

	
	return Seqmode;

}


int Avs2BweDecMDFTsyn(unsigned int *st_decin, unsigned int *st_deccommon,float *ptimedata,int ch)
{
	StAvs2BweDecMDFT *pstBweMDFT; 
	StAvs2BweDecCommon *pstBweCommon; 
	int (*BlockseqTabletmp)[16];
	int (*WinseqTabletmp)[20];

	pstBweMDFT = (StAvs2BweDecMDFT*)(st_decin);
	pstBweCommon = (StAvs2BweDecCommon *)(st_deccommon);

	BlockseqTabletmp = (int(*)[16])getBlockseqTable(pstBweMDFT->Groupmode);
        WinseqTabletmp = (int(*)[20])getWinseqTable(pstBweMDFT->Groupmode);
	if((pstBweMDFT->Seqmode)>-1)
	{

        /*对处理后的mdft系数进行逆变换，并进行加窗混叠重构，得到时域信号（FRAME_SIZE*2=1024*2长度）*/
		//逆变换
	

		imdft_frame4096block_multi(ptimedata, (int*)((WinseqTabletmp[pstBweMDFT->Seqmode]+3)), pstBweCommon->bweMdftSpectrum, WinseqTabletmp[pstBweMDFT->Seqmode][2]+4096/4,pstBweMDFT->BWESrstereo[0]);

		

	}//if(preWindowSequence!=-1)

	return 0;
}

int Avs2DecMDFTupdate(unsigned int *st_decin, unsigned int *st_deccommon,int ch,int *lf_winseq_dec)
{
	int Groupmode, Seqmode;
    StAvs2BweDecMDFT *pstBweMDFT; 
	StAvs2BweDecCommon *pstBweCommon; 

	pstBweMDFT = (StAvs2BweDecMDFT*)(st_decin);
	pstBweCommon = (StAvs2BweDecCommon *)(st_deccommon);

	
	Groupmode = pstBweCommon->Groupmode;
	Seqmode = pstBweCommon->Seqmode;	

	mdftmultiblock_decodranalysisUpdate(pstBweMDFT,Seqmode,Groupmode,ch); //

	memcpy(pstBweMDFT->lf_winseq_pre,lf_winseq_dec,20*4);

	return 0;
}


int Avs2LFDecMDFTsyn(unsigned int *st_decin, float *ptimedata, float *Mdftout,int useBWE, int *lf_winseq_dec)
{
	StAvs2BweDecMDFT *pstBweMDFT; 


	pstBweMDFT = (StAvs2BweDecMDFT*)(st_decin);


	if(useBWE)
		imdft_lowpassframe4096block_multi(ptimedata, pstBweMDFT->lf_winseq_pre, Mdftout, 0, 4096/4/2, pstBweMDFT->Srstereo[0]);
	else
		imdft_lowpassframe4096block_multi(ptimedata, lf_winseq_dec, Mdftout, 0, 4096/4/2, pstBweMDFT->Srstereo[0]);



	return 0;
}