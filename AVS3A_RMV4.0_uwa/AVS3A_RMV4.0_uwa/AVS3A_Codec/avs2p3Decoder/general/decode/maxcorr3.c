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
#include "mc_rom.h"

#ifndef M_SQRT1_2
#define M_SQRT1_2 (0.707106781f)
#endif

#define MAX_CH (6)		// maximum channel number to support

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
 *                                                                                      
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
		mixed[i] = l_e[i] + r_e[i];
		mixed[i] /= 2;
		mixed[i + N/2] = l_o[i] + r_o[i];
		mixed[i + N/2] /= 2;
	}
	
}

static void apply_window(float* ws, const float* win)
{
	int i;
	int j;

	for(i = 0, j = BLOCKSIZE - 1; i < OVERLAP; i++, j--)
	{
		ws[i]  *=  win[i];
		ws[j]  *=  win[i];
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

static float *vec_retrieve(int cb_idx, float *cb_pt, int dim)
{
	return (float *)(cb_pt + (cb_idx * dim)); 
}

int bitrate_init(ST_RATE_CONFIG *srateInfo, int bitrate, int nchannel, int headchannel)
{
	if(nchannel==1)
	{
		memset(srateInfo, 0 , sizeof(ST_RATE_CONFIG));
		
		srateInfo->nchannel = 1;
		srateInfo->brate = bitrate;
		srateInfo->core_brate[0] = bitrate;
	}
	if(nchannel==2)
	{
		if(bitrate<=20)
		{
			memset(srateInfo, 0 , sizeof(ST_RATE_CONFIG));
			return 0;
		}
		else if(bitrate==24)
		{
			memcpy(srateInfo, &StRate_Config[1][0], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==32)
		{
			memcpy(srateInfo, &StRate_Config[1][1], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==48)
		{
			memcpy(srateInfo, &StRate_Config[1][2], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==64)
		{
			memcpy(srateInfo, &StRate_Config[1][3], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==80)
		{
			memcpy(srateInfo, &StRate_Config[1][4], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==96)
		{
			memcpy(srateInfo, &StRate_Config[1][5], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==128)
		{
			memcpy(srateInfo, &StRate_Config[1][6], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==144)
		{
			memcpy(srateInfo, &StRate_Config[1][7], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==192)
		{
			memcpy(srateInfo, &StRate_Config[1][8], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==256)
		{
			memcpy(srateInfo, &StRate_Config[1][9], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==320)
		{
			memcpy(srateInfo, &StRate_Config[1][10], sizeof(ST_RATE_CONFIG));
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
			memset(srateInfo, 0 , sizeof(ST_RATE_CONFIG));
			return 0;
		}
		else if(bitrate==128)
		{
			memcpy(srateInfo, &StRate_Config[2][0], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==192)
		{
			memcpy(srateInfo, &StRate_Config[2][1], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==256)
		{
			memcpy(srateInfo, &StRate_Config[2][2], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==320)
		{
			memcpy(srateInfo, &StRate_Config[2][3], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==384)
		{
			memcpy(srateInfo, &StRate_Config[2][4], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==448)
		{
			memcpy(srateInfo, &StRate_Config[2][5], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==512)
		{
			memcpy(srateInfo, &StRate_Config[2][6], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==640)
		{
			memcpy(srateInfo, &StRate_Config[2][7], sizeof(ST_RATE_CONFIG));
		}
		else if(bitrate==720)
		{
			memcpy(srateInfo, &StRate_Config[2][8], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 144)
		{
			memcpy(srateInfo, &StRate_Config[2][9], sizeof(ST_RATE_CONFIG));
		}
		else
		{
			return 0;
		}
		return 1;
	}
	else if (nchannel == 8)
	{
		if (headchannel == 0) /* 7.1 */
		{
			if (bitrate == 192)
			{
				memcpy(srateInfo, &StRate_Config[3][0], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 480)
			{
				memcpy(srateInfo, &StRate_Config[3][1], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 256)
			{
				memcpy(srateInfo, &StRate_Config[3][2], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 384)
			{
				memcpy(srateInfo, &StRate_Config[3][3], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 576)
			{
				memcpy(srateInfo, &StRate_Config[3][4], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 640)
			{
				memcpy(srateInfo, &StRate_Config[3][5], sizeof(ST_RATE_CONFIG));
			}
			else
				return 0;
		}
		else if (headchannel == 2) /*5.1.2*/
		{
			if (bitrate == 152/*128 + 24*/)
			{
				memcpy(srateInfo, &StRate_Config[7][0], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 320/*256 + 64*/)
			{
				memcpy(srateInfo, &StRate_Config[7][1], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 480/*384 + 96*/)
			{
				memcpy(srateInfo, &StRate_Config[7][2], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 576/*448 + 128*/)
			{
				memcpy(srateInfo, &StRate_Config[7][3], sizeof(ST_RATE_CONFIG));
			}
			else
				return 0;
		}
		
		return 1;
	}
	else if (nchannel == 4)
	{
		if (bitrate == 48)
		{
			memcpy(srateInfo, &StRate_Config[6][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 96)
		{
			memcpy(srateInfo, &StRate_Config[6][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 128)
		{
			memcpy(srateInfo, &StRate_Config[6][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 192)
		{
			memcpy(srateInfo, &StRate_Config[6][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 256)
		{
			memcpy(srateInfo, &StRate_Config[6][4], sizeof(ST_RATE_CONFIG));
		}
		else
			return 0;
		return 1;
	}
	else if (nchannel == 10)
	{
		if (headchannel == 4)  /*5.1.4*/
		{
			if (bitrate == 176/*128 + 48*/)
			{
				memcpy(srateInfo, &StRate_Config[8][0], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 384/*256 + 128*/)
			{
				memcpy(srateInfo, &StRate_Config[8][1], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 576/*384 + 192*/)
			{
				memcpy(srateInfo, &StRate_Config[8][2], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 704/*448 + 256*/)
			{
				memcpy(srateInfo, &StRate_Config[8][3], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 256/*192 + 64*/)
			{
				memcpy(srateInfo, &StRate_Config[8][4], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 448/*320 + 128*/)
			{
				memcpy(srateInfo, &StRate_Config[8][5], sizeof(ST_RATE_CONFIG));
			}
			else
				return 0;
		}
		else if (headchannel == 2)  /*7.1.2*/
		{
			if (bitrate == 216/*192 + 24*/)
			{
				memcpy(srateInfo, &StRate_Config[9][0], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 480/*416 + 64*/)         //shumin.xu 20190215
			{
				memcpy(srateInfo, &StRate_Config[9][1], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 576/*480 + 96*/)
			{
				memcpy(srateInfo, &StRate_Config[9][2], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 384/*320 + 64*/)
			{
				memcpy(srateInfo, &StRate_Config[9][3], sizeof(ST_RATE_CONFIG));
			}
			else if (bitrate == 768/*640 + 128*/)
			{
				memcpy(srateInfo, &StRate_Config[9][4], sizeof(ST_RATE_CONFIG));
			}
			else
				return 0;
		}
		return 1;
	}
	else if (nchannel == 12) /*7.1.4*/
	{
		if (bitrate == 240/*192 + 48*/)
		{
			memcpy(srateInfo, &StRate_Config[10][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 608/*480 + 128*/)
		{
			memcpy(srateInfo, &StRate_Config[10][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384/*288 + 96*/)
		{
			memcpy(srateInfo, &StRate_Config[10][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 512/*384 + 128*/)
		{
			memcpy(srateInfo, &StRate_Config[10][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 832/*576 + 256*/)
		{
			memcpy(srateInfo, &StRate_Config[10][4], sizeof(ST_RATE_CONFIG));
		}
		else
			return 0;
		return 1;
	}
	else if (nchannel == 16) //HOA 3rd 16ch, shumin.xu 20210510
	{
		if (bitrate == 256)
		{
			memcpy(srateInfo, &StRate_Config[11][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 320)
		{
			memcpy(srateInfo, &StRate_Config[11][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384)
		{
			memcpy(srateInfo, &StRate_Config[11][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 512)
		{
			memcpy(srateInfo, &StRate_Config[11][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 640)
		{
			memcpy(srateInfo, &StRate_Config[11][4], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 896)
		{
			memcpy(srateInfo, &StRate_Config[11][5], sizeof(ST_RATE_CONFIG));
		}
		else
			return 0;
		return 1;
	}
	else if (nchannel == 9)
	{
		if (bitrate == 192)
		{
			memcpy(srateInfo, &StRate_Config[12][0], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 256)
		{
			memcpy(srateInfo, &StRate_Config[12][1], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 320)
		{
			memcpy(srateInfo, &StRate_Config[12][2], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 384)
		{
			memcpy(srateInfo, &StRate_Config[12][3], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 480)
		{
			memcpy(srateInfo, &StRate_Config[12][4], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 512)
		{
			memcpy(srateInfo, &StRate_Config[12][5], sizeof(ST_RATE_CONFIG));
		}
		else if (bitrate == 640)
		{
			memcpy(srateInfo, &StRate_Config[12][6], sizeof(ST_RATE_CONFIG));
		}
		else
			return 0;
		return 1;
	}
	else
	{
		return 0;
	}
}

void mcr_init(MCR_INFO *mcrInfo, int brate, int useBWE)
{
	if(brate == 0)
	{
		mcrInfo->index = 0;
		mcrInfo->group_id = 2;
		mcrInfo->group_thr[0] = 0.0;
		mcrInfo->group_thr[1] = 0.0;

		mcrInfo->sfb_num[0] = 9;
		mcrInfo->sfb_num[1] = 12;
		mcrInfo->sfb_num[2] = 24;
		mcrInfo->sfb_num[3] = 36;
		memcpy(mcrInfo->sfb_offset, McrSfb_0, sizeof(McrSfb_0));
		// initialize the vq info
		mcrInfo->vq_dim[0] = 3;
		mcrInfo->vq_dim[1] = 3;
		mcrInfo->vq_dim[2] = 3;
		mcrInfo->vq_dim[3] = 3;

		mcrInfo->vq_size[0] = 256;
		mcrInfo->vq_size[1] = 512;
		mcrInfo->vq_size[2] = 512;
		mcrInfo->vq_size[3] = 1024;

		mcrInfo->bit_per_vec[0] = 8;
		mcrInfo->bit_per_vec[1] = 9;
		mcrInfo->bit_per_vec[2] = 9;
		mcrInfo->bit_per_vec[3] = 10;

		mcrInfo->pt_cbook[0] = codebk0_256_3;
		mcrInfo->pt_cbook[1] = codebk0_512_3;
		mcrInfo->pt_cbook[2] = codebk0_512_3;
		mcrInfo->pt_cbook[3] = codebk0_1024_3;

		if(useBWE)
		{
			mcrInfo->sfb_num[0] = 15;
			mcrInfo->sfb_num[3] = 36;
			mcrInfo->pt_mag_cbook[0] = codebk1_256_3;
			mcrInfo->pt_mag_cbook[1] = codebk1_512_3;
			mcrInfo->pt_mag_cbook[2] = codebk1_512_3;
			mcrInfo->pt_mag_cbook[3] = codebk1_1024_3;

			mcrInfo->pt_ang_cbook[0] = codebk2_256_3;
			mcrInfo->pt_ang_cbook[1] = codebk2_512_3;
			mcrInfo->pt_ang_cbook[2] = codebk2_512_3;
			mcrInfo->pt_ang_cbook[3] = codebk2_1024_3;
		}
	}
	else if(brate < 12 )
	{
		mcrInfo->index = 0;
		mcrInfo->group_id = 1;
		if(brate==8)
		{
			mcrInfo->group_thr[0] = 0.01;
			mcrInfo->group_thr[1] = 1.5;
		}
		else if(brate==6)
		{
			mcrInfo->group_thr[0] = 0.04;
			mcrInfo->group_thr[1] = 5;
		}

		mcrInfo->sfb_num[0] = 9;
		mcrInfo->sfb_num[1] = 12;
		mcrInfo->sfb_num[2] = 24;
		mcrInfo->sfb_num[3] = 36;
		memcpy(mcrInfo->sfb_offset, McrSfb_0, sizeof(McrSfb_0));
		// initialize the vq info
		mcrInfo->vq_dim[0] = 3;
		mcrInfo->vq_dim[1] = 3;
		mcrInfo->vq_dim[2] = 3;
		mcrInfo->vq_dim[3] = 3;

		mcrInfo->vq_size[0] = 256;
		mcrInfo->vq_size[1] = 512;
		mcrInfo->vq_size[2] = 512;
		mcrInfo->vq_size[3] = 1024;

		mcrInfo->bit_per_vec[0] = 8;
		mcrInfo->bit_per_vec[1] = 9;
		mcrInfo->bit_per_vec[2] = 9;
		mcrInfo->bit_per_vec[3] = 10;

		mcrInfo->pt_cbook[0] = codebk0_256_3;
		mcrInfo->pt_cbook[1] = codebk0_512_3;
		mcrInfo->pt_cbook[2] = codebk0_512_3;
		mcrInfo->pt_cbook[3] = codebk0_1024_3;

		if(useBWE)
		{
			mcrInfo->sfb_num[0] = 18;
			mcrInfo->sfb_num[3] = 48;
			mcrInfo->pt_mag_cbook[0] = codebk1_256_3;
			mcrInfo->pt_mag_cbook[1] = codebk1_512_3;
			mcrInfo->pt_mag_cbook[2] = codebk1_512_3;
			mcrInfo->pt_mag_cbook[3] = codebk1_1024_3;

			mcrInfo->pt_ang_cbook[0] = codebk2_256_3;
			mcrInfo->pt_ang_cbook[1] = codebk2_512_3;
			mcrInfo->pt_ang_cbook[2] = codebk2_512_3;
			mcrInfo->pt_ang_cbook[3] = codebk2_1024_3;
		}
	}
	else if(brate == 12)
	{
		mcrInfo->index = 0;
		mcrInfo->group_id = 0;
		mcrInfo->group_thr[0] = 0.0;
		mcrInfo->group_thr[1] = 0.0;

		mcrInfo->sfb_num[0] = 9;
		mcrInfo->sfb_num[1] = 12;
		mcrInfo->sfb_num[2] = 24;
		mcrInfo->sfb_num[3] = 36;
		memcpy(mcrInfo->sfb_offset, McrSfb_0, sizeof(McrSfb_0));
		// initialize the vq info
		mcrInfo->vq_dim[0] = 3;
		mcrInfo->vq_dim[1] = 3;
		mcrInfo->vq_dim[2] = 3;
		mcrInfo->vq_dim[3] = 3;

		mcrInfo->vq_size[0] = 256;
		mcrInfo->vq_size[1] = 512;
		mcrInfo->vq_size[2] = 512;
		mcrInfo->vq_size[3] = 1024;

		mcrInfo->bit_per_vec[0] = 8;
		mcrInfo->bit_per_vec[1] = 9;
		mcrInfo->bit_per_vec[2] = 9;
		mcrInfo->bit_per_vec[3] = 10;

		mcrInfo->pt_cbook[0] = codebk0_256_3;
		mcrInfo->pt_cbook[1] = codebk0_512_3;
		mcrInfo->pt_cbook[2] = codebk0_512_3;
		mcrInfo->pt_cbook[3] = codebk0_1024_3;

		if(useBWE)
		{
			mcrInfo->sfb_num[0] = 18;
			mcrInfo->sfb_num[3] = 48;
			mcrInfo->pt_mag_cbook[0] = codebk1_256_3;
			mcrInfo->pt_mag_cbook[1] = codebk1_512_3;
			mcrInfo->pt_mag_cbook[2] = codebk1_512_3;
			mcrInfo->pt_mag_cbook[3] = codebk1_1024_3;

			mcrInfo->pt_ang_cbook[0] = codebk2_256_3;
			mcrInfo->pt_ang_cbook[1] = codebk2_512_3;
			mcrInfo->pt_ang_cbook[2] = codebk2_512_3;
			mcrInfo->pt_ang_cbook[3] = codebk2_1024_3;
		}
	}
	else if(brate == 16)
	{
		mcrInfo->index = 1;
		mcrInfo->group_id = 0;
		mcrInfo->group_thr[0] = 0.0;
		mcrInfo->group_thr[1] = 0.0;

		mcrInfo->sfb_num[0] = 9;
		mcrInfo->sfb_num[1] = 15;
		mcrInfo->sfb_num[2] = 30;
		mcrInfo->sfb_num[3] = 48;
		memcpy(mcrInfo->sfb_offset, McrSfb_1, sizeof(McrSfb_1));
		// initialize the vq info
		mcrInfo->vq_dim[0] = 3;
		mcrInfo->vq_dim[1] = 3;
		mcrInfo->vq_dim[2] = 3;
		mcrInfo->vq_dim[3] = 3;

		mcrInfo->vq_size[0] = 512;
		mcrInfo->vq_size[1] = 512;
		mcrInfo->vq_size[2] = 512;
		mcrInfo->vq_size[3] = 1024;

		mcrInfo->bit_per_vec[0] = 9;
		mcrInfo->bit_per_vec[1] = 9;
		mcrInfo->bit_per_vec[2] = 9;
		mcrInfo->bit_per_vec[3] = 10;

		mcrInfo->pt_cbook[0] = codebk0_512_3;
		mcrInfo->pt_cbook[1] = codebk0_512_3;
		mcrInfo->pt_cbook[2] = codebk0_512_3;
		mcrInfo->pt_cbook[3] = codebk0_1024_3;
	}
	else if(brate == 20)
	{
		mcrInfo->index = 2;
		mcrInfo->group_id = 0;
		mcrInfo->group_thr[0] = 0.0;
		mcrInfo->group_thr[1] = 0.0;

		mcrInfo->sfb_num[0] = 9;
		mcrInfo->sfb_num[1] = 18;
		mcrInfo->sfb_num[2] = 36;
		mcrInfo->sfb_num[3] = 48;
		memcpy(mcrInfo->sfb_offset, McrSfb_2, sizeof(McrSfb_2));
		// initialize the vq info
		mcrInfo->vq_dim[0] = 3;
		mcrInfo->vq_dim[1] = 3;
		mcrInfo->vq_dim[2] = 3;
		mcrInfo->vq_dim[3] = 2;

		mcrInfo->vq_size[0] = 512;
		mcrInfo->vq_size[1] = 1024;
		mcrInfo->vq_size[2] = 1024;
		mcrInfo->vq_size[3] = 256;

		mcrInfo->bit_per_vec[0] = 9;
		mcrInfo->bit_per_vec[1] = 10;
		mcrInfo->bit_per_vec[2] = 10;
		mcrInfo->bit_per_vec[3] = 8;

		mcrInfo->pt_cbook[0] = codebk0_512_3;
		mcrInfo->pt_cbook[1] = codebk0_1024_3;
		mcrInfo->pt_cbook[2] = codebk0_1024_3;
		mcrInfo->pt_cbook[3] = codebk0_256_2;
	}
	else if(brate == 24)
	{
		mcrInfo->index = 3;
		mcrInfo->group_id = 0;
		mcrInfo->group_thr[0] = 0.0;
		mcrInfo->group_thr[1] = 0.0;

		mcrInfo->sfb_num[0] = 12;
		mcrInfo->sfb_num[1] = 18;
		mcrInfo->sfb_num[2] = 36;
		mcrInfo->sfb_num[3] = 64;
		memcpy(mcrInfo->sfb_offset, McrSfb_4, sizeof(McrSfb_4));
		// initialize the vq info
		mcrInfo->vq_dim[0] = 3;
		mcrInfo->vq_dim[1] = 3;
		mcrInfo->vq_dim[2] = 3;
		mcrInfo->vq_dim[3] = 2;

		mcrInfo->vq_size[0] = 512;
		mcrInfo->vq_size[1] = 1024;
		mcrInfo->vq_size[2] = 1024;
		mcrInfo->vq_size[3] = 256;

		mcrInfo->bit_per_vec[0] = 9;
		mcrInfo->bit_per_vec[1] = 10;
		mcrInfo->bit_per_vec[2] = 10;
		mcrInfo->bit_per_vec[3] = 8;

		mcrInfo->pt_cbook[0] = codebk0_512_3;
		mcrInfo->pt_cbook[1] = codebk0_1024_3;
		mcrInfo->pt_cbook[2] = codebk0_1024_3;
		mcrInfo->pt_cbook[3] = codebk0_256_2;
	}
}

void getMcrInfo(MCR_INFO *mcrInfo, int index)
{
	if(index == 0)
	{
		mcrInfo->group_id = 2;
		mcrInfo->group_thr[0] = 0.0;
		mcrInfo->group_thr[1] = 0.0;

		mcrInfo->sfb_num[0] = 9;
		mcrInfo->sfb_num[1] = 12;
		mcrInfo->sfb_num[2] = 24;
		mcrInfo->sfb_num[3] = 36;
		memcpy(mcrInfo->sfb_offset, McrSfb_0, sizeof(McrSfb_0));
		// initialize the vq info
		mcrInfo->vq_dim[0] = 3;
		mcrInfo->vq_dim[1] = 3;
		mcrInfo->vq_dim[2] = 3;
		mcrInfo->vq_dim[3] = 3;

		mcrInfo->vq_size[0] = 256;
		mcrInfo->vq_size[1] = 512;
		mcrInfo->vq_size[2] = 512;
		mcrInfo->vq_size[3] = 1024;

		mcrInfo->bit_per_vec[0] = 8;
		mcrInfo->bit_per_vec[1] = 9;
		mcrInfo->bit_per_vec[2] = 9;
		mcrInfo->bit_per_vec[3] = 10;

		mcrInfo->pt_cbook[0] = codebk0_256_3;
		mcrInfo->pt_cbook[1] = codebk0_512_3;
		mcrInfo->pt_cbook[2] = codebk0_512_3;
		mcrInfo->pt_cbook[3] = codebk0_1024_3;
	}
	else if(index == 1)
	{
		//mcrInfo->index = 1;
		mcrInfo->group_id = 0;
		mcrInfo->group_thr[0] = 0.0;
		mcrInfo->group_thr[1] = 0.0;

		mcrInfo->sfb_num[0] = 9;
		mcrInfo->sfb_num[1] = 15;
		mcrInfo->sfb_num[2] = 30;
		mcrInfo->sfb_num[3] = 48;
		memcpy(mcrInfo->sfb_offset, McrSfb_1, sizeof(McrSfb_1));
		// initialize the vq info
		mcrInfo->vq_dim[0] = 3;
		mcrInfo->vq_dim[1] = 3;
		mcrInfo->vq_dim[2] = 3;
		mcrInfo->vq_dim[3] = 3;

		mcrInfo->vq_size[0] = 512;
		mcrInfo->vq_size[1] = 512;
		mcrInfo->vq_size[2] = 512;
		mcrInfo->vq_size[3] = 1024;

		mcrInfo->bit_per_vec[0] = 9;
		mcrInfo->bit_per_vec[1] = 9;
		mcrInfo->bit_per_vec[2] = 9;
		mcrInfo->bit_per_vec[3] = 10;

		mcrInfo->pt_cbook[0] = codebk0_512_3;
		mcrInfo->pt_cbook[1] = codebk0_512_3;
		mcrInfo->pt_cbook[2] = codebk0_512_3;
		mcrInfo->pt_cbook[3] = codebk0_1024_3;
	}
	else if(index == 2)
	{
		//mcrInfo->index = 2;
		mcrInfo->group_id = 0;
		mcrInfo->group_thr[0] = 0.0;
		mcrInfo->group_thr[1] = 0.0;

		mcrInfo->sfb_num[0] = 9;
		mcrInfo->sfb_num[1] = 18;
		mcrInfo->sfb_num[2] = 36;
		mcrInfo->sfb_num[3] = 48;
		memcpy(mcrInfo->sfb_offset, McrSfb_2, sizeof(McrSfb_2));
		// initialize the vq info
		mcrInfo->vq_dim[0] = 3;
		mcrInfo->vq_dim[1] = 3;
		mcrInfo->vq_dim[2] = 3;
		mcrInfo->vq_dim[3] = 2;

		mcrInfo->vq_size[0] = 512;
		mcrInfo->vq_size[1] = 1024;
		mcrInfo->vq_size[2] = 1024;
		mcrInfo->vq_size[3] = 256;

		mcrInfo->bit_per_vec[0] = 9;
		mcrInfo->bit_per_vec[1] = 10;
		mcrInfo->bit_per_vec[2] = 10;
		mcrInfo->bit_per_vec[3] = 8;

		mcrInfo->pt_cbook[0] = codebk0_512_3;
		mcrInfo->pt_cbook[1] = codebk0_1024_3;
		mcrInfo->pt_cbook[2] = codebk0_1024_3;
		mcrInfo->pt_cbook[3] = codebk0_256_2;
	}
	else if(index == 3)
	{
		mcrInfo->index = 3;
		mcrInfo->group_id = 0;
		mcrInfo->group_thr[0] = 0.0;
		mcrInfo->group_thr[1] = 0.0;

		mcrInfo->sfb_num[0] = 12;
		mcrInfo->sfb_num[1] = 18;
		mcrInfo->sfb_num[2] = 36;
		mcrInfo->sfb_num[3] = 64;
		memcpy(mcrInfo->sfb_offset, McrSfb_4, sizeof(McrSfb_4));
		// initialize the vq info
		mcrInfo->vq_dim[0] = 3;
		mcrInfo->vq_dim[1] = 3;
		mcrInfo->vq_dim[2] = 3;
		mcrInfo->vq_dim[3] = 2;

		mcrInfo->vq_size[0] = 512;
		mcrInfo->vq_size[1] = 1024;
		mcrInfo->vq_size[2] = 1024;
		mcrInfo->vq_size[3] = 256;

		mcrInfo->bit_per_vec[0] = 9;
		mcrInfo->bit_per_vec[1] = 10;
		mcrInfo->bit_per_vec[2] = 10;
		mcrInfo->bit_per_vec[3] = 8;

		mcrInfo->pt_cbook[0] = codebk0_512_3;
		mcrInfo->pt_cbook[1] = codebk0_1024_3;
		mcrInfo->pt_cbook[2] = codebk0_1024_3;
		mcrInfo->pt_cbook[3] = codebk0_256_2;
	}
}

/****************************************************************************************
 *          Decoding with MDCT domain Maximal Coherence Rotation                        
 *                                                                                      
 * Return:	- subband number                                                                      
 * Params: 	- float* 			freq_left	<- OUT, left channel   
 *			- float*			freq_right	<- OUT, right channel  	
 *			- float*			freq_mono	<- IN,  downmixed channel
 *          - int*              theta_buff  <- IN,  rotation angles               
 *          - int*              kappa_buff  <- IN,  rotation angles               
 *                                                                                      
 ****************************************************************************************/

/****************************************************************************************
 *          Decoding with Multi-block Maximal Coherence Rotation                        
 *                                                                                      
 * Return:	- subband number                                                                      
 * Params: 	
 *          - int*              mcr_buff	<- IN/OUT,  new angles to save
 *			- float* 			freq_left	<- OUT, left channel   
 *			- float*			freq_right	<- OUT, right channel  	
 *			- float*			freq_mono	<- IN,  downmixed channel              
 *          - int*              win_seq_pre	<- IN,  block length for mcr decoding
 *          - int*              win_seq		<- IN,  block length for reading new angles
 *          - FILE*             f_input		<- IN,  bitstream file 
 *          - int				useBWE		<- IN,  using BWE flag
 *                                                                                      
 ****************************************************************************************/

int mcr_decode(PS_DATA		*ps_data,
			   MCR_INFO		mcrInfo,
			   int			len,
			   int			pos,
			   int			mcrpos)
{
	int i,j;
	int sfb = 0, nsfb = 0;
	int vq_bit, vq_dim, vq_size;
	int sfnum;

	short	*sfband;
	float	*cbook;

	float	qtheta[NSFBAND];
	float	qkappa[NSFBAND];
	MAXCORR	*qtheta_pt = ps_data->mcr_data + mcrpos;
	MAXCORR	*qkappa_pt = ps_data->mcr_data + mcrpos + MAX_NSF;

	float	band_even_1[FRAMESIZE], band_even_2[FRAMESIZE];
	float	band_odd_1[FRAMESIZE], band_odd_2[FRAMESIZE];
	float	*band_e_1_pt, *band_e_2_pt, *band_o_1_pt, *band_o_2_pt;

	int id = mlog2(len/128);
	int encLen = (ps_data->bandwidth*len+512)/1024;

	sfnum	= mcrInfo.sfb_num[id];
	vq_dim	= mcrInfo.vq_dim[id];
	vq_size	= mcrInfo.vq_size[id];
	vq_bit	= mcrInfo.bit_per_vec[id];
	sfband	= mcrInfo.sfb_offset[id];
	cbook	= mcrInfo.pt_cbook[id];

	for(sfb = 0; sfb < sfnum; sfb++)
	{
		if (sfband[sfb+1]<encLen/2) nsfb = sfb + 1;
	}
	nsfb = (nsfb + vq_dim - 1) / vq_dim * vq_dim;

	for(sfb = 0; sfb < nsfb; sfb += vq_dim)
	{
		float*	pv;
		pv = vec_retrieve(qtheta_pt->theta_pos, cbook, vq_dim);	
		for (j = 0; j < vq_dim; j++){
			qtheta[sfb + j] = pv[j];
			qtheta_pt->theta_q = pv[j];
			qtheta_pt++;
		}
		
		pv = vec_retrieve(qkappa_pt->theta_pos, cbook, vq_dim);
		for (j = 0; j < vq_dim; j++){
			qkappa[sfb + j] = pv[j];
			qkappa_pt->theta_q = pv[j];
			qkappa_pt++;
		}

	}
	for( ; sfb < sfnum; sfb++)
	{
		qtheta[sfb] = qtheta[sfb-1];
		qtheta_pt->theta_q = (qtheta_pt-1)->theta_q;
		qtheta_pt++;

		qkappa[sfb] = qkappa[sfb-1];
		qkappa_pt->theta_q = (qkappa_pt-1)->theta_q;
		qkappa_pt++;
	}

	for(j = 0; j < len/2; j++)
	{
		band_even_1[j] = ps_data->sum_data[j*2+pos] + ps_data->dif_data[j*2+pos];
		band_even_2[j] = ps_data->sum_data[j*2+pos] - ps_data->dif_data[j*2+pos];
			
		band_odd_1[j] = ps_data->sum_data[j*2+1+pos] + ps_data->dif_data[j*2+1+pos];
		band_odd_2[j] = ps_data->sum_data[j*2+1+pos] - ps_data->dif_data[j*2+1+pos];
	}
	band_e_1_pt = band_even_1;
	band_e_2_pt = band_even_2;
	band_o_1_pt = band_odd_1;
	band_o_2_pt = band_odd_2;

	// rotate transform
	for(sfb = 0; sfb < sfnum; sfb++)
	{
		int n = sfband[sfb + 1] - sfband[sfb];

		rotate_transform(band_e_1_pt, band_e_2_pt, n, -qtheta[sfb]);

		rotate_transform(band_o_1_pt, band_o_2_pt, n, -qkappa[sfb]);

		band_e_1_pt += n;
		band_e_2_pt += n;
		band_o_1_pt += n;
		band_o_2_pt += n;
	}
	
	for (j = 0; j < len/2; j++)
	{
		ps_data->left_data[j*2+pos]		= band_even_1[j];
		ps_data->left_data[j*2+1+pos]	= band_odd_1[j];

		ps_data->right_data[j*2+pos]	= band_even_2[j];
		ps_data->right_data[j*2+1+pos]	= band_odd_2[j];
	}

	return nsfb;
	
}


int mcr_bwe_decode(PS_BWE_DATA	*psBweData,
				   MCR_INFO		mcrInfo,
				   int			len,
				   int			nblock,
				   int			pos,
				   int			mcrpos)
{
	int i,j;
	int sfb = 0, nsfb = 0;
	int vq_bit, vq_dim, vq_size;
	int sfnum;

	short	*sfband;
	float	*mag_cb, *ang_cb;
	float	mag[MAX_NSF*2], ang[MAX_NSF*2];
	float	mag_left[FRAMESIZE*2], mag_right[FRAMESIZE*2];
	float	ang_left[FRAMESIZE*2], ang_right[FRAMESIZE*2];
	float	*mag_l, *mag_r;
	MAXCORR	*mag_pt = psBweData->mcr_data + mcrpos;
	MAXCORR	*ang_pt = psBweData->ang_data + mcrpos;

	int id = mlog2(len/256);
	int encLen = (psBweData->bandwidth * len + 512)/1024;

	sfnum	= mcrInfo.sfb_num[id];
	vq_dim	= mcrInfo.vq_dim[id];
	vq_size	= mcrInfo.vq_size[id];
	vq_bit	= mcrInfo.bit_per_vec[id];
	sfband	= mcrInfo.sfb_offset[id];
	mag_cb	= mcrInfo.pt_mag_cbook[id];
	ang_cb	= mcrInfo.pt_ang_cbook[id];

	for(j = 0; j < len * nblock; j++)
	{
		float tmp1, tmp2;

		tmp1 = psBweData->sum_data[j*2+pos] + psBweData->dif_data[j*2+pos];  // real part
		tmp2 = psBweData->sum_data[j*2+1+pos] + psBweData->dif_data[j*2+1+pos];  // imag part
		mag_left[j] = sqrt(tmp1*tmp1+tmp2*tmp2);
		ang_left[j] = atan2(tmp2,tmp1);

		tmp1 = psBweData->sum_data[j*2+pos] - psBweData->dif_data[j*2+pos];
		tmp2 = psBweData->sum_data[j*2+1+pos] - psBweData->dif_data[j*2+1+pos];
		mag_right[j] = sqrt(tmp1*tmp1+tmp2*tmp2);
		ang_right[j] = atan2(tmp2,tmp1);
	}

	for(sfb = 0; sfb < sfnum; sfb++)
	{
		if (sfband[sfb+1]<=encLen/4) nsfb = sfb + 1;
	}
	nsfb = (nsfb + vq_dim - 1) / vq_dim * vq_dim;

	/*for(sfb = 0; sfb < nsfb; sfb += vq_dim)
	{
		float *pv;
		pv = vec_retrieve(mag_pt->theta_pos, mag_cb, vq_dim);	
		for (j = 0; j < vq_dim; j++){
			mag[sfb + j] = pv[j];
			mag_pt->theta_q = pv[j];
			mag_pt++;
		}
		
		pv = vec_retrieve(ang_pt->theta_pos, ang_cb, vq_dim);
		for (j = 0; j < vq_dim; j++){
			ang[sfb + j] = pv[j];
			ang_pt->theta_q = pv[j];
			ang_pt++;
		}

	}
	for( ; sfb < sfnum; sfb++)
	{
		mag[sfb] = mag[sfb-1];
		mag_pt->theta_q = (mag_pt-1)->theta_q;
		mag_pt++;

		ang[sfb] = ang[sfb-1];
		ang_pt->theta_q = (ang_pt-1)->theta_q;
		ang_pt++;
	}*/

	mag_l = mag_left;
	mag_r = mag_right;
	// rotate transform
	for(i = 0; i < nblock; i++)
	{
		for(sfb = 0; sfb < sfnum; sfb++)
		{
			int n = (sfband[sfb + 1] - sfband[sfb]) * 4;

			rotate_transform(mag_l, mag_r, n, -((mag_pt+sfb)->theta_q));

			if(sfb<sfnum/2)
				for(j = sfband[sfb]*4; j < sfband[sfb+1]*4; j++)
			{
				ang_right[j+i*len] = ang_left[j+i*len];// + ang[sfb];
			}
			else
			for(j = sfband[sfb]*4; j < sfband[sfb+1]*4; j++)
			{
				ang_right[j+i*len] = ang_left[j+i*len];// + ang[sfb];
			}

			mag_l += n;
			mag_r += n;
		}
	}
	
	for(j = 0; j < len * nblock; j++)
	{
		psBweData->left_data[j*2+pos]	= mag_left[j] * cos(ang_left[j]);
		psBweData->left_data[j*2+1+pos]	= mag_left[j] * sin(ang_left[j]);

		psBweData->right_data[j*2+pos]	= mag_right[j] * cos(ang_right[j]);
		psBweData->right_data[j*2+1+pos]= mag_right[j] * sin(ang_right[j]);
	}

	return nsfb;
	
}


/****************************************************************************************
 *          Decoding with Multi-block Maximal Coherence Rotation                        
 *                                                                                      
 * Return:	- subband number                                                                      
 * Params: 	
 *          - int*              mcr_buff	<- IN/OUT,  new angles to save
 *			- float* 			freq_left	<- OUT, left channel   
 *			- float*			freq_right	<- OUT, right channel  	
 *			- float*			freq_mono	<- IN,  downmixed channel              
 *          - int*              win_seq_pre	<- IN,  block length for mcr decoding
 *          - int*              win_seq		<- IN,  block length for reading new angles
 *          - FILE*             f_input		<- IN,  bitstream file 
 *          - int				useBWE		<- IN,  using BWE flag
 *                                                                                      
 ****************************************************************************************/
int MCR_Decoder(PS_DATA		*ps_data,
				MCR_INFO	mcrInfo,
				PS_DATA		*ps_data_pre,
				//FILE		*f_input,
				int			superflag,
				avs2audiopack_buffer *opb)
{
	int i, j, sfnum = 0, id, nsfb = 0;
	int tt,jj,kk,ll;
	int index,offset, mdftoffset;
	int noft, nbyte, vq_dim, vq_bit;
	int encLen, sfb;
	short *sfband;
	int pos0, pos1;
	int bitcount = 0;
	MAXCORR *mcr_pt = ps_data->mcr_data;

	unsigned int buf_mcr[256];

	avs2audiopack_buffer *data_mcr = (avs2audiopack_buffer *)calloc(1, sizeof(avs2audiopack_buffer));
	
	offset = 0;
	mdftoffset = 0;
	noft = opb->endbit;
	nbyte = 2;  // 2 bits for mcr_index

	if(superflag==0)
	{
		for(index = 1; index < (ps_data->winseq[0]+1); index++)
		{
			tt = ps_data->winseq[index];
			jj = ps_data->winseq[index+1];
    
			if(LL[tt-1]>LL[jj-1])
				ll = LL[tt-1]/2;
			else
				ll = LL[jj-1]/2;

			id = mlog2(ll/128);
			sfnum  = mcrInfo.sfb_num[id];
			sfband = mcrInfo.sfb_offset[id];
			vq_dim = mcrInfo.vq_dim[id];
			vq_bit = mcrInfo.bit_per_vec[id];

			encLen = (ps_data->bandwidth*ll+512)/1024;
			for(sfb = 0; sfb < sfnum; sfb++)
			{
				if (sfband[sfb+1]<encLen/2) nsfb = sfb + 1;
			}
			nsfb = (nsfb + vq_dim - 1)/vq_dim * vq_dim;	
			nbyte += nsfb / vq_dim * 2 * vq_bit;
		}
		nbyte = (nbyte + 7)/8;

		//avs2audiopack_writeinit(data_mcr);
		memset(data_mcr, 0, sizeof(avs2audiopack_buffer));	

		//fread(&nbyte, sizeof(unsigned char), 1, f_input);
		//fread(buf_mcr, 1, nbyte, f_input);
		/*{for(i=0;i<nbyte;i++)
			buf_mcr[i] = (unsigned char) avs2audiopack_read(opb,8);
		}*/
		data_mcr->buffer = data_mcr->ptr = buf_mcr;
		data_mcr->storage = nbyte;

		mcrInfo.index = avs2audiopack_read(opb, 2);
		bitcount += 2;
		getMcrInfo(&mcrInfo, mcrInfo.index);


		for(index = 1; index < (ps_data->winseq[0]+1); index++)
		{
			tt = ps_data->winseq[index];
			jj = ps_data->winseq[index+1];
    
			if(LL[tt-1]>LL[jj-1])
				ll = LL[tt-1]/2;
			else
				ll = LL[jj-1]/2;

			id = mlog2(ll/128);
			sfnum  = mcrInfo.sfb_num[id];
			sfband = mcrInfo.sfb_offset[id];
			vq_dim = mcrInfo.vq_dim[id];
			vq_bit = mcrInfo.bit_per_vec[id];

			encLen = (ps_data->bandwidth*ll+512)/1024;
			for(sfb = 0; sfb < sfnum; sfb++)
			{
				if (sfband[sfb+1]<encLen/2) nsfb = sfb + 1;
			}
			nsfb = (nsfb + vq_dim - 1)/vq_dim * vq_dim;	

			for(j = 0; j < nsfb/vq_dim; j++)
			{
				pos0 = avs2audiopack_read(opb, vq_bit);
				bitcount += vq_bit;
				pos1 = avs2audiopack_read(opb, vq_bit);
				bitcount += vq_bit;
				for(i = 0; i<vq_dim; i++)
				{
					mcr_pt->theta_pos = pos0;
					(mcr_pt+MAX_NSF)->theta_pos = pos1;
					mcr_pt++;
				}			
			}
			for(j = nsfb; j < sfnum; j++)
			{
				mcr_pt->theta_pos = pos0;
				(mcr_pt+MAX_NSF)->theta_pos = pos1;
				mcr_pt++;
			}

			offset  += sfnum;
		}
	}
	else
	{
		memcpy(ps_data->mcr_data, ps_data_pre->mcr_data, MAX_NSF * 2 * sizeof(MAXCORR));
	}

	if (bitcount % 8 > 0)
	{
		avs2audiopack_read(opb, 8 - bitcount % 8);
	}

	offset = 0;
	mdftoffset = 0;
	for(index = 1; index < (ps_data->winseq[0]+1); index++)
	{
		tt = ps_data->winseq[index];
		jj = ps_data->winseq[index+1];
    
		if(LL[tt-1]>LL[jj-1])
			ll = LL[tt-1]/2;
		else
			ll = LL[jj-1]/2;
		
		id = mlog2(ll/128);
		sfnum  = mcrInfo.sfb_num[id];

		kk = mcr_decode(ps_data, mcrInfo, ll, mdftoffset, offset);

		offset += sfnum;
		mdftoffset += ll;
	}

	memcpy(ps_data_pre, ps_data, sizeof(PS_DATA));
	free(data_mcr);

	return nbyte;
}

	
int MCR_BWE_Decoder(int bitRate,
					PS_BWE_DATA			*psBweData,
					MCR_INFO			mcrInfo,
					PS_BWE_DATA			*psBweData_pre,
					int					superflag,
					avs2audiopack_buffer *opb)
{
	int i, j, sfnum, id, nsfbn,ngp;
	int kk,ll;
	int index, offset, mdftoffset;
	int nbyte, vq_dim, vq_bit;
	int encLen, sfb = 0, nsfb = 0;
	short *sfband;
	int pos0, pos1;
	float	*mag_cb, *ang_cb;
	int bitcount = 0;
	MAXCORR *mcr_pt = psBweData->mcr_data;
	MAXCORR *ang_pt = psBweData->ang_data;

	//unsigned char buf_mcr[256];

	int (*WinseqTabletmp)[20];
	int (*BlockTabletmp)[16];
	int (*GroupTabletmp)[10];
	int blockNum, groupNum, groupStart;
	//float timeBuffer[4096+2048];

	BlockTabletmp = (int(*)[16])getBlockseqTable(psBweData->Groupmode);
	GroupTabletmp = (int(*)[10])getGroupseqTable(psBweData->Groupmode);
	WinseqTabletmp = (int(*)[20])getWinseqTable(psBweData->Groupmode);

	//avs2audiopack_buffer *data_mcr = (avs2audiopack_buffer *)calloc(1, sizeof(avs2audiopack_buffer));
	
	offset = 0;
	mdftoffset = 0;
	nbyte = 0;

	if(superflag==0)
	{
		/*for(index = 0; index < (GroupTabletmp[psBweData->Seqmode][1]); index++)
		{
			groupStart = GroupTabletmp[psBweData->Seqmode][index+2];
			groupNum = GroupTabletmp[psBweData->Seqmode][index+3] - groupStart;

			ll = BlockTabletmp[psBweData->Seqmode][groupStart+2];
			encLen  = (psBweData->bandwidth*ll+512)/1024;
			id		= mlog2(ll/256);
			sfnum	= mcrInfo.sfb_num[id];
			vq_dim	= mcrInfo.vq_dim[id];
			vq_bit	= mcrInfo.bit_per_vec[id];
			sfband	= mcrInfo.sfb_offset[id];
			for(sfb = 0; sfb < sfnum; sfb++)
			{
				if (sfband[sfb+1]<encLen/4) nsfb = sfb + 1;
			}
			nsfb = (nsfb + vq_dim - 1)/vq_dim * vq_dim;	
			nbyte += nsfb / vq_dim * 2 * vq_bit;
		}
		nbyte = (nbyte + 7)/8;

		avs2audiopack_writeinit(data_mcr);
		memset(data_mcr, 0, sizeof(avs2audiopack_buffer));	

		for(i = 0; i < nbyte; i++)
			buf_mcr[i] = (unsigned char) avs2audiopack_read(opb,8);

		data_mcr->buffer = data_mcr->ptr = buf_mcr;
		data_mcr->storage = nbyte;*/

		
		if(bitRate>=48000)
		{
			ngp = GroupTabletmp[psBweData->Seqmode][1];
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
				sfband	= mcrInfo.sfb_offset[id];
				mag_cb	= mcrInfo.pt_mag_cbook[id];
				//ang_cb	= mcrInfo.pt_ang_cbook[id];
				for(sfb = 0; sfb < sfnum; sfb++)
				{
					if (sfband[sfb+1]<=encLen/4) nsfb = sfb + 1;
				}
				nsfb = (nsfb + vq_dim - 1)/vq_dim * vq_dim;	

				for(j = 0; j < nsfb/vq_dim; j++)
				{
					float *pv0, *pv1;
					pos0 = avs2audiopack_read(opb, vq_bit);
					//pos1 = avs2audiopack_read(opb, vq_bit);
					bitcount += vq_bit;
					pv0 = vec_retrieve(pos0, mag_cb, vq_dim);	
					//pv1 = vec_retrieve(pos1, ang_cb, vq_dim);
					for(i = 0; i < vq_dim; i++)
					{
						mcr_pt->theta_pos	= pos0;
						mcr_pt->theta_q		= pv0[i];
						//ang_pt->theta_pos	= pos1;
						//ang_pt->theta_q		= pv1[i];
						mcr_pt++;
						//ang_pt++;
					}	
					nbyte += vq_bit;
				}
				for(j = nsfb; j < sfnum; j++)
				{
					mcr_pt->theta_q = (mcr_pt-1)->theta_q;
					//ang_pt->theta_q = (ang_pt-1)->theta_q;
					mcr_pt++;
					//ang_pt++;
				}

				offset  += sfnum;
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
				sfband	= mcrInfo.sfb_offset[id];
				mag_cb	= mcrInfo.pt_mag_cbook[id];
				//ang_cb	= mcrInfo.pt_ang_cbook[id];
				for(sfb = 0; sfb < sfnum; sfb++)
				{
					if (sfband[sfb+1]<=encLen/4) nsfb = sfb + 1;
				}
				nsfb = (nsfb + vq_dim - 1)/vq_dim * vq_dim;	

				for(j = 0; j < nsfb/vq_dim; j++)
				{
					float *pv0, *pv1;
					pos0 = avs2audiopack_read(opb, vq_bit);
					//pos1 = avs2audiopack_read(opb, vq_bit);
					bitcount += vq_bit;
					pv0 = vec_retrieve(pos0, mag_cb, vq_dim);	
					//pv1 = vec_retrieve(pos1, ang_cb, vq_dim);
					for(i = 0; i < vq_dim; i++)
					{
						mcr_pt->theta_pos	= pos0;
						mcr_pt->theta_q		= pv0[i];
						//ang_pt->theta_pos	= pos1;
						//ang_pt->theta_q		= pv1[i];
						mcr_pt++;
						//ang_pt++;
					}	
					nbyte += vq_bit;
				}
				for(j = nsfb; j < sfnum; j++)
				{
					mcr_pt->theta_q = (mcr_pt-1)->theta_q;
					//ang_pt->theta_q = (ang_pt-1)->theta_q;
					mcr_pt++;
					//ang_pt++;
				}

				offset  += sfnum;
			}
        }

	}
	else
	{
		memcpy(psBweData->mcr_data, &psBweData_pre->mcr_data[MAX_NSF], MAX_NSF * sizeof(MAXCORR));
		memcpy(psBweData->ang_data, &psBweData_pre->ang_data[MAX_NSF], MAX_NSF * sizeof(MAXCORR));
	}

	nbyte = (nbyte + 7)/8;

	if (bitcount % 8 > 0)
	{
		avs2audiopack_read(opb, 8 - bitcount % 8);
	}

	offset = 0;
	mdftoffset = 0;
	BlockTabletmp = (int(*)[16])getBlockseqTable(psBweData_pre->Groupmode);
	GroupTabletmp = (int(*)[10])getGroupseqTable(psBweData_pre->Groupmode);
	WinseqTabletmp = (int(*)[20])getWinseqTable(psBweData_pre->Groupmode);
	
	
	ngp = GroupTabletmp[psBweData_pre->Seqmode][1];
	for(index = 0; index < (GroupTabletmp[psBweData_pre->Seqmode][ngp+2]); index++)
	  {
	   groupStart = index;
	   groupNum = 1;
	

		ll = BlockTabletmp[psBweData_pre->Seqmode][groupStart+2];
		encLen  = (psBweData_pre->bandwidth*ll+512)/1024;
		id		= mlog2(ll/256);
		sfnum	= mcrInfo.sfb_num[id];
		vq_dim	= mcrInfo.vq_dim[id];
		vq_bit	= mcrInfo.bit_per_vec[id];

		kk = mcr_bwe_decode(psBweData_pre, mcrInfo, ll, groupNum, mdftoffset, offset);

		offset += sfnum;
		mdftoffset += ll * groupNum * 2;
	}
	/*{
		int j;
		FILE *fp0 = fopen("samples1.txt","a");
		for(j=0; j<1024*4; j++)
			fprintf(fp0,"%f\n",psBweData->left_data[j]);
		fclose(fp0);
	}*/

	psBweData_pre->bandwidth = psBweData->bandwidth;
	//psBweData_pre->Groupmode = psBweData->Groupmode;
	//psBweData_pre->Seqmode = psBweData->Seqmode;
	memcpy(psBweData_pre->mcr_data, &psBweData_pre->mcr_data[MAX_NSF], MAX_NSF*sizeof(MAXCORR));
	memcpy(psBweData_pre->ang_data, &psBweData_pre->ang_data[MAX_NSF], MAX_NSF*sizeof(MAXCORR));
	memcpy(&psBweData_pre->mcr_data[MAX_NSF], psBweData->mcr_data, MAX_NSF*sizeof(MAXCORR));
	memcpy(&psBweData_pre->ang_data[MAX_NSF], psBweData->ang_data, MAX_NSF*sizeof(MAXCORR));

	return nbyte;
}


