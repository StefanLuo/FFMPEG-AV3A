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

/*
  Temporal Noise Shaping
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "tns1.h"
#include "lfenc.h"

/*
  TNS constants
*/

/* 3 bit resolution */
const float tns_coeff3[8]=
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

const float tns_coeff3Borders[8]={
-1.0f,
-0.9396926324f,
-0.7660444587f,
-0.5000000126f,
-0.1736481824f,
0.2225209400f,
0.6234898165f,
0.9009688814f,
};

/* 4 bit resolution */

const float tns_coeff4[16]=
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

const float tns_coeff4Borders[16]=
{
  -1.0f,
  -0.9829731068f,
  -0.9324722415f,
  -0.8502171506f,
  -0.7390089328f,
  -0.6026346507f,
  -0.4457383673f,
  -0.2736629975f,
  -0.0922683620f,
  0.1045284662f,
  0.3090170027f,
  0.5000000126f,
  0.6691306215f,
  0.8090170098f,
  0.9135454707f,
  0.9781476086f
};

/*
    22050 Hz
*/
const unsigned int sfb_22050_long[] = {
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
const unsigned int sfb_22050_short[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64,
	 76, 92, 108, 128
};


const unsigned int sfb_22050_short_long[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 56, 64,
	 72, 80, 92, 104, 116, 128, 144, 160, 176, 192, 212,
	 232, 256
};

const unsigned int sfb_22050_long_short[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 44, 52, 60, 68,
	 76, 84, 92, 104, 116, 128, 144, 160, 176, 196, 216,
	 240, 264, 292, 328, 360, 396, 432, 472, 512
};


/*
    24000 Hz
*/
const unsigned int sfb_24000_long[] = {
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
const unsigned int sfb_24000_short[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64,
	 76, 92, 108, 128
};

const unsigned int sfb_24000_short_long[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 56, 64,
	 72, 80, 92, 104, 116, 128, 144, 160, 176, 192, 212,
	 232, 256
};

const unsigned int sfb_24000_long_short[] = {
//     4,  4,  4,  4,  4,  4,  4,  8,  8,  8,
//     12, 12, 16, 16, 20
	 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 44, 52, 60, 68,
	 76, 84, 92, 104, 116, 128, 144, 160, 176, 196, 216,
	 240, 264, 292, 328, 360, 396, 432, 472, 512
};


const tns_config_tabulated  tns_16000_mono_long ={
  1.2f,
  2000,
  16000,
  0.6f
};
const tns_config_tabulated  tns_16000_mono_short ={
  1.2f,
  3750,
  16000,
  0.6f
};
const tns_config_tabulated  tns_16000_mono_long_short ={
  1.2f,
  2000,
  16000,
  0.6f
};
const tns_config_tabulated  tns_16000_mono_short_long ={
  1.2f,
  3750,
  16000,
  0.6f
};

const tns_config_tabulated tns_24000_mono_long ={
  1.41f,
  2500,
  16000,
  0.5f
};
const tns_config_tabulated tns_24000_mono_short ={
  1.41f,
  3750,
  16000,
  0.5f
};
const tns_config_tabulated tns_24000_mono_long_short ={
  1.41f,
  2500,
  16000,
  0.5f
};
const tns_config_tabulated tns_24000_mono_short_long ={
  1.41f,
  3750,
  16000,
  0.5f
};

const tns_config_tabulated tns_32000_mono_long ={
  1.41f,
  2750,//2500,  lijing 2750
  16000,
  0.8f
};
const tns_config_tabulated tns_32000_mono_short ={
  1.41f,
  3750,
  16000,
  0.3f
};
const tns_config_tabulated tns_32000_mono_long_short ={
  1.41f,
  2750,//2500,  lijing 2750
  16000,
  0.8f
};
const tns_config_tabulated tns_32000_mono_short_long ={
  1.41f,
  3750,
  16000,
  0.3f
};

const tns_config_tabulated tns_44000more_mono_long ={
  1.41f,
  2750,//2500,  lijing 2750
  18000,
  0.8f
};
const tns_config_tabulated tns_44000more_mono_short ={
  1.41f,
  3750,
  18000,
  0.3f
};
const tns_config_tabulated tns_44000more_mono_long_short ={
  1.41f,
  2750,//2500,  lijing 2750
  18000,
  0.8f
};
const tns_config_tabulated tns_44000more_mono_short_long ={
  1.41f,
  3750,
  18000,
  0.3f
};


/*****************************************************************************

    functionname: FreqToBandWithRounding
    description:  Returns index of nearest band border
    returns:
    input:        frequency, sampling frequency, total number of bands,
                  table of band borders
    output:
    globals:

*****************************************************************************/
static int FreqToBandWithRounding(int freq, int fs, int numOfBands,
                                  const int *bandStartOffset)
{
  int lineNumber, band;



  lineNumber = (freq*bandStartOffset[numOfBands]*4/fs+1)/2;

  /* freq > fs/2 */
  if (lineNumber >= bandStartOffset[numOfBands])
  {
    return numOfBands;
  }

  /* find band the line number lies in */
  for (band=0; band<numOfBands; band++) {
    if (bandStartOffset[band+1]>lineNumber) break;
  }

  /* round to nearest band border */
  if (lineNumber - bandStartOffset[band] >
      bandStartOffset[band+1] - lineNumber )
    {
      band++;
    }


  return band;
};

/*****************************************************************************

    functionname: CalcGaussWindow
    edscription:  calculates Gauss window for acf windowing depending on desired
                  temporal resolution, transform size and sampling rate
    returns:      -
    input:        window size, fs, bitRate, no. of transform lines, time res.
    output:       window coefficients (right half)

*****************************************************************************/
static void CalcGaussWindow(float  *win,
                            const int winSize,
                            const int samplingRate,
                            const int windowLen,//const int blockType,//
                            const float timeResolution )
{
  int     i;
  float gaussExp = 3.14159265358979323f * samplingRate * 0.001f * (float)timeResolution / (float)windowLen/*(blockType != SHORT_WINDOW ? 1024.0f:128.0f)*/;

    gaussExp = -0.5f * gaussExp * gaussExp;

    for(i=0; i<winSize; i++) {

      win[i] = (float) exp( gaussExp * (i+0.5) * (i+0.5) );
    }

}

/*****************************************************************************

    functionname: InitTnsConfiguration
    description:  fill TNS_CONFIG structure with sensible content
    returns:
    input:        bitrate, samplerate, number of channels,
                  TNS Config struct (modified),
                  psy config struct,
                  tns active flag
    output:

*****************************************************************************/
int init_tns_configuration(   int bitRate,
                            long sampleRate,
                            int channels,
							int bandWidth,
                            tns_config *tC,
                            int active) 
{
  int i, len;
  int sfb;
  int *sfboffset_long;
  int *sfboffset_short;
  int *sfboffset_short_long;
  int *sfboffset_long_short;

  tC[0].maxOrder     = TNS_MAX_ORDER_SHORT;
  tC[0].tnsStartFreq = 2750;
  tC[0].coefRes      = 3;

  tC[1].maxOrder     = TNS_MAX_ORDER;
  tC[1].tnsStartFreq = 1275;
  tC[1].coefRes      = 4;

  tC[2].maxOrder     = TNS_MAX_ORDER_LONG_SHORT; //512
  tC[2].tnsStartFreq = 2750;
  tC[2].coefRes      = 4;

  tC[3].maxOrder     = TNS_MAX_ORDER_SHORT_LONG; //256
  tC[3].tnsStartFreq = 1275;
  tC[3].coefRes      = 3;
  
  if(bitRate<24000)
  {
	  tC[0].confTab = tns_16000_mono_short;
	  tC[1].confTab = tns_16000_mono_long;
	  tC[2].confTab = tns_16000_mono_long_short;
	  tC[3].confTab = tns_16000_mono_short_long;
  }
  else if(bitRate<32000)
  {
	  tC[0].confTab = tns_24000_mono_short;
	  tC[1].confTab = tns_24000_mono_long;
	  tC[2].confTab = tns_24000_mono_long;
	  tC[3].confTab = tns_24000_mono_short_long;
  }
  else if(bitRate<44000)
  {
	  tC[0].confTab = tns_32000_mono_short;
	  tC[1].confTab = tns_32000_mono_long;
	  tC[2].confTab = tns_32000_mono_long_short;
	  tC[3].confTab = tns_32000_mono_short_long;
  }
  else
  {
	  tC[0].confTab = tns_44000more_mono_short;
	  tC[1].confTab = tns_44000more_mono_long;
	  tC[2].confTab = tns_44000more_mono_long_short;
	  tC[3].confTab = tns_44000more_mono_short_long;
 
  }

  switch(sampleRate) {
  case 22050:
	    tC[0].sfbCnt = 15;
		sfboffset_short = sfb_22050_short;
		tC[1].sfbCnt = 47;
		sfboffset_long = sfb_22050_long;
		tC[2].sfbCnt = 33;
		sfboffset_long_short = sfb_22050_long_short;
	    tC[3].sfbCnt = 25;
		sfboffset_short_long = sfb_22050_short_long;
		tC[0].tnsMaxSfb = 14;
		tC[1].tnsMaxSfb = 46;
		tC[2].tnsMaxSfb = 36;
		tC[3].tnsMaxSfb = 26;
  	break;
  case 24000:
	    tC[0].sfbCnt = 15;
		sfboffset_short = sfb_24000_short;
		tC[1].sfbCnt = 47;
		sfboffset_long = sfb_24000_long;
		tC[2].sfbCnt = 33;
		sfboffset_long_short = sfb_24000_long_short;
	    tC[3].sfbCnt = 25;
		sfboffset_short_long = sfb_24000_short_long;
		tC[0].tnsMaxSfb = 14;
		tC[1].tnsMaxSfb = 46;
		tC[2].tnsMaxSfb = 36;
		tC[3].tnsMaxSfb = 26;
	break;
  }

  memcpy(tC[0].sfbOffset, sfboffset_short, (tC[0].sfbCnt+1)*sizeof(int));
  memcpy(tC[1].sfbOffset, sfboffset_long, (tC[1].sfbCnt+1)*sizeof(int));
  memcpy(tC[2].sfbOffset, sfboffset_long_short, (tC[2].sfbCnt+1)*sizeof(int));
  memcpy(tC[3].sfbOffset, sfboffset_short_long, (tC[3].sfbCnt+1)*sizeof(int));

  for(i = 0; i < 4; i++)
  {
	  switch(i) {
	  case 0:
			len = FRAME_LEN_SHORT;
	  	break;
	  case 1:
            len = FRAME_LEN_LONG;
		break;
	  case 2:
			len = FRAME_LEN_LONG / 2;
		break;
	  case 3:
			len = FRAME_LEN_LONG / 4;
		break;
	  }

      CalcGaussWindow(tC[i].acfWindow, 
                  tC[i].maxOrder+1,
                  sampleRate, 
                  len,
                  tC[i].confTab.tnsTimeResolution);

      
	  tC[i].tnsActive = 1;
	  if (active==0)  
        tC[i].tnsActive=0;

	  /*now calc band and line borders */

	  tC[i].tnsStopBand=min(tC[i].sfbCnt, tC[i].tnsMaxSfb);

	  tC[i].tnsStopLine = tC[i].sfbOffset[tC[i].tnsStopBand];

      tC[i].tnsStartBand=FreqToBandWithRounding(tC->tnsStartFreq, sampleRate,
                                          tC[i].sfbCnt, tC[i].sfbOffset);

	  tC[i].tnsStartLine = tC[i].sfbOffset[tC[i].tnsStartBand];
	  
      tC[i].tnsModifyBeginCb = FreqToBandWithRounding(TNS_MODIFY_BEGIN,
                                                sampleRate,
                                                tC[i].sfbCnt,
                                                tC[i].sfbOffset);

      tC[i].tnsRatioPatchLowestCb = FreqToBandWithRounding(RATIO_PATCH_LOWER_BORDER,
                                                     sampleRate,
                                                     tC[i].sfbCnt,
                                                     tC[i].sfbOffset);
  
      tC[i].lowpassline = bandWidth * len / 1024/*2 * bandWidth * len / sampleRate*/;

      for (sfb = 0; sfb < tC[i].sfbCnt; sfb++){
      if (tC[i].sfbOffset[sfb] >= tC[i].lowpassline)
        break;
	  }

      tC[i].sfbActive  = sfb;

      tC[i].lpcStopBand=FreqToBandWithRounding(tC[i].confTab.lpcStopFreq, sampleRate,
                                         tC[i].sfbCnt, tC[i].sfbOffset);

      tC[i].lpcStopBand=min(tC[i].lpcStopBand, tC[i].sfbActive);

      tC[i].lpcStopLine = tC[i].sfbOffset[tC[i].lpcStopBand];

      tC[i].lpcStartBand=FreqToBandWithRounding(tC[i].confTab.lpcStartFreq, sampleRate,
                                           tC[i].sfbCnt, tC[i].sfbOffset);

      tC[i].lpcStartLine = tC[i].sfbOffset[tC[i].lpcStartBand];

      tC[i].threshold =tC[i].confTab.threshOn;
  }

  return 0;
}

/*****************************************************************************

    functionname: CalcWeightedSpectrum
    description:  calculate weighted spectrum for LPC calculation
    returns:      -
    input:        input spectrum, ptr. to weighted spectrum, no. of lines,
                  sfb energies
    output:       weighted spectrum coefficients

*****************************************************************************/
static void CalcWeightedSpectrum(const float    spectrum[],
                                 float          weightedSpectrum[],
                                 float        *sfbEnergy,
                                 const int     *sfbOffset,
                                 int            lpcStartLine,
                                 int            lpcStopLine,
                                 int            lpcStartBand,
                                 int            lpcStopBand)
{
    int     i, sfb;
    float   tmp;
    float   tnsSfbMean[MAX_SFB];    /* length [lpcStopBand-lpcStartBand] should be sufficient here */


    /* calc 1/sqrt(en) */
    for( sfb = lpcStartBand; sfb < lpcStopBand; sfb++){

      tnsSfbMean[sfb] = (float) ( 1.0 / sqrt(sfbEnergy[sfb] + 1e-30f) );
    }

    /* spread normalized values from sfbs to lines */
    sfb = lpcStartBand;

    tmp = tnsSfbMean[sfb];
    for ( i=lpcStartLine; i<lpcStopLine; i++){

        if (sfbOffset[sfb+1]==i){

            sfb++;

            if (sfb+1 < lpcStopBand){

                tmp = tnsSfbMean[sfb];
            }
        }

        weightedSpectrum[i] = tmp;
    }

    /*filter down*/
    for (i=lpcStopLine-2; i>=lpcStartLine; i--){
      weightedSpectrum[i] = (weightedSpectrum[i] + weightedSpectrum[i+1]) * 0.5f;
    }

    /* filter up */
    for (i=lpcStartLine+1; i<lpcStopLine; i++){

      weightedSpectrum[i] = (weightedSpectrum[i] + weightedSpectrum[i-1]) * 0.5f;
    }

    /* weight and normalize */
    for (i=lpcStartLine; i<lpcStopLine; i++){

      weightedSpectrum[i] = weightedSpectrum[i] * spectrum[i];
    }

}

/*****************************************************************************

    functionname: AutoToParcor
    description:  conversion autocorrelation to reflection coefficients
    returns:      prediction gain
    input:        <order+1> input values, no. of output values (=order),
                  ptr. to workbuffer (required size: 2*order)
    output:       <order> reflection coefficients

*****************************************************************************/
static float AutoToParcor(const float input[], float reflCoeff[], int numOfCoeff,
                          float workBuffer[]) {
  int i, j;
  float  *pWorkBuffer; /* temp pointer */
  float predictionGain = 1.0f;


  for(i=0;i<numOfCoeff;i++)
  {
    reflCoeff[i] = 0;
  }

  if(input[0] == 0.0)
  {
    return(predictionGain);
  }

  for(i=0; i<numOfCoeff; i++) {

    workBuffer[i] = input[i];
    workBuffer[i+numOfCoeff] = input[i+1];
  }

  for(i=0; i<numOfCoeff; i++) {
    float refc, tmp;

    tmp = workBuffer[numOfCoeff + i];

    if(tmp < 0.0)
    {
      tmp = -tmp;
    }

    if(workBuffer[0] < tmp)
      break;

    if(workBuffer[0] == 0.0f) {

      refc = 0.0f;
    } else {

      refc = tmp / workBuffer[0];
    }

    if(workBuffer[numOfCoeff + i] > 0.0)
    {
      refc = -refc;
    }

    reflCoeff[i] = refc;

    pWorkBuffer = &(workBuffer[numOfCoeff]);

    for(j=i; j<numOfCoeff; j++) {
      float accu1, accu2;

      accu1 = pWorkBuffer[j]  + refc * workBuffer[j-i];
      accu2 = workBuffer[j-i] + refc * pWorkBuffer[j];
      pWorkBuffer[j] = accu1;
      workBuffer[j-i] = accu2;
    }
  }

  predictionGain = (input[0] + 1e-30f) / (workBuffer[0] + 1e-30f);


  return(predictionGain);
}

/*****************************************************************************

    functionname: AutoCorrelation
    description:  calc. autocorrelation (acf)
    returns:      -
    input:        input values, no. of input values, no. of acf values
    output:       acf values

*****************************************************************************/
static void AutoCorrelation(const float input[],
                            float       corr[],
                            int         samples,
                            int         corrCoeff) {
    int         i, j;
    float       accu;
    int         scf;

 
    /* 
      get next power of 2 of samples 
    */
    for(scf=0;(1<<scf) < samples;scf++);
   
    accu = 0.0;

    for(j=0; j<samples; j++) {

      accu += input[j] * input[j];
    }

    corr[0] = accu;

    samples--;

    for(i=1; i<corrCoeff; i++) {

        accu = 0.0;

        for(j=0; j<samples; j++) {

          accu += input[j] * input[j+i];
        }

        corr[i] = accu;

        samples--;
    }

}    


/*****************************************************************************

    functionname: CalcTnsFilter
    description:  LPC calculation for one TNS filter
    returns:      prediction gain
    input:        signal spectrum, acf window, no. of spectral lines,
                  max. TNS order, ptr. to reflection ocefficients
    output:       reflection coefficients

*****************************************************************************/
static float CalcTnsFilter(const float *signal,
                            const float window[],
                            int numOfLines,
                            int tnsOrder,
                            float parcor[])
{
    float   autoCorrelation[TNS_MAX_ORDER+1];
    float   parcorWorkBuffer[2*TNS_MAX_ORDER];
    float  predictionGain;
    int     i;


    AutoCorrelation(signal, autoCorrelation, numOfLines, tnsOrder+1);

    if(window) {

        for(i=0; i<tnsOrder+1; i++) {

          autoCorrelation[i] = autoCorrelation[i] * window[i];
        }
    }

    predictionGain = AutoToParcor(autoCorrelation, parcor, tnsOrder, parcorWorkBuffer);


    return(predictionGain);
}

/*****************************************************************************
    functionname: TnsDetect
    description:  do decision, if TNS shall be used or not
    returns:
    input:        tns data structure (modified),
                  tns config structure,
                  pointer to scratch space,
                  scalefactor size and table,
                  spectrum,
                  subblock num, blocktype,
                  sfb-wise energy.

*****************************************************************************/
int tns_detect(tns_info* tnsInfo,
              tns_config tC,
//              float* pScratchTns,
              float* spectrum,
              int subBlockNumber
              /*int blockType,*/
              /*float * sfbEnergy*/)
{
  float predictionGain;
//  float* pWeightedSpectrum = pScratchTns + subBlockNumber*FRAME_LEN_SHORT;

  if (tC.tnsActive) {

        predictionGain = CalcTnsFilter( &spectrum[tC.lpcStartLine],
                                        tC.acfWindow,
                                        tC.lpcStopLine-tC.lpcStartLine,
                                        tC.maxOrder,
                                        tnsInfo->subblockInfo[subBlockNumber].parcor);

        tnsInfo->subblockInfo[subBlockNumber].tnsActive =
            (predictionGain > tC.threshold)?1:0;

        tnsInfo->subblockInfo[subBlockNumber].predictionGain = predictionGain;
//    }

  }
  else{
        tnsInfo->subblockInfo[subBlockNumber].tnsActive = 0;
        tnsInfo->subblockInfo[subBlockNumber].predictionGain = 0.0f;
  }


  return 0;
}


static int Search3(float parcor)
{
  int index=0;
  int i;

  for(i=0;i<8;i++){

    if(parcor > tns_coeff3Borders[i])
    {
      index=i;
    }
  }


  return(index-4);
}

static int Search4(float parcor)
{
  int index=0;
  int i;

  for(i=0;i<16;i++){

    if(parcor > tns_coeff4Borders[i])
    {
      index=i;
    }
  }


  return(index-8);
}

/*****************************************************************************

    functionname: Parcor2Index

*****************************************************************************/
static void Parcor2Index(const float parcor[], int index[], int order,
                         int bitsPerCoeff) {
  int i;

  for(i=0; i<order; i++) {

    if(bitsPerCoeff == 3)
    {
      index[i] = Search3(parcor[i]);
    }
    else
    {
      index[i] = Search4(parcor[i]);
    }
  }

}


/*****************************************************************************

    functionname: Index2Parcor
    description:  inverse quantization for reflection coefficients
    returns:      -
    input:        quantized values, ptr. to reflection coefficients,
                  no. of coefficients, resolution
    output:       reflection coefficients

*****************************************************************************/
static void Index2Parcor(const int index[], float parcor[], int order,
                         int bitsPerCoeff) {
  int i;

  for(i=0; i<order; i++) {

    parcor[i] = bitsPerCoeff == 4 ? tns_coeff4[index[i]+8] : tns_coeff3[index[i]+4];
  }

}



/*****************************************************************************

    functionname: FIRLattice

*****************************************************************************/
static float FIRLattice(int order, 
                        float x,
                        float *state_par,
                        const float *coef_par)
{
   int i;
   float accu, tmp, tmpSave;

   tmpSave = x;

   for(i=0; i<order-1; i++) {

     tmp      = coef_par[i] * x;

     tmp     += state_par[i];

     accu     = coef_par[i] * state_par[i];

     accu    += x;

     x        = accu;
     state_par[i] = tmpSave;
     tmpSave  = tmp;
  }

  /* last stage: only need half operations */

  accu  = state_par[order-1] * coef_par[order-1];

  accu += x;

  x     = accu;
  state_par[order-1] = tmpSave;


  return x;
}



/*****************************************************************************

    functionname: AnalysisFilterLattice
    description:  TNS analysis filter
    returns:      -
    input:        input signal values, no. of lines, PARC coefficients,
                  filter order, ptr. to output values, filtering direction
    output:       filtered signal values

*****************************************************************************/
static void AnalysisFilterLattice(const float signal[], int numOfLines,
                                  const float parCoeff[], int order,
                                  float output[])
{

  float state_par[TNS_MAX_ORDER] = {0.0f};
  int j;

  for(j=0; j<numOfLines; j++) {

    output[j] = FIRLattice(order,signal[j],state_par,parCoeff);
  }

}


/*****************************************************************************
    functionname: TnsEncode
    description:

*****************************************************************************/
int tns_encode(tns_info* tnsInfo,
//              TNS_DATA* tnsData,
//              int numOfSfb,
              tns_config tC,
              int lowPassLine,
              float* spectrum,
              int subBlockNumber/*,
              int blockType*/)
{
  int i;

    if (tnsInfo->subblockInfo[subBlockNumber].tnsActive == 0) {

      tnsInfo->tnsActive[subBlockNumber] = 0;

      return(0);
    }
    else {

      Parcor2Index(tnsInfo->subblockInfo[subBlockNumber].parcor,
                   &tnsInfo->coef[subBlockNumber*TNS_MAX_ORDER_SHORT],
                   tC.maxOrder,
                   tC.coefRes);

      Index2Parcor(&tnsInfo->coef[subBlockNumber*TNS_MAX_ORDER_SHORT],
                   tnsInfo->subblockInfo[subBlockNumber].parcor,
                   tC.maxOrder,
                   tC.coefRes);

      for (i=tC.maxOrder-1; i>=0; i--)  {

        if (tnsInfo->subblockInfo[subBlockNumber].parcor[i]>TNS_PARCOR_THRESH  ||
            tnsInfo->subblockInfo[subBlockNumber].parcor[i]<-TNS_PARCOR_THRESH)
          break;
      }

      tnsInfo->order[subBlockNumber]=i+1;

      tnsInfo->tnsActive[subBlockNumber]=1;

      tnsInfo->coefRes[subBlockNumber]=tC.coefRes;

      tnsInfo->length[subBlockNumber]= tC.sfbCnt-tC.tnsStartBand;


      AnalysisFilterLattice(&(spectrum[tC.tnsStartLine]), min(tC.tnsStopLine, tC.lowpassline)-tC.tnsStartLine,
                 tnsInfo->subblockInfo[subBlockNumber].parcor,
                 tnsInfo->order[subBlockNumber],
                 &(spectrum[tC.tnsStartLine]));
      
    }


  return 0;
}

