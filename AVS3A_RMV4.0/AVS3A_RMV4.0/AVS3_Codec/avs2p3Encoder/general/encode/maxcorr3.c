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
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "maxcorr.h"
#include "filterbank.h"
#include "mc_rom.h"

#ifndef M_SQRT1_2
#define M_SQRT1_2 (0.707106781f)
#endif

static int mlog2(int x)
{
	int y = 0;
	if(x<=0) return -1;
	while(x>1)
	{
		x /= 2;
		y++;
	}
	return y;
}
/****************************************************************************************
 *           Maximal Coherence Rotation Angle Calculation
 *                                                                                       
 * Retrun:	- 0																			 
 * Params: 	- MAXCORR* 			corr_info	<- OUT, correlations and angles                
 *			- const float*		left		<- IN,	left channel vector     	
 *			- const float*      right		<- IN,  right channel vector
 *			- int				n			<- IN,  dimension of vector                 
 *                                                                                      
 ****************************************************************************************/
static int rotate_angle(MAXCORR*       corr_info,
                         const float*	left,
                         const float*	right,
                         int            n)
{
    int     i;
	int		j;
    float   enrg0, enrg1;
    float	enrgd, enrgx;
	float   xcorr;
    float   tan2;
    float   theta;
	float   corr0, corr1;

    enrg0  = 0.f;
    enrg1  = 0.f;    
    xcorr  = 0.f;
    for(i = 0; i < n; i++) 
    {       
        float tmp0;
        float tmp1;
        
        tmp0 = left[i]; 
        tmp1 = right[i];
        
        enrg0 += tmp0 * tmp0;
        enrg1 += tmp1 * tmp1;
        xcorr += tmp0 * tmp1;
    }

	enrgd = (enrg1 - enrg0) * .5f;
	enrgx = sqrt(enrgd * enrgd + xcorr * xcorr);

	if(xcorr==0.f) 
	{
		if(enrgd > 0.f)
			tan2 = FLT_MAX;
		else
			tan2 = -FLT_MAX;
	} 
	else 
	{
		tan2  = enrgd / xcorr;
	}
	
	theta = atan(tan2) * .5f;

	if(theta > 0.f && enrgd < 0.f)
		theta -= M_PI_2;
	else if(theta < 0.f && enrgd > 0.f)
		theta += M_PI_2;
	
	if(enrgx < 1.e-6)
		theta = 0.f;
	
    corr_info->theta		= theta;
    
	return 0;

}                       

/****************************************************************************************
 *          Maximal Coherence Roation Transform                      
 *                                                                                      
 * Retrun:	- 0                                                                         
 * Params: 	- float*			left		<- IN/OUT, left channel vector 	
 *			- float*      		right		<- IN/OUT, right channel vector  	
 *  		- int				n			<- IN, dimension of vector                  
 *			- float* 			theta   	<- IN, rotation angle                       
 *                                                                                      
 ****************************************************************************************/

static int rotate_transform(float*	 left,
                             float*  right, 
                             int     n, 
                             float   theta)
{
    float  c, s;
    int    i;
    
    c = cos(theta);
    s = sin(theta);
    
    for(i = 0; i < n; i++)
    {
        float tmpr0;
        float tmpr1;
        
        tmpr0 = left[i]; 
        tmpr1 = right[i];
        
        left[i]  =  c * tmpr0 + s * tmpr1;
        right[i] = -s * tmpr0 + c * tmpr1;
    }
    
    return 0;
}

/****************************************************************************************
 *          Couple Channel Downmix                    
 *                                                                                      
 * Retrun:	-                                                                          
 * Params: 	- float* 			mixed   	<- OUT, downmixed channel( first N/2 for even, later N/2 for odd)
 *			- float*			l_e			<- IN, even coefficient of left channel	
 *			- float*			r_e			<- IN, even coefficient of right channel	
 *			- float*      		l_o			<- IN, odd coefficient of left channel 	
 *			- float*      		r_o			<- IN, odd coefficient of right channel 	
 *  		- int				N			<- IN, length of coefficient                  		                                                                                                           
 ****************************************************************************************/

static void downmix2(float* mixed,
					 float* l_e, 
					 float* r_e, 
					 float* l_o, 
					 float* r_o,
					 int    N)
{
	int i;
	for(i = 0; i < N/2; i++) 
	{        
		mixed[i] = (l_e[i] + r_e[i]) / 2;
		mixed[i + N/2] = (l_o[i] + r_o[i]) / 2;
	}	
}



/****************************************************************************************
 *							  Vector Quantization                                      
 *                                                                                     
 * Retrun:	- float pointer to code vector                                             
 * Params: 	- float*      		cb_idx	<- OUT, code vector index                              
 *			- float*      		cb_pt	<- IN, codebook vector     
 *			- float*			vec		<- IN, input vector,
 *                                                                                      
 ****************************************************************************************/
static void vector_quantize(float *vec, int *cb_idx, float *cb_pt)
{
	float min = FLT_MAX;
	float tmp;

	int i, j;

	VQ = (int)pow(2, BIT_PER_VEC);

	for (i = 0; i < VQ; i++, cb_pt += VQ_DIM)
	{
		tmp = 0;
		for (j = 0; j < VQ_DIM; j++)
		{
			tmp += (vec[j] - cb_pt[j]) * (vec[j] - cb_pt[j]);
		}
		if (tmp < min)
		{
			min = tmp;
			*cb_idx = i;
		}
	}
}



static float* retrieve_theta_from_pos(int cb_idx, float* codebook)
{
	return (float*)(codebook + (cb_idx * VQ_DIM)); 
}

static void vec_quantize(int *cb_idx, float *vec, float *cb_pt, int size, int dim)
{
	float min = FLT_MAX;
	float tmp;

	int i, j;

	for (i = 0; i < size; i++, cb_pt += dim)
	{
		tmp = 0;
		for (j = 0; j < dim; j++)
		{
			tmp += (vec[j] - cb_pt[j]) * (vec[j] - cb_pt[j]);
		}
		if (tmp < min)
		{
			min = tmp;
			*cb_idx = i;
		}
	}
}

