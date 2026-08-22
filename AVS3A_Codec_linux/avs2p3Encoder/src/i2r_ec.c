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

#include <math.h>

#define LN2 0.69314718055994529

/*
 * Compute mean index
 */
short GetMeanIndex(int *x, int N)
{
	short s;
	int n;
	double mean;

	/* compute mean */ 
	for (mean=0, n=0; n<N; n++) mean += labs(x[n]);
	mean /= N;

	/* compute log2(mean) */
	if (mean <= 1) s = 0;
	else s = (short)floor(8*log(mean)/LN2+0.5);

	return(s);
}

/*
 * write bits to output buffer
 */
static int buf_no;
static void write_bits(int bits, int num)
{
	EncodeWaveletPutBits(bits, num, buf_no);
}

/***************************** arithmetic coding probability table ***************************/
static unsigned short cp0 [6] = {65535,57537, 9615, 1208,  107,   10};

static unsigned short cp [448] = {
65535,65511,65473,65410,65373,65334,65327,65275,65190,65061,64960,64877,64757,64630,64507,64360,
64213,64046,63895,63742,63543,63295,63039,62822,62612,62362,62126,61871,61590,61324,61035,60741,
60459,60164,59833,59480,59115,58769,58480,58156,57761,57375,56987,56627,56264,55858,55444,55039,
54624,54228,53801,53353,52920,52486,52078,51634,51170,50719,50270,49804,49347,48886,48401,47936,
47488,46995,46495,45990,45506,45001,44512,44027,43521,43028,42550,42058,41547,41062,40574,40078,
39573,39081,38617,38131,37608,37103,36593,36087,35592,35117,34646,34181,33714,33230,32706,32202,
31717,31266,30807,30322,29854,29392,28936,28480,28012,27559,27107,26657,26207,25751,25322,24911,
24479,24033,23589,23169,22766,22365,21958,21545,21146,20762,20361,19959,19590,19238,18864,18484,
18123,17772,17431,17084,16723,16380,16045,15705,15388,15062,14730,14425,14130,13835,13533,13243,
12956,12663,12372,12107,11841,11564,11296,11033,10776,10518,10278,10050, 9813, 9585, 9354, 9123,
 8905, 8696, 8487, 8279, 8063, 7839, 7642, 7476, 7301, 7112, 6933, 6755, 6577, 6403, 6226, 6059,
 5905, 5761, 5611, 5457, 5310, 5163, 5022, 4885, 4753, 4627, 4498, 4373, 4251, 4128, 4009, 3900,
 3798, 3689, 3581, 3477, 3376, 3271, 3177, 3095, 3006, 2915, 2831, 2754, 2679, 2595, 2512, 2428,
 2351, 2287, 2223, 2154, 2086, 2020, 1954, 1889, 1833, 1778, 1723, 1673, 1625, 1570, 1514, 1468,
 1422, 1375, 1327, 1285, 1247, 1211, 1171, 1133, 1094, 1053, 1021,  993,  961,  926,  897,  867,
  836,  804,  772,  747,  727,  706,  684,  663,  643,  622,  600,  577,  555,  536,  518,  502,
  484,  465,  447,  432,  417,  404,  390,  377,  364,  352,  340,  329,  319,  308,  295,  285,
  277,  268,  258,  248,  239,  230,  222,  215,  209,  202,  193,  186,  181,  176,  170,  164,
  160,  153,  145,  142,  138,  133,  127,  123,  120,  116,  111,  107,  104,  100,   96,   94,
   92,   88,   85,   83,   82,   80,   77,   75,   73,   70,   66,   64,   63,   61,   59,   57,
   54,   53,   52,   51,   50,   49,   47,   45,   44,   42,   42,   40,   39,   37,   36,   35,
   34,   33,   32,   31,   30,   30,   29,   28,   27,   27,   26,   25,   25,   25,   25,   24,
   23,   22,   22,   21,   21,   20,   20,   19,   18,   18,   18,   17,   17,   16,   16,   16,
   16,   16,   16,   16,   15,   14,   14,   14,   14,   14,   13,   13,   12,   12,   12,   12,
   12,   12,   11,   11,   11,   11,   10,   10,   10,   10,   10,   10,    9,    9,    9,    9,
    9,    9,    9,    9,    9,    8,    8,    8,    8,    8,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,    5
};

static unsigned short inv_mean [48] = {
32768,30048,27554,25268,23170,21247,19484,17867,16384,15024,13777,12634,11585,10624, 9742, 8933,
 8192, 7512, 6889, 6317, 5793, 5312, 4871, 4467, 4096, 3756, 3444, 3158, 2896, 2656, 2435, 2233,
 2048, 1878, 1722, 1579, 1448, 1328, 1218, 1117, 1024,  939,  861,  790,  724,  664,  609,  558
};

static unsigned short index_tab[4] = {13, 16, 0, 19};

static unsigned int count[450];
static unsigned short max_sym;

/**** For arithmetic coding *****/
#define	Quarter		0x40000000
#define	Half		0x80000000

/* encoder states */
static unsigned int	ar_low;				/* lower bound */
static unsigned int	ar_range;			/* code range */
static unsigned int	ar_scale;			/* E3 scale count */

/*
 * initialize arithmetic encoder
 */
static void ar_start_encoding()
{
   ar_low = 0; ar_scale = 0;
   ar_range = 0xFFFFFFFF; 
}

/*
 * output 1 bit followed by a sequence of opposite bits
 */
