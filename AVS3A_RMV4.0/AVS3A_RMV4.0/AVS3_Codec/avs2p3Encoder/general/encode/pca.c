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

/*********************************/
/* Principal Components Analysis */
/*********************************/

/*********************************************************************/
/* Principal Components Analysis or the Karhunen-Loeve expansion is a
   classical method for dimensionality reduction or exploratory data
   analysis.  One reference among many is: F. Murtagh and A. Heck,
   Multivariate Data Analysis, Kluwer Academic, Dordrecht, 1987 
   (hardbound, paperback and accompanying diskette).

   This program is public-domain.  If of importance, please reference 
   the author.  Please also send comments of any kind to the author:

   F. Murtagh
   Schlossgartenweg 1          or        35 St. Helen's Road
   D-8045 Ismaning                       Booterstown, Co. Dublin
   W. Germany                            Ireland

   Phone:        + 49 89 32006298 (work)
                 + 49 89 965307 (home)
   Telex:        528 282 22 eo d
   Fax:          + 49 89 3202362
   Earn/Bitnet:  fionn@dgaeso51,  fim@dgaipp1s,  murtagh@stsci
   Span:         esomc1::fionn
   Internet:     murtagh@scivax.stsci.edu
   

   A Fortran version of this program is also available.     
    
   F. Murtagh, Munich, 6 June 1989                                   */   
/*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "mc_rom.h"

#define EVENODD 1
#define PCAQUANT4 8
#define MULBANDSTART 14
#define FRAME_LEN_LONG 1024

#define SIGN(a, b) ( (b) < 0 ? -fabs(a) : fabs(a) )

void corcol(float **data, int n, int m,  float**symmat);
void covcol(float **data, int n, int m, float **symmat);
void scpcol(float **data, int n, int m, float **symmat);
float *vector(int n);
float **matrix(int n, int m);

void free_vector(float *v,int n);
void free_matrix(float **mat,int n,int m);
void tred2(float **a, int n,float *d, float *e);
void tqli(float d[], float e[], int n, float **z);
extern int flooroffset_A;

int PCA_analysis(float **data2,float **data,int  n, int  m,float **corrMatrix,float*evalsset ,float **anaMatrix,int flag);

//static int LL[5]={4096/8/2, 4096/4/2, 4096/2/2, 4096/2,4096/16/2};

//2014.11.13  wchg 修改划分表，以利于编码方式的切换
short longsubbandoffset[40 + 1] = {
	   0,   8,   16, 24, 32,  40,  48, 56,
	   64,  72,  80, 88, 96, 104, 112, 120,
	  128, 138, 148, 158, 168, 180, 192, 206,
	 220, 236, 252, 270, 290, 314, 344, 380,
	 420, 464, 512, 568, 636, 712, 800, 908,
	1024
};//USE_MDCT
short mid4subbandoffset[24 + 1] = {
	   0,   8,   16,  24,
	   32,  40,  48,  60,
	   72,  84,  96,  112,
	   128, 148, 168, 192,
	   216, 240, 268, 296,
	   328, 360, 400, 448,
	   512
};
short mid2subbandoffset[12 + 1] = {
		0,   8,   16,  24,
		36,  48,  64,  84,
		108, 134, 168, 208,
		256
};
short shortsubbandoffset[9 + 1] = {
		 0, 8, 16, 24, 32, 44, 56, 72, 96, 128 };

int bandnumset[5]={40,24,12,9};//{16,12,12,8,12};
//int bandnumset_L[5]={14,7,7,4,7};


float anaMatrixdata[3][8][50][2][8*8];
unsigned int allbandsetflag[3][8][50];

int bandmulflag[3][8];
int pre_bandmulflag[3][8]={{1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1}};
int bandmulavg[3]={0,0,0};


int usePCAitemnum_Optimization(int nChannelsInEl, int bitRate, int nChannels, int *usePCAitemnum)
{
	if (nChannels == 2)
	{
		if (bitRate < 80000)
			*usePCAitemnum = 1;
		else
			*usePCAitemnum = 2;

	}
	if (nChannels == 6)
	{
		if (bitRate <= 192000)
		{
			switch (nChannelsInEl) {
			case 2:
				*usePCAitemnum = 1;
				break;
			case 4:
				*usePCAitemnum = 2;
				break;
			case 6:
				*usePCAitemnum = 3;
				break;
			default:
				printf("error: unsuport\n");
				return 1;
			}
			flooroffset_A = 8;
		}
		else if (bitRate <= 256000)
		{
			switch (nChannelsInEl) {
			case 2:
				*usePCAitemnum = 1;
				break;
			case 4:
				*usePCAitemnum = 3;
				break;
			case 6:
				*usePCAitemnum = 3;
				break;
			default:
				printf("error: unsuport\n");
				return 1;
			}
			flooroffset_A = 7;
		}
		else if (bitRate <= 320000)
		{
			switch (nChannelsInEl) {
			case 2:
				*usePCAitemnum = 2;
				break;
			case 4:
				*usePCAitemnum = 3;
				break;
			case 6:
				*usePCAitemnum = 4;
				break;
			default:
				printf("error: unsuport\n");
				return 1;
			}
			flooroffset_A = 5;
		}
		else if (bitRate <= 384000)
		{
			switch (nChannelsInEl) {
			case 2:
				*usePCAitemnum = 2;
				break;
			case 4:
				*usePCAitemnum = 3;
				break;
			case 6:
				*usePCAitemnum = 4;
				break;
			default:
				printf("error: unsuport\n");
				return 1;
			}
			flooroffset_A = 4;
		}
		else
		{
			switch (nChannelsInEl) {
			case 2:
				*usePCAitemnum = 2;
				break;
			case 4:
				*usePCAitemnum = 3;
				break;
			case 6:
				*usePCAitemnum = 4;
				break;
			default:
				printf("error: unsuport\n");
				return 1;
			}
			flooroffset_A = 2;
		}
	}
	else if (nChannels == 8)
	{
		if (bitRate >= 480000)
		{
			switch (nChannelsInEl) {
			case 2:
				*usePCAitemnum = 2;
				break;
			case 4:
				*usePCAitemnum = 3;
				break;
			case 6:
				*usePCAitemnum = 4;
				break;
			default:
				printf("error: unsuport\n");
				return 1;
			}
			flooroffset_A = 4;
		}
	}
	return 0;
}

int multichannelMDCT_PCA_1(float Mdctin[][FRAME_LEN_LONG * 2], int *Swinseq, int channelnum, float Mdctout[][FRAME_LEN_LONG * 2], int elementindex, float *ratiotmp, double *engallout, double engsumallset[])
{
	int mdctoffset;
	int tt, jj, ll, kk;
	int index;
	int m = channelnum;
	int bandnum;
	double engall00, engelement2, engallsetall, engallset[6], engelement1set[6];

	int bandnumall;
	engall00 = 0;
	engelement2 = 0;
	engallsetall = 0;

	for (tt = 0; tt < 6; tt++)
	{
		engallset[tt] = 0;
		engelement1set[tt] = 0;
		engsumallset[tt] = 0;
	}

	tt = Swinseq[1];
	jj = Swinseq[1 + 1];

	if (LL[tt - 1] > LL[jj - 1])
		ll = LL[tt - 1] / 2;
	else
		ll = LL[jj - 1] / 2;


	mdctoffset = 0;
	*ratiotmp = 0;
	bandnumall = 0;

	for (index = 1; index < (Swinseq[0] + 1); index++)
	{
		tt = Swinseq[index];
		jj = Swinseq[index + 1];

		if (LL[tt - 1] > LL[jj - 1])
			ll = LL[tt - 1] / 2;
		else
			ll = LL[jj - 1] / 2;



		switch (ll * 2) {
		case 4096:
			// mdft4096(St, mdft4096_Sr, mdft4096_Si);

			break;
		case 2048:
			bandnum = bandnumset[0];
			// mdft2048(St, mdft4096_Sr, mdft4096_Si);



			for (kk = 1; kk < bandnum; kk += 1)
			{
				int n = (longsubbandoffset[kk + 1] - longsubbandoffset[kk]);
				int i, j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset;
				float **anaMatrix;
				data = matrix(n, m);  /* Storage allocation for input data */

				data2 = matrix(n, m);  /* Storage allocation for input data */
				corrMatrix = matrix(m, m);
				evalsset = vector(m);
				anaMatrix = matrix(m, m);

#if EVENODD
				n = (longsubbandoffset[kk + 1] - longsubbandoffset[kk]) / 2;
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][2 * i + mdctoffset + longsubbandoffset[kk] - 2];
						data2[i][j] = Mdctin[j - 1][2 * i + mdctoffset + longsubbandoffset[kk] - 2];

						engall00 += (data[i][j] * data[i][j]);

						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}




				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);



				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][2 * i + mdctoffset + longsubbandoffset[kk] - 2] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}

				////////////////////////////////////////////////////////////////////////
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][2 * i + 1 + mdctoffset + longsubbandoffset[kk] - 2];
						data2[i][j] = Mdctin[j - 1][2 * i + 1 + mdctoffset + longsubbandoffset[kk] - 2];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}




				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][2 * i + 1 + mdctoffset + longsubbandoffset[kk] - 2] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}

#else

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][i + mdctoffset + longsubbandoffset[kk] - 1];
						data2[i][j] = Mdctin[j - 1][i + mdctoffset + longsubbandoffset[kk] - 1];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}




				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);



				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][i + mdctoffset + longsubbandoffset[kk] - 1] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}
