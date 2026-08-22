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
#include "pca.h"
#include "mc_rom.h"

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


int PCA_analysis(float **data2,float **data,int  n, int  m,float **corrMatrix,float*evalsset ,float **anaMatrix);
int PCA_synthesis(float **data2,float **data,int  n, int  m,float **anaMatrix,int usePCAitemnum);

//static int LL[5]={4096/8/2, 4096/4/2, 4096/2/2, 4096/2,4096/16/2};

//2014.11.13  wchg 修改划分表，以利于编码方式的切换
static short longsubbandoffset[40+1]={
		0,   8,   16, 24, 32,  40,  48, 56,
		64,  72,  80, 88, 96, 104, 112, 120,	 
       128, 138, 148, 158, 168, 180, 192, 206,
      220, 236, 252, 270, 290, 314, 344, 380,
      420, 464, 512, 568, 636, 712, 800, 908,
	 1024
	};//USE_MDCT
const short mid4subbandoffset[24 + 1] = {
		0,   8,   16,  24,
		32,  40,  48,  60,
		72,  84,  96,  112,
		128, 148, 168, 192, 
		216, 240, 268, 296,
		328, 360, 400, 448,
		512
	};
const short mid2subbandoffset[12 + 1] = {
		 0,   8,   16,  24, 
		 36,  48,  64,  84, 
		 108, 134, 168, 208,
		 256
	};
const short shortsubbandoffset[9 + 1] = {
		  0, 8, 16, 24, 32, 44, 56, 72, 96, 128  };

int bandnumset[5]={40,24,12,9};//{16,12,12,8,12};
//int bandnumset_L[5]={14,7,7,4,7};


float anaMatrixdata[3][8][50][2][8*8];
extern int bandmulflag[3][8];




int multichannelMDCT_PCA_syn(float Mdctin[][FRAME_LEN_LONG * 4 + 2048], int *Swinseq,int channelnum,float Mdctout[][FRAME_LEN_LONG * 4 + 2048],int elementindex,int usePCAitemnum)
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
			bandnum=bandnumset[0];
         // mdft2048(St, mdft4096_Sr, mdft4096_Si);
				for(kk=0;kk<bandnum;kk+=((kk>=MULBANDSTART) ? bandmulflag[elementindex][index-1]:1))
			{int n=(longsubbandoffset[kk+((kk>=MULBANDSTART) ? bandmulflag[elementindex][index-1]:1)]-longsubbandoffset[kk]);
				int i,j;
				float **data;
				float **data2;
			
				float **anaMatrix;
			 data = matrix(n, m);  /* Storage allocation for input data */

				data2 = matrix(n, m);  /* Storage allocation for input data */
				
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
				anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1]  ;
			}

			PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


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

				
	for (i = 1; i <= m; i++)
			{
					for (j = 1; j <= m; j++)
				anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1]  ;
			}

			PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


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

				

				for (i = 1; i <= m; i++)
			{
					for (j = 1; j <= m; j++)
				anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1]  ;
			}

			PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


				for (i = 1; i <= n; i++)
					{
					for (j = 1; j <= m; j++)
						{
					//	fscanf(stream, "%f	", &in_value);
						Mdctout[j-1][i+mdctoffset+longsubbandoffset[kk]-1]=data[i][j];
					//	data2[i][j] = Mdctin[j][i+mdctoffset+longsubbandoffset[kk+1]];
						}
					 }
#endif
		
   free_matrix(anaMatrix, m, m);
   

	free_matrix(data, n, m);
	free_matrix(data2, n, m);
			}
          break;
		case 1024 :
			bandnum=bandnumset[1];
       
				for(kk=0;kk<bandnum;kk++)
			{int n=mid4subbandoffset[kk+1]-mid4subbandoffset[kk];
				int i,j;
				float **data;
				float **data2;
			
				float **anaMatrix;
			 
				data = matrix(n, m);  /* Storage allocation for input data */
				data2 = matrix(n, m);  /* Storage allocation for input data */
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
				anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1]  ;
				}

				PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][2*i+mdctoffset+mid4subbandoffset[kk]-2]=data[i][j];
					
						if((channelnum==4)&&(kk==0)&&(j==4)&&(i>4))
						{
							Mdctout[j-1][2*i+mdctoffset+shortsubbandoffset[kk]-2]=0;
						}
						}
				 }

				/////////////////////////////////
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
				anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1]  ;
				}

				PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+1+mdctoffset+mid4subbandoffset[kk]-2]=data[i][j];
					
						if((channelnum==4)&&(kk==0)&&(j==4)&&(i>4))
						{
							Mdctout[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2]=0;
						}
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
				anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1]  ;
				}

				PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][i+mdctoffset+mid4subbandoffset[kk]-1]=data[i][j];
					
						}
				 }

