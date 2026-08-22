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

#ifdef WIN32
typedef __int64 INT64;
#else
#include <stdint.h>
typedef int64_t INT64;
#endif

#include <limits.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <assert.h>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "i2r_ec.h"
#include "i2r_lpc.h"
#include "ky_rice.h"
#include "ky_midside.h"
#include "i2r_decoder.h"

#define min(a, b)  (((a) < (b)) ? (a) : (b))
#define max(a, b)  (((a) > (b)) ? (a) : (b))

/* reconstruction levels for 1st and 2nd coefficients: */
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

static unsigned short RA_shift12[128] = {
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

static unsigned short RA_shift[65] = {
    0,	    1,	    6,	   13,	   23,	   36,	   52,	   71,
   93,	  118,	  146,	  177,	  211,	  249,	  290,	  334,
  381,	  432,	  487,	  545,	  607,	  673,	  743,	  817,
  896,	  978,	 1066,	 1158,	 1255,	 1358,	 1466,	 1580,
 1700,	 1826,	 1960,	 2100,	 2248,	 2404,	 2569,	 2743,
 2927,	 3122,	 3329,	 3548,	 3781,	 4030,	 4296,	 4580,
 4885,	 5214,	 5570,	 5956,	 6378,	 6841,	 7354,	 7927,
 8573,	 9313,	10176,	11205,	12476,	14128,	16477,	20526,
23147 };

Decoder_t *i2r_DecoderInit(int ChanNum, int SampFreq, int Res)
{
    Decoder_t * pDec;
    long i;

    pDec = (Decoder_t *)calloc(1, sizeof(Decoder_t));
    pDec->Chan = ChanNum;

    pDec->Freq = SampFreq;			/* sampling frequency*/
    pDec->Res = (Res + 1) << 3;		/* resolution			*/

    //shy 2011/06/23 Fix 
    //pDec->pcm_scale = (pDec->Res == 24)?256:1;
    pDec->pcm_scale = (pDec->Res == 24) ? 7 : ((pDec->Res == 16) ? 0 : -6);


    pDec->P = MAXODR;			/* max. pred. order */

    for (i = 0; i < ChanNum; i++) {
        pDec->x[i] = (long *)calloc(FRAME_LEN, sizeof(long));
    }

    for (i = 0; i < ChanNum; i++) {
        pDec->parRec[i] = (long *)calloc(MAXODR, sizeof(long));
    }
    pDec->cofQ = (long *)calloc(MAXODR, sizeof(long));     // Quantized coefficients

    pDec->cacd.ca = (long  *)calloc(FRAME_LEN, sizeof(long));
    pDec->cacd.cd = (long *)calloc(FRAME_LEN, sizeof(long));
    return(pDec);
}

void i2r_DecoderRelease(Decoder_t *pDec)
{
    long i;

    // Deallocate memory
    for (i = 0; i < pDec->Chan; i++)
        free(pDec->x[i]);

    free(pDec->cofQ);

    for (i = 0; i < pDec->Chan; i++) {
        free(pDec->parRec[i]);
    }
    free(pDec->cacd.ca);
    free(pDec->cacd.cd);
}
#ifdef LL_OPTIMIZATION
void MidSide2ChannelD1(const long *in0, const long *in1, long *out0, long *out1)
{
    int i;

    /* Other channels correlation process */
    for (i = 0; i < FRAME_LEN; i++)
    {
        long mid, side;

        side = in1[i];
        mid = (in0[i] << 1) | (side & 0x1);

        out0[i] = (mid + side) >> 1;
        out1[i] = (side - mid) >> 1;
    }
}


void MidSide2ChannelD2(const long *in0, const long *in1, long *out0, long *out1)
{
    int i;

    /* Other channels correlation process */
    for (i = 0; i < FRAME_LEN; i++)
    {
        long mid, side;

        side = in1[i];
        mid = (in0[i] << 1) | (side & 0x1);

        out0[i] = (mid + side) >> 1;
        out1[i] = (mid - side) >> 1;
    }
}
#endif

// Decode frame
void i2r_DecodeFrame(Decoder_t *pDec,
    int Chan, int ChanNum,
    long output_pcm[][FRAME_LEN])// Decode one frame
{
    int c, level, tmp;
#ifdef LL_OPTIMIZATION
    /*the initial value must be 2*/
    long corr = 2;
#else
    long corr = 0;
#endif


    /* Read channel pair correlation flag */
    if (ChanNum == 2) {
#ifdef LL_OPTIMIZATION
        corr = getbits(1);
        corr += getbits(1) << 1;
#else
        corr = getbits(1);
#endif
    }

    for (c = Chan; c < Chan + ChanNum; c++)
    {
        /* ZERO FRAME */
        if (getbits(1) == 0) {
            byteAlign();
            memset(pDec->x[c], 0, sizeof(long)*FRAME_LEN);
        }
        /* NORMAL FRAME */
        else
        {
            level = getbits(1);
            switch (level)
            {
            case 0:
                DecodeBlockParameter(pDec, c, FRAME_LEN);
                DecodeBlockReconstruct(pDec, c, pDec->x[c], FRAME_LEN);
                byteAlign();
                break;
            case 1:
                pDec->cacd.lenca = (FRAME_LEN + 1) / 2;
                pDec->cacd.lencd = FRAME_LEN / 2;
                switch (pDec->Res)
                {
                case 8:
                    tmp = getbits(8);
                    pDec->cacd.ca[0] = ((int)tmp << 24) >> 24;
                    break;
                case 16: pDec->cacd.ca[0] = (short)getbits(16); break;
                case 24:
                    tmp = getbits(24);
                    pDec->cacd.ca[0] = ((int)tmp << 8) >> 8;
                    break;
                case 32: pDec->cacd.ca[0] = (int)getbits(32); break;
                }
                DecodeBlockParameter(pDec, c, pDec->cacd.lenca - 2);
                DecodeBlockReconstruct(pDec, c, pDec->cacd.ca + 1, pDec->cacd.lenca - 2);
                switch (pDec->Res)
                {
                case 8:
                    tmp = getbits(8);
                    pDec->cacd.ca[pDec->cacd.lenca - 1] = ((int)tmp << 24) >> 24;
                    tmp = getbits(8);
                    pDec->cacd.cd[0] = ((int)tmp << 24) >> 24;
                    break;
                case 16:
                    pDec->cacd.ca[pDec->cacd.lenca - 1] = (short)getbits(16);
                    pDec->cacd.cd[0] = (short)getbits(16);
                    break;
                case 24:
                    tmp = getbits(24);
                    pDec->cacd.ca[pDec->cacd.lenca - 1] = ((int)tmp << 8) >> 8;
                    tmp = getbits(24);
                    pDec->cacd.cd[0] = ((int)tmp << 8) >> 8;
                    break;
                case 32:
                    pDec->cacd.ca[pDec->cacd.lenca - 1] = (int)getbits(32);
                    pDec->cacd.cd[0] = (int)getbits(32);
                    break;
                }
                DecodeBlockParameter(pDec, c, pDec->cacd.lencd - 2);
                DecodeBlockReconstruct(pDec, c, pDec->cacd.cd + 1, pDec->cacd.lencd - 2);
                switch (pDec->Res)
                {
                case 8:
                    tmp = getbits(8);
                    pDec->cacd.cd[pDec->cacd.lencd - 1] = ((int)tmp << 24) >> 24;
                    break;
                case 16: pDec->cacd.cd[pDec->cacd.lencd - 1] = (short)getbits(16); break;
                case 24:
                    tmp = getbits(24);
                    pDec->cacd.cd[pDec->cacd.lencd - 1] = ((int)tmp << 8) >> 8;
                    break;
                case 32: pDec->cacd.cd[pDec->cacd.lencd - 1] = (int)getbits(32); break;
                }
                byteAlign();

                ilwt(pDec->x[c], &pDec->cacd);
            }
        }
    }

    /* Channel de-correlation */
#ifdef LL_OPTIMIZATION
    if (0 == corr) {
        MidSide2ChannelD1(pDec->x[Chan],
            pDec->x[Chan + 1], output_pcm[Chan], output_pcm[Chan + 1]);
    }
    else if (1 == corr) {

        MidSide2ChannelD2(pDec->x[Chan],
            pDec->x[Chan + 1], output_pcm[Chan], output_pcm[Chan + 1]);
    }
#else

    if (1 == corr) {
        MidSide2Channel(pDec->x[Chan],
            pDec->x[Chan + 1], output_pcm[Chan], output_pcm[Chan + 1]);
    }
#endif 
    else {
        for (c = Chan; c < Chan + ChanNum; c++) {
            memcpy(output_pcm[c], pDec->x[c], FRAME_LEN * sizeof(long));
        }
    }
}

// Decode block parameter
void DecodeBlockParameter(Decoder_t *pDec,
    long Channel,
    long length)
{
    int  entropy;
    long *d = pDec->x[Channel];
    short *oP = &pDec->optPredOrd[Channel];

    short i, j, optP, s[8], sub;
    long parq[MAXODR], par_rec[MAXODR];

    short sbits = pDec->Res <= 16 ? 7 : 8;

    entropy = getbits(1);
    /* LPC parameters */
    {
        optP = getbits(7);  /* max P = 127 */

        /* decode quantized coefficients: */
        for (i = 0; i < optP; i++)
            parq[i] = getbits(7) - 64;

        /* reconstruct parcor coefficients: */
        if (optP > 0)
            par_rec[0] = pc12_tbl[parq[0] + 64];
        if (optP > 1)
            par_rec[1] = -pc12_tbl[parq[1] + 64];
        for (i = 2; i < optP; i++)
            par_rec[i] = (parq[i] << (LPC_Q - 6)) + (1 << (LPC_Q - 7));

        for (i = 0; i < optP; i++)
            pDec->parRec[Channel][i] = par_rec[i];

        *oP = optP;
    }

    if (1 == entropy)/* Golomb-rice decoding */
    {
        short minit, blksizebit;
        unsigned int ulMval;
        unsigned int ulRiceParam;
        unsigned int ulRiceSum;

        /* Block size */
        blksizebit = getbits(3) + 1;

        if (pDec->Res < 8) {
            minit = 3;
        }
        else if (pDec->Res < 16) {
            minit = 4;
        }
        else {
            minit = 5;
        };
        ulMval = getbits(minit);

        ulRiceParam = ulMval;
        ulRiceSum = ((1 << ulMval)*RICE_NUM_MUL);
        vGolombRiceDecode(length, min(optP, RA_LEN),
            pDec->Res, d, ulRiceSum, ulRiceParam, blksizebit);

        /* scale up residuals at frame beginning */
        {
            int shift[RA_LEN], tmp, j;

            /* compute scale-up levels */
            j = min(optP, RA_LEN);
            for (tmp = 0, i = j - 1; i >= 0; i--) {
                if (i < 2)	tmp += RA_shift12[parq[i] + 64];
                else		tmp += RA_shift[abs(parq[i])];
                shift[i] = (tmp + (1 << 12)) >> 13;
                shift[i] = min(shift[i], 30);
            }

            /* scale-up residuals */
            for (i = 0; i < j; i++) {
                if (shift[i]) {
                    d[i] <<= shift[i];
                    d[i] |= getbits(shift[i]);
                }
            }

            /* decode sign */
            for (i = 0; i < j; i++) {
                if (d[i]) {
                    if (getbits(1)) {
                        d[i] = -d[i];
                    }
                }
            }
        }
    }
    else {
        /* EC parameters */
        sub = 1 << getbits(2);
        s[0] = getbits(sbits);

        arith_decode_blocks(d, s, sub, length);
        /* scale up residuals at frame beginning */
        {
            int shift[RA_LEN], tmp;

            /* compute scale-up levels */
            j = min(optP, RA_LEN);
            for (tmp = 0, i = j - 1; i >= 0; i--) {
                if (i < 2)	tmp += RA_shift12[parq[i] + 64];
                else		tmp += RA_shift[abs(parq[i])];
                shift[i] = (tmp + (1 << 12)) >> 13;
                shift[i] = min(shift[i], 30);
            }

            /* scale-up residuals */
            for (i = 0; i < j; i++)
                if (shift[i]) {
                    d[i] <<= shift[i];
                    d[i] |= getbits(shift[i]);
                }

        }

        /* decode sign */
        for (i = 0; i < length; i++)
            if (d[i])
                if (getbits(1)) d[i] = -d[i];
    }
}

short DecodeBlockReconstruct(Decoder_t *pDec,
    long Channel,
    long *x,
    long length)
{
    long *par_rec = pDec->parRec[Channel];
    long *d = pDec->x[Channel];
    short optP = pDec->optPredOrd[Channel];

    GetSignal(x, length, optP, LPC_Q, par_rec, pDec->cofQ, d);

    return 0;
}