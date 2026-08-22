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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "avs2audio.h"
#include "codebook.h"
#include "lfenc.h"


#define floor1_rangedB 140 /* floor 1 fixed at -140dB to 0dB range */

typedef struct lsfit_acc{
  int x0;
  int x1;

  int xa;
  int ya;
  int x2a;
  int y2a;
  int xya;
  int an;

  int xb;
  int yb;
  int x2b;
  int y2b;
  int xyb;
  int bn;
} lsfit_acc;

/***********************************************/

static void floor1_free_info(tianlai_info_floor *i){
  tianlai_info_floor1 *info=(tianlai_info_floor1 *)i;
  if(info){
    memset(info,0,sizeof(*info));
    free(info);
  }
}
static void floor1_free_look(tianlai_look_floor *i){
  tianlai_look_floor1 *look=(tianlai_look_floor1 *)i;
  if(look){
    
    memset(look,0,sizeof(*look));
    free(look);
  }
}

static int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static int ilog2(unsigned int v){
  int ret=0;
  if(v)--v;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}


static int icomp(const void *a,const void *b){
  return(**(int **)a-**(int **)b);
}


static tianlai_look_floor *floor1_look(tianlai_dsp_state *vd,
                                      tianlai_info_floor *in){
  int *sortpointer[VIF_POSIT+2];
  tianlai_info_floor1 *info=(tianlai_info_floor1 *)in;
  tianlai_look_floor1 *look=calloc(1,sizeof(*look));
  int i,j,n=0;
  look->vi=info;
  look->n=info->postlist[1];
  /* we drop each position value in-between already decoded values,
     and use linear interpolation to predict each new value past the
     edges.  The positions are read in the order of the position
     list... we precompute the bounding positions in the lookup.  Of
     course, the neighbors can change (if a position is declined), but
     this is an initial mapping */
  for(i=0;i<info->partitions;i++)n+=info->class_dim[info->partitionclass[i]];
  n+=2;
  look->posts=n;
  /* also store a sorted position index */
  for(i=0;i<n;i++)sortpointer[i]=info->postlist+i;
  qsort(sortpointer,n,sizeof(*sortpointer),icomp);
  /* points from sort order back to range number */
  for(i=0;i<n;i++)look->forward_index[i]=sortpointer[i]-info->postlist;
  /* points from range order to sorted position */
  for(i=0;i<n;i++)look->reverse_index[look->forward_index[i]]=i;
  /* we actually need the post values too */
  for(i=0;i<n;i++)look->sorted_index[i]=info->postlist[look->forward_index[i]];
  /* quantize values to multiplier spec */
  switch(info->mult){
  case 1: /* 1024 -> 256 */
    look->quant_q=256;
    break;
  case 2: /* 1024 -> 128 */
    look->quant_q=128;
    break;
  case 3: /* 1024 -> 86 */
    look->quant_q=86;
    break;
  case 4: /* 1024 -> 64 */
    look->quant_q=64;
    break;
  }
  /* discover our neighbors for decode where we don't use fit flags
     (that would push the neighbors outward) */
  for(i=0;i<n-2;i++){
    int lo=0;
    int hi=1;
    int lx=0;
    int hx=look->n;
    int currentx=info->postlist[i+2];
    for(j=0;j<i+2;j++){
      int x=info->postlist[j];
      if(x>lx && x<currentx){
        lo=j;
        lx=x;
      }
      if(x<hx && x>currentx){
        hi=j;
        hx=x;
      }
    }
    look->loneighbor[i]=lo;
    look->hineighbor[i]=hi;
  }
  return(look);
}

static int render_point(int x0,int x1,int y0,int y1,int x){
  y0&=0x7fff; /* mask off flag */
  y1&=0x7fff;

  {
    int dy=y1-y0;
    int adx=x1-x0;
    int ady=abs(dy);
    int err=ady*(x-x0);

    int off=err/adx;
    if(dy<0)return(y0-off);
    return(y0+off);
  }
}