#endif

				free_matrix(corrMatrix, m, m);
				free_matrix(anaMatrix, m, m);
				free_vector(evalsset, m);

				free_matrix(data, n, m);
				free_matrix(data2, n, m);

				{
					engall00 = engallset[1];
					if (engall00 < engallset[2])
						engall00 = engallset[2];
					engall00 *= 2;

					if ((engelement1set[1] + 0.0000001) / (engallset[1] + 0.0000001) > (engelement1set[2] + 0.0000001) / (engallset[2] + 0.0000001))
						*ratiotmp += ((engelement1set[2] + 0.0000001) / (engallset[2] + 0.0000001)) / 1;
					else
						*ratiotmp += ((engelement1set[1] + 0.0000001) / (engallset[1] + 0.0000001)) / 1;

					bandnumall++;
					engelement2 = 0;
					engall00 = 0;
					engallset[1] = 0;
					engallset[2] = 0;
					engelement1set[1] = 0;
					engelement1set[2] = 0;
				}
			}
			break;
		case 1024:
			bandnum = bandnumset[1];
			// mdft1024(St, mdft4096_Sr, mdft4096_Si);

			for (kk = 1; kk < bandnum; kk++)
			{
				int n = mid4subbandoffset[kk + 1] - mid4subbandoffset[kk];
				int i, j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset;
				float **anaMatrix;

				data = matrix(n, m);  /* Storage allocation for input data */
				data2 = matrix(n, m);  /* Storage allocation for input data */
				corrMatrix = matrix(m, m);
				evalsset = vector(m);
				anaMatrix = matrix(m, m);

#if EVENODD
				n = (mid4subbandoffset[kk + 1] - mid4subbandoffset[kk]) / 2;
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][2 * i + mdctoffset + mid4subbandoffset[kk] - 2];
						data2[i][j] = Mdctin[j - 1][2 * i + mdctoffset + mid4subbandoffset[kk] - 2];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}


				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);

				/*for (i = 1; i <= m; i++)
				{
						for (j = 1; j <= m; j++)
					anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}*/

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][2 * i + mdctoffset + mid4subbandoffset[kk] - 2] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);

					}
				}

				////////////////////////////////////////////////////////
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][2 * i + 1 + mdctoffset + mid4subbandoffset[kk] - 2];
						data2[i][j] = Mdctin[j - 1][2 * i + 1 + mdctoffset + mid4subbandoffset[kk] - 2];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}


				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);



				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][2 * i + 1 + mdctoffset + mid4subbandoffset[kk] - 2] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}
#else
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][i + mdctoffset + mid4subbandoffset[kk] - 1];
						data2[i][j] = Mdctin[j - 1][i + mdctoffset + mid4subbandoffset[kk] - 1];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}


				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][i + mdctoffset + mid4subbandoffset[kk] - 1] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}
#endif
				free_matrix(corrMatrix, m, m);
				free_matrix(anaMatrix, m, m);
				free_vector(evalsset, m);

				free_matrix(data, n, m);
				free_matrix(data2, n, m);

				{
					engall00 = engallset[1];
					if (engall00 < engallset[2])
						engall00 = engallset[2];
					engall00 *= 2;

					if ((engelement1set[1] + 0.0000001) / (engallset[1] + 0.0000001) > (engelement1set[2] + 0.0000001) / (engallset[2] + 0.0000001))
						*ratiotmp += ((engelement1set[2] + 0.0000001) / (engallset[2] + 0.0000001)) / 1;
					else
						*ratiotmp += ((engelement1set[1] + 0.0000001) / (engallset[1] + 0.0000001)) / 1;

					bandnumall++;
					engelement2 = 0;
					engall00 = 0;
					engallset[1] = 0;
					engallset[2] = 0;
					engelement1set[1] = 0;
					engelement1set[2] = 0;
				}
			}
			break;
		case 512:
			bandnum = bandnumset[2];
			//   mdft512(St, mdft4096_Sr, mdft4096_Si);


			for (kk = 1; kk < bandnum; kk++)
			{
				int n = mid2subbandoffset[kk + 1] - mid2subbandoffset[kk];
				int i, j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset;
				float **anaMatrix;

				data = matrix(n, m);  /* Storage allocation for input data */
				data2 = matrix(n, m);  /* Storage allocation for input data */
				corrMatrix = matrix(m, m);
				evalsset = vector(m);
				anaMatrix = matrix(m, m);

#if EVENODD
				n = (mid2subbandoffset[kk + 1] - mid2subbandoffset[kk]) / 2;
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][2 * i + mdctoffset + mid2subbandoffset[kk] - 2];
						data2[i][j] = Mdctin[j - 1][2 * i + mdctoffset + mid2subbandoffset[kk] - 2];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}



				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);



				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][2 * i + mdctoffset + mid2subbandoffset[kk] - 2] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][2 * i + 1 + mdctoffset + mid2subbandoffset[kk] - 2];
						data2[i][j] = Mdctin[j - 1][2 * i + 1 + mdctoffset + mid2subbandoffset[kk] - 2];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}



				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);



				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][2 * i + 1 + mdctoffset + mid2subbandoffset[kk] - 2] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}

#else
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][i + mdctoffset + mid2subbandoffset[kk] - 1];
						data2[i][j] = Mdctin[j - 1][i + mdctoffset + mid2subbandoffset[kk] - 1];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}



				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);



				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][i + mdctoffset + mid2subbandoffset[kk] - 1] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}
#endif
				free_matrix(corrMatrix, m, m);
				free_matrix(anaMatrix, m, m);
				free_vector(evalsset, m);

				free_matrix(data, n, m);
				free_matrix(data2, n, m);

				{
					engall00 = engallset[1];
					if (engall00 < engallset[2])
						engall00 = engallset[2];
					engall00 *= 2;

					if ((engelement1set[1] + 0.0000001) / (engallset[1] + 0.0000001) > (engelement1set[2] + 0.0000001) / (engallset[2] + 0.0000001))
						*ratiotmp += ((engelement1set[2] + 0.0000001) / (engallset[2] + 0.0000001)) / 1;
					else
						*ratiotmp += ((engelement1set[1] + 0.0000001) / (engallset[1] + 0.0000001)) / 1;

					bandnumall++;
					engelement2 = 0;
					engall00 = 0;
					engallset[1] = 0;
					engallset[2] = 0;
					engelement1set[1] = 0;
					engelement1set[2] = 0;
				}

			}
			break;
		case 256:
			bandnum = bandnumset[3];
			//  mdft256(St, mdft4096_Sr, mdft4096_Si);


			for (kk = 1; kk < bandnum; kk++)
			{
				int n = shortsubbandoffset[kk + 1] - shortsubbandoffset[kk];

				int i, j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset;
				float **anaMatrix;

				data = matrix(n, m);  /* Storage allocation for input data */

				data2 = matrix(n, m);  /* Storage allocation for input data */

				corrMatrix = matrix(m, m);
				evalsset = vector(m);
				anaMatrix = matrix(m, m);
#if EVENODD
				n = (shortsubbandoffset[kk + 1] - shortsubbandoffset[kk]) / 2;

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][2 * i + mdctoffset + shortsubbandoffset[kk] - 2];
						data2[i][j] = Mdctin[j - 1][2 * i + mdctoffset + shortsubbandoffset[kk] - 2];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}



				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][2 * i + mdctoffset + shortsubbandoffset[kk] - 2] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}


				/////////////////////	/////////////////////	/////////////////////
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][2 * i + 1 + mdctoffset + shortsubbandoffset[kk] - 2];
						data2[i][j] = Mdctin[j - 1][2 * i + 1 + mdctoffset + shortsubbandoffset[kk] - 2];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}



				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][2 * i + 1 + mdctoffset + shortsubbandoffset[kk] - 2] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}

#else
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j - 1][i + mdctoffset + shortsubbandoffset[kk] - 1];
						data2[i][j] = Mdctin[j - 1][i + mdctoffset + shortsubbandoffset[kk] - 1];
						engall00 += (data[i][j] * data[i][j]);
						engallset[j] += (data[i][j] * data[i][j]);
						engsumallset[j] += (data[i][j] * data[i][j]);
						engallsetall += (data[i][j] * data[i][j]);
					}
				}



				PCA_analysis(data, data2, n, m, corrMatrix, evalsset, anaMatrix, 1);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
						//	fscanf(stream, "%f	", &in_value);
						Mdctout[j - 1][i + mdctoffset + shortsubbandoffset[kk] - 1] = data[i][j];
						//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						if (j == 1)
							engelement2 += (data[i][j] * data[i][j]);
						engelement1set[j] += (data2[i][j] * data2[i][j]);
					}
				}
