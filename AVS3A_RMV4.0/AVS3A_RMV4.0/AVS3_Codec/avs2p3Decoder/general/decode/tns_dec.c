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
#include "avs2audio.h"
#include "lfdec.h"

#define min(a, b) ((a) < (b) ? (a) : (b))


/* 3 bit resolution */
const float tns_coeff3_dec[8]=
{
-0.98480775F,
-0.86602539F,
-0.64278758F,
-0.34202015F,
 0.00000000F,
 0.43388373F,
 0.78183150F,
 0.97492790F,
};

/* 4 bit resolution */

const float tns_coeff4_dec[16]=
{
-0.99573418F,
-0.96182567F,
-0.89516330F,
-0.79801726F,
-0.67369568F,
-0.52643222F,
-0.36124170F,
-0.18374953F,
 0.00000000F,
 0.20791170F,
 0.40673664F,
 0.58778524F,
 0.74314481F,
 0.86602539F,
 0.95105654F,
 0.99452192F
};

/*
    22050 Hz
*/
const unsigned int sfb_22050_long_dec[] = {
//      4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
//      4,  8,  8,  8,  8,  8,  8,  8,  8,  8,
//      8, 12, 12, 12, 12, 16, 16, 16, 20, 20,
//     24, 24, 28, 28, 32, 36, 36, 40, 44, 48,
//     52, 52, 64, 64, 64, 64, 64
	 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40,
	 44, 52, 60, 68, 76, 84, 92, 100, 108, 116,
	 124, 136, 148, 160, 172, 188, 204, 220, 240,
	 260, 284, 308, 336, 364, 396, 432, 468, 508, 
     552, 600, 652, 704, 768, 832, 896, 960, 1024
    
};
const unsigned int sfb_22050_short_dec[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64,
	 76, 92, 108, 128
};


const unsigned int sfb_22050_short_long_dec[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 56, 64,
	 72, 80, 92, 104, 116, 128, 144, 160, 176, 192, 212,
	 232, 256
};

const unsigned int sfb_22050_long_short_dec[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 44, 52, 60, 68,
	 76, 84, 92, 104, 116, 128, 144, 160, 176, 196, 216,
	 240, 264, 292, 328, 360, 396, 432, 472, 512
};


/*
    24000 Hz
*/
const unsigned int sfb_24000_long_dec[] = {
//      4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
//      4,  8,  8,  8,  8,  8,  8,  8,  8,  8,
//      8, 12, 12, 12, 12, 16, 16, 16, 20, 20,
//     24, 24, 28, 28, 32, 36, 36, 40, 44, 48,
//     52, 52, 64, 64, 64, 64, 64
	 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40,
	 44, 52, 60, 68, 76, 84, 92, 100, 108, 116,
	 124, 136, 148, 160, 172, 188, 204, 220, 240,
	 260, 284, 308, 336, 364, 396, 432, 468, 508, 
     552, 600, 652, 704, 768, 832, 896, 960, 1024
};
const unsigned int sfb_24000_short_dec[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64,
	 76, 92, 108, 128
};

const unsigned int sfb_24000_short_long_dec[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 56, 64,
	 72, 80, 92, 104, 116, 128, 144, 160, 176, 192, 212,
	 232, 256
};

const unsigned int sfb_24000_long_short_dec[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 44, 52, 60, 68,
	 76, 84, 92, 104, 116, 128, 144, 160, 176, 196, 216,
	 240, 264, 292, 328, 360, 396, 432, 472, 512
};

/*
  The function returns the minimum of 3 values.

  return:  minimum value
*/
static int Minimum(int a, /*!< value 1 */
                   int b, /*!< value 2 */
                   int c) /*!< value 3 */
{
  int t;

  t = (a < b ) ? a : b;
  return (t < c) ? t : c;
}

/*
  The function converts the decoded index values into
  parcor coefficients, which are used in the lattice
  filter.
*/
static void TnsDecodeCoefficients(TnsFilter *filter, /*!< pointer to filter side info */
                                  float *a)        /*!< pointer to parcor coefficients */
{
  int i;

  for (i=0; i < filter->Order; i++)
  {
    if (filter->Resolution == 3)
      a[i+1] = tns_coeff3_dec[filter->Coeff[i]+4];
    else
      a[i+1] = tns_coeff4_dec[filter->Coeff[i]+8];
  }

}


