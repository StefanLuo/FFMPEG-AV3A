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

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "i2r_lpc.h"
#include "i2r_ec.h"
#include "ky_rice.h"
#include "ky_midside.h"
#include "i2r_encoder.h"

/* reconstruction levels for 1st and 2nd coefficients: */
//1.5	
static int pc12_tbl[128] = {
-1048549, 	-1048440, 	-1048220, 	-1047887, 	-1047439, 	-1046874, 	-1046189, 	-1045382,
-1044452, 	-1043395, 	-1042209, 	-1040893, 	-1039443, 	-1037857, 	-1036133, 	-1034269,
-1032260, 	-1030106, 	-1027803, 	-1025349, 	-1022741, 	-1019976, 	-1017052, 	-1013965,
-1010713, 	-1007293, 	-1003702, 	-999937, 	-995995, 	-991873, 	-987567, 	-983075,
-978394, 	-973519, 	-968448, 	-963178, 	-957705, 	-952025, 	-946135, 	-940032,
-933712, 	-927171, 	-920405, 	-913412, 	-906186, 	-898724, 	-891023, 	-883078, 
-874885, 	-866440, 	-857739, 	-848778, 	-839552, 	-830058, 	-820290, 	-810245,
-799917, 	-789303, 	-778397, 	-767195, 	-755692, 	-743883, 	-731764, 	-719329,
-706573, 	-693492, 	-680079, 	-666330, 	-652240, 	-637803, 	-623013, 	-607865, 
-592353, 	-576472, 	-560216, 	-543578, 	-526553, 	-509136, 	-491319, 	-473096,
-454462, 	-435410, 	-415933, 	-396024, 	-375678, 	-354887, 	-333645, 	-311944, 
-289778, 	-267139, 	-244021, 	-220415, 	-196315, 	-171712, 	-146600, 	-120970, 
-94815, 	-68126, 	-40896, 	-13117, 	15221,		44125,		73603, 		103665,
134320, 	165575, 	197441, 	229925, 	263038, 	296789, 	331188, 	366243, 
401965, 	438364, 	475450, 	513232, 	551721, 	590929, 	630864, 	671538,
712962, 	755147, 	798104, 	841845, 	886380, 	931722, 	977882, 	1024872
}; 

static unsigned short RA_shift12 [128] = {
58348,	48794,	43108,	39207,	36249,	33866,	31870,	30151,	
28643,	27298,	26083,	24977,	23959,	23018,	22141,	21321,	
20551,	19824,	19136,	18483,	17862,	17269,	16702,	16159,	
15638,	15136,	14654,	14189,	13740,	13305,	12885,	12479,	
12084,	11702,	11330,	10969,	10618,	10277,	 9944,	 9620,	
 9305,	 8997,	 8697,	 8404,	 8118,	 7839,	 7566,	 7300,	
 7039,	 6785,	 6536,	 6293,	 6055,	 5822,	 5594,	 5372,	
 5154,	 4941,	 4733,	 4529,	 4330,	 4135,	 3944,	 3758,	
 3577,	 3399,	 3226,	 3056,	 2891,	 2730,	 2573,	 2420,	
 2271,	 2127,	 1986,	 1849,	 1717,	 1589,	 1465,	 1345,	
 1229,	 1118,	 1012,	  909,	  812,	  719,	  631,	  548,	
  469,	  397,	  329,	  267,	  211,	  161,	  117,	   79,	
   49,	   25,	    9,	    1,	    1,	   10,	   29,	   58,	
   98,	  149,	  213,	  291,	  384,	  493,	  621,	  769,	
  939,	 1135,	 1360,	 1618,	 1915,	 2258,	 2655,	 3119,	
 3667,	 4320,	 5117,	 6113,	 7409,	 9209,	12043,	18365,
};

static unsigned short RA_shift [65] = {
    0,	    1,	    6,	   13,	   23,	   36,	   52,	   71,	
   93,	  118,	  146,	  177,	  211,	  249,	  290,	  334,	
  381,	  432,	  487,	  545,	  607,	  673,	  743,	  817,	
  896,	  978,	 1066,	 1158,	 1255,	 1358,	 1466,	 1580,	
 1700,	 1826,	 1960,	 2100,	 2248,	 2404,	 2569,	 2743,	
 2927,	 3122,	 3329,	 3548,	 3781,	 4030,	 4296,	 4580,	
 4885,	 5214,	 5570,	 5956,	 6378,	 6841,	 7354,	 7927,	
 8573,	 9313,	10176,	11205,	12476,	14128,	16477,	20526,	
23147};