#endif
				free_matrix(corrMatrix, m, m);
				free_matrix(anaMatrix, m, m);
				free_vector(evalsset, m);

				free_matrix(data, n, m);
				free_matrix(data2, n, m);

				{
					engall00 = engallset[1];
					if (engall00 < engallset[2])
						engall00 = engallset[2];
					engall00 *= 2;

					if ((engelement1set[1] + 0.0000001) / (engallset[1] + 0.0000001) > (engelement1set[2] + 0.0000001) / (engallset[2] + 0.0000001))
						*ratiotmp += ((engelement1set[2] + 0.0000001) / (engallset[2] + 0.0000001)) / 1;
					else
						*ratiotmp += ((engelement1set[1] + 0.0000001) / (engallset[1] + 0.0000001)) / 1;

					bandnumall++;
					engelement2 = 0;
					engall00 = 0;
					engallset[1] = 0;
					engallset[2] = 0;
					engelement1set[1] = 0;
					engelement1set[2] = 0;
				}

			}
			break;
		case 128:
			//  mdft128(St, mdft4096_Sr, mdft4096_Si);

			break;
		}
		for (kk = 0; kk < ll; kk++)
		{
			//	Mdftout[mdftoffset+kk*2]=mdft4096_Sr[kk];
			//	Mdftout[mdftoffset+kk*2+1]=mdft4096_Si[kk];
		}

		mdctoffset += (ll * 1);

	}
	/*if(engall00>0)
		*ratiotmp = engelement2/engall00;
	else
		*ratiotmp =1;
		*/

	*ratiotmp /= bandnumall;
	*engallout = engallsetall / 10000;

	return 0;
}

int multichannelMDCT_PCAElementana(float Mdctin[][FRAME_LEN_LONG * 2], int *Swinseq, int channelnum, int elementindex) {
	int mdctoffset;
	int tt, jj, ll, kk;
	int index;
	int m = channelnum;
	int bandnum;
	int i, j, k;
	double engelement[6][2];
	int imflag[60][6];
	int nChannelsInEl = channelnum;
	double engtmp[60][6], engtmp2[60][6];
	tt = Swinseq[1];
	jj = Swinseq[1 + 1];

	if (LL[tt - 1] > LL[jj - 1])
		ll = LL[tt - 1] / 2;
	else
		ll = LL[jj - 1] / 2;



	mdctoffset = 0;

	for (index = 1; index < (Swinseq[0] + 1); index++)
	{
		tt = Swinseq[index];
		jj = Swinseq[index + 1];

		if (LL[tt - 1] > LL[jj - 1])
			ll = LL[tt - 1] / 2;
		else
			ll = LL[jj - 1] / 2;



		switch (ll * 2) {
		case 4096:
			// mdft4096(St, mdft4096_Sr, mdft4096_Si);

			break;
		case 2048:

			bandnum = bandnumset[0];
			for (kk = 0; kk < bandnum; kk++)
			{
				int n = (longsubbandoffset[kk + 1] - longsubbandoffset[kk]);




				for (j = 0; j < nChannelsInEl; j++)
				{
					engelement[j][0] = 0;
					engelement[j][1] = 0;
					for (i = 0; i < n / 2; i++)
					{
						engelement[j][0] += (Mdctin[j][2 * i + mdctoffset + longsubbandoffset[kk]])*(Mdctin[j][2 * i + mdctoffset + longsubbandoffset[kk]]);
						engelement[j][1] += (Mdctin[j][2 * i + 1 + mdctoffset + longsubbandoffset[kk]])*(Mdctin[j][2 * i + 1 + mdctoffset + longsubbandoffset[kk]]);
					}
				}


				for (j = 0; j < nChannelsInEl; j++)
					imflag[kk][j] = 0;

				for (k = 0; k < nChannelsInEl; k++)
				{//double engtmp=0;


					engtmp[kk][k] = 0;
					for (j = 0; j < nChannelsInEl; j++)
					{
						engtmp[kk][k] += (engelement[j][0] * pow(anaMatrixdata[elementindex][index - 1][kk][0][(j)*nChannelsInEl + k], 2));
						engtmp[kk][k] += (engelement[j][1] * pow(anaMatrixdata[elementindex][index - 1][kk][1][(j)*nChannelsInEl + k], 2));

					}


					for (j = 0; j < nChannelsInEl; j++)
					{
						if ((engelement[j][0] * pow(anaMatrixdata[elementindex][index - 1][kk][0][(j)*nChannelsInEl + k], 2)) +
							(engelement[j][1] * pow(anaMatrixdata[elementindex][index - 1][kk][1][(j)*nChannelsInEl + k], 2)) > engtmp[kk][k] * 0.3 + 10000)
						{

							imflag[kk][j] = 1;
						}
					}

				}
			}//for(kk=2;kk<bandnum;kk++)


			for (k = 0; k < nChannelsInEl; k++)
			{
				for (kk = 0; kk < bandnum; kk++)
				{
					engtmp2[kk][k] = engtmp[kk][k];
				}
			}
			for (k = 0; k < nChannelsInEl; k++)
			{
				for (kk = 0; kk < bandnum; kk++)
				{
					if ((kk > 1) && (kk < bandnum - 1))
						engtmp[kk][k] = engtmp2[kk][k] * 0.5 + engtmp2[kk - 1][k] * 0.25 + engtmp2[kk + 1][k] * 0.25;
				}
			}

			for (kk = 0; kk < bandnum; kk++)
			{
				int n = (longsubbandoffset[kk + 1] - longsubbandoffset[kk]);


				for (j = 0; j < nChannelsInEl; j++)
				{
					engelement[j][0] = 0;
					engelement[j][1] = 0;
					for (i = 0; i < n / 2; i++)
					{
						engelement[j][0] += (Mdctin[j][2 * i + mdctoffset + longsubbandoffset[kk]])*(Mdctin[j][2 * i + mdctoffset + longsubbandoffset[kk]]);
						engelement[j][1] += (Mdctin[j][2 * i + 1 + mdctoffset + longsubbandoffset[kk]])*(Mdctin[j][2 * i + 1 + mdctoffset + longsubbandoffset[kk]]);
					}
				}




				for (j = 0; j < nChannelsInEl; j++)
					imflag[kk][j] = 0;

				for (k = 0; k < nChannelsInEl; k++)
				{//double engtmp=0;

					for (j = 0; j < nChannelsInEl; j++)
					{
						if ((engelement[j][0] * pow(anaMatrixdata[elementindex][index - 1][kk][0][(j)*nChannelsInEl + k], 2)) +
							(engelement[j][1] * pow(anaMatrixdata[elementindex][index - 1][kk][1][(j)*nChannelsInEl + k], 2)) > engtmp[kk][k] * 1.2 + 10000)
						{

							imflag[kk][j] = 1;
						}
					}

				}
			}//for(kk=2;kk<bandnum;kk++)

			break;
		}

		/*{FILE *fp_out = fopen("imflag.txt","a");


			for(j=0;j<nChannelsInEl;j++)
			{

				fprintf(fp_out,"\n");
				for(kk=0;kk<bandnum;kk++)
					fprintf(fp_out,"%d	",imflag[kk][j]);
				fprintf(fp_out,"\n");
			}

			fprintf(fp_out,"-------------------\n");
			fclose(fp_out);

		}*/
		mdctoffset += (ll * 1);

	}//for(index=1;index<(Swinseq[0]+1);index++)

	return 0;
}