/*
  The function applies the conversion of the parcor coefficients
  to the lpc coefficients.
*/
static void 
TnsParcor2Lpc(float *parcor, /*!< pointer to parcor coefficients */
              float *lpc,    /*!< pointer to lpc coefficients */
              int order)     /*!< filter order */

{
  int i,j;
  float z1;
  float z[MaximumOrder2+1];
  float w[MaximumOrder2+1];
  float accu;


  {  
    for (i=0; i<MaximumOrder2+1; i++)
    {
      z[i] = 0.0;
      w[i] = 0.0;
    }

    for (i=0; i<=order; i++)
    {
      if (i == 0)
        accu = 1.0;
      else
        accu = 0.0;
      
      z1 = accu;
      
      for (j=0; j<order; j++)
      {
        w[j] = accu;

        accu += parcor[j] * z[j];
      }

      for (j=order-1; j>=0; j--)
      {
        z[j+1] = parcor[j] * w[j] + z[j];
      }

      z[0] = z1;
      lpc[i] = accu;
    }
  }

}

/*
  The function applies the tns filtering to the
  spectrum.
*/
static void 
TnsFilterIIR(float *spec,   /*!< pointer to spectrum */
             float *lpc,    /*!< pointer to lpc coefficients */
             float *state,  /*!< pointer to states */
             int size,      /*!< nunber of filtered spectral lines */
             char inc,      /*!< increment or decrement */
             int order)     /*!< filter order */

{
  int i,j;
  float accu;

  for (i=0; i<order; i++)
  {
    state[i] = 0.0F;
  }

  if (inc == -1)
  {
    spec += size-1;
  }

  for (i=0; i<size; i++)
  {
    accu = *spec * lpc[0];

    for (j=0; j<order; j++)
    {
      accu -= lpc[j+1] * state[j];
    }

    for (j=order-1; j>0; j--)
    {
      state[j] = state[j-1];
    }

    state[0] = accu;
    *spec = accu;

    spec += inc;
  }

}



/*
  The function applies the tns to the spectrum,
*/
void ApplyTns (tns_data *pTnsData, float *pSpectrum, int blocknum, int W, int encLen)
{
  float tnsState[MaximumOrder2];

  int index,start,stop,size;
  char n_filt;
  int sfb;

  float lpc[MaximumOrder2+1];
  float CoeffParc[MaximumOrder2+1];

  if (!pTnsData->TnsDataPresent[blocknum]) {
    return;
  }

  n_filt = pTnsData->TnsDataPresent[blocknum];

  if(n_filt)
  {
	  for (index=0; index < n_filt; index++)
	  {
		  TnsFilter *filter = &(pTnsData->Filter[blocknum][index]);

          TnsDecodeCoefficients(filter, CoeffParc);	  

		  start = filter->StartBand;

		  start = pTnsData->sfbOffset[W][start];

	      for (sfb = 0; sfb < pTnsData->sfbCnt[W]; sfb++){
			  if (pTnsData->sfbOffset[W][sfb] >= encLen)
				break;
		  }

		  stop = Minimum(filter->StopBand, sfb, pTnsData->tnsMaxSfb[W]);

		  stop = pTnsData->sfbOffset[W][stop];
      
		  size = stop - start;

		  if (size <= 0) continue;
		  if (filter->Order <= 0) continue;

		  TnsParcor2Lpc(&CoeffParc[1],lpc,filter->Order);

		  TnsFilterIIR(&pSpectrum[start],lpc,tnsState,size, filter->Direction, filter->Order);
	  }
  }

}