static void vec_quantize_weighted(int *cb_idx, float *vec, float *cb_pt, int size, int dim, float *eng_sum_set)
{
	float min = FLT_MAX;
	float tmp;

	int i, j;

	for (i = 0; i < size; i++, cb_pt += dim)
	{
		tmp = 0;
		for (j = 0; j < dim; j++)
		{
#if 1
			tmp += (vec[j] - cb_pt[j]) * (vec[j] - cb_pt[j])*sqrt(eng_sum_set[j]);
#else
			tmp = max(tmp, (vec[j] - cb_pt[j]) * (vec[j] - cb_pt[j]));
#endif
		}
		if (tmp < min)
		{
			min = tmp;
			*cb_idx = i;
		}
	}
}

static float *vec_retrieve(int cb_idx, float *cb_pt, int dim)
{
	return (float *)(cb_pt + (cb_idx * dim)); 
}

void ps_data_init(PS_DATA *ps_data)
{
	memset(ps_data, 0, sizeof(PS_DATA));
}

#if 1
int bitrate_init(ST_RATE_CONFIG *srate_info, int bitrate, int channel_number_index)
{
	if (channel_number_index == 1)  //s
	{
		if (bitrate <= 20)
		{
			memset(srate_info, 0, sizeof(ST_RATE_CONFIG));
			return 0;
		}
		else if (bitrate == 24)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 32)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 48)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 64)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 80)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][4], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 96)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][5], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 128)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][6], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 144)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][7], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 192)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][8], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 256)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][9], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 320)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][10], sizeof(ST_RATE_CONFIG));
		}
		else
		{
			return 0;
		}
		return 1;
	}

	else if (channel_number_index == 2)  //5.1
	{
		if (bitrate < 128)
		{
			memset(srate_info, 0, sizeof(ST_RATE_CONFIG));
			return 0;
		}
		else if (bitrate == 128)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 192)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 256)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 320)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][4], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 448)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][5], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 512)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][6], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 640)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][7], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 720)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][8], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 144)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][9], sizeof(ST_RATE_CONFIG));
		}
		else
		{
			return 0;
		}
		return 1;
	}

	else if (channel_number_index == 3) //7.1
	{
		if (bitrate == 192)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 480)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 256)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 576)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][4], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 640)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][5], sizeof(ST_RATE_CONFIG));
		}
		else
			return 0;
	}

	else if (channel_number_index == 6)  /* 4.0 */
	{
		if (bitrate == 48)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 96)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 128)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 192)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 256)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][4], sizeof(ST_RATE_CONFIG));
		}
		else
		{
			return 0;
		}
		return 1;
	}

	else if (channel_number_index == 7)    /* 5.1.2 */       //shumin.xu 20190313
	{
		if (bitrate == 152/*128 + 24*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 320/*256 + 64*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 480/*384 + 96*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 576/*448 + 128*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][3], sizeof(ST_RATE_CONFIG));
		}
		else
			return 0;
	}

	else if (channel_number_index == 8)  /* 5.1.4 */
	{
		if (bitrate == 176/*128 + 48*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384/*256 + 128*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 576/*384 + 192*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 704/*448 + 256*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 256/*176 + 80*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][4], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 448/*320 + 128*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][5], sizeof(ST_RATE_CONFIG));
		}
		else
			return 0;
	}

	else if (channel_number_index == 9)  /* 7.1.2 */
	{
		if (bitrate == 216/*192 + 24*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 480/*416 + 64*/)         //shumin.xu 20190215
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 576/*480 + 96*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384/*320 + 64*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 768/*640 + 128*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][4], sizeof(ST_RATE_CONFIG));
		}
		else
			return 0;
	}

	else if (channel_number_index == 10)  /* 7.1.4 */
	{
		if (bitrate == 240/*192 + 48*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 608/*480 + 128*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384/*288 + 96*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 512/*384 + 128*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 832/*576 + 256*/)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][4], sizeof(ST_RATE_CONFIG));
		}
		else
			return 0;
	}
	
	else if (channel_number_index == 11)  /* 16ch 3rd HOA, shumin.xu 210510 */
	{
		if (bitrate == 256)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 320)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 512)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 640)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][4], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 896)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][5], sizeof(ST_RATE_CONFIG));
		}
		else
			return 0;
	}
	else if (channel_number_index == 12) /* 9ch 2nd HOA, shumin.xu 211123*/
	{
		if (bitrate == 192)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 256)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 320)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 480)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][4], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 512)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][5], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 640)
		{
			memcpy(srate_info, &StRate_Config[channel_number_index][6], sizeof(ST_RATE_CONFIG));
		}
		else return 0;
	}
	else
	{
		return 0;
	}

	return 1;
}