int multichannelMDCT_PCA_2(float Mdctin[][FRAME_LEN_LONG * 2], int *Swinseq,int channelnum,float Mdctout[][FRAME_LEN_LONG * 2],int elementindex)
{
int mdctoffset;
int tt,jj,ll,kk;
int index;
int m=channelnum;
int bandnum;
	tt=Swinseq[1];
	jj=Swinseq[1+1];
       
	 if(LL[tt-1]>LL[jj-1])
		ll=LL[tt-1]/2;
	 else
		ll=LL[jj-1]/2;



	  mdctoffset=0;

	for(index=1;index<(Swinseq[0]+1);index++)
	{
		tt=Swinseq[index];
		jj=Swinseq[index+1];
    
	   if(LL[tt-1]>LL[jj-1])
		ll=LL[tt-1]/2;
	   else
		ll=LL[jj-1]/2;



		switch(ll*2) {
        case 4096 :
        // mdft4096(St, mdft4096_Sr, mdft4096_Si);
		
          break;
        case 2048 :
			
         // mdft2048(St, mdft4096_Sr, mdft4096_Si);

			{
		
				float engbandset[50];
				float maxeng;
				int i,j;
				int peakcount;
				
				bandnum=bandnumset[0];

				peakcount=0;
				for(kk=0;kk<bandnum;kk++)
				{
					int n=(longsubbandoffset[kk+1]-longsubbandoffset[kk]);
					engbandset[kk]=0;
					for (i = 1; i <= n; i++)
					{
					
						for (j = 1; j <= m; j++)
						{
					
						engbandset[kk] += (Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1]*Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1]);
						
						}
					 }
					engbandset[kk] =engbandset[kk]/n;

				}
				maxeng=0.0001;
				for(kk=0;kk<bandnum;kk++)
				{
					if(engbandset[kk]>maxeng)
					{
						maxeng = engbandset[kk]+0.0001;
					}
				}

				allbandsetflag[elementindex][index-1][0]=1;
				allbandsetflag[elementindex][index-1][1]=1;
				for(kk=2;kk<bandnum;kk++)
				{int startindex,endindex;
				float bandmaxeng;
			
					startindex= kk-4;
					endindex = kk+4;

					if((kk>6)&&(kk<=12))
					{startindex= kk-3;
					endindex = kk+3;

					}
						if((kk>14))
					{startindex= kk-2;
					endindex = kk+2;

					}
					if(startindex<=0)
						startindex=0;
					if(endindex>=bandnum)
						endindex=bandnum-1;
					bandmaxeng=0;
					for(j=startindex;j<=endindex;j++)
					{
						if(engbandset[j]>bandmaxeng)
						{
						bandmaxeng = engbandset[j]+0.0001;
						}
					}

					
					if((engbandset[kk]<bandmaxeng/(kk>=14? 200:1000))||(engbandset[kk]<maxeng/100000))
					{
						allbandsetflag[elementindex][index-1][kk]=0;
						if(kk>18)
							allbandsetflag[elementindex][index-1][kk]=1;
					}else
					{
						allbandsetflag[elementindex][index-1][kk]=1;
					}

					
					
					if(kk>12){int n=(longsubbandoffset[kk+1]-longsubbandoffset[kk])/8;
					float engtmp[64];
					 int indext;
					 int maxengtmp,maxindextmp,minengtmp;
					
						for (indext = 0; indext < n; indext++)
						{engtmp[indext]=0;
								for (j = 1; j <= m; j++)
									for(i=1;i<=8;i++)
									{
								
									engtmp[indext] += (Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1+indext*8]*Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1+indext*8]);
									
									}
						}//for

						maxengtmp=0;
						maxindextmp=0;
						minengtmp=100000000000000;
						for (indext = 0; indext < n; indext++)
						{
							if(engtmp[indext]>maxengtmp)
							{
								maxindextmp = indext;
								maxengtmp = engtmp[indext];
							}
							if(engtmp[indext]<minengtmp)
							{
								//maxindextmp = indext;
								minengtmp = engtmp[indext];
							}
						}
						
						if((engbandset[kk]/100000>30)&&(engbandset[kk]*n*8<maxengtmp*2))
						{
							if((engbandset[kk]*n*8<maxengtmp*3))
							{
								peakcount++;
							}else
						if(maxindextmp>3)
						{
							if((engbandset[kk]/100000>10)&&(engtmp[indext]>200*engtmp[indext-1])&&(engtmp[indext]>1000*engtmp[indext-2])&&(engtmp[indext]>1000*engtmp[indext-3]))
								peakcount++;
							else if((engbandset[kk]/100000>1000)&&(engtmp[indext]>1000*engtmp[indext-2]))
							{
								peakcount++;
							}
						}
							else if(maxindextmp<n-3)
							{
								if((engbandset[kk]/100000>10)&&(engtmp[indext]>200*engtmp[indext+1])&&(engtmp[indext]>1000*engtmp[indext+2])&&(engtmp[indext]>1000*engtmp[indext+3]))
								peakcount++;
								else if((engbandset[kk]/100000>1000)&&(engtmp[indext]>1000*engtmp[indext+2]))
								{
									peakcount++;
								}
							}
							
						}
						

					}
				

				

				bandmulavg[elementindex] =bandmulavg[elementindex] *0.8 +peakcount;
				if((maxeng/100000<1000))
					bandmulavg[elementindex] =0;
				if((peakcount>=5)&&(maxeng/100000>1000))
					bandmulavg[elementindex] =peakcount;
				if(pre_bandmulflag[elementindex][index-1]==1)
				{
					if((bandmulavg[elementindex]<=2)||(maxeng/100000<1000))
					{
						bandmulflag[elementindex][index-1]=2;
						for(kk=0;kk<bandnum;kk++)
							allbandsetflag[elementindex][index-1][kk]=1;
					}
					else
						bandmulflag[elementindex][index-1]=1;
				}else
				{
						if((bandmulavg[elementindex]<=4)||(maxeng/100000<1000))
					{
						bandmulflag[elementindex][index-1]=2;
						for(kk=0;kk<bandnum;kk++)
							allbandsetflag[elementindex][index-1][kk]=1;
					}
					else
						bandmulflag[elementindex][index-1]=1;
				}

				bandmulflag[elementindex][index-1]=1;

				pre_bandmulflag[elementindex][index-1] = bandmulflag[elementindex][index-1];

			

			}

			
	for(kk=0;kk<bandnum;kk+=((kk>=MULBANDSTART) ? bandmulflag[elementindex][index-1]:1))
			{int n=(longsubbandoffset[kk+((kk>=MULBANDSTART) ? bandmulflag[elementindex][index-1]:1)]-longsubbandoffset[kk]);
				int i,j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset ;
				float **anaMatrix;
			 
				data = matrix(n, m);  /* Storage allocation for input data */

				data2 = matrix(n, m);  /* Storage allocation for input data */
				corrMatrix =matrix(m, m);
				evalsset =vector(m);  
				anaMatrix=matrix(m, m);

#if EVENODD
				n=(longsubbandoffset[kk+((kk>=MULBANDSTART) ? bandmulflag[elementindex][index-1]:1)]-longsubbandoffset[kk])/2;
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
				
						data[i][j] = Mdctin[j-1][2*i+mdctoffset+longsubbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+mdctoffset+longsubbandoffset[kk]-2];
					}
				}

				

				if(allbandsetflag[elementindex][index-1][kk]==1)
					PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				else
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
							anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk-((kk>MULBANDSTART) ? bandmulflag[elementindex][index-1]:1)][0][(i-1)*m+j-1];
					}

					PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);
				}

				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
					anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrix[j][i];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][2*i+mdctoffset+longsubbandoffset[kk]-2]=data[i][j];
					
						}
				}*/

////////////////////////////////////////////////////////////////////////
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][2*i+1+mdctoffset+longsubbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+1+mdctoffset+longsubbandoffset[kk]-2];
						}
				}

				

				if(allbandsetflag[elementindex][index-1][kk]==1)
					PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				else
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
							anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk-((kk>MULBANDSTART) ? bandmulflag[elementindex][index-1]:1)][1][(i-1)*m+j-1];
					}

					PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);
				}
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][2*i+1+mdctoffset+longsubbandoffset[kk]-2]=data[i][j];
					
						}
				}*/

#else

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
					
						data[i][j] = Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1];
						data2[i][j] = Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1];
					}
				}

				

				if(allbandsetflag[elementindex][index-1][kk]==1)
					PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				else
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
							anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk-((kk>MULBANDSTART) ? bandmulflag[elementindex][index-1]:1)][0][(i-1)*m+j-1];
					}

					PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);
				}
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrix[j][i];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][i+mdctoffset+longsubbandoffset[kk]-1]=data[i][j];
					
						}
				}*/
#endif

			free_matrix(corrMatrix, m, m);
			free_matrix(anaMatrix, m, m);
			free_vector(evalsset, m);

			free_matrix(data, n, m);
			free_matrix(data2, n, m);
			}

			/**/
			
			for(kk=2;kk<=4;kk+=2)
			{
				if(engbandset[kk]>4*engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1];
						
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}

				if(engbandset[kk]<1.0/4*engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1];
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}

			}//for(kk=0;kk<=8;kk+=2)

		

			for(kk=6;kk<=14;kk+=2)
			{
				if(engbandset[kk]>engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1];
						
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}

				if(engbandset[kk]<engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1];
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}

			}//for(kk=10;kk<=24;kk+=2)


			for(kk=16;kk<=bandnum;kk+=4)
			{int maxindextmp=kk;
			 int tmp26;
			 float maxengtmp= engbandset[kk];
			 for(tmp26=kk;tmp26<(kk+4>bandnum?bandnum:kk+4);tmp26++)
				{
					if(maxengtmp<engbandset[tmp26])
					{
						maxindextmp = tmp26;
						maxengtmp = engbandset[tmp26];
					}
				}
				allbandsetflag[elementindex][index-1][kk]=1;
				for(tmp26=kk;tmp26<(kk+4>bandnum?bandnum:kk+4);tmp26++)
				{
				if(tmp26!=maxindextmp)
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][tmp26][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][maxindextmp][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][tmp26][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][maxindextmp][1][(i-1)*m+j-1];
						
						}
					}

				
					//allbandsetflag[elementindex][index-1][tmp26]=0;
				}
					allbandsetflag[elementindex][index-1][tmp26]=0;
				}
				allbandsetflag[elementindex][index-1][kk]=1;

				

			}//for(kk=16;kk<=bandnum;kk+=4)

				for(kk=0;kk<bandnum;kk+=((kk>=MULBANDSTART) ? bandmulflag[elementindex][index-1]:1))
			{int n=(longsubbandoffset[kk+((kk>=MULBANDSTART) ? bandmulflag[elementindex][index-1]:1)]-longsubbandoffset[kk]);
				int i,j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset ;
				float **anaMatrix;
			 
				data = matrix(n, m);  /* Storage allocation for input data */

				data2 = matrix(n, m);  /* Storage allocation for input data */
				corrMatrix =matrix(m, m);
				evalsset =vector(m);  
				anaMatrix=matrix(m, m);

#if EVENODD
				n=(longsubbandoffset[kk+((kk>=MULBANDSTART) ? bandmulflag[elementindex][index-1]:1)]-longsubbandoffset[kk])/2;
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
				
						data[i][j] = Mdctin[j-1][2*i+mdctoffset+longsubbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+mdctoffset+longsubbandoffset[kk]-2];
					}
				}

				

				
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
							anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1];
					}

					PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);
			

				

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][2*i+mdctoffset+longsubbandoffset[kk]-2]=data[i][j];
					
						}
				}

////////////////////////////////////////////////////////////////////////
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][2*i+1+mdctoffset+longsubbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+1+mdctoffset+longsubbandoffset[kk]-2];
						}
				}

				

			
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
							anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1];
					}

					PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);
				}
			

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][2*i+1+mdctoffset+longsubbandoffset[kk]-2]=data[i][j];
					
						}
				}