/*
  The function reads the elements for tns from
  the bitstream.
*/
void decodeTnsData(avs2audiopack_buffer *opb,  /*!< pointer to bitstream */
				   	int blocknum,
  				    int blocktype,
					int W,
                    tns_data *pTnsData) 
{
  char n_filt,order;
  char length,coef_res,coef_compress;  

  pTnsData->TnsDataPresent[blocknum] = (char) avs2audiopack_read(opb, 1);  

  if (!pTnsData->TnsDataPresent[blocknum]) {

    return;
  }

    n_filt = pTnsData->TnsDataPresent[blocknum];

    if (n_filt)
    {
      char index;
      char nextstopband;

      coef_res = (char) avs2audiopack_read(opb, 1);

      nextstopband = pTnsData->TotalSfBands[W];

      for (index=0; index < n_filt; index++)
      {
        TnsFilter *filter = &(pTnsData->Filter[blocknum][index]);

        length = (char)  avs2audiopack_read(opb, blocktype==2 ? 4 : 6);

        filter->StartBand = nextstopband - length;

        filter->StopBand  = nextstopband;

        nextstopband = filter->StartBand;

        filter->Order = order = (char)  avs2audiopack_read(opb,blocktype==2 ? 3 : 5);

        if (order)
        {
          char i,coef,s_mask,n_mask;
          static const char sgn_mask[] = {  0x2,  0x4,  0x8 };
          static const char neg_mask[] = { ~0x3, ~0x7, ~0xF };

          filter->Direction = (char)  avs2audiopack_read(opb,1) ? -1 : 1;

          coef_compress = (char) avs2audiopack_read(opb,1);

          filter->Resolution = coef_res + 3;

          s_mask = sgn_mask[coef_res + 1 - coef_compress];
          n_mask = neg_mask[coef_res + 1 - coef_compress];

          for (i=0; i < order; i++)
          {
            coef = (char) avs2audiopack_read(opb,filter->Resolution - coef_compress);

            if (coef & s_mask) {
              filter->Coeff[i] =  (coef | n_mask);
            } else {
              filter->Coeff[i] = coef;
            }
          }
        }
      }
    }

}

void InitTns(tns_data *TnsData, int sampleRate)
{
  int *sfboffset_long;
  int *sfboffset_short;
  int *sfboffset_short_long;
  int *sfboffset_long_short;

  TnsData->TotalSfBands[0] = 15;
  TnsData->TotalSfBands[1] = 47;
  TnsData->TotalSfBands[2] = 33;
  TnsData->TotalSfBands[3] = 25;

  switch(sampleRate) {
  case 22050:
	    TnsData->sfbCnt[0] = 15;
		sfboffset_short = sfb_22050_short_dec;
		TnsData->sfbCnt[1] = 47;
		sfboffset_long = sfb_22050_long_dec;
		TnsData->sfbCnt[2] = 33;
		sfboffset_long_short = sfb_22050_long_short_dec;
	    TnsData->sfbCnt[3] = 25;
		sfboffset_short_long = sfb_22050_short_long_dec;
		TnsData->tnsMaxSfb[0] = 14;
		TnsData->tnsMaxSfb[1] = 46;
		TnsData->tnsMaxSfb[2] = 36;
		TnsData->tnsMaxSfb[3] = 26;
  	break;
  case 24000:
	    TnsData->sfbCnt[0] = 15;
		sfboffset_short = sfb_22050_short_dec;
		TnsData->sfbCnt[1] = 47;
		sfboffset_long = sfb_22050_long_dec;
		TnsData->sfbCnt[2] = 33;
		sfboffset_long_short = sfb_22050_long_short_dec;
	    TnsData->sfbCnt[3] = 25;
		sfboffset_short_long = sfb_22050_short_long_dec;
		TnsData->tnsMaxSfb[0] = 14;
		TnsData->tnsMaxSfb[1] = 46;
		TnsData->tnsMaxSfb[2] = 36;
		TnsData->tnsMaxSfb[3] = 26;
    break;
  }

  memcpy(TnsData->sfbOffset[0], sfboffset_short, (TnsData->sfbCnt[0]+1)*sizeof(int));
  memcpy(TnsData->sfbOffset[1], sfboffset_long, (TnsData->sfbCnt[1]+1)*sizeof(int));
  memcpy(TnsData->sfbOffset[2], sfboffset_long_short, (TnsData->sfbCnt[2]+1)*sizeof(int));
  memcpy(TnsData->sfbOffset[3], sfboffset_short_long, (TnsData->sfbCnt[3]+1)*sizeof(int));

}