static const float FLOOR1_fromdB_LOOKUP[256]={
  1.0649863e-07F, 1.1341951e-07F, 1.2079015e-07F, 1.2863978e-07F,
  1.3699951e-07F, 1.4590251e-07F, 1.5538408e-07F, 1.6548181e-07F,
  1.7623575e-07F, 1.8768855e-07F, 1.9988561e-07F, 2.128753e-07F,
  2.2670913e-07F, 2.4144197e-07F, 2.5713223e-07F, 2.7384213e-07F,
  2.9163793e-07F, 3.1059021e-07F, 3.3077411e-07F, 3.5226968e-07F,
  3.7516214e-07F, 3.9954229e-07F, 4.2550680e-07F, 4.5315863e-07F,
  4.8260743e-07F, 5.1396998e-07F, 5.4737065e-07F, 5.8294187e-07F,
  6.2082472e-07F, 6.6116941e-07F, 7.0413592e-07F, 7.4989464e-07F,
  7.9862701e-07F, 8.5052630e-07F, 9.0579828e-07F, 9.6466216e-07F,
  1.0273513e-06F, 1.0941144e-06F, 1.1652161e-06F, 1.2409384e-06F,
  1.3215816e-06F, 1.4074654e-06F, 1.4989305e-06F, 1.5963394e-06F,
  1.7000785e-06F, 1.8105592e-06F, 1.9282195e-06F, 2.0535261e-06F,
  2.1869758e-06F, 2.3290978e-06F, 2.4804557e-06F, 2.6416497e-06F,
  2.8133190e-06F, 2.9961443e-06F, 3.1908506e-06F, 3.3982101e-06F,
  3.6190449e-06F, 3.8542308e-06F, 4.1047004e-06F, 4.3714470e-06F,
  4.6555282e-06F, 4.9580707e-06F, 5.2802740e-06F, 5.6234160e-06F,
  5.9888572e-06F, 6.3780469e-06F, 6.7925283e-06F, 7.2339451e-06F,
  7.7040476e-06F, 8.2047000e-06F, 8.7378876e-06F, 9.3057248e-06F,
  9.9104632e-06F, 1.0554501e-05F, 1.1240392e-05F, 1.1970856e-05F,
  1.2748789e-05F, 1.3577278e-05F, 1.4459606e-05F, 1.5399272e-05F,
  1.6400004e-05F, 1.7465768e-05F, 1.8600792e-05F, 1.9809576e-05F,
  2.1096914e-05F, 2.2467911e-05F, 2.3928002e-05F, 2.5482978e-05F,
  2.7139006e-05F, 2.8902651e-05F, 3.0780908e-05F, 3.2781225e-05F,
  3.4911534e-05F, 3.7180282e-05F, 3.9596466e-05F, 4.2169667e-05F,
  4.4910090e-05F, 4.7828601e-05F, 5.0936773e-05F, 5.4246931e-05F,
  5.7772202e-05F, 6.1526565e-05F, 6.5524908e-05F, 6.9783085e-05F,
  7.4317983e-05F, 7.9147585e-05F, 8.4291040e-05F, 8.9768747e-05F,
  9.5602426e-05F, 0.00010181521F, 0.00010843174F, 0.00011547824F,
  0.00012298267F, 0.00013097477F, 0.00013948625F, 0.00014855085F,
  0.00015820453F, 0.00016848555F, 0.00017943469F, 0.00019109536F,
  0.00020351382F, 0.00021673929F, 0.00023082423F, 0.00024582449F,
  0.00026179955F, 0.00027881276F, 0.00029693158F, 0.00031622787F,
  0.00033677814F, 0.00035866388F, 0.00038197188F, 0.00040679456F,
  0.00043323036F, 0.00046138411F, 0.00049136745F, 0.00052329927F,
  0.00055730621F, 0.00059352311F, 0.00063209358F, 0.00067317058F,
  0.00071691700F, 0.00076350630F, 0.00081312324F, 0.00086596457F,
  0.00092223983F, 0.00098217216F, 0.0010459992F, 0.0011139742F,
  0.0011863665F, 0.0012634633F, 0.0013455702F, 0.0014330129F,
  0.0015261382F, 0.0016253153F, 0.0017309374F, 0.0018434235F,
  0.0019632195F, 0.0020908006F, 0.0022266726F, 0.0023713743F,
  0.0025254795F, 0.0026895994F, 0.0028643847F, 0.0030505286F,
  0.0032487691F, 0.0034598925F, 0.0036847358F, 0.0039241906F,
  0.0041792066F, 0.0044507950F, 0.0047400328F, 0.0050480668F,
  0.0053761186F, 0.0057254891F, 0.0060975636F, 0.0064938176F,
  0.0069158225F, 0.0073652516F, 0.0078438871F, 0.0083536271F,
  0.0088964928F, 0.009474637F, 0.010090352F, 0.010746080F,
  0.011444421F, 0.012188144F, 0.012980198F, 0.013823725F,
  0.014722068F, 0.015678791F, 0.016697687F, 0.017782797F,
  0.018938423F, 0.020169149F, 0.021479854F, 0.022875735F,
  0.024362330F, 0.025945531F, 0.027631618F, 0.029427276F,
  0.031339626F, 0.033376252F, 0.035545228F, 0.037855157F,
  0.040315199F, 0.042935108F, 0.045725273F, 0.048696758F,
  0.051861348F, 0.055231591F, 0.058820850F, 0.062643361F,
  0.066714279F, 0.071049749F, 0.075666962F, 0.080584227F,
  0.085821044F, 0.091398179F, 0.097337747F, 0.10366330F,
  0.11039993F, 0.11757434F, 0.12521498F, 0.13335215F,
  0.14201813F, 0.15124727F, 0.16107617F, 0.17154380F,
  0.18269168F, 0.19456402F, 0.20720788F, 0.22067342F,
  0.23501402F, 0.25028656F, 0.26655159F, 0.28387361F,
  0.30232132F, 0.32196786F, 0.34289114F, 0.36517414F,
  0.38890521F, 0.41417847F, 0.44109412F, 0.46975890F,
  0.50028648F, 0.53279791F, 0.56742212F, 0.60429640F,
  0.64356699F, 0.68538959F, 0.72993007F, 0.77736504F,
  0.82788260F, 0.88168307F, 0.9389798F, 1.F,
};