#else

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
					{
					
						data[i][j] = Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1];
						data2[i][j] = Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1];
					}
				}

				

				
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
							anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1];
					}

					PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);
				}
			

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][i+mdctoffset+longsubbandoffset[kk]-1]=data[i][j];
					
						}
				}
#endif

			free_matrix(corrMatrix, m, m);
			free_matrix(anaMatrix, m, m);
			free_vector(evalsset, m);

			free_matrix(data, n, m);
			free_matrix(data2, n, m);
			}

			}
          break;
		case 1024 :
			
			{
			float engbandset[50];
			float maxeng;
			int i,j;
			bandnum=bandnumset[1];

			bandmulflag[elementindex][index-1]=1;

			for(kk=0;kk<bandnum;kk++)
			{int n=(longsubbandoffset[kk+1]-longsubbandoffset[kk]);
				engbandset[kk]=0;
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						engbandset[kk] += (Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1]*Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1]);
						
						}
				}
				engbandset[kk] =engbandset[kk]/n;
			}
			maxeng=0.0001;
			for(kk=0;kk<bandnum;kk++)
			{
					if(engbandset[kk]>maxeng)
					{
						maxeng = engbandset[kk]+0.0001;
					}
			}

			allbandsetflag[elementindex][index-1][0]=1;
			allbandsetflag[elementindex][index-1][1]=1;
			for(kk=2;kk<bandnum;kk++)
			{int startindex,endindex;
				float bandmaxeng;
			
					startindex= kk-3;
					endindex = kk+3;
					if(startindex<=0)
						startindex=0;
					if(endindex>=bandnum)
						endindex=bandnum-1;
					bandmaxeng=0;
					for(j=startindex;j<=endindex;j++)
					{
						if(engbandset[j]>bandmaxeng)
						{
						bandmaxeng = engbandset[j]+0.0001;
						}
					}

					if((engbandset[kk]<bandmaxeng/200)||(engbandset[kk]<maxeng/100000))
					{
						allbandsetflag[elementindex][index-1][kk]=0;
					}else
					{
						allbandsetflag[elementindex][index-1][kk]=1;
					}

					allbandsetflag[elementindex][index-1][kk]=1;
			}

			
			for(kk=0;kk<bandnum;kk++)
			{int n=mid4subbandoffset[kk+1]-mid4subbandoffset[kk];
				int i,j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset ;
				float **anaMatrix;
			 
				data = matrix(n, m);  /* Storage allocation for input data */
				data2 = matrix(n, m);  /* Storage allocation for input data */
				corrMatrix =matrix(m, m);
				evalsset =vector(m);  
				anaMatrix=matrix(m, m);

#if EVENODD
				n=(mid4subbandoffset[kk+1]-mid4subbandoffset[kk])/2;
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
						data[i][j] = Mdctin[j-1][2*i+mdctoffset+mid4subbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+mdctoffset+mid4subbandoffset[kk]-2];
						}
				}

				
				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				for (i = 1; i <= m; i++)
				{
						for (j = 1; j <= m; j++)
					anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][2*i+mdctoffset+mid4subbandoffset[kk]-2]=data[i][j];
					
						}
				}*/

				////////////////////////////////////////////////////////
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						data[i][j] = Mdctin[j-1][2*i+1+mdctoffset+mid4subbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+1+mdctoffset+mid4subbandoffset[kk]-2];
						}
				}

				
				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				for (i = 1; i <= m; i++)
				{
						for (j = 1; j <= m; j++)
					anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+1+mdctoffset+mid4subbandoffset[kk]-2]=data[i][j];
				
						}
				}*/
#else
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][i+mdctoffset+mid4subbandoffset[kk]-1];
						data2[i][j] = Mdctin[j-1][i+mdctoffset+mid4subbandoffset[kk]-1];
						}
				}

				
				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				for (i = 1; i <= m; i++)
				{
						for (j = 1; j <= m; j++)
					anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][i+mdctoffset+mid4subbandoffset[kk]-1]=data[i][j];
				
						}
				}*/
#endif
			free_matrix(corrMatrix, m, m);
			free_matrix(anaMatrix, m, m);
			free_vector(evalsset, m);

			free_matrix(data, n, m);
			free_matrix(data2, n, m);
		}


		for(kk=2;kk<=4;kk+=2)
			{
				if(engbandset[kk]>6*engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1];
						
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}

				if(engbandset[kk]<1.0/6*engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1];
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}

			}//for(kk=0;kk<=8;kk+=2)

		

			for(kk=6;kk<=14;kk+=2)
			{
				if(engbandset[kk]>engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1];
						
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}

				if(engbandset[kk]<engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1];
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}

			}//for(kk=10;kk<=24;kk+=2)


			for(kk=16;kk<=bandnum;kk+=4)
			{int maxindextmp=kk;
			 int tmp26;
			 float maxengtmp= engbandset[kk];
			 for(tmp26=kk;tmp26<(kk+4>bandnum?bandnum:kk+4);tmp26++)
				{
					if(maxengtmp<engbandset[tmp26])
					{
						maxindextmp = tmp26;
						maxengtmp = engbandset[tmp26];
					}
				}
				allbandsetflag[elementindex][index-1][kk]=1;
				for(tmp26=kk;tmp26<(kk+4>bandnum?bandnum:kk+4);tmp26++)
				{
				if(tmp26!=maxindextmp)
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][tmp26][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][maxindextmp][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][tmp26][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][maxindextmp][1][(i-1)*m+j-1];
						
						}
					}

				
					allbandsetflag[elementindex][index-1][tmp26]=0;
				}
				allbandsetflag[elementindex][index-1][tmp26]=0;
				}

				allbandsetflag[elementindex][index-1][kk]=1;
				

			}//for(kk=16;kk<=bandnum;kk+=4)


			
			
			for(kk=0;kk<bandnum;kk++)
			{int n=mid4subbandoffset[kk+1]-mid4subbandoffset[kk];
				int i,j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset ;
				float **anaMatrix;
			 
				data = matrix(n, m);  /* Storage allocation for input data */
				data2 = matrix(n, m);  /* Storage allocation for input data */
				corrMatrix =matrix(m, m);
				evalsset =vector(m);  
				anaMatrix=matrix(m, m);

#if EVENODD
				n=(mid4subbandoffset[kk+1]-mid4subbandoffset[kk])/2;
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
						data[i][j] = Mdctin[j-1][2*i+mdctoffset+mid4subbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+mdctoffset+mid4subbandoffset[kk]-2];
						}
				}

				
				
				for (i = 1; i <= m; i++)
				{
						for (j = 1; j <= m; j++)
							anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1]; //[i][j];
				}

				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][2*i+mdctoffset+mid4subbandoffset[kk]-2]=data[i][j];
					
						}
				}

				////////////////////////////////////////////////////////
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						data[i][j] = Mdctin[j-1][2*i+1+mdctoffset+mid4subbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+1+mdctoffset+mid4subbandoffset[kk]-2];
						}
				}

				
				
				for (i = 1; i <= m; i++)
				{
						for (j = 1; j <= m; j++)
					anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1];//[i][j];
				}

				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);
				/**/for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+1+mdctoffset+mid4subbandoffset[kk]-2]=data[i][j];
				
						}
				}
#else
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][i+mdctoffset+mid4subbandoffset[kk]-1];
						data2[i][j] = Mdctin[j-1][i+mdctoffset+mid4subbandoffset[kk]-1];
						}
				}

				
				
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] ;//[i][j];
				}
				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);

				/**/for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][i+mdctoffset+mid4subbandoffset[kk]-1]=data[i][j];
				
						}
				}
#endif
			free_matrix(corrMatrix, m, m);
			free_matrix(anaMatrix, m, m);
			free_vector(evalsset, m);

			free_matrix(data, n, m);
			free_matrix(data2, n, m);
		}


		}
          break;
        case 512 :
			
      
			{
			float engbandset[30];
			float maxeng;
			int i,j;

				bandnum=bandnumset[2];
				bandmulflag[elementindex][index-1]=1;
				for(kk=0;kk<bandnum;kk++)
				{int n=(longsubbandoffset[kk+1]-longsubbandoffset[kk]);
					engbandset[kk]=0;
					for (i = 1; i <= n; i++)
					{
					for (j = 1; j <= m; j++)
						{
					
						engbandset[kk] += (Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1]*Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1]);
						
						}
					 }
					engbandset[kk] =engbandset[kk]/n;
				}
				maxeng=0.0001;
				for(kk=0;kk<bandnum;kk++)
				{
					if(engbandset[kk]>maxeng)
					{
						maxeng = engbandset[kk]+0.0001;
					}
				}

				allbandsetflag[elementindex][index-1][0]=1;
				allbandsetflag[elementindex][index-1][1]=1;
				for(kk=2;kk<bandnum;kk++)
				{int startindex,endindex;
				float bandmaxeng;
			
					startindex= kk-3;
					endindex = kk+3;
					if(startindex<=0)
						startindex=0;
					if(endindex>=bandnum)
						endindex=bandnum-1;
					bandmaxeng=0;
					for(j=startindex;j<=endindex;j++)
					{
						if(engbandset[j]>bandmaxeng)
						{
						bandmaxeng = engbandset[j]+0.0001;
						}
					}

					if((engbandset[kk]<bandmaxeng/200)||(engbandset[kk]<maxeng/100000))
					{
						allbandsetflag[elementindex][index-1][kk]=0;
					}else
					{
						allbandsetflag[elementindex][index-1][kk]=1;
					}

				}

			
			for(kk=0;kk<bandnum;kk++)
			{int n=mid2subbandoffset[kk+1]-mid2subbandoffset[kk];
				int i,j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset ;
				float **anaMatrix;

				data = matrix(n, m);  /* Storage allocation for input data */
				data2 = matrix(n, m);  /* Storage allocation for input data */
				corrMatrix =matrix(m, m);
				evalsset =vector(m);  
				anaMatrix=matrix(m, m);

#if EVENODD
				n=(mid2subbandoffset[kk+1]-mid2subbandoffset[kk])/2;
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][2*i+mdctoffset+mid2subbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+mdctoffset+mid2subbandoffset[kk]-2];
						}
				 }

					

				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+mdctoffset+mid2subbandoffset[kk]-2]=data[i][j];
				
						}
				 }*/


								for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][2*i+1+mdctoffset+mid2subbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+1+mdctoffset+mid2subbandoffset[kk]-2];
						}
				 }

					

				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][2*i+1+mdctoffset+mid2subbandoffset[kk]-2]=data[i][j];
				
						}
				 }*/