Encoder_t *i2r_EncoderInit(int ChanNum, int SampFreq, int Res, int maxLpcOrder)
{
	Encoder_t * pEnc;
	long i;

	pEnc = (Encoder_t *)calloc(1,sizeof(Encoder_t));
	pEnc->Chan = ChanNum;

	pEnc->P    = maxLpcOrder;
	pEnc->Res  = (Res+1)<<3;	

	//shy 2011/06/23 
    //pEnc->pcm_scale = (pEnc->Res == 24)?256:1; 
	pEnc->pcm_scale = (pEnc->Res == 24)?7:((pEnc->Res == 16)?0:-6); 
	pEnc->Freq = SampFreq;	

	for (i = 0; i < ChanNum; i++){
		pEnc->x[i]  = (long *)calloc(FRAME_LEN, sizeof(long));
		memset(pEnc->x[i], 0, sizeof(long)*(FRAME_LEN));
	}

	pEnc->par = (double *)calloc(pEnc->P,sizeof(double)); 
	pEnc->cof = (long *)calloc(pEnc->P,sizeof(long)); 

	for( i = 0; i < ChanNum;  i++){
		pEnc->residual[i] = (long *)calloc(FRAME_LEN + 1,sizeof(long)); 
		pEnc->parQ[i] = (long *)calloc(MAXODR,sizeof(long)); 
	}
	
	pEnc->wBuf[0] = (int *)calloc(FRAME_LEN, sizeof(int));
	pEnc->wBuf[1] = (int *)calloc(FRAME_LEN, sizeof(int));
	pEnc->cacd.ca = pEnc->wBuf[0];//(int *)calloc(FRAME_LEN, sizeof(int));
	pEnc->cacd.cd = pEnc->wBuf[1];//(int*)calloc(FRAME_LEN,  sizeof(int));

	return(pEnc);
}


void i2r_EncoderRelease(Encoder_t *pEnc)
{
	int i;

	// Deallocate memory
	for (i = 0; i < pEnc->Chan; i++)
		free(pEnc->x[i]); // delete [] x[i];

	free(pEnc->par); //delete [] par;
	free(pEnc->cof); //delete [] cof;

	for( i = 0; i < pEnc->Chan; i++ ) {
		// Allocate long buffers
		free(pEnc->residual[i]); 
		free(pEnc->parQ[i]); 
	}
	
	free(pEnc->cacd.ca);
	free(pEnc->cacd.cd);
}

#ifdef LL_OPTIMIZATION
long CalcCorr(long *in0, long *in1, double *corrvalue)
{
    int i;
    double escape = 0.000001;

    double divssum = 0;
    double ex0 = 0, ex1 = 0;
    double mx0 = 0, mx1 = 0;

    /* Calculating the mean value */
    for (i = 0; i < FRAME_LEN; i++) {
        mx0 = mx0 + in0[i];
        mx1 = mx1 + in1[i];
    }
    mx0 = mx0 / FRAME_LEN;
    mx1 = mx1 / FRAME_LEN;

    /* Calculating the square mean value */
    for (i = 0; i < FRAME_LEN; i++)
    {
        divssum = divssum + (double)(in0[i] - mx0)*(in1[i] - mx1);
        ex0 = ex0 + (double)(in0[i] - mx0)*(in0[i] - mx0);
        ex1 = ex1 + (double)(in1[i] - mx1)*(in1[i] - mx1);
    }
    ex0 = sqrt(ex0);
    ex1 = sqrt(ex1);

    /* Calculating the correlation coefficient */
    *corrvalue = divssum / (ex0*ex1 + escape);
}

long MidSide2Channel1(long *in0, long *in1, long *out0, long *out1)
{
    int i;

    for (i = 0; i < FRAME_LEN; i++)
    {
        long left, right;

        left = in0[i];
        right = in1[i];

        out0[i] = (left - right) >> 1;
        out1[i] = (left + right);
    }
}

long MidSide2Channel2(long *in0, long *in1, long *out0, long *out1)
{
    int i;
    for (i = 0; i < FRAME_LEN; i++)
    {
        long left, right;

        left = in0[i];
        right = in1[i];

        out0[i] = (left + right) >> 1;
        out1[i] = (left - right);
    }
}
#endif