#else
int bitrate_init(ST_RATE_CONFIG *srate_info, int bitrate, int nchannel)
{
	if(nchannel==2)
	{
		if(bitrate<=20)
		{
			memset(srate_info, 0 , sizeof(ST_RATE_CONFIG));
			return 0;
		}
		else if(bitrate==24)
		{
			memcpy(srate_info, &StRate_Config[0], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==32)
		{
			memcpy(srate_info, &StRate_Config[1], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==48)
		{
			memcpy(srate_info, &StRate_Config[2], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==64)
		{
			memcpy(srate_info, &StRate_Config[3], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==80)
		{
			memcpy(srate_info, &StRate_Config[4], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==96)
		{
			memcpy(srate_info, &StRate_Config[5], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==128)
		{
			memcpy(srate_info, &StRate_Config[6], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==144)
		{
			memcpy(srate_info, &StRate_Config[7], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==192)
		{
			memcpy(srate_info, &StRate_Config[8], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==256)
		{
			memcpy(srate_info, &StRate_Config[9], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==320)
		{
			memcpy(srate_info, &StRate_Config[10], sizeof(ST_RATE_CONFIG));
		}
		else
		{
			return 0;
		}
		return 1;
	}
	else if(nchannel==6)
	{
		if(bitrate<128)
		{
			memset(srate_info, 0 , sizeof(ST_RATE_CONFIG));
			return 0;
		}
		else if(bitrate==128)
		{
			memcpy(srate_info, &StRate_Config[11], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==192)
		{
			memcpy(srate_info, &StRate_Config[12], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==256)
		{
			memcpy(srate_info, &StRate_Config[13], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==320)
		{
			memcpy(srate_info, &StRate_Config[14], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==384)
		{
			memcpy(srate_info, &StRate_Config[15], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==448)
		{
			memcpy(srate_info, &StRate_Config[16], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==512)
		{
			memcpy(srate_info, &StRate_Config[17], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==640)
		{
			memcpy(srate_info, &StRate_Config[18], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==720)
		{
			memcpy(srate_info, &StRate_Config[19], sizeof(ST_RATE_CONFIG));
		}
		else
		{
			return 0;
		}
		return 1;
	}
	else if(nchannel==8)
	{
		if (bitrate == 192)
		{
			memcpy(srate_info, &StRate_Config[20], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 256)
		{
			memcpy(srate_info, &StRate_Config[21], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384)
		{
			memcpy(srate_info, &StRate_Config[22], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 448)
		{
			memcpy(srate_info, &StRate_Config[23], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate == 480)
		{
			memcpy(srate_info, &StRate_Config[24], sizeof(ST_RATE_CONFIG));
		}
		return 1;
	}
	else
	{
		return 0;
	}
}
#endif

void mcr_init(MCR_INFO *mcr_info, int brate, int useBWE)
{
	if(brate == 0)
	{
		mcr_info->index = 0;
		mcr_info->group_id = 1;
		mcr_info->group_thr[0] = 0.0;
		mcr_info->group_thr[1] = 0.0;

		mcr_info->sfb_num[0] = 9;
		mcr_info->sfb_num[1] = 12;
		mcr_info->sfb_num[2] = 24;
		mcr_info->sfb_num[3] = 36;
		memcpy(mcr_info->sfb_offset, McrSfb_0, sizeof(McrSfb_0));
		// initialize the vq info
		mcr_info->vq_dim[0] = 3;
		mcr_info->vq_dim[1] = 3;
		mcr_info->vq_dim[2] = 3;
		mcr_info->vq_dim[3] = 3;

		mcr_info->vq_size[0] = 256;
		mcr_info->vq_size[1] = 512;
		mcr_info->vq_size[2] = 512;
		mcr_info->vq_size[3] = 1024;

		mcr_info->bit_per_vec[0] = 8;
		mcr_info->bit_per_vec[1] = 9;
		mcr_info->bit_per_vec[2] = 9;
		mcr_info->bit_per_vec[3] = 10;

		mcr_info->pt_cbook[0] = codebk0_256_3;
		mcr_info->pt_cbook[1] = codebk0_512_3;
		mcr_info->pt_cbook[2] = codebk0_512_3;
		mcr_info->pt_cbook[3] = codebk0_1024_3;

		if(useBWE)
		{
			mcr_info->sfb_num[0] = 15;
			mcr_info->sfb_num[3] = 36;
			mcr_info->pt_mag_cbook[0] = codebk1_256_3;
			mcr_info->pt_mag_cbook[1] = codebk1_512_3;
			mcr_info->pt_mag_cbook[2] = codebk1_512_3;
			mcr_info->pt_mag_cbook[3] = codebk1_1024_3;

			mcr_info->pt_ang_cbook[0] = codebk2_256_3;
			mcr_info->pt_ang_cbook[1] = codebk2_512_3;
			mcr_info->pt_ang_cbook[2] = codebk2_512_3;
			mcr_info->pt_ang_cbook[3] = codebk2_1024_3;
		}
	}
	else if(brate < 12 )
	{
		mcr_info->index = 0;
		mcr_info->group_id = 1;
		if(brate==8)
		{
			mcr_info->group_thr[0] = 0.02f;
			mcr_info->group_thr[1] = 2.5f;
		}
		else if(brate==6)
		{
			mcr_info->group_thr[0] = 0.05f;
			mcr_info->group_thr[1] = 5.0f;
		}

		mcr_info->sfb_num[0] = 9;
		mcr_info->sfb_num[1] = 12;
		mcr_info->sfb_num[2] = 24;
		mcr_info->sfb_num[3] = 36;
		memcpy(mcr_info->sfb_offset, McrSfb_0, sizeof(McrSfb_0));
		// initialize the vq info
		mcr_info->vq_dim[0] = 3;
		mcr_info->vq_dim[1] = 3;
		mcr_info->vq_dim[2] = 3;
		mcr_info->vq_dim[3] = 3;

		mcr_info->vq_size[0] = 256;
		mcr_info->vq_size[1] = 512;
		mcr_info->vq_size[2] = 512;
		mcr_info->vq_size[3] = 1024;

		mcr_info->bit_per_vec[0] = 8;
		mcr_info->bit_per_vec[1] = 9;
		mcr_info->bit_per_vec[2] = 9;
		mcr_info->bit_per_vec[3] = 10;

		mcr_info->pt_cbook[0] = codebk0_256_3;
		mcr_info->pt_cbook[1] = codebk0_512_3;
		mcr_info->pt_cbook[2] = codebk0_512_3;
		mcr_info->pt_cbook[3] = codebk0_1024_3;

		if(useBWE)
		{
			mcr_info->sfb_num[0] = 18;
			mcr_info->sfb_num[3] = 48;
			mcr_info->pt_mag_cbook[0] = codebk1_256_3;
			mcr_info->pt_mag_cbook[1] = codebk1_512_3;
			mcr_info->pt_mag_cbook[2] = codebk1_512_3;
			mcr_info->pt_mag_cbook[3] = codebk1_1024_3;

			mcr_info->pt_ang_cbook[0] = codebk2_256_3;
			mcr_info->pt_ang_cbook[1] = codebk2_512_3;
			mcr_info->pt_ang_cbook[2] = codebk2_512_3;
			mcr_info->pt_ang_cbook[3] = codebk2_1024_3;
		}
	}
	else if(brate == 12)
	{
		mcr_info->index = 0;
		mcr_info->group_id = 0;
		mcr_info->group_thr[0] = 0.0f;
		mcr_info->group_thr[1] = 0.0f;

		mcr_info->sfb_num[0] = 9;
		mcr_info->sfb_num[1] = 12;
		mcr_info->sfb_num[2] = 24;
		mcr_info->sfb_num[3] = 36;
		memcpy(mcr_info->sfb_offset, McrSfb_0, sizeof(McrSfb_0));
		// initialize the vq info
		mcr_info->vq_dim[0] = 3;
		mcr_info->vq_dim[1] = 3;
		mcr_info->vq_dim[2] = 3;
		mcr_info->vq_dim[3] = 3;

		mcr_info->vq_size[0] = 256;
		mcr_info->vq_size[1] = 512;
		mcr_info->vq_size[2] = 512;
		mcr_info->vq_size[3] = 1024;

		mcr_info->bit_per_vec[0] = 8;
		mcr_info->bit_per_vec[1] = 9;
		mcr_info->bit_per_vec[2] = 9;
		mcr_info->bit_per_vec[3] = 10;

		mcr_info->pt_cbook[0] = codebk0_256_3;
		mcr_info->pt_cbook[1] = codebk0_512_3;
		mcr_info->pt_cbook[2] = codebk0_512_3;
		mcr_info->pt_cbook[3] = codebk0_1024_3;

		if(useBWE)
		{
			mcr_info->sfb_num[0] = 18;
			mcr_info->sfb_num[3] = 48;
			mcr_info->pt_mag_cbook[0] = codebk1_256_3;
			mcr_info->pt_mag_cbook[1] = codebk1_512_3;
			mcr_info->pt_mag_cbook[2] = codebk1_512_3;
			mcr_info->pt_mag_cbook[3] = codebk1_1024_3;

			mcr_info->pt_ang_cbook[0] = codebk2_256_3;
			mcr_info->pt_ang_cbook[1] = codebk2_512_3;
			mcr_info->pt_ang_cbook[2] = codebk2_512_3;
			mcr_info->pt_ang_cbook[3] = codebk2_1024_3;
		}
	}
	else if(brate == 16)
	{
		mcr_info->index = 1;
		mcr_info->group_id = 0;
		mcr_info->group_thr[0] = 0.0f;
		mcr_info->group_thr[1] = 0.0f;

		mcr_info->sfb_num[0] = 9;
		mcr_info->sfb_num[1] = 15;
		mcr_info->sfb_num[2] = 30;
		mcr_info->sfb_num[3] = 48;
		memcpy(mcr_info->sfb_offset, McrSfb_1, sizeof(McrSfb_1));
		// initialize the vq info
		mcr_info->vq_dim[0] = 3;
		mcr_info->vq_dim[1] = 3;
		mcr_info->vq_dim[2] = 3;
		mcr_info->vq_dim[3] = 3;

		mcr_info->vq_size[0] = 512;
		mcr_info->vq_size[1] = 512;
		mcr_info->vq_size[2] = 512;
		mcr_info->vq_size[3] = 1024;

		mcr_info->bit_per_vec[0] = 9;
		mcr_info->bit_per_vec[1] = 9;
		mcr_info->bit_per_vec[2] = 9;
		mcr_info->bit_per_vec[3] = 10;

		mcr_info->pt_cbook[0] = codebk0_512_3;
		mcr_info->pt_cbook[1] = codebk0_512_3;
		mcr_info->pt_cbook[2] = codebk0_512_3;
		mcr_info->pt_cbook[3] = codebk0_1024_3;

	}
	else if(brate == 20)
	{
		mcr_info->index = 2;
		mcr_info->group_id = 0;
		mcr_info->group_thr[0] = 0.0f;
		mcr_info->group_thr[1] = 0.0f;

		mcr_info->sfb_num[0] = 9;
		mcr_info->sfb_num[1] = 18;
		mcr_info->sfb_num[2] = 36;
		mcr_info->sfb_num[3] = 48;
		memcpy(mcr_info->sfb_offset, McrSfb_2, sizeof(McrSfb_2));
		// initialize the vq info
		mcr_info->vq_dim[0] = 3;
		mcr_info->vq_dim[1] = 3;
		mcr_info->vq_dim[2] = 3;
		mcr_info->vq_dim[3] = 2;

		mcr_info->vq_size[0] = 512;
		mcr_info->vq_size[1] = 1024;
		mcr_info->vq_size[2] = 1024;
		mcr_info->vq_size[3] = 256;

		mcr_info->bit_per_vec[0] = 9;
		mcr_info->bit_per_vec[1] = 10;
		mcr_info->bit_per_vec[2] = 10;
		mcr_info->bit_per_vec[3] = 8;

		mcr_info->pt_cbook[0] = codebk0_512_3;
		mcr_info->pt_cbook[1] = codebk0_1024_3;
		mcr_info->pt_cbook[2] = codebk0_1024_3;
		mcr_info->pt_cbook[3] = codebk0_256_2;

	}
	else if(brate == 24)
	{
		mcr_info->index = 3;
		mcr_info->group_id = 0;
		mcr_info->group_thr[0] = 0.0f;
		mcr_info->group_thr[1] = 0.0f;

		mcr_info->sfb_num[0] = 12;
		mcr_info->sfb_num[1] = 18;
		mcr_info->sfb_num[2] = 36;
		mcr_info->sfb_num[3] = 64;
		memcpy(mcr_info->sfb_offset, McrSfb_4, sizeof(McrSfb_4));
		// initialize the vq info
		mcr_info->vq_dim[0] = 3;
		mcr_info->vq_dim[1] = 3;
		mcr_info->vq_dim[2] = 3;
		mcr_info->vq_dim[3] = 2;

		mcr_info->vq_size[0] = 512;
		mcr_info->vq_size[1] = 1024;
		mcr_info->vq_size[2] = 1024;
		mcr_info->vq_size[3] = 256;

		mcr_info->bit_per_vec[0] = 9;
		mcr_info->bit_per_vec[1] = 10;
		mcr_info->bit_per_vec[2] = 10;
		mcr_info->bit_per_vec[3] = 8;

		mcr_info->pt_cbook[0] = codebk0_512_3;
		mcr_info->pt_cbook[1] = codebk0_1024_3;
		mcr_info->pt_cbook[2] = codebk0_1024_3;
		mcr_info->pt_cbook[3] = codebk0_256_2;
	}
}

/****************************************************************************************
 *          Encoding with MDCT domain Maximal Coherence Rotation                        
 *          (need to calculate the angle info)                                                                             
 * Return:	- None                                                                      
 * Params: 	- MAXCORR* 			corr_info	<- OUT, MCR angles
 *			- float*			freq_mono	<- OUT,	downmixed channel   
 *			- float*			freq_left	<- IN,  left channel  
 *			- float*			freq_right	<- IN,  right channel,               
 *			- int				len			<- IN,  MDCT total length
 *          - int				nblock		<- IN,  number of MDCT blocks                                                                            
 ****************************************************************************************/

int mcr_encode(PS_DATA	*ps_data,
			   MCR_INFO	mcr_info,
			   int		len,
			   int		pos,
			   int		mcrpos,
			   int		flag)
{

	int		i, j;
	int     sfb, vq_bit, vq_dim, vq_size;
	int		sfnum, nsfb;
	short	*sfband;
	float	*cbook;

	float	band_left_even[FRAMESIZE*2], band_left_odd[FRAMESIZE*2];
	float	band_right_even[FRAMESIZE*2], band_right_odd[FRAMESIZE*2];
	float	band_mixed[FRAMESIZE*2];
	float	*band_l_e_pt, *band_l_o_pt, *band_r_e_pt, *band_r_o_pt;

	int nblock	= 1;//mcr_info.group_id + 1;
	int len0	= len / nblock;
	int id		= mlog2(len0/128);
	int encLen  = (ps_data->bandwidth * len0 + 512)/1024;

	MAXCORR *pt_mcr = ps_data->mcr_data + mcrpos;

	sfnum	= mcr_info.sfb_num[id];
	vq_dim	= mcr_info.vq_dim[id];
	vq_size	= mcr_info.vq_size[id];
	vq_bit	= mcr_info.bit_per_vec[id];
	sfband	= mcr_info.sfb_offset[id];
	cbook	= mcr_info.pt_cbook[id];

	//printf("*******mcr_encode*****\n");

	// rearrange mdct coeffs
	for(j = 0; j < len0/2; j++)
	{
		for(i = 0; i < nblock; i++)
		{
			band_left_even[j*nblock+i]	= ps_data->left_data[j*2+i*len0+pos];
			band_right_even[j*nblock+i] = ps_data->right_data[j*2+i*len0+pos];

			band_left_odd[j*nblock+i]	= ps_data->left_data[j*2+1+i*len0+pos];
			band_right_odd[j*nblock+i]	= ps_data->right_data[j*2+1+i*len0+pos];
		}
	}

	if(flag==1)
	{
	band_l_e_pt = band_left_even;
	band_l_o_pt = band_left_odd;
	band_r_e_pt = band_right_even;
	band_r_o_pt = band_right_odd;
	
	// get maxcorr rotation angle
	for(sfb = 0; sfb < sfnum; sfb++)
	{
		int n;
		n = sfband[sfb+1] - sfband[sfb];

		//calc theta for even freq
		rotate_angle( pt_mcr + sfb,
					  band_l_e_pt,
					  band_r_e_pt,
					  n * nblock);
		//calc kappa for odd freq
		rotate_angle( pt_mcr + MAX_NSF + sfb,
					  band_l_o_pt,
					  band_r_o_pt,
					  n * nblock);

		band_l_e_pt += n * nblock;
		band_l_o_pt += n * nblock;
		band_r_e_pt += n * nblock;
		band_r_o_pt += n * nblock;
	}

	// vector quantize rotation angles
	for(sfb = 0; sfb < sfnum; sfb += vq_dim)
	{
		float 	vec[4];
		int		cb_idx;
		int		j;
		//vector quantize
		for(j = 0; j < vq_dim; j++)
		{
			vec[j] = pt_mcr[sfb + j].theta;
		}
		vec_quantize(&cb_idx, vec, cbook, vq_size, vq_dim);
		for(j = 0; j < vq_dim; j++)
		{
			pt_mcr[sfb + j].theta_pos = cb_idx;
			pt_mcr[sfb + j].theta_q = cbook[cb_idx * vq_dim + j];
		}

		//vector quantize
		for(j = 0; j < vq_dim; j++)
		{
			vec[j] = pt_mcr[sfb + MAX_NSF + j].theta;
		}
		vec_quantize(&cb_idx, vec, cbook, vq_size, vq_dim);
		for(j = 0; j < vq_dim; j++)
		{
			pt_mcr[sfb + MAX_NSF + j].theta_pos = cb_idx;
			pt_mcr[sfb + MAX_NSF + j].theta_q = cbook[cb_idx * vq_dim + j];
		}
	}
	/*for(sfb = nsfb; sfb < sfnum; sfb++)
	{
		pt_mcr[sfb].theta_q = pt_mcr[sfb - 1].theta_q;
		pt_mcr[sfb + MAX_NSF].theta_q = pt_mcr[sfb + MAX_NSF - 1].theta_q;
	}*/
	}

	//rotate(transform)
	band_l_e_pt = band_left_even;
	band_l_o_pt = band_left_odd;
	band_r_e_pt = band_right_even;
	band_r_o_pt = band_right_odd;
	for(sfb = 0; sfb < sfnum; sfb++)
	{
		int n = sfband[sfb+1] - sfband[sfb];
		if (sfband[sfb+1]<encLen/2) nsfb = sfb + 1;

		rotate_transform(band_l_e_pt,
						  band_r_e_pt,
						  n * nblock,
						  pt_mcr[sfb].theta_q);

		rotate_transform(band_l_o_pt,
						  band_r_o_pt,
						  n * nblock,
						  pt_mcr[sfb + MAX_NSF].theta_q);

		band_l_e_pt += n * nblock;
		band_l_o_pt += n * nblock;
		band_r_e_pt += n * nblock;
		band_r_o_pt += n * nblock;
	}
	
	nsfb = (nsfb + vq_dim - 1) / vq_dim * vq_dim;
	//downmix and rearrange
	for(j = 0; j < len0/2; j++)
	{
		for(i = 0; i < nblock; i++)
		{
			ps_data->sum_data[j*2+i*len0+pos] = (band_left_even[j*nblock+i] + band_right_even[j*nblock+i]) / 2;
			ps_data->dif_data[j*2+i*len0+pos] = (band_left_even[j*nblock+i] - band_right_even[j*nblock+i]) / 2;

			ps_data->sum_data[j*2+1+i*len0+pos] = (band_left_odd[j*nblock+i] + band_right_odd[j*nblock+i]) / 2;
			ps_data->dif_data[j*2+1+i*len0+pos] = (band_left_odd[j*nblock+i] - band_right_odd[j*nblock+i]) / 2;
		}
	}
	
	return nsfb;
}

int MCR_Encoder(avs2audiopack_buffer	*data_mcr,
				PS_DATA					*ps_data,
				MCR_INFO				mcr_info,
				PS_DATA					*ps_data_pre)
{
	int i, numBytes;
	int tt,jj,kk,ll;
	int index;
	int pos0, pos1;
	int id, vq_dim, vq_bit, sfnum;
	int encLen;

	memset(data_mcr, 0, sizeof(avs2audiopack_buffer));
	avs2audiopack_writeinit(data_mcr);

	if(mcr_info.group_id==1&&ps_data->winseq[0]==1&&ps_data_pre->winseq[0]==1&&ps_data_pre->sflag==0)
	{
		float tmp0,tmp1,tmp2;
		MAXCORR mcr_temp[MAX_NSF*2];
		float sum_temp[FRAMESIZE];
		float dif_temp[FRAMESIZE];

		float sum_eng, dif_eng0, dif_eng1;

		id = 3;
		ll = CORE_FRAMESIZE;
		encLen  = (ps_data->bandwidth*ll+512)/1024;
		sfnum	= mcr_info.sfb_num[id];
		vq_dim	= mcr_info.vq_dim[id];
		vq_bit	= mcr_info.bit_per_vec[id];

		kk = mcr_encode(ps_data, mcr_info, ll, 0, 0, 0);
		memcpy(mcr_temp, ps_data->mcr_data, MAX_NSF * 2 * sizeof(MAXCORR));
		memcpy(sum_temp, ps_data->sum_data, ll * sizeof(float));
		memcpy(dif_temp, ps_data->dif_data, ll * sizeof(float));

		kk = mcr_encode(ps_data, mcr_info, ll, 0, 0, 1);

		sum_eng  = 0.0f;
		dif_eng0 = 0.0f;
		dif_eng1 = 0.0f;
		for(i = 0; i < encLen; i++)
		{
			sum_eng  += sum_temp[i] * sum_temp[i];
			dif_eng0 += dif_temp[i] * dif_temp[i];
			dif_eng1 += ps_data->dif_data[i] * ps_data->dif_data[i];
		}
		sum_eng += dif_eng0;

		tmp0 = dif_eng0/(sum_eng+1.0);
		tmp1 = dif_eng1/(sum_eng+1.0);
		tmp2 = dif_eng0/(dif_eng1+1.0);
		/*{
			FILE* fp0 = fopen("ratio.txt","a");
			fprintf(fp0,"%5.4f, %5.4f, %4.2f\n", tmp0, tmp1, tmp2);
			fclose(fp0);
		}*/

		if(tmp0<mcr_info.group_thr[0] || (/*tmp1<mcr_info.group_thr[0]&&*/tmp2<mcr_info.group_thr[1]))
		{
			memcpy(ps_data->mcr_data, mcr_temp, MAX_NSF * 2 * sizeof(MAXCORR));
			memcpy(ps_data->sum_data, sum_temp, ll * sizeof(float));
			memcpy(ps_data->dif_data, dif_temp, ll * sizeof(float));

			ps_data->sflag = 1;
		}
		else
		{
			avs2audiopack_write(data_mcr, mcr_info.index, 2);
			for(i = 0; i < kk; i += vq_dim)
			{
				avs2audiopack_write(data_mcr, ps_data->mcr_data[i].theta_pos, vq_bit);
				avs2audiopack_write(data_mcr, ps_data->mcr_data[i+MAX_NSF].theta_pos, vq_bit);
			}
			ps_data->sflag = 0;
		}
	}
	else
	{
		pos0 = 0;
		pos1 = 0;

		avs2audiopack_write(data_mcr, mcr_info.index, 2);

		for(index = 1; index < (ps_data->winseq[0]+1); index++)
		{
			tt = ps_data->winseq[index] - 1;
			jj = ps_data->winseq[index+1] - 1;

			ll = MAX(LL[tt], LL[jj]) / 2;
			encLen  = (ps_data->bandwidth*ll+512)/1024;
			id		= mlog2(ll/128);
			sfnum	= mcr_info.sfb_num[id];
			vq_dim	= mcr_info.vq_dim[id];
			vq_bit	= mcr_info.bit_per_vec[id];

			kk = mcr_encode(ps_data, mcr_info, ll, pos1, pos0, 1);

			for(i = 0; i < kk; i += vq_dim)
			{
				avs2audiopack_write(data_mcr, ps_data->mcr_data[i+pos0].theta_pos, vq_bit);
				avs2audiopack_write(data_mcr, ps_data->mcr_data[i+pos0+MAX_NSF].theta_pos, vq_bit);
			}
			
			pos0 += sfnum;
			pos1 += ll;
		}
		ps_data->sflag = 0;
	}

	/* 2014.09.02 - adding byte alignement */
/*	if(data_mcr->endbit>0)
	{
		avs2audiopack_write(data_mcr, 0, (8-data_mcr->endbit));
	}*/
	numBytes = data_mcr->endbyte;

	memcpy(ps_data_pre, ps_data, sizeof(PS_DATA));

	return numBytes;
}





int mcr_bwe_encode(PS_BWE_DATA	*psBweData,
				   MCR_INFO	mcr_info,
				   int		len,
				   int		nblock,
				   int		pos,
				   int		mcrpos,
				   int		flag)
{

	int		i, j;
	int     sfb, vq_bit, vq_dim, vq_size;
	int		sfnum, nsfb;
	short	*sfband;
	float	*mag_cb, *ang_cb;

	float	mag_left[FRAMESIZE*2], mag_right[FRAMESIZE*2];
	float	ang_left[FRAMESIZE*2], ang_right[FRAMESIZE*2];
	float	*mag_l, *mag_r;// *ang_l, *ang_r;

	int len0	= len * nblock;
	int id		= mlog2(len/256);
	int encLen  = (psBweData->bandwidth * len + 512)/1024;

	MAXCORR *pt_mcr = psBweData->mcr_data + mcrpos;
	MAXCORR *pt_ang = psBweData->ang_data + mcrpos;

	float eng_sum_set[100];
	sfnum	= mcr_info.sfb_num[id];
	vq_dim	= mcr_info.vq_dim[id];
	vq_size	= mcr_info.vq_size[id];
	vq_bit	= mcr_info.bit_per_vec[id];
	sfband	= mcr_info.sfb_offset[id];
	mag_cb	= mcr_info.pt_mag_cbook[id];
	ang_cb	= mcr_info.pt_ang_cbook[id];

	for(i = 0; i < len; i++)
	{
		for(j = 0; j < nblock; j++)
		{
			float tmp;
			int id0 = pos + (j * len + i) * 2;
			int id1 = i * nblock + j;
			
			tmp = psBweData->left_data[id0]*psBweData->left_data[id0]+psBweData->left_data[id0+1]*psBweData->left_data[id0+1];
			mag_left[id1] = sqrt(tmp);
			//ang_left[id1] = atan2(psBweData->left_data[id0+1], psBweData->left_data[id0]);
			ang_left[id1] = atan2(psBweData->left_data[id0 + 1] + psBweData->right_data[id0 + 1], psBweData->left_data[id0] + psBweData->right_data[id0]);

			tmp = psBweData->right_data[id0]*psBweData->right_data[id0]+psBweData->right_data[id0+1]*psBweData->right_data[id0+1];
			mag_right[id1] = sqrt(tmp);
			ang_right[id1] = atan2(psBweData->right_data[id0+1], psBweData->right_data[id0]);
	
		}
	}

	mag_l = mag_left;
	mag_r = mag_right;
	for (sfb = 0; sfb < sfnum; sfb++)
	{
		int n = (sfband[sfb + 1] - sfband[sfb]) * 4;
		eng_sum_set[sfb] = 0;
		for (j = 0; j < n * nblock; j++)
		{
			eng_sum_set[sfb] += (mag_left[j] * mag_left[j] + mag_right[j] * mag_right[j]);
		}
		eng_sum_set[sfb] = sqrt(eng_sum_set[sfb]);
		mag_l += n * nblock;
		mag_r += n * nblock;
	}
	if(flag == 1)
	{
		mag_l = mag_left;
		mag_r = mag_right;
	
		// get maxcorr rotation angle
		for(sfb = 0; sfb < sfnum; sfb++)
		{
			int n = (sfband[sfb+1] - sfband[sfb]) * 4;

			//calc theta for freq
			rotate_angle(pt_mcr + sfb, mag_l, mag_r, n * nblock);

			mag_l += n * nblock;
			mag_r += n * nblock;
		}

		// vector quantize rotation angles
		for(sfb = 0; sfb < sfnum; sfb += vq_dim)
		{
			float 	vec[4];
			int		cb_idx;
			//vector quantize
			for(j = 0; j < vq_dim; j++)
			{
				vec[j] = pt_mcr[sfb + j].theta;
			}
#if 0
			vec_quantize(&cb_idx, vec, mag_cb, vq_size, vq_dim);
#else
			vec_quantize_weighted(&cb_idx, vec, mag_cb, vq_size, vq_dim, eng_sum_set + sfb);
#endif
			for(j = 0; j < vq_dim; j++)
			{
				pt_mcr[sfb + j].theta_pos = cb_idx;
				pt_mcr[sfb + j].theta_q = mag_cb[cb_idx * vq_dim + j];
			}
		}
	}

	//rotate(transform)
	mag_l = mag_left;
	mag_r = mag_right;
	for(sfb = 0; sfb < sfnum; sfb++)
	{
		int n = (sfband[sfb+1] - sfband[sfb]) * 4;
		if (sfband[sfb+1]*4<=encLen) nsfb = sfb + 1;

		rotate_transform(mag_l, mag_r, n * nblock, pt_mcr[sfb].theta_q);

		mag_l += n * nblock;
		mag_r += n * nblock;
	}
	
	nsfb = (nsfb + vq_dim - 1) / vq_dim * vq_dim;
	//downmix and rearrange
	for(j = 0; j < len0; j++)
	{
		mag_left[j] = (mag_left[j] + mag_right[j]) / 2;
		mag_right[j] = mag_left[j] - mag_right[j];
	}
	
	for(sfb = 0; sfb < sfnum; sfb++)
	{
		float tmp0 = 0.0;
		float tmp1 = 0.0;

		for(j = sfband[sfb]*4; j < sfband[sfb+1]*4; j++)
		{
			tmp0 += mag_left[j];
			tmp1 += (ang_right[j] - ang_left[j]) * mag_left[j];
		}
		pt_ang[sfb].theta = tmp1/(tmp0+0.1);
	}
	// vector quantize angle differences
	for(sfb = 0; sfb < sfnum; sfb += vq_dim)
	{
		float 	vec[4];
		int		cb_idx;
		//vector quantize
		for(j = 0; j < vq_dim; j++)
		{
			vec[j] = pt_ang[sfb + j].theta;
		}
#if 0
		vec_quantize(&cb_idx, vec, ang_cb, vq_size, vq_dim);
#else
		vec_quantize_weighted(&cb_idx, vec, ang_cb, vq_size, vq_dim, eng_sum_set + sfb);
#endif
		for(j = 0; j < vq_dim; j++)
		{
			pt_ang[sfb + j].theta_pos = cb_idx;
			pt_ang[sfb + j].theta_q = ang_cb[cb_idx * vq_dim + j];
		}
	}
	
	for(i = 0; i < len; i++)
	{
		for(j = 0; j < nblock; j++)
		{
			int id0 = i * nblock + j;
			int id1 = (j * len + i) * 2 + pos;
			psBweData->sum_data[id1] = mag_left[id0] * cos(ang_left[id0]);
			psBweData->sum_data[id1+1] = mag_left[id0] * sin(ang_left[id0]);

			psBweData->dif_data[id1] = mag_right[id0] * cos(ang_right[id0]);
			psBweData->dif_data[id1+1] = mag_right[id0] * sin(ang_right[id0]);
		}
	}
	
	return nsfb;
}


int MCR_BWE_Encoder(int bitRate,
					float					*timeOutput,
					avs2audiopack_buffer	*data_mcr,
					PS_BWE_DATA				*psBweData,
					MCR_INFO				mcrInfo,
					PS_BWE_DATA				*psBweData_pre)
{
	int i, numBytes;
	int tt,jj,kk,ll;
	int index;
	int pos0, pos1;
	int id, vq_dim, vq_bit, sfnum;
	int encLen;
	int (*WinseqTabletmp)[20];
	int (*BlockTabletmp)[16];
	int (*GroupTabletmp)[10];
	int blockNum, blockStart, groupNum, groupStart;
	float timeBuffer[4096+2048];

	BlockTabletmp	= (int(*)[16])getBlockseqTable(psBweData->Groupmode);
	GroupTabletmp	= (int(*)[10])getGroupseqTable(psBweData->Groupmode);
	WinseqTabletmp	= (int(*)[20])getWinseqTable(psBweData->Groupmode);

	memset(data_mcr, 0, sizeof(avs2audiopack_buffer));
	avs2audiopack_writeinit(data_mcr);

	/*if(mcrInfo.group_id==1&&GroupTabletmp[psBweData->Seqmode][1]>1)
	{
		MAXCORR mcr_temp[MAX_NSF*2];
		MAXCORR ang_temp[MAX_NSF*2];
		float sum_temp[FRAMESIZE];
		float dif_temp[FRAMESIZE];

		float sum_eng, dif_eng0, dif_eng1;

		memcpy(mcr_temp, psBweData->mcr_data, MAX_NSF * 2 * sizeof(MAXCORR));
		memcpy(ang_temp, psBweData->ang_data, MAX_NSF * 2 * sizeof(MAXCORR));
		memcpy(sum_temp, psBweData->sum_data, ll * 2 * sizeof(float));
		memcpy(dif_temp, psBweData->dif_data, ll * 2 * sizeof(float));

		blockStart = WinseqTabletmp[psBweData->Seqmode][2];
		blockNum = WinseqTabletmp[psBweData->Seqmode][3];
		ll = BlockTabletmp[psBweData->Seqmode][2];
		id = mlog2(ll/256);

		encLen  = (psBweData->bandwidth*ll+512)/1024;

		sfnum	= mcrInfo.sfb_num[id];
		vq_dim	= mcrInfo.vq_dim[id];
		vq_bit	= mcrInfo.bit_per_vec[id];

		kk = mcr_bwe_encode(psBweData, mcrInfo, ll, blockNum, 0, 0, 1);
		
		

		kk = mcr_bwe_encode(psBweData, mcrInfo, ll, 1, 0, 0, 1);

		sum_eng  = 0.0f;
		dif_eng0 = 0.0f;
		dif_eng1 = 0.0f;
		for(i = 0; i < encLen*2; i++)
		{
			sum_eng  += sum_temp[i] * sum_temp[i];
			dif_eng0 += dif_temp[i] * dif_temp[i];
			dif_eng1 += psBweData->dif_data[i] * psBweData->dif_data[i];
		}
		sum_eng += dif_eng0;

		if(dif_eng0/(sum_eng+1.0)<mcrInfo.group_thr[0] || dif_eng0/(dif_eng1+1.0)<mcrInfo.group_thr[1])
		{
			memcpy(psBweData->mcr_data, mcr_temp, MAX_NSF * 2 * sizeof(MAXCORR));
			memcpy(psBweData->ang_data, ang_temp, MAX_NSF * 2 * sizeof(MAXCORR));
			memcpy(psBweData->sum_data, sum_temp, ll * 2 * sizeof(float));
			memcpy(psBweData->dif_data, dif_temp, ll * 2 * sizeof(float));

			psBweData->sflag = 1;
		}
		else
		{
			for(i = 0; i < kk; i += vq_dim)
			{
				avs2audiopack_write(data_mcr, psBweData->mcr_data[i].theta_pos, vq_bit);
				avs2audiopack_write(data_mcr, psBweData->ang_data[i].theta_pos, vq_bit);
			}
			psBweData->sflag = 0;
		}
	}
	else*/
	{

		
		int ngp = GroupTabletmp[psBweData->Seqmode][1];
		pos0 = 0;
		pos1 = 0;

        if(bitRate >= 48000)
		{
			for(index = 0; index < (GroupTabletmp[psBweData->Seqmode][ngp+2]); index++)
			{   
			   groupStart = index;
			   groupNum = 1;

				ll = BlockTabletmp[psBweData->Seqmode][groupStart+2];

				encLen  = (psBweData->bandwidth*ll+512)/1024;
				id		= mlog2(ll/256);
				sfnum	= mcrInfo.sfb_num[id];
				vq_dim	= mcrInfo.vq_dim[id];
				vq_bit	= mcrInfo.bit_per_vec[id];

				kk = mcr_bwe_encode(psBweData, mcrInfo, ll, groupNum, pos1, pos0, 1);

				for(i = 0; i < kk; i += vq_dim)
				{
					avs2audiopack_write(data_mcr, psBweData->mcr_data[i+pos0].theta_pos, vq_bit);
					//avs2audiopack_write(data_mcr, psBweData->ang_data[i+pos0].theta_pos, vq_bit);
				}
				
				pos0 += sfnum;
				pos1 += ll * groupNum * 2;
			}
		}
        else{
			for(index = 0; index < (GroupTabletmp[psBweData->Seqmode][1]); index++)
			{			
				groupStart = GroupTabletmp[psBweData->Seqmode][index+2];
				groupNum = GroupTabletmp[psBweData->Seqmode][index+3] - groupStart;

				ll = BlockTabletmp[psBweData->Seqmode][groupStart+2];


				encLen  = (psBweData->bandwidth*ll+512)/1024;
				id		= mlog2(ll/256);
				sfnum	= mcrInfo.sfb_num[id];
				vq_dim	= mcrInfo.vq_dim[id];
				vq_bit	= mcrInfo.bit_per_vec[id];

				kk = mcr_bwe_encode(psBweData, mcrInfo, ll, groupNum, pos1, pos0, 1);

				for(i = 0; i < kk; i += vq_dim)
				{
					avs2audiopack_write(data_mcr, psBweData->mcr_data[i+pos0].theta_pos, vq_bit);
					//avs2audiopack_write(data_mcr, psBweData->ang_data[i+pos0].theta_pos, vq_bit);
				}
				
				pos0 += sfnum;
				pos1 += ll * groupNum * 2;
			}
        }

		psBweData->sflag = 0;
	}

	/* 2014.09.02 - adding byte alignement */
//	if(data_mcr->endbit>0)
//	{
//		avs2audiopack_write(data_mcr, 0, (8-data_mcr->endbit));
//	}
	numBytes = data_mcr->endbyte;

	memcpy(psBweData_pre, psBweData, sizeof(PS_BWE_DATA));

	imdft_frame4096block_multi(timeOutput,WinseqTabletmp[psBweData->Seqmode]+3,psBweData->sum_data,WinseqTabletmp[psBweData->Seqmode][2]+1024,psBweData->timeBuffer);

	for(i = 0; i < CORE_FRAMESIZE * 2; i++)
		 timeOutput[i] *= 0.5f;

	return numBytes;
}