#else
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][i+mdctoffset+mid2subbandoffset[kk]-1];
						data2[i][j] = Mdctin[j-1][i+mdctoffset+mid2subbandoffset[kk]-1];
						}
				 }

					

				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][i+mdctoffset+mid2subbandoffset[kk]-1]=data[i][j];
				
						}
				 }*/
#endif
				free_matrix(corrMatrix, m, m);
				free_matrix(anaMatrix, m, m);
				free_vector(evalsset, m);

				free_matrix(data, n, m);
				free_matrix(data2, n, m);
			}

			for(kk=2;kk<=bandnum-2;kk+=2)
			{if(engbandset[kk]>10*engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1];
						
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}

				if(engbandset[kk]<0.1*engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1];
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}
							

			}//for(kk=16;kk<=bandnum;kk+=4)

			for(kk=0;kk<bandnum;kk++)
			{int n=mid2subbandoffset[kk+1]-mid2subbandoffset[kk];
				int i,j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset ;
				float **anaMatrix;

				data = matrix(n, m);  /* Storage allocation for input data */
				data2 = matrix(n, m);  /* Storage allocation for input data */
				corrMatrix =matrix(m, m);
				evalsset =vector(m);  
				anaMatrix=matrix(m, m);

#if EVENODD
				n=(mid2subbandoffset[kk+1]-mid2subbandoffset[kk])/2;
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][2*i+mdctoffset+mid2subbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+mdctoffset+mid2subbandoffset[kk]-2];
						}
				 }

					

				
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrix[j][i] =anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1];//[i][j];
				}
				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+mdctoffset+mid2subbandoffset[kk]-2]=data[i][j];
				
						}
				 }


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][2*i+1+mdctoffset+mid2subbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+1+mdctoffset+mid2subbandoffset[kk]-2];
						}
				 }

					

				
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1];//[i][j];
				}

				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);
				/**/for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][2*i+1+mdctoffset+mid2subbandoffset[kk]-2]=data[i][j];
				
						}
				 }

#else
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][i+mdctoffset+mid2subbandoffset[kk]-1];
						data2[i][j] = Mdctin[j-1][i+mdctoffset+mid2subbandoffset[kk]-1];
						}
				 }

					

				
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1];//[i][j];
				}

				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);
				/**/for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][i+mdctoffset+mid2subbandoffset[kk]-1]=data[i][j];
				
						}
				 }
#endif
				free_matrix(corrMatrix, m, m);
				free_matrix(anaMatrix, m, m);
				free_vector(evalsset, m);

				free_matrix(data, n, m);
				free_matrix(data2, n, m);
			}


			}
          break;
		 case 256 :
			 
			 {
				float engbandset[30];
				float maxeng;
				int i,j;
				bandnum=bandnumset[3];

				bandmulflag[elementindex][index-1]=1;

				for(kk=0;kk<bandnum;kk++)
				{int n=(longsubbandoffset[kk+1]-longsubbandoffset[kk]);
					engbandset[kk]=0;
					for (i = 1; i <= n; i++)
					{
					for (j = 1; j <= m; j++)
						{
					
						engbandset[kk] += (Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1]*Mdctin[j-1][i+mdctoffset+longsubbandoffset[kk]-1]);
						
						}
					 }
					engbandset[kk] =engbandset[kk]/n;
				}
				maxeng=0.0001;
				for(kk=0;kk<bandnum;kk++)
				{
					if(engbandset[kk]>maxeng)
					{
						maxeng = engbandset[kk]+0.0001;
					}
				}

				allbandsetflag[elementindex][index-1][0]=1;
				allbandsetflag[elementindex][index-1][1]=1;
				for(kk=2;kk<bandnum;kk++)
				{int startindex,endindex;
				float bandmaxeng;
			
					startindex= kk-3;
					endindex = kk+3;
					if(startindex<=0)
						startindex=0;
					if(endindex>=bandnum)
						endindex=bandnum-1;
					bandmaxeng=0;
					for(j=startindex;j<=endindex;j++)
					{
						if(engbandset[j]>bandmaxeng)
						{
						bandmaxeng = engbandset[j]+0.0001;
						}
					}

					if((engbandset[kk]<bandmaxeng/200)||(engbandset[kk]<maxeng/100000))
					{
						allbandsetflag[elementindex][index-1][kk]=0;
					}else
					{
						allbandsetflag[elementindex][index-1][kk]=1;
					}

				}

			
			 	for(kk=0;kk<bandnum;kk++)
				{int n=shortsubbandoffset[kk+1]-shortsubbandoffset[kk];

				int i,j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset ;
				float **anaMatrix;
			 
				data = matrix(n, m);  /* Storage allocation for input data */

				data2 = matrix(n, m);  /* Storage allocation for input data */

				corrMatrix =matrix(m, m);
				evalsset =vector(m);  
				anaMatrix=matrix(m, m);
#if EVENODD
				n=(shortsubbandoffset[kk+1]-shortsubbandoffset[kk])/2;

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						data[i][j] = Mdctin[j-1][2*i+mdctoffset+shortsubbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+mdctoffset+shortsubbandoffset[kk]-2];
						}
				}

				

				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+mdctoffset+shortsubbandoffset[kk]-2]=data[i][j];
				
						}
				 }*/


				/////////////////////	/////////////////////	/////////////////////
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						data[i][j] = Mdctin[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2];
						}
				}

				

				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2]=data[i][j];
				
						}
				 }*/

#else
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][i+mdctoffset+shortsubbandoffset[kk]-1];
						data2[i][j] = Mdctin[j-1][i+mdctoffset+shortsubbandoffset[kk]-1];
						}
				 }

				
				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,1);
			
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrix[j][i];//[i][j];
				}

				/*for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][i+mdctoffset+shortsubbandoffset[kk]-1]=data[i][j];
				
						}
				 }*/
#endif
				free_matrix(corrMatrix, m, m);
				free_matrix(anaMatrix, m, m);
				free_vector(evalsset, m);

				free_matrix(data, n, m);
				free_matrix(data2, n, m);
			}

			for(kk=2;kk<=bandnum-2;kk+=2)
			{if(engbandset[kk]>10*engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1];
						
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}

				if(engbandset[kk]<0.1*engbandset[kk+1])
				{
					for (i = 1; i <= m; i++)
					{
						for (j = 1; j <= m; j++)
						{
							anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][0][(i-1)*m+j-1];
							anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1] = anaMatrixdata[elementindex][index-1][kk+1][1][(i-1)*m+j-1];
						}
					}

					allbandsetflag[elementindex][index-1][kk]=1;
					allbandsetflag[elementindex][index-1][kk+1]=0;
				}
			}

				for(kk=0;kk<bandnum;kk++)
				{int n=shortsubbandoffset[kk+1]-shortsubbandoffset[kk];

				int i,j;
				float **data;
				float **data2;
				float **corrMatrix;
				float*evalsset ;
				float **anaMatrix;
			 
				data = matrix(n, m);  /* Storage allocation for input data */

				data2 = matrix(n, m);  /* Storage allocation for input data */

				corrMatrix =matrix(m, m);
				evalsset =vector(m);  
				anaMatrix=matrix(m, m);
#if EVENODD
				n=(shortsubbandoffset[kk+1]-shortsubbandoffset[kk])/2;

				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						data[i][j] = Mdctin[j-1][2*i+mdctoffset+shortsubbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+mdctoffset+shortsubbandoffset[kk]-2];
						}
				}

				

				
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] ;//[i][j];
				}
				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);

				/**/for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+mdctoffset+shortsubbandoffset[kk]-2]=data[i][j];
				
						}
				 }


				/////////////////////	/////////////////////	/////////////////////
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						data[i][j] = Mdctin[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2];
						}
				}

				

				
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1];//[i][j];
				}
				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);

				/**/for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2]=data[i][j];
				
						}
				 }

#else
				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						data[i][j] = Mdctin[j-1][i+mdctoffset+shortsubbandoffset[kk]-1];
						data2[i][j] = Mdctin[j-1][i+mdctoffset+shortsubbandoffset[kk]-1];
						}
				 }

				
				
			
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
						anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1] ;//[i][j];
				}
				PCA_analysis(data,data2,n,m,corrMatrix,evalsset,anaMatrix,0);

				/**/for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][i+mdctoffset+shortsubbandoffset[kk]-1]=data[i][j];
				
						}
				 }
