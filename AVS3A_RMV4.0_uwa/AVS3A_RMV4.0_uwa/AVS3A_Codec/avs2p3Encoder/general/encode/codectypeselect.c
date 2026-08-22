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

//not used, shumin.xu 20200105
#if 0
#include "lfEnc.h"
#include "pca.h"
#define MAX_ALLCHANNEL 14//8

int codectypeselect(struct AVS2_ENCODER *lfEncset[], int nchannels, float mdftSpectrum[][FRAME_LEN_LONG * 2], unsigned int *PCAGroupmode)
{
	float mdftSpectrum_tmp[MAX_ALLCHANNEL][FRAME_LEN_LONG * 2];
	int i;

	float ratio_tmp[6][6];
	int chan01, chan02;
	double engallout[6][6];
	//		double eng_elementout[6];
	//		int maxj;
	//		unsigned int elementgroupmode,;
	//		float maxratio;
	int PCAGroupflang;
	char codectype = 0;
	double engsumallset[6], engsumallset2[6];
	static double engLFEglobal = 0;

	static char codectype_pre = 0;
	static double engLFE = 0;

	PCAGroupflang = 1;

	/*if(nchannels!=5)
	{
		return 0;
	}*/
	if (nchannels == 2)
	{
		if (!((lfEncset[0]->lf_winseq[0] == 1) && (lfEncset[0]->lf_winseq[1] == 4) && (lfEncset[0]->lf_winseq[2] == 4)))
			codectype = 1;
		else
			codectype = 0;

		return codectype;
	}

	for (chan01 = 0; chan01 < 4; chan01 += 2)
	{
		for (chan02 = 0; chan02 < lfEncset[0]->lf_winseq[0] + 2; chan02++)
		{
			if ((lfEncset[0]->lf_winseq[chan02]) != (lfEncset[chan01]->lf_winseq[chan02]))
				PCAGroupflang = 0;

		}

	}

	if (PCAGroupflang == 0)
	{
		codectype = 0;
		return codectype;
	}
	/*	if(!((lfEncset[0]->lf_winseq[0]==1)&&(lfEncset[0]->lf_winseq[1]==4)&&(lfEncset[0]->lf_winseq[2]==4)))
		{
				codectype=0;
				return codectype;
		}*/

	for (chan01 = 0; chan01 < 1; chan01++)
	{
		for (chan02 = chan01 + 1; chan02 < 4; chan02++)
		{

			for (i = 0; i < 1024; i++)
				mdftSpectrum_tmp[0][i] = mdftSpectrum[chan01][i];
			for (i = 0; i < 1024; i++)
				mdftSpectrum_tmp[1][i] = mdftSpectrum[chan02][i];

			multichannelMDCT_PCA_1(&mdftSpectrum_tmp[0], lfEncset[0]->lf_winseq, 2, &mdftSpectrum_tmp[0], 0, &ratio_tmp[chan01][chan02], &engallout[chan01][chan02], engsumallset);
			engsumallset2[chan01] = engsumallset[1];
			engsumallset2[chan02] = engsumallset[2];

			if ((chan01 == 3) || (chan02 == 3))
				ratio_tmp[chan01][chan02] = 0;

		}// for(chan02=0;chan02<6;chan02++)
	}//for(chan01=0;chan01<6;chan01++)

	engLFE = engLFE * 0.9 + 0.1*(engsumallset2[3] * 3 / (engsumallset2[0] + engsumallset2[1] + engsumallset2[2] + 0.001));
	// if(((( ratio_tmp[0][2]>0.8 )||( ratio_tmp[1][2]>0.8 ))&&(( ratio_tmp[0][3]>0.8 )||( ratio_tmp[1][3]>0.8 )))&&(engallset[]))
	if ((((ratio_tmp[0][2] > 0.8) || (ratio_tmp[1][2] > 0.8)) && ((ratio_tmp[0][2] > 0.8) || (ratio_tmp[1][2] > 0.8))) &&
		((engsumallset2[3] + engsumallset2[2] > (engsumallset2[0] + engsumallset2[1]) / 8) && (engsumallset2[3] + engsumallset2[2] < (engsumallset2[0] + engsumallset2[1]) * 10)))
	{
		codectype = 1;
		*PCAGroupmode = 0;
	}
	else
	{
		codectype = 0;
		*PCAGroupmode = 1;
	}

	engLFEglobal = engLFEglobal * 0.95 + engsumallset2[3] * 0.05;
	if ((engsumallset2[3] > 1024 * 100))
		engLFEglobal = engsumallset2[3];
	if ((engLFEglobal > 100))
		codectype = codectype_pre;

	// codectype = codectype_pre;


	if ((engLFE > 0.05))
		codectype = codectype_pre;

	if ((engsumallset2[3] > 1024 * 100))
		codectype = codectype_pre;

	if (((engLFE > 0.05) || ((engsumallset2[3] * 3 / (engsumallset2[0] + engsumallset2[1] + engsumallset2[2] + 0.001)) > 0.3)) && (engsumallset2[3] > 1024 * 10))
		codectype = 1;

	if (engsumallset2[3] > 1024 * 100)
		codectype = codectype_pre;

	codectype_pre = codectype;
	//if(((lfEncset[4]->lf_winseq[0]==1)&&(lfEncset[4]->lf_winseq[1]==4)&&(lfEncset[4]->lf_winseq[2]==4)))
   //	 codectype=0;

	return codectype;
}
#endif