static void render_line0(int n, int x0,int x1,int y0,int y1,int *d){
  int dy=y1-y0;
  int adx=x1-x0;
  int ady=abs(dy);
  int base=dy/adx;
  int sy=(dy<0?base-1:base+1);
  int x=x0;
  int y=y0;
  int err=0;

  ady-=abs(base*adx);

  if(n>x1)n=x1;

  if(x<n)
    d[x]=y;

  while(++x<n){
    err=err+ady;
    if(err>=adx){
      err-=adx;
      y+=sy;
    }else{
      y+=base;
    }
    d[x]=y;
  }
}

static void *floor1_inverse1(tianlai_block *vb,tianlai_look_floor *in, int *fit_value,
							 float *decmask)
{
  tianlai_look_floor1 *look=(tianlai_look_floor1 *)in;
  tianlai_info_floor1 *info=look->vi;
  codec_setup_info   *ci=vb->vd->vi->codec_setup;
  int i,j,k;
  codebook *books=ci->fullbooks;
  /* unpack wrapped/predicted values from stream */
  if(avs2audiopack_read(&vb->opb,1)==1){
    fit_value[0]=avs2audiopack_read(&vb->opb,ilog(look->quant_q-1));
    fit_value[1]=avs2audiopack_read(&vb->opb,ilog(look->quant_q-1));
    /* partition by partition */
    for(i=0,j=2;i<info->partitions;i++){
      int class=info->partitionclass[i];
      int cdim=info->class_dim[class];
      int csubbits=info->class_subs[class];
      int csub=1<<csubbits;
      int cval=0;
      /* decode the partition's first stage cascade value */
      if(csubbits){
        cval=tianlai_book_decode(books+info->class_book[class],&vb->opb);
        if(cval==-1)goto eop;
      }
      for(k=0;k<cdim;k++){
        int book=info->class_subbook[class][cval&(csub-1)];
        cval>>=csubbits;
        if(book>=0){
          if((fit_value[j+k]=tianlai_book_decode(books+book,&vb->opb))==-1)
            goto eop;
        }else{
          fit_value[j+k]=0;
        }
      }
      j+=cdim;
    }
    /* unwrap positive values and reconsitute via linear interpolation */
    for(i=2;i<look->posts;i++){
      int predicted=render_point(info->postlist[look->loneighbor[i-2]],
                                 info->postlist[look->hineighbor[i-2]],
                                 fit_value[look->loneighbor[i-2]],
                                 fit_value[look->hineighbor[i-2]],
                                 info->postlist[i]);
      int hiroom=look->quant_q-predicted;
      int loroom=predicted;
      int room=(hiroom<loroom?hiroom:loroom)<<1;
      int val=fit_value[i];
      if(val){
        if(val>=room){
          if(hiroom>loroom){
            val = val-loroom;
          }else{
            val = -1-(val-hiroom);
          }
        }else{
          if(val&1){
            val= -((val+1)>>1);
          }else{
            val>>=1;
          }
        }
        fit_value[i]=(val+predicted)&0x7fff;
        fit_value[look->loneighbor[i-2]]&=0x7fff;
        fit_value[look->hineighbor[i-2]]&=0x7fff;
      }else{
        fit_value[i]=predicted|0x8000;
      }
    }


	///////////
	{
		int decilogmask[1024*2];	//wuchaogang 2013.7.18
	    int n=ci->blocksizes[vb->W]/2/2;//wuchaogang 2013.7.18
      int hx=0;
    int lx=0;
    int ly=fit_value[0]*info->mult;

    /* guard lookup against out-of-range values */
    ly=(ly<0?0:ly>255?255:ly);


    for(j=1;j<look->posts;j++){
      int current=look->forward_index[j];
      int hy=fit_value[current]&0x7fff;
      if(hy==fit_value[current]){

        hx=info->postlist[current];
        hy*=info->mult;
        /* guard lookup against out-of-range values */
        hy=(hy<0?0:hy>255?255:hy);

        
		render_line0(n,lx,hx,ly,hy,decilogmask);
        
        lx=hx;
        ly=hy;
      }
    }
    for(j=hx;j<n;j++){
		
		decilogmask[j]=ly;
	}

	for(j=0;j<n;j++){
        decmask[j]=FLOOR1_fromdB_LOOKUP[decilogmask[j]]; /* be certain */
	}
    }
	///////////
    return(fit_value);
  }
 eop:
  return(NULL);
}

static int floor1_inverse2(tianlai_block *vb,tianlai_look_floor *in,float *decmask,
                          float *out){
  tianlai_look_floor1 *look=(tianlai_look_floor1 *)in;
  tianlai_info_floor1 *info=look->vi;

  codec_setup_info   *ci=vb->vd->vi->codec_setup;
  int                n=ci->blocksizes[vb->W]/2/2;//wuchaogang 2013.7.18
  int j;

  if(decmask){
  

	 for(j=0;j<n;j++){
		out[j]*=decmask[j]; /* be certain */ 
	
		}
	
	
    return(1);
  }
  memset(out,0,sizeof(*out)*n);
  return(0);
}

/* export hooks */
const tianlai_func_floor floor1_exportbundle={
	  &floor1_look,&floor1_free_info,
  &floor1_free_look,&floor1_inverse1,&floor1_inverse2
};