#endif
				free_matrix(corrMatrix, m, m);
				free_matrix(anaMatrix, m, m);
				free_vector(evalsset, m);

				free_matrix(data, n, m);
				free_matrix(data2, n, m);
			}
			

		}
		  break;
		  case 128 :
      
         
          break;
        }
		

	
		mdctoffset+=(ll*1);
	
		
}


	return 0;
}



int PCA_analysis(float **data2, float **data, int  n, int  m, float **corrMatrix, float*evalsset, float **anaMatrix, int flag)

{

	int   i, j, k, k2;
	float  **symmat, **symmat2, *evals, *interm;

	//float in_value;
	char option, *strncpy();

	for (i = 1; i <= n; i++)
	{
		for (j = 1; j <= m; j++)
		{
			data[i][j] = data2[i][j];
		}
	}



	symmat = matrix(m, m);  /* Allocation of correlation (etc.) matrix */

   /* Look at analysis option; branch in accordance with this. */

	option = 'V';
	switch (option)
	{
	case 'R':
	case 'r':
		corcol(data, n, m, symmat);
		break;
	case 'V':
	case 'v':
		covcol(data, n, m, symmat);
		break;
	case 'S':
	case 's':
		scpcol(data, n, m, symmat);
		break;
	default:
		printf("Option: %s\n", option);
		printf("For option, please type R, V, or S\n");
		printf("(upper or lower case).\n");
		printf("Exiting to system.\n");
		exit(1);
		break;
	}

	/*********************************************************************
		Eigen-reduction
	**********************************************************************/

	/* Allocate storage for dummy and new vectors. */
	evals = vector(m);     /* Storage alloc. for vector of eigenvalues */
	interm = vector(m);    /* Storage alloc. for 'intermediate' vector */
	symmat2 = matrix(m, m);  /* Duplicate of correlation (etc.) matrix */
	for (i = 1; i <= m; i++)
	{
		for (j = 1; j <= m; j++)
		{
			symmat2[i][j] = symmat[i][j]; /* Needed below for col. projections */

		}

	}

	for (i = 1; i <= m; i++)
	{
		for (j = 1; j <= m; j++)
		{
			corrMatrix[i][j] = symmat[i][j]; /* Needed below for col. projections */

		}

	}
	//symmat  相关矩阵
	//evals 特征值
	tred2(symmat, m, evals, interm);  /* Triangular decomposition */

	tqli(evals, interm, m, symmat);   /* Reduction of sym. trid. matrix */
	/* evals now contains the eigenvalues,
	   columns of symmat now contain the associated eigenvectors. */

	for (j = 1; j < m; j++)
	{
		int maxj = j;
		double minevals = fabs(evals[j]);
		for (i = j; i < m; i++)
			if (minevals > fabs(evals[i + 1]))
			{
				maxj = i + 1;
				minevals = fabs(evals[i + 1]);
			}

		if (maxj != j)
		{
			minevals = evals[j];
			evals[j] = evals[maxj];
			evals[maxj] = minevals;

			for (i = 1; i <= m/*3*/; i++)
			{
				minevals = symmat[i][j];
				symmat[i][j] = symmat[i][maxj];
				symmat[i][maxj] = minevals;
			}
		}

	}



	for (i = 1; i <= m/*3*/; i++)
	{
		evalsset[i] = evals[m - i + 1];
	}


	if (flag == 1)
	{
		for (i = 1; i <= m; i++)
		{
			for (j = 1; j <= m; j++)
			{
				anaMatrix[i][j] = ((float)(symmat[i][m - j + 1] * PCAQUANT4 + 0.0)) / PCAQUANT4;
			}
		}

		if ((m == 2))
		{
			if ((anaMatrix[1][2] == anaMatrix[2][1]) && (anaMatrix[2][2] == -anaMatrix[1][1]))
			{
				for (i = 1; i <= 2; i++)
				{
					for (j = 2; j <= 2; j++)
					{
						anaMatrix[i][j] = -((float)(symmat[i][m - j + 1] * PCAQUANT4 + 0.0)) / PCAQUANT4;//symmat[i][m-j+1]; /* Needed below for col. projections */
						symmat[i][m - j + 1] = -symmat[i][m - j + 1];
					}
				}
			}
		}

		if ((m == 4))
		{
			for (j = 1; j <= 4; j++)
			{
				int flag;
				flag = 0;

				for (i = 1; i <= 4; i++)
					if ((symmat[i][m - j + 1]) < -0.70710678118654752440084436210485)
						flag = 1;

				if (flag == 0)
				{
					float mintmp;
					int maxindextmp;
					mintmp = -1;
					maxindextmp = 1;
					for (i = 1; i <= 4; i++)
					{
						if (fabs(symmat[i][m - j + 1]) > mintmp)
						{
							mintmp = fabs(symmat[i][m - j + 1]);
							maxindextmp = i;
						}
					}
					if (symmat[maxindextmp][m - j + 1] < 0)
						flag = 1;
				}
				if (flag == 1)
				{
					for (i = 1; i <= 4; i++)
					{
						anaMatrix[i][j] = -((float)(symmat[i][m - j + 1] * PCAQUANT4 + 0.0)) / PCAQUANT4;//symmat[i][m-j+1]; /* Needed below for col. projections */
						symmat[i][m - j + 1] = -symmat[i][m - j + 1];
					}
				}
			}
		}

	}
	else
	{
		for (i = 1; i <= m; i++)
		{
			for (j = 1; j <= m; j++)
			{
				symmat[i][m - j + 1] = anaMatrix[i][j];//symmat[i][m-j+1]; /* Needed below for col. projections */
			}
		}
	}

	/* Form projections of row-points on first three prin. components. */
	/* Store in 'data', overwriting original data. */
	for (i = 1; i <= n; i++)
	{
		for (j = 1; j <= m; j++)
		{
			interm[j] = data2[i][j];
		}   /* data[i][j] will be overwritten */
		for (k = 1; k <= m/*3*/; k++)
		{
			data[i][k] = 0.0;
			for (k2 = 1; k2 <= m; k2++)
			{
				data[i][k] += interm[k2] * symmat[k2][m - k + 1];
			}
		}
	}


	for (i = 1; i <= n; i++)
	{
		for (j = 1; j <= m; j++)
		{
			data2[i][j] = data[i][j];
		}   /* data[i][j] will be overwritten */

	}

	for (i = 1; i <= m; i++)
	{
		for (j = 1; j <= m; j++)
		{
			symmat[i][m - j + 1] = ((float)(symmat[i][m - j + 1] * PCAQUANT4 + 0.0)) / PCAQUANT4; /* Needed below for col. projections */

		}
	}

	/* Form projections of col.-points on first three prin. components. */
	/* Store in 'symmat2', overwriting what was stored in this. */
	for (j = 1; j <= m; j++)
	{
		for (k = 1; k <= m; k++)
		{
			interm[k] = symmat2[j][k];
		}  /*symmat2[j][k] will be overwritten*/
		for (i = 1; i <= m/*3*/; i++)
		{
			symmat2[j][i] = 0.0;
			for (k2 = 1; k2 <= m; k2++)
			{
				symmat2[j][i] += interm[k2] * symmat[k2][m - i + 1];
			}
			if (evals[m - i + 1] > 0.0005)   /* Guard against zero eigenvalue */
				symmat2[j][i] /= sqrt(evals[m - i + 1]);   /* Rescale */
			else
				symmat2[j][i] = 0.0;    /* Standard kludge */
		}
	}

	// free_matrix(data2, n, m);
	free_matrix(symmat, m, m);
	free_matrix(symmat2, m, m);
	free_vector(evals, m);
	free_vector(interm, m);

	return 0;

}




int PCA_synthesis(float **data2, float **data, int  n, int  m, float **anaMatrix){

	int   i, j, k, k2;
	float  **symmat, *interm;

	interm = vector(m);    /* Storage alloc. for 'intermediate' vector */
	symmat = matrix(m, m);

	for (i = 1; i <= m; i++)
	{
		for (j = 1; j <= m; j++)
		{
			symmat[i][j] = anaMatrix[i][m - j + 1]; /* Needed below for col. projections */

		}

	}

	for (i = 1; i <= n; i++)
	{
		for (j = 1; j <= m; j++)
		{
			interm[j] = data[i][j];
		}   /* data[i][j] will be overwritten */
		for (k = 1; k <= m/*3*/; k++)
		{
			data2[i][k] = 0.0;
			for (k2 = 1; k2 <= 2/*m*/; k2++)
			{
				data2[i][k] += interm[k2] * symmat[k][m - k2 + 1]; //symmat[m-k+1][k2]; //symmat[k][m-k2+1]; 
			}
		}
	}

	free_matrix(symmat, m, m);
	free_vector(interm, m);

	return 0;
}
/**  Correlation matrix: creation  ***********************************/

void corcol(float **data, int n, int m, float**symmat)