#endif
   free_matrix(anaMatrix, m, m);
   

	free_matrix(data, n, m);
	free_matrix(data2, n, m);
			}
          break;
        case 512 :
			bandnum=bandnumset[2];
       
				for(kk=0;kk<bandnum;kk++)
			{int n=mid2subbandoffset[kk+1]-mid2subbandoffset[kk];
				int i,j;
				float **data;
				float **data2;
				
				float **anaMatrix;
			 data = matrix(n, m);  /* Storage allocation for input data */
				data2 = matrix(n, m);  /* Storage allocation for input data */
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
					anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1]  ;
				}

				PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+mdctoffset+mid2subbandoffset[kk]-2]=data[i][j];
					
						if((channelnum==4)&&(kk==0)&&(j==4)&&(i>2))
						{
							Mdctout[j-1][2*i+mdctoffset+shortsubbandoffset[kk]-2]=0;
						}
						}
				}

				//////////////////////////////////////////////////
								for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j-1][2*i+1+mdctoffset+mid2subbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+1+mdctoffset+mid2subbandoffset[kk]-2];
						}
				}

				
					
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
					anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1]  ;
				}

				PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+1+mdctoffset+mid2subbandoffset[kk]-2]=data[i][j];
					
						if((channelnum==4)&&(kk==0)&&(j==4)&&(i>2))
						{
							Mdctout[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2]=0;
						}
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
					anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1]  ;
				}

				PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][i+mdctoffset+mid2subbandoffset[kk]-1]=data[i][j];
					

						}
				}
#endif
			
   free_matrix(anaMatrix, m, m);
   

	free_matrix(data, n, m);
	free_matrix(data2, n, m);
			}
          break;
		 case 256 :
			 bandnum=bandnumset[3];
        
			 		for(kk=0;kk<bandnum;kk++)
			{int n=shortsubbandoffset[kk+1]-shortsubbandoffset[kk];
				int i,j;
				float **data;
				float **data2;
				
				float **anaMatrix;
			 data = matrix(n, m);  /* Storage allocation for input data */

				data2 = matrix(n, m);  /* Storage allocation for input data */

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
				anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1]  ;
				}

				PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][2*i+mdctoffset+shortsubbandoffset[kk]-2]=data[i][j];
						if((channelnum==4)&&(kk==0)&&(j==4)&&(i>1))
						{
							Mdctout[j-1][2*i+mdctoffset+shortsubbandoffset[kk]-2]=0;
						}
				
						}
				}

				/////////////////////////////////////////////////////
								for (i = 1; i <= n; i++)
					{
					for (j = 1; j <= m; j++)
						{
					//	fscanf(stream, "%f	", &in_value);
						data[i][j] = Mdctin[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2];
						data2[i][j] = Mdctin[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2];
						}
					 }

						
				for (i = 1; i <= m; i++)
				{
					for (j = 1; j <= m; j++)
				anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][1][(i-1)*m+j-1]  ;
				}

				PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
				
						Mdctout[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2]=data[i][j];
					
							if((channelnum==4)&&(kk==0)&&(j==4)&&(i>1))
						{
							Mdctout[j-1][2*i+1+mdctoffset+shortsubbandoffset[kk]-2]=0;
						}
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
				anaMatrix[j][i] = anaMatrixdata[elementindex][index-1][kk][0][(i-1)*m+j-1]  ;
				}

				PCA_synthesis(data,data2,n,m,anaMatrix,usePCAitemnum);


				for (i = 1; i <= n; i++)
				{
					for (j = 1; j <= m; j++)
						{
					
						Mdctout[j-1][i+mdctoffset+shortsubbandoffset[kk]-1]=data[i][j];
					
							if((channelnum==4)&&(kk==0)&&(j==4)&&(i>2))
						{
							Mdctout[j-1][i+mdctoffset+shortsubbandoffset[kk]-1]=0;
						}
						}
				}