//static void bit_plus_follow (unsigned int bit) 
//{
//	if (!ar_scale)
//		write_bits(bit, 1);
//	else {
//		register int temp = (1<<ar_scale) - (bit^1);
//		write_bits(temp, ar_scale+1);
//		ar_scale = 0;
//	}
//}
// Bugfix 20110623
static void bit_plus_follow (unsigned int bit) {
	write_bits(bit, 1);
	while (ar_scale) {
		write_bits(bit^1, 1); ar_scale--;
	}		
}

/* 
 * arithmetic encode one symbol
 */
static void ar_encode_symbol (unsigned short sym)
{
	register unsigned int low, range;	
	unsigned int step, temp;

	/* load encoder states */
	low = ar_low; range = ar_range;
	
	/* update states */
	step = range/count[0];					/* Calc range:freq ratio */
	temp = step*count[sym+1];				/* Calc low increment */
	low += temp;							/* Increase low */
	if (sym) range = step*(count[sym]-count[sym+1]);		/* Restrict range */		
	else	 range -= temp;					/* If at end of freq range */
											/* Give symbol excess code range */
	/* encode and re-normalize range */
	while (range <= Quarter)
	{ 		
		if (low >= Half) { bit_plus_follow(1); low -= Half; }
		else if ( low+range <= Half ) bit_plus_follow(0);
		else { ar_scale++; low -= Quarter; }
    	low <<= 1; range <<= 1;
    }

	/* store encoder states */
	ar_low = low; ar_range = range;

}

/* 
 * terminate arithmetic encoder
 */
static void ar_finish_encoding ()
{
	ar_low >>= 29;
	ar_low += 1;
	bit_plus_follow(ar_low>>2);
	write_bits(ar_low&3, 2);
}

/*
 * calculate probability table for arithmetic coding of mean indexes
 */
void cal_index_prob(int sub)
{
	int i, temp;

	/* set coding parameter*/
	temp = index_tab[(sub>>1)-1];

	/* compute max symbol value */
	max_sym = 228864/inv_mean[temp];

	/* interpolate probability density table */	
	for (count[0]=cp[0], i=1; i<=max_sym; i++)
		count[i] = cp[(i*inv_mean[temp]+256)>>9];

	/* generate cumulative probabilty table */	
	for (count[max_sym+1]=0, i=max_sym; i>=0; i--)
		count[i] += count[i+1];

}

/*
 * arithmetic encode mean indexes
 */
void ar_encode_index(short *s, int sub)
{
	int i, x;

	/* compute probability table */
	cal_index_prob(sub);

	/* encoding loop */
	for (i=1; i<sub; i++)
	{
		/* compute difference */
		if (i < 2)	x = s[i] - s[i-1];
		else		x = s[i] - ((3*s[i-1]+s[i-2]+1) >> 2);

		/* embed sign */
		if (x < 0)	x = -2*x - 1;
		else		x *= 2;

		/* encode a symbol  */
		for ( ; x >= max_sym; x -= max_sym)
			ar_encode_symbol (max_sym);
		ar_encode_symbol (x);
	}

}

/*
 * calculate probability table for arithmetic coding of MSB symbols
 */
int cal_prob(int s)
{
	int i, k;

	/* check MSB position for number of LSBs to shift-down */
	k = (s>>3) - 5;
	if (k <= 0)	k = 0;
	else		s -= (k<<3);

	if (s > 0)
	{	/* normal mode */		
		/* compute max symbol value */
		max_sym = 228864/inv_mean[s];		

		/* interpolate probability density table */
		count[0] = cp[0] - ((cp[0])>>(k+1));
		for (i=1; i<=max_sym; i++)
			count[i] = cp[(i*inv_mean[s]+256)>>9];
	}
	else
	{	/* low energy mode */		
		max_sym = 5;
		for (i=0; i<=max_sym; i++) count[i] = cp0[i];
	}

	/* generate cumulative probabilty table */	
	for (count[max_sym+1]=0, i=max_sym; i>=0; i--)
		count[i] += count[i+1];

	return(k);
	
}

/* 
 * arithmetic encode one block of samples
 */
void arith_encode_blocks (int *blocks, short *s, int sub, int block_len, int bufno)
{

	register int i, j, x;
	register int *block;	
	int temp, N[8], k[8];

	buf_no = bufno;
	/* compute sub-block length */
	temp = block_len/sub;
	for (i=0; i<sub-1; i++)	N[i] = temp;
	N[sub-1] = block_len - (sub-1)*temp;

	/* start arithmetic encoder */
    ar_start_encoding();

	/* encode mean index */
	if (sub > 1)
		ar_encode_index(s, sub);

	/* encode MSBs */
	block = blocks;
    for (j=0; j<sub; j++)
	{
		/* compute probability table */
		if (j==0)
			k[j] = cal_prob(s[j]);
		else if (s[j]==s[j-1]) 
			k[j] = k[j-1];
		else
			k[j] = cal_prob(s[j]);	

		/* encoding loop */
        for (i=0; i<N[j]; i++)
		{
			/* encode MSB symbol  */
			for (x = labs(block[i])>>k[j]; x >= max_sym; x -= max_sym)
				ar_encode_symbol (max_sym);
			ar_encode_symbol (x);
		}

		/* next sub-block */
        block += N[j];
	}
	
	/* terminate arithmetic encoder */
    ar_finish_encoding();

	/* encode LSBs */
    block = blocks;
    for (j=0; j<sub; j++)
	{
		/* encoding loop */
		if (k[j])
			for (i=0; i<N[j]; i++)
				write_bits(labs(block[i]), k[j]);

		/* next sub-block */
        block += N[j];
	}

}