/* Create m * m correlation matrix from given n * m data matrix. */
{
	float eps = 0.005;
	float x, *mean, *stddev;
	int i, j, j1, j2;

	/* Allocate storage for mean and std. dev. vectors */

	mean = vector(m);
	stddev = vector(m);

	/* Determine mean of column vectors of input data matrix */

	for (j = 1; j <= m; j++)
	{
		mean[j] = 0.0;
		for (i = 1; i <= n; i++)
		{
			mean[j] += data[i][j];
		}
		mean[j] /= (float)n;
	}


	/* Determine standard deviations of column vectors of data matrix. */

	for (j = 1; j <= m; j++)
	{
		stddev[j] = 0.0;
		for (i = 1; i <= n; i++)
		{
			stddev[j] += ((data[i][j] - mean[j]) *
				(data[i][j] - mean[j]));
		}
		stddev[j] /= (float)n;
		stddev[j] = sqrt(stddev[j]);
		/* The following in an inelegant but usual way to handle
		near-zero std. dev. values, which below would cause a zero-
		divide. */
		if (stddev[j] <= eps) stddev[j] = 1.0;
	}

	/* Center and reduce the column vectors. */

	for (i = 1; i <= n; i++)
	{
		for (j = 1; j <= m; j++)
		{
			data[i][j] -= mean[j];
			x = sqrt((float)n);
			x *= stddev[j];
			data[i][j] /= x;
		}
	}

	/* Calculate the m * m correlation matrix. */
	for (j1 = 1; j1 <= m - 1; j1++)
	{
		symmat[j1][j1] = 1.0;
		for (j2 = j1 + 1; j2 <= m; j2++)
		{
			symmat[j1][j2] = 0.0;
			for (i = 1; i <= n; i++)
			{
				symmat[j1][j2] += (data[i][j1] * data[i][j2]);
			}
			symmat[j2][j1] = symmat[j1][j2];
		}
	}
	symmat[m][m] = 1.0;

	return;

}

/**  Variance-covariance matrix: creation  *****************************/

void covcol(float **data, int n, int m, float **symmat)

/* Create m * m covariance matrix from given n * m data matrix. */
{
	float *mean;
	int i, j, j1, j2;

	/* Allocate storage for mean vector */

	mean = vector(m);

	/* Determine mean of column vectors of input data matrix */

	for (j = 1; j <= m; j++)
	{
		mean[j] = 0.0;
		for (i = 1; i <= n; i++)
		{
			mean[j] += data[i][j];
		}
		mean[j] /= (float)n;
	}

	/* Center the column vectors. */

	for (i = 1; i <= n; i++)
	{
		for (j = 1; j <= m; j++)
		{
			data[i][j] -= mean[j];   //wuchaogang
		}
	}

	/* Calculate the m * m covariance matrix. */
	for (j1 = 1; j1 <= m; j1++)
	{
		for (j2 = j1; j2 <= m; j2++)
		{
			symmat[j1][j2] = 0.0;
			for (i = 1; i <= n; i++)
			{
				symmat[j1][j2] += data[i][j1] * data[i][j2];
			}
			symmat[j2][j1] = symmat[j1][j2];
		}
	}

	return;

}

/**  Sums-of-squares-and-cross-products matrix: creation  **************/

void scpcol(float **data, int n, int m, float **symmat)

/* Create m * m sums-of-cross-products matrix from n * m data matrix. */
{
	int i, j1, j2;

	/* Calculate the m * m sums-of-squares-and-cross-products matrix. */

	for (j1 = 1; j1 <= m; j1++)
	{
		for (j2 = j1; j2 <= m; j2++)
		{
			symmat[j1][j2] = 0.0;
			for (i = 1; i <= n; i++)
			{
				symmat[j1][j2] += data[i][j1] * data[i][j2];
			}
			symmat[j2][j1] = symmat[j1][j2];
		}
	}
	return;
}

/**  Error handler  **************************************************/

void erhand(char err_msg[])
/* Error handler */
{
    fprintf(stderr,"Run-time error:\n");
    fprintf(stderr,"%s\n", err_msg);
    fprintf(stderr,"Exiting to system.\n");
    exit(1);
}

/**  Allocation of vector storage  ***********************************/

float *vector(int n)
/* Allocates a float vector with range [1..n]. */
{
    float *v;

    v = (float *) malloc ((unsigned) n*sizeof(float));
    if (!v) erhand("Allocation failure in vector().");
    return v-1;

}

/**  Allocation of float matrix storage  *****************************/

float **matrix(int n, int m)

/* Allocate a float matrix with range [1..n][1..m]. */
{
    int i;
    float **mat;

    /* Allocate pointers to rows. */
    mat = (float **) malloc((unsigned) (n)*sizeof(float*));
    if (!mat) erhand("Allocation failure 1 in matrix().");
    mat -= 1;

    /* Allocate rows and set pointers to them. */
    for (i = 1; i <= n; i++)
        {
        mat[i] = (float *) malloc((unsigned) (m)*sizeof(float));
        if (!mat[i]) erhand("Allocation failure 2 in matrix().");
        mat[i] -= 1;
        }

     /* Return pointer to array of pointers to rows. */
     return mat;
}

/**  Deallocate vector storage  *********************************/

void free_vector(float *v,int n)
/* Free a float vector allocated by vector(). */
{
   free((char*) (v+1));
}

/**  Deallocate float matrix storage  ***************************/

void free_matrix(float **mat,int n,int m)
/* Free a float matrix allocated by matrix(). */
{
   int i;

   for (i = n; i >= 1; i--)
   {
	   free((char*)(mat[i] + 1));
   }
   free ((char*) (mat+1));
}

/**  Reduce a real, symmetric matrix to a symmetric, tridiag. matrix. */

void tred2(float **a, int n, float *d, float *e)
/* Householder reduction of matrix a to tridiagonal form.
   Algorithm: Martin et al., Num. Math. 11, 181-195, 1968.
   Ref: Smith et al., Matrix Eigensystem Routines -- EISPACK Guide
		Springer-Verlag, 1976, pp. 489-494.
		W H Press et al., Numerical Recipes in C, Cambridge U P,
		1988, pp. 373-374.  */
{
	int l, k, j, i;
	float scale, hh, h, g, f;

	for (i = n; i >= 2; i--)
	{
		l = i - 1;
		h = scale = 0.0;
		if (l > 1)
		{
			for (k = 1; k <= l; k++)
				scale += fabs(a[i][k]);
			if (scale == 0.0)
				e[i] = a[i][l];
			else
			{
				for (k = 1; k <= l; k++)
				{
					a[i][k] /= scale;
					h += a[i][k] * a[i][k];
				}
				f = a[i][l];
				g = f > 0 ? -sqrt(h) : sqrt(h);
				e[i] = scale * g;
				h -= f * g;
				a[i][l] = f - g;
				f = 0.0;
				for (j = 1; j <= l; j++)
				{
					a[j][i] = a[i][j] / h;
					g = 0.0;
					for (k = 1; k <= j; k++)
						g += a[j][k] * a[i][k];
					for (k = j + 1; k <= l; k++)
						g += a[k][j] * a[i][k];
					e[j] = g / h;
					f += e[j] * a[i][j];
				}
				hh = f / (h + h);
				for (j = 1; j <= l; j++)
				{
					f = a[i][j];
					e[j] = g = e[j] - hh * f;
					for (k = 1; k <= j; k++)
						a[j][k] -= (f * e[k] + g * a[i][k]);
				}
			}
		}
		else
			e[i] = a[i][l];
		d[i] = h;
	}
	d[1] = 0.0;
	e[1] = 0.0;
	for (i = 1; i <= n; i++)
	{
		l = i - 1;
		if (d[i])
		{
			for (j = 1; j <= l; j++)
			{
				g = 0.0;
				for (k = 1; k <= l; k++)
					g += a[i][k] * a[k][j];
				for (k = 1; k <= l; k++)
					a[k][j] -= g * a[k][i];
			}
		}
		d[i] = a[i][i];
		a[i][i] = 1.0;
		for (j = 1; j <= l; j++)
			a[j][i] = a[i][j] = 0.0;
	}
}

/**  Tridiagonal QL algorithm -- Implicit  **********************/

void tqli(float d[], float e[], int n, float **z)
{
	int m, l, iter, i, k;
	float s, r, p, g, f, dd, c, b;
	void erhand();

	for (i = 2; i <= n; i++)
		e[i - 1] = e[i];
	e[n] = 0.0;
	for (l = 1; l <= n; l++)
	{
		iter = 0;
		do
		{
			for (m = l; m <= n - 1; m++)
			{
				dd = fabs(d[m]) + fabs(d[m + 1]);
				if (fabs(e[m]) + dd == dd) break;
			}
			if (m != l)
			{
				if (iter++ == 30) erhand("No convergence in TLQI.");
				g = (d[l + 1] - d[l]) / (2.0 * e[l]);
				r = sqrt((g * g) + 1.0);
				g = d[m] - d[l] + e[l] / (g + SIGN(r, g));
				s = c = 1.0;
				p = 0.0;
				for (i = m - 1; i >= l; i--)
				{
					f = s * e[i];
					b = c * e[i];
					if (fabs(f) >= fabs(g))
					{
						c = g / f;
						r = sqrt((c * c) + 1.0);
						e[i + 1] = f * r;
						c *= (s = 1.0 / r);
					}
					else
					{
						s = f / g;
						r = sqrt((s * s) + 1.0);
						e[i + 1] = g * r;
						s *= (c = 1.0 / r);
					}
					g = d[i + 1] - p;
					r = (d[i] - g) * s + 2.0 * c * b;
					p = s * r;
					d[i + 1] = g + p;
					g = c * r - b;
					for (k = 1; k <= n; k++)
					{
						f = z[k][i + 1];
						z[k][i + 1] = s * z[k][i] + c * f;
						z[k][i] = c * z[k][i] - s * f;
					}
				}
				d[l] = d[l] - p;
				e[l] = g;
				e[m] = 0.0;
			}
		} while (m != l);
	}
}