#endif
   free_matrix(anaMatrix, m, m);
    
	free_matrix(data, n, m);
	free_matrix(data2, n, m);
			}
		  break;
		  case 128 :
        
         
          break;
        }

       
	

	
		mdctoffset+=(ll*1);
	
		
}


	return 0;
}



int PCA_synthesis(float **data2,float **data,int  n, int  m,float **anaMatrix,int usePCAitemnum)

{

int   i, j, k, k2;
float  **symmat, *interm;






    interm = vector(m);    /* Storage alloc. for 'intermediate' vector */
  symmat = matrix(m, m); 
		 
 for (i = 1; i <= m; i++) 
	{
     for (j = 1; j <= m; j++) 
	 {
      symmat[i][j] = anaMatrix[i][m-j+1]; /* Needed below for col. projections */
                             
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
          for (k2 = 1; k2 <= usePCAitemnum/*m*/; k2++)
		  {
            data2[i][k] += interm[k2] * symmat[k][m-k2+1]; //symmat[m-k+1][k2]; //symmat[k][m-k2+1]; 
		  }
        }
     }


 

 free_matrix(symmat, m, m);
    free_vector(interm, m);

	return 0;

}
/**  Correlation matrix: creation  ***********************************/

void corcol(float **data, int n, int m,  float**symmat)

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
        stddev[j] += (   ( data[i][j] - mean[j] ) *
                         ( data[i][j] - mean[j] )  );
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
for (j1 = 1; j1 <= m-1; j1++)
    {
    symmat[j1][j1] = 1.0;
    for (j2 = j1+1; j2 <= m; j2++)
        {
        symmat[j1][j2] = 0.0;
        for (i = 1; i <= n; i++)
            {
            symmat[j1][j2] += ( data[i][j1] * data[i][j2]);
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
		mat[i] = (float *)malloc((unsigned)(m)*sizeof(float));
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
       free ((char*) (mat[i]+1));
       }
   free ((char*) (mat+1));
}

/**  Reduce a real, symmetric matrix to a symmetric, tridiag. matrix. */

void tred2(float **a, int n,float *d, float *e)

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
          g = f>0 ? -sqrt(h) : sqrt(h);
          e[i] = scale * g;
          h -= f * g;
          a[i][l] = f - g;
          f = 0.0;
          for (j = 1; j <= l; j++)
              {
              a[j][i] = a[i][j]/h;
              g = 0.0;
              for (k = 1; k <= j; k++)
                  g += a[j][k] * a[i][k];
              for (k = j+1; k <= l; k++)
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
    e[i-1] = e[i];
e[n] = 0.0;
for (l = 1; l <= n; l++)
    {
    iter = 0;
    do
      {
      for (m = l; m <= n-1; m++)
          {
          dd = fabs(d[m]) + fabs(d[m+1]);
          if (fabs(e[m]) + dd == dd) break;
          }
          if (m != l)
             {
             if (iter++ == 30) erhand("No convergence in TLQI.");
             g = (d[l+1] - d[l]) / (2.0 * e[l]);
             r = sqrt((g * g) + 1.0);
             g = d[m] - d[l] + e[l] / (g + SIGN(r, g));
             s = c = 1.0;
             p = 0.0;
             for (i = m-1; i >= l; i--)
                 {
                 f = s * e[i];
                 b = c * e[i];
                 if (fabs(f) >= fabs(g))
                    {
                    c = g / f;
                    r = sqrt((c * c) + 1.0);
                    e[i+1] = f * r;
                    c *= (s = 1.0/r);
                    }
                 else
                    {
                    s = f / g;
                    r = sqrt((s * s) + 1.0);
                    e[i+1] = g * r;
                    s *= (c = 1.0/r);
                    }
                 g = d[i+1] - p;
                 r = (d[i] - g) * s + 2.0 * c * b;
                 p = s * r;
                 d[i+1] = g + p;
                 g = c * r - b;
                 for (k = 1; k <= n; k++)
                     {
                     f = z[k][i+1];
                     z[k][i+1] = s * z[k][i] + c * f;
                     z[k][i] = c * z[k][i] - s * f;
                     }
                 }
                 d[l] = d[l] - p;
                 e[l] = g;
                 e[m] = 0.0;
             }
          }  while (m != l);
      }
 }