void i2r_EncodeFrame(Encoder_t *pEnc,
    int nCh,
    int nChanNum)
{

#ifdef LL_OPTIMIZATION

    long c;
    long corr1 = 0;
    long corr2 = 1;
    double corrvalue1 = 0.f;
    double corrvalue2 = 0.f;
    double corrvalue3 = 0.f;
    int i;
    static int corrflag = 2;
    static int corrflag3 = 0;
    double delta = 0.05f;
    static double Thres1 = -0.47f;
    static double Thres2 = 0.47f;
    double Thres0 = 0.47f;

#else
    long c, corr = 0;
#endif
    EncodeWaveletStart(0);
    EncodeWaveletStart(1);

    start_outputing_bits();
    /* Channel correlation process */
    if (nChanNum == 2)
    {
        if (pEnc->msenc)
#ifdef LL_OPTIMIZATION
        {/* De-correlation mode selection */
            CalcCorr(pEnc->x[nCh],
                pEnc->x[nCh + 1], &corrvalue1);

            if (corrflag == 0)
            {
                Thres1 = -Thres0 + delta;
                Thres2 = Thres0;
            }
            else if (corrflag == 1)
            {
                Thres2 = Thres0 - delta;
                Thres1 = Thres0;
            }
            else if (corrflag == 2)
            {
                if (corrflag3 == 0)
                {
                    Thres1 = -Thres0 - delta;
                    Thres2 = Thres0;
                }
                else if (corrflag3 == 1)
                {
                    Thres2 = Thres0 + delta;
                    Thres1 = Thres0;
                }
                else
                {
                    Thres1 = -Thres0;
                    Thres2 = Thres0;
                }

            }

            if (corrvalue1 < Thres1)
            {
                MidSide2Channel1(&pEnc->x[nCh][0],
                    &pEnc->x[nCh + 1][0], pEnc->wBuf[0],
                    pEnc->wBuf[1]);

                CalcCorr(pEnc->wBuf[0],
                    pEnc->wBuf[1], &corrvalue2);
                if (corrvalue1 < corrvalue2)
                {
                    memcpy(pEnc->x[nCh], pEnc->wBuf[0], FRAME_LEN * sizeof(long));
                    memcpy(pEnc->x[nCh + 1], pEnc->wBuf[1], FRAME_LEN * sizeof(long));
                    corr1 = 0;
                    corr2 = 0;
                }
                else
                {
                    corr1 = 0;
                    corr2 = 1;
                    corrflag3 = 0;
                }
            }
            else if (corrvalue1 > Thres2)
            {
                MidSide2Channel2(pEnc->x[nCh],
                    pEnc->x[nCh + 1], pEnc->wBuf[0],
                    pEnc->wBuf[1]);

                CalcCorr(pEnc->wBuf[0],
                    pEnc->wBuf[1], &corrvalue3);

                if (corrvalue1 > corrvalue3)
                {
                    memcpy(pEnc->x[nCh], pEnc->wBuf[0], FRAME_LEN * sizeof(long));
                    memcpy(pEnc->x[nCh + 1], pEnc->wBuf[1], FRAME_LEN * sizeof(long));
                    corr1 = 1;
                    corr2 = 0;
                }
                else
                {
                    corr1 = 0;
                    corr2 = 1;
                    corrflag3 = 1;
                }
            }
            else
            {
                corr1 = 0;
                corr2 = 1;
                corrflag3 = 2;
            }
            corrflag = corr2 << 1 | corr1;
        }

#else
        {/* De-correlation decision */
            corr = MidSide2Channel(pEnc->x[nCh],
                pEnc->x[nCh + 1], pEnc->wBuf[0],
                pEnc->wBuf[1], nCh + 1);
            UpdateThres(corr, &Thres[nCh + 1]);
        }
#endif
    }

    {
        // Save original pointers
        if (nChanNum == 2) {
            /* Channel pair correlation flag */
#ifdef LL_OPTIMIZATION
            //write the flag into bitstream
            EncodeWaveletPutBits(corr1, 1, 0);
            EncodeWaveletPutBits(corr1, 1, 1);
            EncodeWaveletPutBits(corr2, 1, 0);
            EncodeWaveletPutBits(corr2, 1, 1);
#else
            EncodeWaveletPutBits(corr, 1, 0);
            EncodeWaveletPutBits(corr, 1, 1);
#endif
        }

        // Channels ///////////////////////////////////////////////////////////////////////////////////
        for (c = nCh; c < nCh + nChanNum; c++)
        {
            // ZERO BLOCK
            if (BlockIsZero(pEnc->x[c], FRAME_LEN))
            {
                EncodeWaveletPutBits(0, 1, 0);
                EncodeWaveletByteAlign(0);
                EncodeWaveletFlush(0);

                EncodeWaveletStart(0);
                EncodeWaveletStart(1);
            }
            else
            {// NORMAL BLOCK
                long tmplength0, tmplength1;
                if (pEnc->wavelet)
                {
                    int flag, level;

                    /* Block header + level*/
                    // level 0
                    EncodeWaveletPutBits(2, 2, 0);
                    // level 1
                    EncodeWaveletPutBits(3, 2, 1);

                    // wavelet transform
                    flag = lwt(pEnc->x[c], &pEnc->cacd, FRAME_LEN, pEnc->Res);

                    // level 0
                    EncodeBlockAnalysis(pEnc, c, pEnc->x[c], FRAME_LEN);
                    EncodeBlockCoding(pEnc, c, pEnc->residual[c], FRAME_LEN, 0);
                    EncodeWaveletByteAlign(0);
                    tmplength0 = EncodeWaveletGetSize(0);
                    level = 0;

                    if (!flag)
                    {
                        // level 1: ca
                        EncodeWaveletPutBits(pEnc->cacd.ca[0], pEnc->Res, 1);
                        EncodeBlockAnalysis(pEnc, c, pEnc->cacd.ca + 1, pEnc->cacd.lenca - 2);
                        EncodeBlockCoding(pEnc, c, pEnc->residual[c], pEnc->cacd.lenca - 2, 1);
                        EncodeWaveletPutBits(pEnc->cacd.ca[pEnc->cacd.lenca - 1], pEnc->Res, 1);

                        // level1: cd
                        EncodeWaveletPutBits(pEnc->cacd.cd[0], pEnc->Res, 1);
                        EncodeBlockAnalysis(pEnc, c, pEnc->cacd.cd + 1, pEnc->cacd.lencd - 2);
                        EncodeBlockCoding(pEnc, c, pEnc->residual[c], pEnc->cacd.lencd - 2, 1);
                        EncodeWaveletPutBits(pEnc->cacd.cd[pEnc->cacd.lencd - 1], pEnc->Res, 1);
                        EncodeWaveletByteAlign(1);
                        tmplength1 = EncodeWaveletGetSize(1);

                        if (tmplength1 < tmplength0) {
                            level = 1;
                        }
                    }
                    // copy final bitstream to OutputBuffer
                    EncodeWaveletFlush(level);

                    // refresh the wavelet buffers
                    EncodeWaveletStart(0);
                    EncodeWaveletStart(1);
                }
                else {
                    /* Block header + level*/
                    // level 0
                    EncodeWaveletPutBits(2, 2, 0);

                    // level 0
                    EncodeBlockAnalysis(pEnc, c, pEnc->x[c], FRAME_LEN);
                    EncodeBlockCoding(pEnc, c, pEnc->residual[c], FRAME_LEN, 0);
                    EncodeWaveletByteAlign(0);
                    tmplength0 = EncodeWaveletGetSize(0);

                    // copy final bitstream to OutputBuffer
                    EncodeWaveletFlush(0);

                    // refresh the wavelet buffers
                    EncodeWaveletStart(0);
                    EncodeWaveletStart(1);
                }
            }
        }
        // End of Channels ////////////////////////////////////////////////////////////////////////////		
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Encode a single block analysis
void EncodeBlockAnalysis(Encoder_t *pEnc, long Channel, long *x
						,long length)
{
	long  *d  = pEnc->residual[Channel];
	short *oP = &pEnc->optPredOrd[Channel];

	short optP, overflow=1;
	int i, lpc_order;

	{
		long parq[MAXODR], parRec[MAXODR];
		short Pmax = pEnc->P;
		int a;
		double q;

		optP      = pEnc->P;
		lpc_order = pEnc->P;

		while (overflow){
		optP = lpc_order;
		GetCof(x, 
			length, 
			optP, 0, pEnc->par
			,&lpc_order
			);

		optP = lpc_order;
		q = PI / 256;	// Quantizer step size

		// Quantization of the coefficients

		/* first coefficient: */
		a = (int) floor((log(1/1.5 + sqrt(pEnc->par[0]+1.0)*(1.5 - 1/1.5)/sqrt(2.0))/log(1.5)) * 64);

		if (a > 63) a = 63; else if (a < -64) a = -64;
		parq[0] = a;
		parRec[0] = pc12_tbl[a + 64];

		/* second coefficient: */
		if ( optP > 1) {
			a = (int) floor((log(1/1.5 + sqrt(-pEnc->par[1]+1.0)*(1.5 - 1/1.5)/sqrt(2.0))/log(1.5)) * 64);
			if (a > 63) a = 63; else if (a < -64) a = -64;
			parq[1] = a;
			parRec[1] = -pc12_tbl[a + 64];
		}

		/* the remaining coeffs: */
		for (i=2; i<optP; i++) {
			a = (int) floor(pEnc->par[i]*64);
			if (a > 63) a = 63; else if (a < -64) a = -64;
			parq[i] = a;
			parRec[i] = (a << (LPC_Q-6)) + (1 << (LPC_Q-7));
		}

		/* compute prediction residual */
		overflow = GetResidual(x,
			length, 
			optP, LPC_Q, parRec, pEnc->cof, d);

		lpc_order >>= 1;
		}

		for(i=0; i<optP; i++)
			pEnc->parQ[Channel][i] = parq[i];
		
		*oP=optP;
	}
}

//int ccc_blknum = 0;
/* Arithmetic encoder block */
void vArithEncBlock(Encoder_t *pEnc,
					int Channel,
					int *d,
					int length,
					int bufno,
					int blksizebit)
{
	short i, j, s[8], num, sub, Ns;
	
	int *parq  = pEnc->parQ[Channel];
	short optP = pEnc->optPredOrd[Channel];
	
	short sbits= pEnc->Res <= 16 ? 7 : 8;
	short m    = (1<<sbits) - 1;
	
	/* Confirm the number of EC sub block */
	sub		   = 4;  //1, 2, 4, 8
	blksizebit = 0;

	//for testing, shumin.xu 20211105
	/*while(1)
	{
		sub = pow(2, (ccc_blknum%4));
		ccc_blknum += 1;
		break;
	}*/

	/* LPC coef */
	EncodeWaveletPutBits(optP, 7, bufno);
	for (j = 0; j < optP; j++)
		EncodeWaveletPutBits((parq[j]+64)&0x7F, 7, bufno);
	
	/* Sub block number */
	if (sub == 8){/* 8 -> 11 */
		i = 3;
	}else{/* 4 -> 10, 2 -> 01, 1 -> 00 */
		i = sub >> 1;
	}
	EncodeWaveletPutBits(i, 2, bufno);	

	/* compute mean index */
	Ns   = length / sub;
	num  = min(optP, 3);
	s[0] = min(GetMeanIndex(d+num, Ns-num), m);
	for (j=1; j<(sub-1); j++)
		s[j] = min(GetMeanIndex(d+j*Ns, Ns), m);
	s[j] = min(GetMeanIndex(d+j*Ns, length-Ns*j), m);

	/* code mean index of the first EC block */
	EncodeWaveletPutBits((s[0]&m), sbits, bufno);

	/* prediction residual */
	{
		int mask, tmp;
		int d_save[RA_LEN], shift[RA_LEN];
		
		j = min(optP, RA_LEN);
		
		for (tmp=0, i=j-1; i>=0; i--) {
			if (i < 2)	tmp += RA_shift12[parq[i] + 64];
			else		tmp += RA_shift[abs(parq[i])];
			shift[i] = (tmp + (1<<12)) >> 13;
			shift[i] = min(shift[i], 30);
		}

		/* shift down starting residuals */			
		for (i=0; i<j; i++) {
			d_save[i] = d[i];
			d[i] = labs(d[i]) >> shift[i];
		}
		arith_encode_blocks(d, s, sub, length, bufno);

		/* encode shifted bits in starting residuals */			
		for (i=0; i<j; i++) {
			d[i] = d_save[i];
			mask = (1 << shift[i]) - 1;
			if (shift[i]){
				EncodeWaveletPutBits((labs(d[i]))&mask, shift[i], bufno);
			}
		}
	}

	/* encode sign */
	for (i=0; i<length; i++){
		if (d[i] > 0) EncodeWaveletPutBits(0, 1, bufno); 		/* 0: positive */
		else if (d[i] < 0) EncodeWaveletPutBits(1, 1, bufno);	/* 1: negative */
	}
}

/* Adaptive Golomb-rice encoder block */
void vAdpGbREncBlock(Encoder_t *pEnc,
					 int Channel,
					 int *d,
					 int length,
					 int bufno,
					 int blksizebit)
{

	short i, minit, block;
	int mask, tmp, j;
	unsigned int ulMval;
	unsigned int ulRiceSum;
	unsigned int ulRiceParam;
	int d_save[RA_LEN], shift[RA_LEN];
	
	int *parq = pEnc->parQ[Channel];
	short optP= pEnc->optPredOrd[Channel];
	
	double mean = 0;
	if(pEnc->Res < 8){
		minit   = 3;
	}else if(pEnc->Res < 16){
		minit   = 4;
	}else{
		minit   = 5;
	};

	/* LPC coef */
	EncodeWaveletPutBits(optP, 7, bufno);
	for (i = 0; i < optP; i++)
		EncodeWaveletPutBits((parq[i]+64)&0x7F, 7, bufno);

	/* Block size(1 << blksizebit) */
	EncodeWaveletPutBits(blksizebit-1, 3, bufno);

	/* prediction residual */
	j = min(optP, RA_LEN);
	
	for (tmp=0, i=j-1; i>=0; i--) {
		if (i < 2)	tmp += RA_shift12[parq[i] + 64];
		else		tmp += RA_shift[abs(parq[i])];
		shift[i] = (tmp + (1<<12)) >> 13;
		shift[i] = min(shift[i], 30);
	}

	/* shift down starting residuals */			
	for (i=0; i<j; i++) {
		d_save[i] = d[i];
		d[i] = labs(d[i]) >> shift[i];
	}
	for (i = 0;  i < j; i++){
		mean  += d[i];
	}
	mean	   = mean / j;

	if (mean <= 1){
		ulMval = 0;
	}else {
		ulMval = (unsigned int)floor(log(mean)/LN2+0.5);
	}		
	EncodeWaveletPutBits(ulMval, minit, bufno);

	ulRiceParam= ulMval;
	ulRiceSum  = ((1<<ulMval)*RICE_NUM_MUL);
	vGolombRiceEncode(d, length, pEnc->Res,
		j, ulRiceSum, ulRiceParam, bufno, blksizebit);

	/* encode shifted bits in starting residuals */			
	for (i=0; i<j; i++) {
		d[i] = d_save[i];
		mask = (1 << shift[i]) - 1;
		if (shift[i]){
			EncodeWaveletPutBits((labs(d[i]))&mask, shift[i], bufno);
		}
	}

	/* encode sign */
	for (i=0; i<j; i++){
		if (d[i] > 0) EncodeWaveletPutBits(0, 1, bufno); 		/* 0: positive */
		else if (d[i] < 0) EncodeWaveletPutBits(1, 1, bufno);	/* 1: negative */
	}
}

/* Initialize function pointer array */
static pFuncEntropy vFuncEntropy[2]={
	vArithEncBlock, vAdpGbREncBlock
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Encode a single block coding
//int ccc_blksize = 1;
void EncodeBlockCoding(Encoder_t *pEnc, long Channel, long *d
					  ,long length, int bufno)
{	
	short blksizebit=0, blksize = 2; // 2, 4, 8, 16, 32
	int nEntropy  = pEnc->ucEntropy;
	int *parq = pEnc->parQ[Channel];
	short optP= pEnc->optPredOrd[Channel];
	
	//for testing, shumin.xu 20211105
	/*while(1)
	{
		if (ccc_blksize % 5 == 0) ccc_blksize = 1;
		blksize = pow(2, (ccc_blksize%5));
		ccc_blksize += 1;
		break;
	}*/

	if (2 == pEnc->ucEntropy){
		if (optP <= 16){
			nEntropy = 0;
		}else{
			nEntropy = 1;
		}
	}

	/* Setting block size of Golomb-rice coding */
	if(1 == nEntropy){
		if(2 == blksize){
			blksizebit = 1;
		}else if(4  == blksize){
			blksizebit = 2;
		}else if(8  == blksize){
			blksizebit = 3;
		}else if(16 == blksize){
			blksizebit = 4;
		}else if(32 == blksize){
			blksizebit = 5;
		}
	}

	/* Encode ENTROPYTYPE */
	EncodeWaveletPutBits(nEntropy, 1, bufno);

	/* Entropy encoding process */
	(*vFuncEntropy[nEntropy])(pEnc,
		Channel, d, length, bufno, blksizebit);
}

