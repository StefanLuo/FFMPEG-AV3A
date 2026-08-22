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
#include <assert.h>
#include <math.h>
#include "lfenc.h"
#include "lfdec.h"
#include "registry.h"
#include "../bwedec/avs2BweDecMDFT.h"
#include "setup_44.h"
#include "setup_44u.h"
#include "../bwedec/decoder.h"

#define EVENODD 1
#define PCAQUANT4 8.0
#define PCAQUANT4LEN 4
#define MULBANDSTART 14

extern long frame;
extern int Maxpcmvalue;
tianlai_block     vb;
extern   codec_setup_info ci_table[13];
extern int road;
extern struct AVS2_DECODER_INSTANCE Avs2DecoderInstance[];
extern struct AVS2_DECODER_INSTANCE Avs2DecoderInstance_frame[];
extern struct AVS2_DECODER_INSTANCE Avs2DecoderInstance_objframe[128][MAX_ALLCHANNEL];
private_state private_state_table[13];

extern int bandnumset[];

extern float anaMatrixdata[3][8][50][2][8*8]; //2014.11.13 wchg 修改PCA频带划分与MCR一致
unsigned int allbandsetflag[3][8][50]; //2014.11.13 wchg 修改PCA频带划分与MCR一致

float alldecmask[6][8][1][2048];
//extern HANDLE_STAvs2Dec objphstAvs2Dec[];
extern int index_obj;

int bandmulflag[3][8];
/* lowpass by mode **************/
double lowpass_44[13]={
   3.45, 7.2,16.2,16.2,16.5,17.2,18.9,22.05,22.05,22.05,22.05,22.05,22.05//lijing added 3.45
};

double lowpass_32[12+1]={
   3.45, 10.5,12.2,12.2,12.5,14.2,14.9,16.0,16.0,16.0,16.0,16.0,16.0//lijing added 3.45
};

double _psy_lowpass_44_multi[5]={//lijing added
   7.2, 17.2, 18.9, 22.05, 22.05
};

 const int lf_winseq_table[40][20]=
{
{1,4,4},//0
{1,4,3},//1
{1,4,2},//2
{1,4,1},//3
{1,3,4},//4
{2,3,3,3},//5
{2,3,3,2},//6
{4,3,1,1,1,2},//7
{2,3,3,1},//8
{5,3,1,1,1,1,1},//9
{1,2,4},//10
{3,2,2,2,3},//11
{4,2,1,1,1,3},//12
{4,2,2,2,2,2},//13
{5,2,1,1,1,2,2},//{4,2,2,2,2,2},//14
{5,2,2,1,1,1,2},//15
{6,2,1,1,1,1,1,2},//16
{2,2,3,2},//17
{5,2,2,2,1,1,1},//18
{5,2,1,1,1,2,1},//19
{6,2,2,1,1,1,1,1},//20
{7,2,1,1,1,1,1,1,1},//21
{2,2,3,1},//22
{1,1,4},//23
{4,1,1,1,2,3},//24
{2,1,3,3},//25
{5,1,1,1,2,2,2},//26
{6,1,1,1,1,1,2,2},//27
{5,1,2,1,1,1,2},//28
{7,1,1,1,1,1,1,1,2},//29
{2,1,3,2},//30
{6,1,1,1,2,1,1,1},//31
{5,1,1,2,2,1,1},//{6,1,1,1,1,1,2,1},//32
{6,1,2,1,1,1,1,1},//{5,1,2,1,1,1,2},//33
{8,1,1,1,1,1,1,1,1,1},//34
{2,1,3,1},//35
};

static const ve_setup_data_template *const setup_list[]={

  &ve_setup_44_uncoupled,
  0
};

static const tianlai_info_mode _mode_template[2*2]={
  {0,0,0,0},
  {1,0,0,1},
   {1,0,0,1},//wuchaogang
   {1,0,0,1}//wuchaogang
};

static const tianlai_info_mapping0 _map_nominal[2*2]={
  {1, {0,0}, {0}, {0}, 1,{0},{1}},
  {1, {0,0}, {1}, {1}, 1,{0},{1}},
  {1, {0,0}, {1}, {1}, 1,{0},{1}},//wuchaogang
  {1, {0,0}, {1}, {1}, 1,{0},{1}}//wuchaogang
};

SR_INFO srInfo =
{
	 49, 14, 22, 35,
	{
		4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  8,
            12, 12, 12, 12, 16, 16, 20, 20, 24, 24, 28, 28, 32, 32, 32, 32, 32, 32,
            32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 96
	}, {
		4,  4,  4,  4,  4,  8,  8,  8, 12, 12, 12, 16, 16, 16
    }, {
        4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  12,  12,  12,  16,  16,  24,
		24, 24,  24, 28
	}, {
		4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  12,  12, 
		12, 12,  16, 16, 16, 20,  20, 20, 24, 24, 24, 24, 28, 28, 28, 28, 32, 32
	}
};


static int ilog2(unsigned int v){
  int ret=0;
  if(v)--v;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

#ifndef WORD_ALIGN
#define WORD_ALIGN 8
#endif

int tianlai_block_init(tianlai_dsp_state *v, tianlai_block *vb){
  int i;

  memset(vb,0,sizeof(*vb));
  vb->vd=v;
  vb->localalloc=0;
  vb->localstore=NULL;

  if(v->analysisp){
    tianlai_block_internal *vbi=
      vb->internal = calloc(1,sizeof(tianlai_block_internal));
    vbi->ampmax=-9999;

    for(i=0;i<PACKETBLOBS;i++){
      if(i==PACKETBLOBS/2){
        vbi->packetblob[i]=&vb->opb;
      }else{
        vbi->packetblob[i]=
          calloc(1,sizeof(avs2audiopack_buffer));
      }
//      avs2audiopack_writeinit(vbi->packetblob[i]);
    }
  }

  return(0);
}

void *_tianlai_block_alloc(tianlai_block *vb,long bytes){
  bytes=(bytes+(WORD_ALIGN-1)) & ~(WORD_ALIGN-1);
  if(bytes+vb->localtop>vb->localalloc){
    /* can't just _avs2audio_realloc... there are outstanding pointers */
    if(vb->localstore){
      struct alloc_chain *link=malloc(sizeof(*link));
      vb->totaluse+=vb->localtop;
      link->next=vb->reap;
      link->ptr=vb->localstore;
      vb->reap=link;
    }
    /* highly conservative */
    vb->localalloc=bytes;
    vb->localstore=malloc(vb->localalloc);
    vb->localtop=0;
  }
  {
    void *ret=(void *)(((char *)vb->localstore)+vb->localtop);
    vb->localtop+=bytes;
    return ret;
  }
}

static void tianlai_encode_floor_setup(codec_setup_info *ci,//tianlai_info *vi,
									   int s,
                                     const static_codebook *const *const *const books,
                                     const tianlai_info_floor1 *in,
                                     const int *x){
  int i,k,is=s;
  tianlai_info_floor1 *f=calloc(1,sizeof(*f));
  //codec_setup_info *ci=vi->codec_setup;

  memcpy(f,in+x[is],sizeof(*f));

  /* books */
  {
    int partitions=f->partitions;
    int maxclass=-1;
    int maxbook=-1;
    for(i=0;i<partitions;i++)
      if(f->partitionclass[i]>maxclass)maxclass=f->partitionclass[i];
    for(i=0;i<=maxclass;i++)
	{
      if(f->class_book[i]>maxbook)maxbook=f->class_book[i];
      f->class_book[i]+=ci->books;
      for(k=0;k<(1<<f->class_subs[i]);k++)
	  {
        if(f->class_subbook[i][k]>maxbook)maxbook=f->class_subbook[i][k];
        if(f->class_subbook[i][k]>=0)f->class_subbook[i][k]+=ci->books;
      }
    }

    for(i=0;i<=maxbook;i++)
      ci->book_param[ci->books++]=(static_codebook *)books[x[is]][i];
  }

  /* for now, we're only using floor 1 */
  ci->floor_type[ci->floors]=1;
  ci->floor_param[ci->floors]=f;
  ci->floors++;

  return;
}

static int book_dup_or_new(codec_setup_info *ci,const static_codebook *book){
  int i;
  for(i=0;i<ci->books;i++)
    if(ci->book_param[i]==book)return(i);

  return(ci->books++);
}
static void tianlai_encode_residue_setup(codec_setup_info *ci,//tianlai_info *vi,
                                        int number, int block,
                                        const tianlai_residue_template *res){

//  codec_setup_info *ci=vi->codec_setup;
  int i;

  tianlai_info_residue0 *r = ci->residue_param[number] = malloc(sizeof(*r));

  memcpy(r,res->res,sizeof(*r));
  if(ci->residues<=number)ci->residues=number+1;

  r->grouping=res->grouping;
  ci->residue_type[number]=res->res_type;

  /* fill in all the books */
  {
    int booklist=0,k;

    if(ci->hi.managed){
      for(i=0;i<r->partitions;i++)
        for(k=0;k<4;k++)
          if(res->books_base_managed->books[i][k])
            r->secondstages[i]|=(1<<k);

      r->groupbook=book_dup_or_new(ci,res->book_aux_managed);
      ci->book_param[r->groupbook]=(static_codebook *)res->book_aux_managed;

      for(i=0;i<r->partitions;i++){
        for(k=0;k<4;k++){
          if(res->books_base_managed->books[i][k]){
            int bookid=book_dup_or_new(ci,res->books_base_managed->books[i][k]);
            r->booklist[booklist++]=bookid;
            ci->book_param[bookid]=(static_codebook *)res->books_base_managed->books[i][k];
          }
        }
      }

    }else{

      for(i=0;i<r->partitions;i++)
        for(k=0;k<4;k++)
          if(res->books_base->books[i][k])
            r->secondstages[i]|=(1<<k);

      r->groupbook=book_dup_or_new(ci,res->book_aux);
      ci->book_param[r->groupbook]=(static_codebook *)res->book_aux;

      for(i=0;i<r->partitions;i++){
        for(k=0;k<4;k++){
          if(res->books_base->books[i][k]){
            int bookid=book_dup_or_new(ci,res->books_base->books[i][k]);
            r->booklist[booklist++]=bookid;
            ci->book_param[bookid]=(static_codebook *)res->books_base->books[i][k];
          }
        }
      }
    }
  }

 
}


static void tianlai_encode_map_n_res_setup(codec_setup_info *ci,//tianlai_info *vi,
										   double s,
                                          const tianlai_mapping_template *maps){

 // codec_setup_info *ci=vi->codec_setup;
  int i,j,is=s,modes=2*2;//wuchaogang 2013.8.26 *2
  const tianlai_info_mapping0 *map=maps[is].map;
  const tianlai_info_mode *mode=_mode_template;
  const tianlai_residue_template *res=maps[is].res;

//  if(ci->blocksizes[0]==ci->blocksizes[1])modes=1;

  for(i=0;i<modes;i++){

    ci->map_param[i]= calloc(1,sizeof(*map));
    ci->mode_param[i]= calloc(1,sizeof(*mode));

    memcpy(ci->mode_param[i],mode+i,sizeof(*_mode_template));
    if(i>=ci->modes)ci->modes=i+1;

    ci->map_type[i]=0;
    memcpy(ci->map_param[i],map+i,sizeof(*map));
    if(i>=ci->maps)ci->maps=i+1;

    for(j=0;j<map[i].submaps;j++)
      tianlai_encode_residue_setup(ci/*vi*/,map[i].residuesubmap[j],i
                                  ,res+map[i].residuesubmap[j]);
  }
}

/* Analysis side code, but directly related to blocking.  Thus it's
   here and not in analysis.c (which is for analysis transforms only).
   The init is here because some of it is shared */

int tianlai_synthesis_init(tianlai_dsp_state *v,tianlai_info *vi, int bitRate){
  int i;
  codec_setup_info *ci=vi->codec_setup;

  int hs;
 
  ve_setup_data_template *setup;
  float base_setting;


  setup = setup_list[0];//setup_list[1]-->setup_list[0] wuchaogang 2013.01.16

  for(i=0;i<setup->mappings;i++)
	  if(bitRate>=setup->rate_mapping[i] && bitRate<setup->rate_mapping[i+1])
		  break;
  /* an all-points match */
  if(i==setup->mappings)
    base_setting=i-.001;
  else
  {
    float low=setup->rate_mapping[i];
    float high=setup->rate_mapping[i+1];
    float del=(bitRate-low)/(high-low);
    base_setting=i+del;
  }
  
  memset(v,0,sizeof(*v));
  // setting vi->codec_setup, point to citable[s]
  {
	  int s=base_setting;
	  //s=0;
	  ci=vi->codec_setup = &(ci_table[s]);
	  v->backend_state =&(private_state_table[s]);
  }
  
  if(ci==NULL) return 1;
  hs=ci->halfrate_flag;

  v->vi=vi;


  /* initialize the storage vectors. blocksize[1] is small for encode,
     but the correct size for decode */
  v->pcm_storage=ci->blocksizes[1];
  v->pcm=_avs2audio_malloc(vi->channels*sizeof(*v->pcm));
  v->pcmret=_avs2audio_malloc(vi->channels*sizeof(*v->pcmret));
  {
    int i;
    for(i=0;i<vi->channels;i++)
      v->pcm[i]=_avs2audio_calloc(v->pcm_storage,sizeof(*v->pcm[i]));
  }

  /* all 1 (large block) or 0 (small block) */
  /* explicitly set for the sake of clarity */
  v->lW=0; /* previous window size */
  v->W=0;  /* current window size */

  /* all vector indexes */
  v->centerW=ci->blocksizes[1]/2;

  v->pcm_current=v->centerW;

  return 0;

}

//ci_table 初始化
int ci_settable_init(codec_setup_info *ci_table)
{  
ve_setup_data_template *setup = setup_list[0];
  private_state *b=NULL;
  
  int i,j;

	for(j=0;j<setup->mappings;j++)
	{
		b = &(private_state_table[j]);
		ci_table[j].blocksizes[0]=512;
		ci_table[j].blocksizes[1]=4096;
		ci_table[j].blocksizes[2]=2048;
		ci_table[j].blocksizes[3]=1024;

		for(i=0;i<setup->floor_mappings;i++)
		tianlai_encode_floor_setup(&ci_table[j], j,
                              setup->floor_books,
                              setup->floor_params,
                              setup->floor_mapping_list[i]);


		tianlai_encode_map_n_res_setup(&ci_table[j]/*vi*/,j,setup->maps);

	 /* finish the codebooks */
		if(!ci_table[j].fullbooks){
      ci_table[j].fullbooks=_avs2audio_calloc(ci_table[j].books,sizeof(*(ci_table[j].fullbooks)));
      for(i=0;i<ci_table[j].books;i++){
        if(ci_table[j].book_param[i]==NULL)
          goto abort_books_table;
        if(tianlai_book_init_decode(ci_table[j].fullbooks+i,ci_table[j].book_param[i]))
          goto abort_books_table;
        /* decode codebooks are now standalone after init */
        tianlai_staticbook_destroy(ci_table[j].book_param[i]);
        ci_table[j].book_param[i]=NULL;
      }
    }

	b->modebits=ilog2(ci_table[j].modes);
  /* initialize all the backend lookups */
	b->flr=_avs2audio_calloc(ci_table[j].floors,sizeof(*b->flr));
	b->residue=_avs2audio_calloc(ci_table[j].residues,sizeof(*b->residue));

	for(i=0;i<ci_table[j].floors;i++)
		b->flr[i]=_floor_P[ci_table[j].floor_type[i]]->
		look(&ci_table[j],ci_table[j].floor_param[i]);

	for(i=0;i<ci_table[j].residues;i++)
		b->residue[i]=_residue_P[ci_table[j].residue_type[i]]->
		look(&ci_table[j],ci_table[j].residue_param[i]);



	abort_books_table:
	for(i=0;i<ci_table[j].books;i++){
	 if(ci_table[j].book_param[i]!=NULL){
      tianlai_staticbook_destroy(ci_table[j].book_param[i]);
      ci_table[j].book_param[i]=NULL;
    }
  }

	}
	return 0;
}


int ci_settable_reset(AVS2Dec_File *vf,int s)//tianlai_dsp_state *v,tianlai_info *vi,int s)
{
	vf->vi->codec_setup = &(ci_table[s]);
  vf->vd.backend_state =&(private_state_table[s]);

	return 0;
}

int vb_ci_settable_reset(tianlai_block *vb,int s)//tianlai_dsp_state *v,tianlai_info *vi,int s)
{ tianlai_dsp_state     *vd=vb->vd;
    tianlai_info          *vi=vd->vi;
	vi->codec_setup = &(ci_table[s]);
  vd->backend_state =&(private_state_table[s]);

	return 0;
}

int reset_bandWidth(int samplingRate,int idx,int *bandWidth, int nChannels, int bitRateIndex, int ifLFE, int useBWE,char ElementInstanceTag)
{
	double lowpass_kHz;

	if(nChannels >= 5 && !ifLFE)
		lowpass_kHz = _psy_lowpass_44_multi[bitRateIndex];
	else if(samplingRate > 16000)
		lowpass_kHz = lowpass_44[ElementInstanceTag];
	else
		lowpass_kHz = lowpass_32[ElementInstanceTag];

  if(!useBWE)
  {
	  if(lowpass_kHz >= 22.05)
		  *bandWidth = (int)(samplingRate * 1024 / samplingRate / 32 + 0.9) * 32 ;
	  else
		  *bandWidth = (int)(lowpass_kHz * 1000 * 1024 / samplingRate / 32 + 0.9) * 32 ;
  }
  else
  { 
	  if(idx >=21 && samplingRate == 24000)
		  *bandWidth = (int)(samplingRate * 1024 / samplingRate / 32 + 0.9) * 32 ;
	  else
		  //*bandWidth = (int)(lowpass_44[idx] * 1000 * 1024 / samplingRate / 32 + 0.9) * 32;
		  setBWEbandWidth(bandWidth, idx);
  }

  return 0;
}

int reset_bandWidth0(int samplingRate,int idx,int *bandWidth, int useBWE,char ElementInstanceTag)
{
	double lowpass_kHz;

	if(samplingRate > 16000)
		lowpass_kHz = lowpass_44[ElementInstanceTag];
	else
		lowpass_kHz = lowpass_32[ElementInstanceTag];

	if(!useBWE)
  {
	  if(lowpass_kHz >= 22.05)
		  *bandWidth = (int)(samplingRate * 1024 / samplingRate / 32 + 0.9) * 32 ;
	  else
		  *bandWidth = (int)(lowpass_kHz * 1000 * 1024 / samplingRate / 32 + 0.9) * 32 ;
  }
  else
  { 
	  if(idx >=21 && samplingRate == 24000)
		  *bandWidth = (int)(samplingRate * 1024 / samplingRate / 32 + 0.9) * 32 ;
	  else
	  {
		  setBWEbandWidth(bandWidth, idx);
	  }
  }

  return 0;
}

void Avs2Decoder_syn(AVS2DECODER self,
					 int useBWE,
					 float MdctSpectrum[],
					 float *pTimeData,
					 int bitRate,
					 int bitsPerSample)
{
	tianlai_block *vb = &(self->vf.vb);
#if 0
	int ch;
	int i,j;
	int Seqmode;
	float Mdftout[4096*2];
	int lf_winseq_dec[20];


	memcpy(lf_winseq_dec,lf_winseq_table[self->lf_winseq_index],20*4);

	/* recover the spectral envelope; store it in the PCM vector for now */
	for (ch=0; ch<(self->pStreamInfo->Channels); ch++) 
	{
        
		if(useBWE)
		{
			for (i = 0; i < FRAME_SIZE; i++) {
			  float tmp2;
			  tmp2 = MdctSpectrum[i];
			  MdctSpectrum[i] = vb->SpectralCoefficient[i];
			  vb->SpectralCoefficient[i] = tmp2;
			}		
		}
        
  		for(j=0;j<4096;j++)
			Mdftout[j]= 0;
		for(j=0;j<1024;j++)
		{
			Mdftout[2*j]= MdctSpectrum[j]*8;
			Mdftout[2*j+1]= 0;
		}

 		 Avs2LFDecMDFTsyn(self->st1_decin, &self->pTimeData[ch*FRAME_SIZE], Mdftout, useBWE, lf_winseq_dec);

		if(useBWE)
		{
			/*wuchaogang mdft decoder analysis */
			Seqmode = Avs2BweDecMDFTana(self->st1_decin, self->st_deccommon,&self->pTimeData[ch*FRAME_SIZE],ch);
         
         
			/* decode one bwe frame */
		//	if(type != ID_LFE)
			if(Seqmode != -1)
				Avs2BweDecoder(bitRate, self->pStreamBWE, self->st2_decin, self->st_deccommon,bitsPerSample,ch);


			 /*对处理后的mdft系数进行逆变换，并进行加窗混叠重构，得到时域信号（FRAME_SIZE*2=1024*2长度）*/
			if(Seqmode != -1)
				Avs2BweDecMDFTsyn(self->st1_decin, self->st_deccommon, &pTimeData[ch*FRAME_SIZE * 2], ch);
		}
		else
			memcpy(&pTimeData[ch * FRAME_SIZE], &self->pTimeData[ch*FRAME_SIZE], FRAME_SIZE * sizeof(float));

		//对本帧的编码模式序号Seqmode进行状态更新，暂时从编码器存储的enc_preSeqmode[ch]来获得此信息
		Avs2DecMDFTupdate(self->st1_decin, self->st_deccommon, ch, lf_winseq_dec);

	}
#else
	int i,j;
	int Seqmode;
	float Mdftout[4096*2];	
	int lf_winseq_dec[20];

	StAvs2BweDecMDFT *pstBweMDFT;
	StAvs2BweDecCommon *pstBweCommon; 

	memcpy(lf_winseq_dec, lf_winseq_table[self->lf_winseq_index], 20*4);

	/* recover the spectral envelope; store it in the PCM vector for now */
	if(useBWE)
	{
		pstBweCommon = (StAvs2BweDecCommon *)(self->st_deccommon);

		memcpy(pstBweCommon->bweMdftSpectrum, MdctSpectrum, (FRAME_SIZE * 4+2048) * sizeof(float));

		/*对处理后的mdft系数进行逆变换，并进行加窗混叠重构，得到时域信号（FRAME_SIZE*2=1024*2长度）*/
		Avs2BweDecMDFTsyn(self->st1_decin, self->st_deccommon, pTimeData, 0);

	}
	else
	{
  		for(j=0;j<4096;j++)
			Mdftout[j]= 0;
		for(j = 0; j < 1024; j++)
		{
			Mdftout[2*j] = MdctSpectrum[j]*8;
			Mdftout[2*j+1] = 0;
		}

		//Avs2LFDecMDFTsyn(self->st1_decin, &self->pTimeData[ch*FRAME_SIZE], Mdftout, useBWE, lf_winseq_dec);
		pstBweMDFT = (StAvs2BweDecMDFT*)(self->st1_decin);
		
		imdft_lowpassframe4096block_multi(self->pTimeData, lf_winseq_dec, Mdftout, 0, 4096/4/2, pstBweMDFT->Srstereo[0]);

		memcpy(pTimeData, self->pTimeData, FRAME_SIZE * sizeof(float));
		
	}
	//对本帧的编码模式序号Seqmode进行状态更新，暂时从编码器存储的enc_preSeqmode[ch]来获得此信息
	Avs2DecMDFTupdate(self->st1_decin, self->st_deccommon, 0, lf_winseq_dec);


#endif
	return ;
}

#define BOOKSCL 12
#define MIDSCALEFAC 60
#define OFFSET_OF_SF 100
#define FNO_ERROR    0
#define FERROR       1

HuffmanWord huffmantablescl[] = {
{     0,    27,  66932106  },
{     1,    27,  66932107  },
{     2,    27,  66932108  },
{     3,    27,  66932109  },
{     4,    27,  66932110  },
{     5,    27,  66932111  },
{     6,    27,  66932112  },
{     7,    27,  66932113  },
{     8,    27,  66932114  },
{     9,    27,  66932115  },
{    10,    27,  66932116  },
{    11,    27,  66932117  },
{    12,    27,  66932118  },
{    13,    27,  66932119  },
{    14,    27,  66932120  },
{    15,    27,  66932121  },
{    16,    27,  66932122  },
{    17,    27,  66932123  },
{    18,    27,  66932124  },
{    19,    27,  66932125  },
{    20,    27,  66932126  },
{    21,    27,  66932127  },
{    22,    26,  33466048  },
{    23,    26,  33466049  },
{    24,    26,  33466050  },
{    25,    16,     14271  },
{    26,    22,   2091629  },
{    27,    16,     14269  },
{    28,    21,   1045815  },
{    29,    20,    228322  },
{    30,    19,    114148  },
{    31,    19,    114149  },
{    32,    16,     32756  },
{    33,    14,      8171  },
{    34,    16,     32757  },
{    35,    14,      8188  },
{    36,    14,      3558  },
{    37,    13,      1778  },
{    38,    15,     16341  },
{    39,    12,       888  },
{    40,    12,      2032  },
{    41,    12,       890  },
{    42,    12,      2046  },
{    43,    11,      1020  },
{    44,    10,       223  },
{    45,    10,       221  },
{    46,     9,       108  },
{    47,     9,       252  },
{    48,     8,        50  },
{    49,     8,        63  },
{    50,     7,        26  },
{    51,     7,        30  },
{    52,     6,         8  },
{    53,     6,        14  },
{    54,     6,        38  },
{    55,     5,         5  },
{    56,     5,        18  },
{    57,     5,        23  },
{    58,     4,         8  },
{    59,     3,         0  },
{    60,     2,         3  },
{    61,     3,         2  },
{    62,     4,        10  },
{    63,     4,         6  },
{    64,     5,        22  },
{    65,     5,        14  },
{    66,     6,        39  },
{    67,     6,        30  },
{    68,     6,         9  },
{    69,     7,        62  },
{    70,     7,        24  },
{    71,     8,        62  },
{    72,     8,        51  },
{    73,     9,       253  },
{    74,     9,       109  },
{    75,    10,       509  },
{    76,    11,      1022  },
{    77,    10,       220  },
{    78,    11,      1017  },
{    79,    12,      2043  },
{    80,    12,      2033  },
{    81,    13,      1782  },
{    82,    14,      8168  },
{    83,    13,      4095  },
{    84,    14,      8169  },
{    85,    14,      3559  },
{    86,    18,    130727  },
{    87,    15,     16379  },
{    88,    15,      7132  },
{    89,    15,      7133  },
{    90,    17,     28536  },
{    91,    19,    114151  },
{    92,    19,    114160  },
{    93,    19,    114150  },
{    94,    20,    522906  },
{    95,    16,     32680  },
{    96,    20,    228323  },
{    97,    20,    228324  },
{    98,    20,    228325  },
{    99,    20,    228326  },
{   100,    20,    228327  },
{   101,    20,    228328  },
{   102,    20,    228329  },
{   103,    20,    228330  },
{   104,    20,    228331  },
{   105,    20,    228332  },
{   106,    20,    228333  },
{   107,    20,    228334  },
{   108,    20,    228335  },
{   109,    20,    522896  },
{   110,    20,    522897  },
{   111,    20,    522898  },
{   112,    20,    522899  },
{   113,    20,    522900  },
{   114,    20,    522901  },
{   115,    20,    522902  },
{   116,    20,    522903  },
{   117,    20,    522904  },
{   118,    20,    522905  },
{   119,    26,  33466051  },
{   120,    26,  33466052  },
};

float FLOOR_LOOKUP[512] = 
{
2.980232e-008f,	3.544113e-008f,	4.214685e-008f,	5.012133e-008f,	5.960464e-008f,	7.088227e-008f,	8.429370e-008f,	1.002427e-007f,	1.192093e-007f,	1.417645e-007f,	
1.685874e-007f,	2.004853e-007f,	2.384186e-007f,	2.835291e-007f,	3.371748e-007f,	4.009707e-007f,	4.768372e-007f,	5.670581e-007f,	6.743496e-007f,	8.019413e-007f,	
9.536743e-007f,	1.134116e-006f,	1.348699e-006f,	1.603883e-006f,	1.907349e-006f,	2.268232e-006f,	2.697398e-006f,	3.207765e-006f,	3.814697e-006f,	4.536465e-006f,	
5.394797e-006f,	6.415531e-006f,	7.629395e-006f,	9.072930e-006f,	1.078959e-005f,	1.283106e-005f,	1.525879e-005f,	1.814586e-005f,	2.157919e-005f,	2.566212e-005f,	
3.051758e-005f,	3.629172e-005f,	4.315837e-005f,	5.132424e-005f,	6.103516e-005f,	7.258344e-005f,	8.631674e-005f,	1.026485e-004f,	1.220703e-004f,	1.451669e-004f,	
1.726335e-004f,	2.052970e-004f,	2.441406e-004f,	2.903338e-004f,	3.452670e-004f,	4.105940e-004f,	4.882813e-004f,	5.806675e-004f,	6.905340e-004f,	8.211879e-004f,	
9.765625e-004f,	1.161335e-003f,	1.381068e-003f,	1.642376e-003f,	1.953125e-003f,	2.322670e-003f,	2.762136e-003f,	3.284752e-003f,	3.906250e-003f,	4.645340e-003f,	
5.524272e-003f,	6.569503e-003f,	7.812500e-003f,	9.290680e-003f,	1.104854e-002f,	1.313901e-002f,	1.562500e-002f,	1.858136e-002f,	2.209709e-002f,	2.627801e-002f,	
3.125000e-002f,	3.716272e-002f,	4.419417e-002f,	5.255603e-002f,	6.250000e-002f,	7.432544e-002f,	8.838835e-002f,	1.051121e-001f,	1.250000e-001f,	1.486509e-001f,	
1.767767e-001f,	2.102241e-001f,	2.500000e-001f,	2.973018e-001f,	3.535534e-001f,	4.204482e-001f,	5.000000e-001f,	5.946035e-001f,	7.071068e-001f,	8.408964e-001f,	
1.000000e+000f,	1.189207e+000f,	1.414214e+000f,	1.681793e+000f,	2.000000e+000f,	2.378414e+000f,	2.828427e+000f,	3.363586e+000f,	4.000000e+000f,	4.756828e+000f,	
5.656854e+000f,	6.727171e+000f,	8.000000e+000f,	9.513657e+000f,	1.131371e+001f,	1.345434e+001f,	1.600000e+001f,	1.902731e+001f,	2.262742e+001f,	2.690869e+001f,	
3.200000e+001f,	3.805463e+001f,	4.525483e+001f,	5.381737e+001f,	6.400000e+001f,	7.610925e+001f,	9.050967e+001f,	1.076347e+002f,	1.280000e+002f,	1.522185e+002f,	
1.810193e+002f,	2.152695e+002f,	2.560000e+002f,	3.044370e+002f,	3.620387e+002f,	4.305390e+002f,	5.120000e+002f,	6.088740e+002f,	7.240773e+002f,	8.610779e+002f,	
1.024000e+003f,	1.217748e+003f,	1.448155e+003f,	1.722156e+003f,	2.048000e+003f,	2.435496e+003f,	2.896309e+003f,	3.444312e+003f,	4.096000e+003f,	4.870992e+003f,	
5.792619e+003f,	6.888624e+003f,	8.192000e+003f,	9.741984e+003f,	1.158524e+004f,	1.377725e+004f,	1.638400e+004f,	1.948397e+004f,	2.317047e+004f,	2.755449e+004f,	
3.276800e+004f,	3.896794e+004f,	4.634095e+004f,	5.510899e+004f,	6.553600e+004f,	7.793588e+004f,	9.268190e+004f,	1.102180e+005f,	1.310720e+005f,	1.558718e+005f,	
1.853638e+005f,	2.204360e+005f,	2.621440e+005f,	3.117435e+005f,	3.707276e+005f,	4.408719e+005f,	5.242880e+005f,	6.234870e+005f,	7.414552e+005f,	8.817438e+005f,	
1.048576e+006f,	1.246974e+006f,	1.482910e+006f,	1.763488e+006f,	2.097152e+006f,	2.493948e+006f,	2.965821e+006f,	3.526975e+006f,	4.194304e+006f,	4.987896e+006f,	
5.931642e+006f,	7.053951e+006f,	8.388608e+006f,	9.975792e+006f,	1.186328e+007f,	1.410790e+007f,	1.677722e+007f,	1.995158e+007f,	2.372657e+007f,	2.821580e+007f,	
3.355443e+007f,	3.990317e+007f,	4.745313e+007f,	5.643160e+007f,	6.710886e+007f,	7.980634e+007f,	9.490626e+007f,	1.128632e+008f,	1.342177e+008f,	1.596127e+008f,	
1.898125e+008f,	2.257264e+008f,	2.684355e+008f,	3.192253e+008f,	3.796251e+008f,	4.514528e+008f,	5.368709e+008f,	6.384507e+008f,	7.592501e+008f,	9.029057e+008f,	
1.073742e+009f,	1.276901e+009f,	1.518500e+009f,	1.805811e+009f,	2.147484e+009f,	2.553803e+009f,	3.037000e+009f,	3.611623e+009f,	4.294967e+009f,	5.107606e+009f,	
6.074001e+009f,	7.223245e+009f,	8.589935e+009f,	1.021521e+010f,	1.214800e+010f,	1.444649e+010f,	1.717987e+010f,	2.043042e+010f,	2.429600e+010f,	2.889298e+010f,	
3.435974e+010f,	4.086084e+010f,	4.859201e+010f,	5.778596e+010f,	6.871948e+010f,	8.172169e+010f,	9.718401e+010f,	1.155719e+011f,	1.374390e+011f,	1.634434e+011f,	
1.943680e+011f,	2.311438e+011f,	2.748779e+011f,	3.268868e+011f,	3.887361e+011f,	4.622877e+011f,	
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

static int comparehuffmanword(const void *word1, const void *word2)
{
    HuffmanWord *temp1, *temp2;

    temp1 = (HuffmanWord *)word1;
    temp2 = (HuffmanWord *)word2;
    if (temp1->nLen < temp2->nLen)
		return -1;
    if ( (temp1->nLen == temp2->nLen) && (temp1->ulCodeWord < temp2->ulCodeWord) )
		return -1;
    return 1;
}

/*
 * initialize the HuffmanTableStruc structure and sort the HuffmanWord
 * codewords by length, shortest (most probable) first
 */
void inithuffmantable(HuffmanTableStruc* phtable, int bookList[][121], HuffmanWord *phword, int nPackedNumber, int nLargestAbsoluteValue, int nBeSigned)
{
    int i, nTableSize;
    
    if (!nBeSigned)			//Unsigned 
	{
		phtable->nLenCodingRange = nLargestAbsoluteValue + 1;
        phtable->nOffset = 0;
    }
    else {
		phtable->nLenCodingRange = 2*nLargestAbsoluteValue + 1;	//Signed
        phtable->nOffset = nLargestAbsoluteValue;
    }
    nTableSize=1;	    
    for (i=0; i<nPackedNumber; i++)
		nTableSize *= phtable->nLenCodingRange;
    phtable->nTableSize = nTableSize;
    phtable->nPackedNumber = nPackedNumber;
    phtable->nLargestAbsoluteValue = nLargestAbsoluteValue;
    phtable->nBeSigned = nBeSigned;
    phtable->hcw = phword;

	
	for(i = 0; i<nTableSize; i++)
	{
		bookList[0][i] = phtable->hcw[i].nIndex;
		bookList[1][i] = phtable->hcw[i].nLen;
		bookList[2][i] = phtable->hcw[i].ulCodeWord;
	}
	qsort(phword, nTableSize, sizeof(HuffmanWord), comparehuffmanword);
    
}

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
#define FLOOR_FIT 0
int DecodeFloor(int anSfbNonZeroIndx[], avs2audiopack_buffer *opb, int nNumOfSFB, int *pnSFBStartPos, int lg, float decmask[])  
{
	HuffmanTableStruc *hcb;
    HuffmanWord *hcw;
    int i, t, n,  nFac, nDiff;
//    int nErrorFlag;
	int first_value, fit_value[MAX_BAND_NUM];
	int nMaxBandNum;
//	float dScale;
    int k, nSfb;
	int bits;
	int decilogmask[1024*2];

	memset(fit_value, 0, MAX_BAND_NUM * sizeof(int));


	first_value = avs2audiopack_read(opb, 9);
	if(nNumOfSFB < 16)
		bits = 4;
	else if(nNumOfSFB >= 16 && nNumOfSFB < 32)
		bits = 5;
	else
		bits = 6;
	
	nMaxBandNum = avs2audiopack_read(opb, bits);

	nFac = first_value;
	n = nMaxBandNum;

	
    hcb = &huffmanbook;
    hcw = hcb->hcw;

	for(i = 0 ; i < n ; i++)
	{
		if(anSfbNonZeroIndx[i]|| i==n-1)
		{
			t = tianlai_book_decode(&huffmanDecodeBook, opb);
            if(t<0)
                return FERROR;
			nDiff = t - MIDSCALEFAC;	/* 1.5 dB */
			nFac += nDiff;
#if 1 //20200604 shumin.xu
			if (nDiff >= 60)
			{
				int delta60;
				delta60 = avs2audiopack_read(opb, 7);
				nFac += delta60;
			}
			if (nDiff <= -60)
			{
				int delta60;
				delta60 = avs2audiopack_read(opb, 7);
				nFac -= delta60;
			}
#endif
			fit_value[i] = nFac;
		}
	}

    { int lx;
	  int ly;

	  for(nSfb=0; nSfb<nMaxBandNum;nSfb++)
	  {
		  if(anSfbNonZeroIndx[nSfb])
		  {
			  n=nSfb;
			  break;
		  }
	  }
	  memset(decilogmask, 0, lg*sizeof(int));
	  lx = pnSFBStartPos[n];
	  ly = fit_value[n];
	  for(nSfb=n+1;nSfb<nMaxBandNum;nSfb++)
	  {
		  if(anSfbNonZeroIndx[nSfb])
		  {
			int hx=pnSFBStartPos[nSfb];
			int hy=fit_value[nSfb];
			{
				render_line0(lg,lx,hx,ly,hy,decilogmask);
				lx=hx;
				ly=hy;
			}
		  }
	  }
	  if(nMaxBandNum>0)
	  {
		for(k=pnSFBStartPos[nMaxBandNum-1]; k<pnSFBStartPos[nMaxBandNum]; k++)
			decilogmask[k] = fit_value[nMaxBandNum-1];
	  }
	}


	for (nSfb=0; nSfb<nNumOfSFB; nSfb++) 
	{
		if(anSfbNonZeroIndx[nSfb])
		{
			for (k = pnSFBStartPos[nSfb] ; k<pnSFBStartPos[nSfb+1]; k++) 
			{
				decmask[k] = FLOOR_LOOKUP[decilogmask[k]];
			}
		}
	}	

	return FNO_ERROR;
}

void LFBlockDecoder(tianlai_block *vb,
					tns_data *pTnsData,
					int ch,
					int n,
					int blocknum,
  				    int W,
				    float *mdctcoeff,
					int nNumOfSFB,
					int anSFBStartPos[]
					)
{
    tianlai_dsp_state     *vd=vb->vd;
    tianlai_info          *vi=vd->vi;
    codec_setup_info     *ci=vi->codec_setup;
    private_state        *b=vd->backend_state;

	float *pcm[2];
	float pcmbuffer[2][2048];
    int i, j;
    int submap=0;
    int submaps = 1;
    int residuesubmap = 0;
    int floorsubmap = 0;
	char tmp;

    int   nonzero;
	int floormemo[64];
	float decmask[2048];
    static int count4=0;

    int usedbits = vb->opb.endbit;

	int k, nSfb, anSfbNonZeroIndx[MAX_BAND_NUM];	

    count4++;

	vb->W = W;

	submap = W;

	  
	if(floormemo)
      nonzero=1;
    else
      nonzero=0;


	for(i = 0; i< ch; i++)
	{

	    memset(pcmbuffer[i],0,sizeof(float)*n);
		pcm[i] = malloc(n*sizeof(float));
		pcm[i] = pcmbuffer[i];
	}


	for(i = 0; i < ch; i++)
	{
		residuesubmap = W;
		_residue_P[ci->residue_type[residuesubmap]]->inverse(vb,b->residue[residuesubmap],pcm,&nonzero,ch);
	}

    /* compute and apply spectral envelope */
    for(i=0;i<ch;i++){
 
		memset(anSfbNonZeroIndx,0,sizeof(int)*MAX_BAND_NUM);
#if FLOOR_FIT //shumin.xu, 20200604
		nSfb = 0;
		for (k = anSFBStartPos[nSfb]; k < anSFBStartPos[nSfb + 1]; k++)
		{
			if (pcm[i][k] != 0)
			{
				anSfbNonZeroIndx[nSfb] = 1;
				anSfbNonZeroIndx[nSfb + 1] = 1;
				break;
			}
		}
		for (nSfb = 1; nSfb < nNumOfSFB; nSfb++)
		{
			for (k = anSFBStartPos[nSfb]; k < anSFBStartPos[nSfb + 1]; k++)
			{
				if (pcm[i][k] != 0)
				{
					anSfbNonZeroIndx[nSfb] = 1;
					{
						anSfbNonZeroIndx[nSfb - 1] = 1;
						anSfbNonZeroIndx[nSfb + 1] = 1;
					}
					break;
				}
			}
		}
#else
		for(nSfb = 0; nSfb <nNumOfSFB; nSfb ++)
		{
			  for(k = anSFBStartPos[nSfb]; k< anSFBStartPos[nSfb+1]; k++)
				  if(pcm[i][k]!=0)
				  {
					  anSfbNonZeroIndx[nSfb] = 1;
					  break;
				  }
		}
#endif
		memset(decmask, 0, n*sizeof(float));
        DecodeFloor(anSfbNonZeroIndx, &vb->opb, nNumOfSFB, anSFBStartPos, n, decmask);

		for(k = 0; k<n; k++)
			pcm[i][k] *= decmask[k]; 

		usedbits = vb->opb.endbyte * 32 + vb->opb.endbit - usedbits;              //chenhan 8改成32

	    //tmp = (char) avs2audiopack_read(&vb->opb, (8-usedbits%8)%8);               //chenhan 此处不应该字节对齐

		/*tns decode */
		if(n >= 512)
			j = 0;
		else
			j = 2;

	    decodeTnsData(&vb->opb, blocknum, j, W, pTnsData);

	    ApplyTns(pTnsData, pcm[i], blocknum, W, vb->encLen);


		memcpy(mdctcoeff, pcm[i], n * sizeof(float));
	}


	for(i = 0; i < n; i++)
	{

        if(road==1)
			mdctcoeff[i] = mdctcoeff[i]*(n)/4*1*(Avs2DecoderInstance_frame[0].vf.vb.Maxpcmvalue/4);
       else if(road==2)
			mdctcoeff[i] = mdctcoeff[i]*(n)/4*1*(Avs2DecoderInstance_objframe[index_obj][0].vf.vb.Maxpcmvalue/4);
	   else
			mdctcoeff[i] = mdctcoeff[i]*(n)/4*1*(Maxpcmvalue/4);

	}
	return;
}


void LFBlockDecoder_PCA(tianlai_block *vb,
					tns_data *pTnsData,
					int ch,
					int n,
					int blocknum,
  				    int W,
				    float *mdctcoeff,
					int indexinelement,
				 int elementindex,
				 int nChannelsInEl,
					int nNumOfSFB,
					int anSFBStartPos[]
					)
{
    tianlai_dsp_state     *vd=vb->vd;
    tianlai_info          *vi=vd->vi;
    codec_setup_info     *ci=vi->codec_setup;
    private_state        *b=vd->backend_state;

	float *pcm[2];
	float pcmbuffer[2][2048];
    int i, j;
    int submap=0;
    int submaps = 1;
    int residuesubmap = 0;
    int floorsubmap = 0;
	char tmp;

    int   nonzero;
	int floormemo[64];
	float decmask[2048];
    static int count4=0;

    int usedbits = vb->opb.endbit;

	int k, nSfb, anSfbNonZeroIndx[MAX_BAND_NUM];	

    count4++;

	vb->W = W;

	submap = W;


	if(floormemo)
      nonzero=1;
    else
      nonzero=0;


	for(i = 0; i< ch; i++)
	{

	    memset(pcmbuffer[i],0,sizeof(float)*n);
		pcm[i] = malloc(n*sizeof(float));
		pcm[i] = pcmbuffer[i];
	}


	for(i = 0; i < ch; i++)
	{
		residuesubmap = W;
		_residue_P[ci->residue_type[residuesubmap]]->inverse(vb,b->residue[residuesubmap],pcm,&nonzero,ch);
	}

    /* compute and apply spectral envelope */
    for(i=0;i<ch;i++){
		memset(anSfbNonZeroIndx,0,sizeof(int)*MAX_BAND_NUM);

#if FLOOR_FIT //shumin.xu, 20200604
		nSfb = 0;
		for (k = anSFBStartPos[nSfb]; k < anSFBStartPos[nSfb + 1]; k++)
		{
			if (pcm[i][k] != 0)
			{
				anSfbNonZeroIndx[nSfb] = 1;
				anSfbNonZeroIndx[nSfb + 1] = 1;
				break;
			}
		}
		for (nSfb = 1; nSfb < nNumOfSFB; nSfb++)
		{
			for (k = anSFBStartPos[nSfb]; k < anSFBStartPos[nSfb + 1]; k++)
			{
				if (pcm[i][k] != 0)
				{
					anSfbNonZeroIndx[nSfb] = 1;
					{
						anSfbNonZeroIndx[nSfb - 1] = 1;
						anSfbNonZeroIndx[nSfb + 1] = 1;
					}
					break;
				}
			}
		}
#else
		for(nSfb = 0; nSfb <nNumOfSFB; nSfb ++)
		{
			  for(k = anSFBStartPos[nSfb]; k< anSFBStartPos[nSfb+1]; k++)
				  if(pcm[i][k]!=0)
				  {
					  anSfbNonZeroIndx[nSfb] = 1;
					  break;
				  }
		}
#endif
        DecodeFloor(anSfbNonZeroIndx, &vb->opb, nNumOfSFB, anSFBStartPos, n, decmask);
		memcpy(alldecmask[0][blocknum][0],decmask,2048*sizeof(float));

		for(k = 0; k<n; k++)
			pcm[i][k] *= decmask[k]; 


		usedbits = vb->opb.endbyte * 32 + vb->opb.endbit - usedbits;

	    //tmp = (char) avs2audiopack_read(&vb->opb, (8-usedbits%8)%8);              //chenhan 此处不应该字节对齐

		/*tns decode */
		if(n >= 512)
			j = 0;
		else
			j = 2;

	    decodeTnsData(&vb->opb, blocknum, j, W, pTnsData);

	    ApplyTns(pTnsData, pcm[i], blocknum, W, vb->encLen);


		memcpy(mdctcoeff, pcm[i], n * sizeof(float));
	}


	for(i = 0; i < n; i++)
	{
       if(road==1)
		mdctcoeff[i] = mdctcoeff[i]*(n)/4*1*(Avs2DecoderInstance_frame[0].vf.vb.Maxpcmvalue/4);
       else if(road==2)
        mdctcoeff[i] = mdctcoeff[i]*(n)/4*1*(Avs2DecoderInstance_objframe[index_obj][0].vf.vb.Maxpcmvalue/4);
	   else
		mdctcoeff[i] = mdctcoeff[i]*(n)/4*1*(Maxpcmvalue/4);
	}
	return;
}

int Avs2LFDecoder(int useBWE,
				  int *bandWidth,
				  int bitRateIndex,
				  int bitRate,
				  int bitPerSample,
				  AVS2DECODER self,
				  unsigned char sampleData[],
				  int numOutBytes,
                   int *sampleRate,            /*!< pointer to sample rate */
                   char frameOK,                /*!< indicates if current frame data is valid */
 				   float MdctSpectrum[],
				   int readcodectypeflag,
				   tianlai_block *vb,
				   int config_idx
				   )
{
	
	tns_data *pTnsData = &(self->vf.TnsData);

	int ifLFE = 0, Seqmode;
	int ErrorStatus = frameOK;
	int avs2Channels = 0;
    int type = 0;
    char ElementInstanceTag;
	int count;

    int   i,j,k;
    int  n, W;
	int lg;

    int lf_winseq_dec[20];
	//int bandWidth;

	int nNumOfSFB;
	int *pSfbWidthTable;
	int sfbStartPos[MAX_BAND_NUM] = { 0 };

	StAvs2BweDecMDFT *pstBweMDFT;
	StAvs2BweDecCommon *pstBweCommon;
    self->frameOK = 1;

	{
        //type = avs2audiopack_read(&vb->opb, 4);
		type = self->type; 

		switch (type)
		{
		case ID_CPE_F:
		case ID_CPE_S:
		case ID_SCE:
        case ID_CPE_L:
		case ID_CPE_H:
      
			/*
			  Consistency check
			*/
			  /* Consistency check */
			//self->type = type;
			if(avs2Channels >= 2/*Channels*/){

			  self->frameOK = 0;
			  break;
			}
          
			self->pStreamBWE->bweElement[self->pStreamBWE->NrElements].ElementID = ID_SCE;

			if(self->frameOK) 
			{

				int mdftoffsetindex=0, mdftlen;
				int seqtow[4]={0,3,2,1};

				n = 1024 / 8;
				lg = n;
				W = 0;
				
				ElementInstanceTag = avs2audiopack_read(&vb->opb, 4);
				self->lf_winseq_index = avs2audiopack_read(&vb->opb, 6);				

				//////////////////////
				//根据ElementInstanceTag来配置当前声道解码所用码本等
				vb_ci_settable_reset(vb, ElementInstanceTag);
				//设置当前解码声道的bandWidth useBWE信息
				//reset_bandWidth(*sampleRate,ElementInstanceTag, &bandWidth, nChannels, bitRateIndex, ifLFE, useBWE);
				reset_bandWidth0(*sampleRate, config_idx, bandWidth, useBWE, ElementInstanceTag);

 				memcpy(lf_winseq_dec,lf_winseq_table[self->lf_winseq_index],20*4);

				for(j = 1; j < lf_winseq_dec[0]+1; j++)
				{ 
					if(lf_winseq_dec[j]>lf_winseq_dec[j+1])
						mdftlen= (1<<(lf_winseq_dec[j]-1));
					else
						mdftlen= (1<<(lf_winseq_dec[j+1]-1));
					
					if(lf_winseq_dec[j]>lf_winseq_dec[j+1])
						W = seqtow[lf_winseq_dec[j]-1];
				    else
						W = seqtow[lf_winseq_dec[j+1]-1];

					lg = n*mdftlen;


					vb->encLen = *bandWidth / (1024 / lg);

					switch(lg) {
					case 1024:
						nNumOfSFB = srInfo.nCBNum;
						pSfbWidthTable = srInfo.anCBWidth;
						break;
					case 128:
						nNumOfSFB = srInfo.nCB128Num;
						pSfbWidthTable = srInfo.anCB128Width;
						break;
					case 256:
						nNumOfSFB = srInfo.nCB256Num;
						pSfbWidthTable = srInfo.anCB256Width;
						break;
					case 512:
						nNumOfSFB = srInfo.nCB512Num;
						pSfbWidthTable = srInfo.anCB512Width;
						break;
					}

					sfbStartPos[0] = 0;
					for (k = 0; k < nNumOfSFB; k++)
					{
					   sfbStartPos[k+1] = sfbStartPos[k] + pSfbWidthTable[k];
					}		

				    LFBlockDecoder(vb, pTnsData, 1, lg, mdftoffsetindex, W, &MdctSpectrum[mdftoffsetindex*n], nNumOfSFB, sfbStartPos);
				  
					if(lf_winseq_dec[j]>lf_winseq_dec[j+1])
						mdftoffsetindex += (1<<(lf_winseq_dec[j]-1));
					else
						mdftoffsetindex += (1<<(lf_winseq_dec[j+1]-1));
				}
			}

			break;


			// case ID_FIL:
			//{
			//	int fill_type = 0;
			//	int fill_byte = 0;
			//	fill_type = avs2audiopack_read(&vb->opb, 4);
			//	
			//	if(fill_type == 0)
			//	{
			//		fill_byte = avs2audiopack_read(&vb->opb, 8);
			//		for (i=0; i < fill_byte; i++)
			//		{
			//			avs2audiopack_read(&vb->opb,8);
			//		}

			//	}
			//}  
			//break;
		}

		if(useBWE)
		{
			int ch = 0;
			float Mdftout[4096] = {0};

			count = self->pStreamBWE->bweElement[self->pStreamBWE->NrElements].Payload = avs2audiopack_read(&vb->opb, 8);
			for(i=0; i<count/4; i++)
		    {
				self->pStreamBWE->bweElement [self->pStreamBWE->NrElements].Data[i] = (unsigned int) avs2audiopack_read(&vb->opb,32);
			}
			if (count % 4 != 0)
			{
				self->pStreamBWE->bweElement[self->pStreamBWE->NrElements].Data[i] = (unsigned int)avs2audiopack_read(&vb->opb, 8 * (count % 4));
			}

			self->pStreamBWE->NrElements += 1;

			/*for(i = 0; i < FRAME_SIZE; i++)
			{
				float tmp2;
				tmp2 = MdctSpectrum[i];
				MdctSpectrum[i] = vb->SpectralCoefficient[i];
				vb->SpectralCoefficient[i] = tmp2;
			}*/
			for(i = 0; i < 1024; i++)
			{
				Mdftout[2*i] = MdctSpectrum[i] * 8;
				Mdftout[2*i+1] = 0;
			}
			
			pstBweMDFT = (StAvs2BweDecMDFT*)(self->st1_decin);
			//imdft_lowpassframe4096block_multi(&self->pTimeData[0], pstBweMDFT->lf_winseq_pre, Mdftout, 0, 4096/4/2, pstBweMDFT->Srstereo[0]);
			imdft_lowpassframe4096block_multi(&self->pTimeData[0], lf_winseq_dec, Mdftout, 0, 4096/4/2, pstBweMDFT->Srstereo[0]);

			/*wuchaogang mdft decoder analysis */
			Seqmode = Avs2BweDecMDFTana(self->st1_decin, self->st_deccommon, &self->pTimeData[0],ch);

			/* decode one bwe frame */
			if(Seqmode != -1)
			{
				int usePS = (self->type==ID_SCE)? 0: 1;
				Avs2BweDecoder( bitRate, self->pStreamBWE, self->st2_decin, self->st_deccommon, bitPerSample, usePS);
			
				pstBweCommon = (StAvs2BweDecCommon *)(self->st_deccommon);
				memcpy(MdctSpectrum, pstBweCommon->bweMdftSpectrum, (FRAME_SIZE * 4 +2048)* sizeof(float));
			}
		}

	}

    return ErrorStatus;
}


float PCAX_0707table32[31]={-0.707,-0.707/15*14,-0.707/15*13,-0.707/15*12,-0.707/15*11,-0.707/15*10,-0.707/15*9,-0.707/15*8,-0.707/15*7,-0.707/15*6,-0.707/15*5,-0.707/15*4,-0.707/15*3,-0.707/15*2, -0.707/15*1,0.000,
                            0.707/15*1, 0.707/15*2, 0.707/15*3,   0.707/15*4,  0.707/15*5,0.707/15*6,  0.707/15*7, 0.707/15*8, 0.707/15*9,0.707/15*10,  0.707/15*11, 0.707/15*12,0.707/15*13, 0.707/15*14,0.707/15*15};
float PCAX_1000table32[31]={-1,-1.0/15*14,-1.0/15*13,-1.0/15*12,-1.0/15*11,-1.0/15*10,-1.0/15*9,-1.0/15*8,-1.0/15*7,-1.0/15*6,-1.0/15*5,-1.0/15*4,-1.0/15*3,-1.0/15*2, -1.0/15*1,0.000,
                            1.0/15*1, 1.0/15*2, 1.0/15*3,   1.0/15*4,  1.0/15*5,1.0/15*6,  1.0/15*7, 1.0/15*8, 1.0/15*9,1.0/15*10,  1.0/15*11, 1.0/15*12,1.0/15*13, 1.0/15*14,1.0/15*15};

float PCAX_0707table16[15]={-0.707,-0.606,-0.505,-0.404,-0.303,-0.202,-0.101,0.000, 0.101, 0.202,0.303, 0.404, 0.505, 0.606, 0.707};
float PCAX_1000table16[15]={-1.000,-0.857,-0.714,-0.571,-0.423,-0.282,-0.101,0.000, 0.101, 0.282,0.423, 0.571, 0.714, 0.857, 1.000};
float PCAX_0615table16[15]={-0.615,-0.528,-0.440,-0.352,-0.264,-0.176,-0.088,0.000, 0.088, 0.176,0.264, 0.352, 0.440, 0.528, 0.615};
float PCAX_0615table08[7]={-0.615,-0.410,-0.205,0.000, 0.205, 0.410, 0.615};
float PCAX_0707table08[7]={-0.707,-0.471,-0.236,0.000, 0.236, 0.471, 0.707};

int anaMatrixdata_decode(tianlai_block *vb,int usePCAitemnum,int *lf_winseq_dec,
						  int elementindex,
						  int nChannelsInEl)
{
	int index,tt,jj,ll,bandnum,kk;
	unsigned int quandata;
	static int LL[5]={4096/8/2, 4096/4/2, 4096/2/2, 4096/2,4096/16/2};	
	int i,j;
	unsigned int PCAMatrixdataEncodeMode=0;
			
	//decode PCA anaMatrixdata
	for (index = 1; index < (lf_winseq_dec[0] + 1); index++)
	{
		tt = lf_winseq_dec[index];
		jj = lf_winseq_dec[index + 1];

		if (LL[tt - 1] > LL[jj - 1])
			ll = LL[tt - 1] / 2;
		else
			ll = LL[jj - 1] / 2;

		switch (ll * 2) {
		case 4096:
			break;
		case 2048:
			bandnum = bandnumset[0];
			break;
		case 1024:
			bandnum = bandnumset[1];
			break;
		case 512:
			bandnum = bandnumset[2];
			break;
		case 256:
			bandnum = bandnumset[3];
			break;
		}

		switch (ll * 2) {
		case 4096:
			break;
		case 2048:
		case 1024:
		case 512:
		case 256:
		{
			unsigned int quandata;
			quandata = avs2audiopack_read(&vb->opb, 1);

			if (quandata == 0)
				bandmulflag[elementindex][index - 1] = 1;
			else
				bandmulflag[elementindex][index - 1] = 2;
		}

		{
			quandata = avs2audiopack_read(&vb->opb, 1);
			if (quandata == 1)
			{
				for (kk = 0; kk < bandnum; kk += ((kk >= MULBANDSTART) ? bandmulflag[elementindex][index - 1] : 1))
				{
					quandata = avs2audiopack_read(&vb->opb, 1);
					allbandsetflag[elementindex][index - 1][kk] = quandata;
				}
			}
			else
			{
				for (kk = 0; kk < bandnum; kk += ((kk >= MULBANDSTART) ? bandmulflag[elementindex][index - 1] : 1))
				{
					allbandsetflag[elementindex][index - 1][kk] = 1;
				}
			}
		}

		//PCAMatrixdataEncodeMode
		PCAMatrixdataEncodeMode = avs2audiopack_read(&vb->opb, 2);

		if (nChannelsInEl == 2)
		{
			for (kk = 0; kk < bandnum; kk += ((kk >= MULBANDSTART) ? bandmulflag[elementindex][index - 1] : 1))
				if (allbandsetflag[elementindex][index - 1][kk] == 1) {
					for (i = 1; i <= usePCAitemnum; i++)
					{
						if ((i == nChannelsInEl) && (nChannelsInEl == 2))
						{
							anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + 1 - 1] = -anaMatrixdata[elementindex][index - 1][kk][0][(i - 2)*nChannelsInEl + 2 - 1];
							anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + 2 - 1] = anaMatrixdata[elementindex][index - 1][kk][0][(i - 2)*nChannelsInEl + 1 - 1];
						}
						else
						{
							unsigned int quandata2;
							float tmp2, tmp;
							int valuet;

							quandata = avs2audiopack_read(&vb->opb, 1);

							if (PCAMatrixdataEncodeMode == 0)
							{
								quandata2 = avs2audiopack_read(&vb->opb, PCAQUANT4LEN);
								tmp = PCAX_0707table16[quandata2];
							}
							else
							{
								quandata2 = avs2audiopack_read(&vb->opb, PCAQUANT4LEN + 1);
								tmp = PCAX_0707table32[quandata2];

							}
							if (tmp > 0.707)
								tmp = 0.707;
							if (tmp < -0.707)
								tmp = -0.707;

							tmp2 = 1 - tmp * tmp;
							if (tmp2 < 0)
								tmp2 = 0;
							tmp2 = sqrt(tmp2);
							//tmp=0;
							if (quandata == 1)
							{
								anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + 1 - 1] = tmp;
								anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + 2 - 1] = tmp2;
							}
							else
							{
								anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + 2 - 1] = tmp;
								anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + 1 - 1] = tmp2;
							}

						}
					}

#if EVENODD
					for (i = 1; i <= usePCAitemnum; i++)
					{
						if ((i == nChannelsInEl) && (nChannelsInEl == 2))
						{

							anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + 1 - 1] = -anaMatrixdata[elementindex][index - 1][kk][1][(i - 2)*nChannelsInEl + 2 - 1];
							anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + 2 - 1] = anaMatrixdata[elementindex][index - 1][kk][1][(i - 2)*nChannelsInEl + 1 - 1];

						}
						else
						{
							unsigned int quandata2;
							float tmp2, tmp;
							int valuet;

							quandata = avs2audiopack_read(&vb->opb, 1);

							if (PCAMatrixdataEncodeMode == 0)
							{
								quandata2 = avs2audiopack_read(&vb->opb, PCAQUANT4LEN);
								tmp = PCAX_0707table16[quandata2];
							}
							else
							{
								quandata2 = avs2audiopack_read(&vb->opb, PCAQUANT4LEN + 1);
								tmp = PCAX_0707table32[quandata2];
							}
							if (tmp > 0.707)
								tmp = 0.707;
							if (tmp < -0.707)
								tmp = -0.707;

							tmp2 = 1 - tmp * tmp;
							if (tmp2 < 0)
								tmp2 = 0;
							tmp2 = sqrt(tmp2);

							if (quandata == 1)
							{
								anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + 1 - 1] = tmp;
								anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + 2 - 1] = tmp2;
							}
							else
							{
								anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + 2 - 1] = tmp;
								anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + 1 - 1] = tmp2;
							}

						}
					}
#endif
				}//bandnum
				else
				{
					for (i = 1; i <= usePCAitemnum; i++)
					{

						anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + 1 - 1] = anaMatrixdata[elementindex][index - 1][kk - ((kk > MULBANDSTART) ? bandmulflag[elementindex][index - 1] : 1)][0][(i - 1)*nChannelsInEl + 1 - 1];
						anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + 2 - 1] = anaMatrixdata[elementindex][index - 1][kk - ((kk > MULBANDSTART) ? bandmulflag[elementindex][index - 1] : 1)][0][(i - 1)*nChannelsInEl + 2 - 1];

						anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + 1 - 1] = anaMatrixdata[elementindex][index - 1][kk - ((kk > MULBANDSTART) ? bandmulflag[elementindex][index - 1] : 1)][1][(i - 1)*nChannelsInEl + 1 - 1];
						anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + 2 - 1] = anaMatrixdata[elementindex][index - 1][kk - ((kk > MULBANDSTART) ? bandmulflag[elementindex][index - 1] : 1)][1][(i - 1)*nChannelsInEl + 2 - 1];

					}
				}

			break;
		}



		if (PCAMatrixdataEncodeMode == 1)
			goto decexc_PCAMatrixdataEncodeMode1;
		if (nChannelsInEl == 4)
		{
			float tmp;
			for (kk = 0; kk < bandnum; kk += ((kk >= MULBANDSTART) ? bandmulflag[elementindex][index - 1] : 1))
				if (allbandsetflag[elementindex][index - 1][kk] == 1) {

					if (kk == 0)
					{
						for (i = 1; i <= usePCAitemnum; i++)
						{
							for (j = 1; j <= nChannelsInEl; j++)
							{
								if ((j == nChannelsInEl) && (elementindex >= 0))//if((j==nChannelsInEl)||( (kk>0)&&(j+1==nChannelsInEl)&&(nChannelsInEl==4) ))
								{
									int nn;
									tmp = 0;
									for (nn = 1; nn < j; nn++)
										tmp += (anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + nn - 1])*(anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + nn - 1]);
									quandata = avs2audiopack_read(&vb->opb, 1);
									tmp = 1 - tmp;
									if (tmp < 0)
										tmp = 0;

									if (quandata == 0)
										tmp = sqrt(tmp);
									else
										tmp = -sqrt(tmp);
									//tmp=0;
									anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + j - 1] = tmp;
								}
								else
								{

									int valuet;

									quandata = avs2audiopack_read(&vb->opb, PCAQUANT4LEN);


									valuet = quandata;

									tmp = PCAX_1000table16[valuet];

									if (tmp > 1.0)
										tmp = 1.0;
									if (tmp < -1.0)
										tmp = -1.0;
									anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + j - 1] = tmp;
								}
							}//for (j = 1; j <= nChannelsInEl; j++)

						}//for (i = 1; i <= usePCAitemnum; i++)
					}//if(kk==0)
					else if (usePCAitemnum == 2)
					{
						unsigned int maxindex1, maxindex2;
						int indextmp, indextmp2;

						float eng11, eng12, eng13, eng21, eng22, eng23;
						float tmp_x;
						int valuet;

						quandata = avs2audiopack_read(&vb->opb, 3);
						if (quandata == 6)
						{
							maxindex1 = 1;
							maxindex2 = 1;
						}
						else
							if (quandata == 7)
							{
								maxindex1 = 2;
								maxindex2 = 2;
								quandata = avs2audiopack_read(&vb->opb, 1);
								if (quandata == 1)
								{
									maxindex1 = 3;
									maxindex2 = 3;
								}
							}
							else
								if (quandata == 8)
								{
									maxindex1 = 3;
									maxindex2 = 3;
								}
								else
								{
									maxindex1 = (quandata >> 1);
									quandata = quandata - maxindex1 * 2;
									maxindex1 = maxindex1 + 1;
									if (maxindex1 == 1)
									{
										if (quandata == 1)
										{
											maxindex2 = 3;
										}
										else
										{
											maxindex2 = 2;
										}

									}
									else
									{
										if (quandata == 1)
										{
											if (maxindex1 == 2)
												maxindex2 = 3;
											if (maxindex1 == 3)
												maxindex2 = 2;

										}
										else
										{
											maxindex2 = 1;
										}
									}
								}

						if (maxindex1 == maxindex2)
						{
							//	printf("error:maxindex1==maxindex2\n");
							//	exit(1);
						}

						/*  chenhan 将读取提到最前面，后面处理按各自方便的顺序进行  */
						{
							int quandata[6] = { 0 };

							quandata[2] = avs2audiopack_read(&vb->opb, 1);
							quandata[4] = avs2audiopack_read(&vb->opb, 1);
							quandata[0] = avs2audiopack_read(&vb->opb, 4);
							quandata[3] = avs2audiopack_read(&vb->opb, 4);
							quandata[1] = avs2audiopack_read(&vb->opb, 3);
							quandata[5] = avs2audiopack_read(&vb->opb, 4);

							valuet = quandata[0];

							tmp_x = PCAX_0615table16[valuet];

							anaMatrixdata[elementindex][index - 1][kk][0][(1 - 1)*nChannelsInEl + maxindex1 - 1] = tmp_x;
							eng11 = anaMatrixdata[elementindex][index - 1][kk][0][(1 - 1)*nChannelsInEl + maxindex1 - 1];
							eng11 = eng11 * eng11;

							valuet = quandata[1];

							tmp_x = PCAX_0615table08[valuet];

							anaMatrixdata[elementindex][index - 1][kk][0][(2 - 1)*nChannelsInEl + maxindex2 - 1] = tmp_x;
							eng21 = anaMatrixdata[elementindex][index - 1][kk][0][(2 - 1)*nChannelsInEl + maxindex2 - 1];
							eng21 = eng21 * eng21;

							indextmp = 1;
							if (maxindex1 == 1)
								indextmp = 2;


							if (maxindex1 == 1)
							{
								indextmp = 2;
								indextmp2 = 3;
								if (quandata[2] == 1)
								{
									indextmp = 3;
									indextmp2 = 2;
								}

							}
							else if (maxindex1 == 2)
							{
								indextmp = 1;
								indextmp2 = 3;
								if (quandata[2] == 1)
								{
									indextmp = 3;
									indextmp2 = 1;
								}

							}
							else if (maxindex1 == 3)
							{
								indextmp = 2;
								indextmp2 = 1;
								if (quandata[2] == 1)
								{
									indextmp = 1;
									indextmp2 = 2;
								}

							}

							valuet = quandata[3];

							tmp_x = PCAX_0707table16[valuet];

							anaMatrixdata[elementindex][index - 1][kk][0][(1 - 1)*nChannelsInEl + indextmp - 1] = tmp_x;
							eng12 = anaMatrixdata[elementindex][index - 1][kk][0][(1 - 1)*nChannelsInEl + indextmp - 1];
							eng12 = eng12 * eng12;


							eng13 = (1 - eng11 - eng12);
							if (eng13 < 0)
								eng13 = 0;
							if (eng13 > 1)
								eng13 = 1;

							anaMatrixdata[elementindex][index - 1][kk][0][(1 - 1)*nChannelsInEl + indextmp2 - 1] = sqrt(eng13);


							///////////////

							if (maxindex2 == 1)
							{
								indextmp = 2;
								indextmp2 = 3;
								if (quandata[4] == 1)
								{
									indextmp = 3;
									indextmp2 = 2;
								}

							}
							else if (maxindex2 == 2)
							{
								indextmp = 1;
								indextmp2 = 3;
								if (quandata[4] == 1)
								{
									indextmp = 3;
									indextmp2 = 1;
								}

							}
							else if (maxindex2 == 3)
							{
								indextmp = 2;
								indextmp2 = 1;
								if (quandata[4] == 1)
								{
									indextmp = 1;
									indextmp2 = 2;
								}
							}
							valuet = quandata[5];

							tmp_x = PCAX_0707table16[valuet];

							anaMatrixdata[elementindex][index - 1][kk][0][(2 - 1)*nChannelsInEl + indextmp - 1] = tmp_x;
							eng22 = anaMatrixdata[elementindex][index - 1][kk][0][(2 - 1)*nChannelsInEl + indextmp - 1];
							eng22 = eng22 * eng22;

						}
						eng23 = (1 - eng21 - eng22);
						if (eng23 < 0)
							eng23 = 0;
						if (eng23 > 1)
							eng23 = 1;
						anaMatrixdata[elementindex][index - 1][kk][0][(2 - 1)*nChannelsInEl + indextmp2 - 1] = sqrt(eng23);
					}
					else if (usePCAitemnum == 3)
					{
						unsigned int quanMatrixdata04_PCN4_pos01;
						int tmp1;
						unsigned int min1pos[3], min2pos[3], min3pos[3];
						short postable[6][3] = { {1 - 1,2 - 1,3 - 1},{1 - 1,3 - 1,2 - 1},{2 - 1,1 - 1,3 - 1},{2 - 1,3 - 1,1 - 1},{3 - 1,1 - 1,2 - 1},{3 - 1,2 - 1,1 - 1} };
						float eng11, eng12, eng13;
						float tmp_x;
						int valuet;

						quanMatrixdata04_PCN4_pos01 = avs2audiopack_read(&vb->opb, 8);
						tmp1 = (quanMatrixdata04_PCN4_pos01) % 6;
						min1pos[0] = postable[tmp1][0]; min2pos[0] = postable[tmp1][1]; min3pos[0] = postable[tmp1][2];
						tmp1 = (quanMatrixdata04_PCN4_pos01 / 6) % 6;
						min1pos[1] = postable[tmp1][0]; min2pos[1] = postable[tmp1][1]; min3pos[1] = postable[tmp1][2];
						tmp1 = (quanMatrixdata04_PCN4_pos01 / 36) % 6;
						min1pos[2] = postable[tmp1][0]; min2pos[2] = postable[tmp1][1]; min3pos[2] = postable[tmp1][2];


						//principal element 1
						quandata = avs2audiopack_read(&vb->opb, 4);
						valuet = quandata;

						tmp_x = PCAX_0615table16[valuet];

						anaMatrixdata[elementindex][index - 1][kk][0][(1 - 1)*nChannelsInEl + min1pos[0]] = tmp_x;
						eng11 = anaMatrixdata[elementindex][index - 1][kk][0][(1 - 1)*nChannelsInEl + min1pos[0]];
						eng11 = eng11 * eng11;

						quandata = avs2audiopack_read(&vb->opb, 4);
						valuet = quandata;

						tmp_x = PCAX_0707table16[valuet];

						anaMatrixdata[elementindex][index - 1][kk][0][(1 - 1)*nChannelsInEl + min2pos[0]] = tmp_x;
						eng12 = anaMatrixdata[elementindex][index - 1][kk][0][(1 - 1)*nChannelsInEl + min2pos[0]];
						eng12 = eng12 * eng12;


						eng13 = (1 - eng11 - eng12);
						if (eng13 < 0)
							eng13 = 0;
						if (eng13 > 1)
							eng13 = 1;

						anaMatrixdata[elementindex][index - 1][kk][0][(1 - 1)*nChannelsInEl + min3pos[0]] = sqrt(eng13);

						//////////principal element 2//////
						quandata = avs2audiopack_read(&vb->opb, 3);
						valuet = quandata;

						tmp_x = PCAX_0615table08[valuet];

						anaMatrixdata[elementindex][index - 1][kk][0][(2 - 1)*nChannelsInEl + min1pos[1]] = tmp_x;
						eng11 = anaMatrixdata[elementindex][index - 1][kk][0][(2 - 1)*nChannelsInEl + min1pos[1]];
						eng11 = eng11 * eng11;

						quandata = avs2audiopack_read(&vb->opb, 4);
						valuet = quandata;

						tmp_x = PCAX_0707table16[valuet];

						anaMatrixdata[elementindex][index - 1][kk][0][(2 - 1)*nChannelsInEl + min2pos[1]] = tmp_x;
						eng12 = anaMatrixdata[elementindex][index - 1][kk][0][(2 - 1)*nChannelsInEl + min2pos[1]];
						eng12 = eng12 * eng12;


						eng13 = (1 - eng11 - eng12);
						if (eng13 < 0)
							eng13 = 0;
						if (eng13 > 1)
							eng13 = 1;

						anaMatrixdata[elementindex][index - 1][kk][0][(2 - 1)*nChannelsInEl + min3pos[1]] = sqrt(eng13);

						/////////principal element 3///////
						quandata = avs2audiopack_read(&vb->opb, 3);
						valuet = quandata;

						tmp_x = PCAX_0615table08[valuet];

						anaMatrixdata[elementindex][index - 1][kk][0][(3 - 1)*nChannelsInEl + min1pos[2]] = tmp_x;
						eng11 = anaMatrixdata[elementindex][index - 1][kk][0][(3 - 1)*nChannelsInEl + min1pos[2]];
						eng11 = eng11 * eng11;

						quandata = avs2audiopack_read(&vb->opb, 3);
						valuet = quandata;

						tmp_x = PCAX_0707table08[valuet];

						anaMatrixdata[elementindex][index - 1][kk][0][(3 - 1)*nChannelsInEl + min2pos[2]] = tmp_x;
						eng12 = anaMatrixdata[elementindex][index - 1][kk][0][(3 - 1)*nChannelsInEl + min2pos[2]];
						eng12 = eng12 * eng12;


						eng13 = (1 - eng11 - eng12);
						if (eng13 < 0)
							eng13 = 0;
						if (eng13 > 1)
							eng13 = 1;

						anaMatrixdata[elementindex][index - 1][kk][0][(3 - 1)*nChannelsInEl + min3pos[2]] = sqrt(eng13);

					}

					///////////////////////////////////////////////

					if (kk == 0)
					{
						for (i = 1; i <= usePCAitemnum; i++)
						{
							for (j = 1; j <= nChannelsInEl; j++)
							{
								if ((j == nChannelsInEl) && (elementindex >= 0))//if((j==nChannelsInEl)||( (kk>0)&&(j+1==nChannelsInEl)&&(nChannelsInEl==4) ))
								{
									int nn;
									tmp = 0;
									for (nn = 1; nn < j; nn++)
										tmp += (anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + nn - 1])*(anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + nn - 1]);
									quandata = avs2audiopack_read(&vb->opb, 1);
									tmp = 1 - tmp;
									if (tmp < 0)
										tmp = 0;

									if (quandata == 0)
										tmp = sqrt(tmp);
									else
										tmp = -sqrt(tmp);
									//tmp=0;
									anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + j - 1] = tmp;
								}
								else
								{

									int valuet;

									quandata = avs2audiopack_read(&vb->opb, PCAQUANT4LEN);


									valuet = quandata;

									tmp = PCAX_1000table16[valuet];

									if (tmp > 0)
										tmp += (float)((0.0) / PCAQUANT4);
									else
										tmp -= (float)((0.0) / PCAQUANT4);
									if (tmp > 1.0)
										tmp = 1.0;
									if (tmp < -1.0)
										tmp = -1.0;
									anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + j - 1] = tmp;
								}
							}//for (j = 1; j <= nChannelsInEl; j++)
						}//for (i = 1; i <= usePCAitemnum; i++)
					}//if(kk==0)
					else  if (usePCAitemnum == 2)
					{
						unsigned int maxindex1, maxindex2;
						int indextmp, indextmp2;

						float eng11, eng12, eng13, eng21, eng22, eng23;
						float tmp_x;
						int valuet;

						quandata = avs2audiopack_read(&vb->opb, 3);
						if (quandata == 6)
						{
							maxindex1 = 1;
							maxindex2 = 1;
						}
						else
							if (quandata == 7)
							{
								maxindex1 = 2;
								maxindex2 = 2;
								quandata = avs2audiopack_read(&vb->opb, 1);
								if (quandata == 1)
								{
									maxindex1 = 3;
									maxindex2 = 3;
								}
							}
							else
								if (quandata == 8)
								{
									maxindex1 = 3;
									maxindex2 = 3;
								}
								else
								{
									maxindex1 = (quandata >> 1);
									quandata = quandata - maxindex1 * 2;
									maxindex1 = maxindex1 + 1;
									if (maxindex1 == 1)
									{
										if (quandata == 1)
										{
											maxindex2 = 3;
										}
										else
										{
											maxindex2 = 2;
										}

									}
									else
									{
										if (quandata == 1)
										{
											if (maxindex1 == 2)
												maxindex2 = 3;
											if (maxindex1 == 3)
												maxindex2 = 2;

										}
										else
										{
											maxindex2 = 1;
										}
									}
								}

						quandata = avs2audiopack_read(&vb->opb, 4);
						valuet = quandata;

						tmp_x = PCAX_0615table16[valuet];

						anaMatrixdata[elementindex][index - 1][kk][1][(1 - 1)*nChannelsInEl + maxindex1 - 1] = tmp_x;
						eng11 = anaMatrixdata[elementindex][index - 1][kk][1][(1 - 1)*nChannelsInEl + maxindex1 - 1];
						eng11 = eng11 * eng11;

						quandata = avs2audiopack_read(&vb->opb, 3);
						valuet = quandata;

						tmp_x = PCAX_0615table08[valuet];

						anaMatrixdata[elementindex][index - 1][kk][1][(2 - 1)*nChannelsInEl + maxindex2 - 1] = tmp_x;
						eng21 = anaMatrixdata[elementindex][index - 1][kk][1][(2 - 1)*nChannelsInEl + maxindex2 - 1];
						eng21 = eng21 * eng21;

						indextmp = 1;
						if (maxindex1 == 1)
							indextmp = 2;
						quandata = avs2audiopack_read(&vb->opb, 1);
						if (maxindex1 == 1)
						{
							indextmp = 2;
							indextmp2 = 3;
							if (quandata == 1)
							{
								indextmp = 3;
								indextmp2 = 2;
							}

						}
						else if (maxindex1 == 2)
						{
							indextmp = 1;
							indextmp2 = 3;
							if (quandata == 1)
							{
								indextmp = 3;
								indextmp2 = 1;
							}

						}
						else if (maxindex1 == 3)
						{
							indextmp = 2;
							indextmp2 = 1;
							if (quandata == 1)
							{
								indextmp = 1;
								indextmp2 = 2;
							}

						}

						quandata = avs2audiopack_read(&vb->opb, 4);
						valuet = quandata;

						tmp_x = PCAX_0707table16[valuet];

						anaMatrixdata[elementindex][index - 1][kk][1][(1 - 1)*nChannelsInEl + indextmp - 1] = tmp_x;
						eng12 = anaMatrixdata[elementindex][index - 1][kk][1][(1 - 1)*nChannelsInEl + indextmp - 1];
						eng12 = eng12 * eng12;


						eng13 = (1 - eng11 - eng12);
						if (eng13 < 0)
							eng13 = 0;
						if (eng13 > 1)
							eng13 = 1;

						anaMatrixdata[elementindex][index - 1][kk][1][(1 - 1)*nChannelsInEl + indextmp2 - 1] = sqrt(eng13);

						///////////////
						quandata = avs2audiopack_read(&vb->opb, 1);
						if (maxindex2 == 1)
						{
							indextmp = 2;
							indextmp2 = 3;
							if (quandata == 1)
							{
								indextmp = 3;
								indextmp2 = 2;
							}

						}
						else if (maxindex2 == 2)
						{
							indextmp = 1;
							indextmp2 = 3;
							if (quandata == 1)
							{
								indextmp = 3;
								indextmp2 = 1;
							}

						}
						else if (maxindex2 == 3)
						{
							indextmp = 2;
							indextmp2 = 1;
							if (quandata == 1)
							{
								indextmp = 1;
								indextmp2 = 2;
							}

						}

						quandata = avs2audiopack_read(&vb->opb, 4);
						valuet = quandata;

						tmp_x = PCAX_0707table16[valuet];

						anaMatrixdata[elementindex][index - 1][kk][1][(2 - 1)*nChannelsInEl + indextmp - 1] = tmp_x;

						eng22 = anaMatrixdata[elementindex][index - 1][kk][1][(2 - 1)*nChannelsInEl + indextmp - 1];
						eng22 = eng22 * eng22;

						eng23 = (1 - eng21 - eng22);
						if (eng23 < 0)
							eng23 = 0;
						if (eng23 > 1)
							eng23 = 1;

						anaMatrixdata[elementindex][index - 1][kk][1][(2 - 1)*nChannelsInEl + indextmp2 - 1] = sqrt(eng23);
					}
					else if (usePCAitemnum == 3)
					{
						unsigned int quanMatrixdata04_PCN4_pos01;
						int tmp1;
						unsigned int min1pos[3], min2pos[3], min3pos[3];
						short postable[6][3] = { {1 - 1,2 - 1,3 - 1},{1 - 1,3 - 1,2 - 1},{2 - 1,1 - 1,3 - 1},{2 - 1,3 - 1,1 - 1},{3 - 1,1 - 1,2 - 1},{3 - 1,2 - 1,1 - 1} };
						float eng11, eng12, eng13;
						float tmp_x;
						int valuet;

						quanMatrixdata04_PCN4_pos01 = avs2audiopack_read(&vb->opb, 8);
						tmp1 = (quanMatrixdata04_PCN4_pos01) % 6;
						min1pos[0] = postable[tmp1][0]; min2pos[0] = postable[tmp1][1]; min3pos[0] = postable[tmp1][2];
						tmp1 = (quanMatrixdata04_PCN4_pos01 / 6) % 6;
						min1pos[1] = postable[tmp1][0]; min2pos[1] = postable[tmp1][1]; min3pos[1] = postable[tmp1][2];
						tmp1 = (quanMatrixdata04_PCN4_pos01 / 36) % 6;
						min1pos[2] = postable[tmp1][0]; min2pos[2] = postable[tmp1][1]; min3pos[2] = postable[tmp1][2];


						//principal element 1
						quandata = avs2audiopack_read(&vb->opb, 4);
						valuet = quandata;

						tmp_x = PCAX_0615table16[valuet];

						anaMatrixdata[elementindex][index - 1][kk][1][(1 - 1)*nChannelsInEl + min1pos[0]] = tmp_x;
						eng11 = anaMatrixdata[elementindex][index - 1][kk][1][(1 - 1)*nChannelsInEl + min1pos[0]];
						eng11 = eng11 * eng11;

						quandata = avs2audiopack_read(&vb->opb, 4);
						valuet = quandata;

						tmp_x = PCAX_0707table16[valuet];

						anaMatrixdata[elementindex][index - 1][kk][1][(1 - 1)*nChannelsInEl + min2pos[0]] = tmp_x;
						eng12 = anaMatrixdata[elementindex][index - 1][kk][1][(1 - 1)*nChannelsInEl + min2pos[0]];
						eng12 = eng12 * eng12;


						eng13 = (1 - eng11 - eng12);
						if (eng13 < 0)
							eng13 = 0;
						if (eng13 > 1)
							eng13 = 1;

						anaMatrixdata[elementindex][index - 1][kk][1][(1 - 1)*nChannelsInEl + min3pos[0]] = sqrt(eng13);


						//////////principal element 2//////
						quandata = avs2audiopack_read(&vb->opb, 3);
						valuet = quandata;

						tmp_x = PCAX_0615table08[valuet];

						anaMatrixdata[elementindex][index - 1][kk][1][(2 - 1)*nChannelsInEl + min1pos[1]] = tmp_x;
						eng11 = anaMatrixdata[elementindex][index - 1][kk][1][(2 - 1)*nChannelsInEl + min1pos[1]];
						eng11 = eng11 * eng11;

						quandata = avs2audiopack_read(&vb->opb, 4);
						valuet = quandata;

						tmp_x = PCAX_0707table16[valuet];

						anaMatrixdata[elementindex][index - 1][kk][1][(2 - 1)*nChannelsInEl + min2pos[1]] = tmp_x;
						eng12 = anaMatrixdata[elementindex][index - 1][kk][1][(2 - 1)*nChannelsInEl + min2pos[1]];
						eng12 = eng12 * eng12;


						eng13 = (1 - eng11 - eng12);
						if (eng13 < 0)
							eng13 = 0;
						if (eng13 > 1)
							eng13 = 1;

						anaMatrixdata[elementindex][index - 1][kk][1][(2 - 1)*nChannelsInEl + min3pos[1]] = sqrt(eng13);

						/////////principal element 3///////
						quandata = avs2audiopack_read(&vb->opb, 3);
						valuet = quandata;

						tmp_x = PCAX_0615table08[valuet];

						anaMatrixdata[elementindex][index - 1][kk][1][(3 - 1)*nChannelsInEl + min1pos[2]] = tmp_x;
						eng11 = anaMatrixdata[elementindex][index - 1][kk][1][(3 - 1)*nChannelsInEl + min1pos[2]];
						eng11 = eng11 * eng11;

						quandata = avs2audiopack_read(&vb->opb, 3);
						valuet = quandata;

						tmp_x = PCAX_0707table08[valuet];

						anaMatrixdata[elementindex][index - 1][kk][1][(3 - 1)*nChannelsInEl + min2pos[2]] = tmp_x;
						eng12 = anaMatrixdata[elementindex][index - 1][kk][1][(3 - 1)*nChannelsInEl + min2pos[2]];
						eng12 = eng12 * eng12;


						eng13 = (1 - eng11 - eng12);
						if (eng13 < 0)
							eng13 = 0;
						if (eng13 > 1)
							eng13 = 1;

						anaMatrixdata[elementindex][index - 1][kk][1][(3 - 1)*nChannelsInEl + min3pos[2]] = sqrt(eng13);
					}

				}//if(allbandsetflag[elementindex][index-1][kk]==1){
				else
				{
					for (i = 1; i <= usePCAitemnum; i++)
						for (j = 1; j <= nChannelsInEl; j++)
						{
							anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + j - 1] = anaMatrixdata[elementindex][index - 1][kk - ((kk > MULBANDSTART) ? bandmulflag[elementindex][index - 1] : 1)][0][(i - 1)*nChannelsInEl + j - 1];
							anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + j - 1] = anaMatrixdata[elementindex][index - 1][kk - ((kk > MULBANDSTART) ? bandmulflag[elementindex][index - 1] : 1)][1][(i - 1)*nChannelsInEl + j - 1];
						}
				}
			break;
		}



	decexc_PCAMatrixdataEncodeMode1:

		for (kk = 0; kk < bandnum; kk++)
			if (allbandsetflag[elementindex][index - 1][kk] == 1) {
				for (i = 1; i <= usePCAitemnum; i++)
				{
					if ((i == nChannelsInEl) && (nChannelsInEl == 2))
					{

						anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + 1 - 1] = -anaMatrixdata[elementindex][index - 1][kk][0][(i - 2)*nChannelsInEl + 2 - 1];
						anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + 2 - 1] = anaMatrixdata[elementindex][index - 1][kk][0][(i - 2)*nChannelsInEl + 1 - 1];


					}
					else
						for (j = 1; j <= nChannelsInEl; j++)
							if (!((kk > 0) && (j == nChannelsInEl) && (nChannelsInEl == 4))) {
								float tmp;
								if ((j == nChannelsInEl) && (elementindex >= 0))//if((j==nChannelsInEl)||( (kk>0)&&(j+1==nChannelsInEl)&&(nChannelsInEl==4) ))
								{
									int nn;
									tmp = 0;
									for (nn = 1; nn < j; nn++)
										tmp += (anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + nn - 1])*(anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + nn - 1]);
									quandata = avs2audiopack_read(&vb->opb, 1);
									tmp = 1 - tmp;
									if (tmp < 0)
										tmp = 0;

									if (quandata == 0)
										tmp = sqrt(tmp);
									else
										tmp = -sqrt(tmp);
									anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + j - 1] = tmp;
								}
								else {
									int valuet;
									quandata = avs2audiopack_read(&vb->opb, PCAQUANT4LEN);
									valuet = quandata;

									tmp = PCAX_1000table16[valuet];

									if (tmp > 0)
										tmp += (float)((0.0) / PCAQUANT4);
									else
										tmp -= (float)((0.0) / PCAQUANT4);
									if (tmp > 1.0)
										tmp = 1.0;
									if (tmp < -1.0)
										tmp = -1.0;
									anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + j - 1] = tmp;
								}
							}
							else
								anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + j - 1] = 0;
				}
			}
			else
			{
				for (i = 1; i <= usePCAitemnum; i++)
					for (j = 1; j <= nChannelsInEl; j++)
					{

						anaMatrixdata[elementindex][index - 1][kk][0][(i - 1)*nChannelsInEl + j - 1] = anaMatrixdata[elementindex][index - 1][kk - 1][0][(i - 1)*nChannelsInEl + j - 1];

					}
			}
#if EVENODD
		for (kk = 0; kk < bandnum; kk++)
			if (allbandsetflag[elementindex][index - 1][kk] == 1) {
				for (i = 1; i <= usePCAitemnum; i++)
				{
					if ((i == nChannelsInEl) && (nChannelsInEl == 2))
					{

						anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + 1 - 1] = -anaMatrixdata[elementindex][index - 1][kk][1][(i - 2)*nChannelsInEl + 2 - 1];
						anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + 2 - 1] = anaMatrixdata[elementindex][index - 1][kk][1][(i - 2)*nChannelsInEl + 1 - 1];


					}
					else
						for (j = 1; j <= nChannelsInEl; j++)
							if (!((kk > 0) && (j == nChannelsInEl) && (nChannelsInEl == 4)))
							{
								float tmp;

								if ((j == nChannelsInEl) && (elementindex >= 0))//if((j==nChannelsInEl)||( (kk>0)&&(j+1==nChannelsInEl)&&(nChannelsInEl==4) ))
								{
									int nn;
									tmp = 0;
									for (nn = 1; nn < j; nn++)
										tmp += (anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + nn - 1])*(anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + nn - 1]);
									quandata = avs2audiopack_read(&vb->opb, 1);
									tmp = 1 - tmp;
									if (tmp < 0)
										tmp = 0;

									if (quandata == 0)
										tmp = sqrt(tmp);
									else
										tmp = -sqrt(tmp);
									anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + j - 1] = tmp;
								}
								else {
									int valuet;
									quandata = avs2audiopack_read(&vb->opb, PCAQUANT4LEN);
									valuet = quandata;

									tmp = PCAX_1000table16[valuet];


									if (tmp > 0)
										tmp += (float)((0.0) / PCAQUANT4);
									else
										tmp -= (float)((0.0) / PCAQUANT4);
									if (tmp > 1.0)
										tmp = 1.0;
									if (tmp < -1.0)
										tmp = -1.0;
									anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + j - 1] = tmp;
								}
							}
							else
								anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + j - 1] = 0;
				}
			}
			else
			{
				for (i = 1; i <= usePCAitemnum; i++)
					for (j = 1; j <= nChannelsInEl; j++)
					{
						anaMatrixdata[elementindex][index - 1][kk][1][(i - 1)*nChannelsInEl + j - 1] = anaMatrixdata[elementindex][index - 1][kk - 1][1][(i - 1)*nChannelsInEl + j - 1];
					}
			}
#endif



		break;
		default:
			printf("error ll\n");
			exit(1);


		}//switch(ll*2)

	}//for(index=1;index<(lfEnc->lf_winseq[0]+1);index++)

	return 0;
}





int Avs2LFDecoder_PCA(int useBWE,
				
				  int nChannels,
				  int bitRateIndex,
				  int bitRate,
				  int bitPerSample,
				  int ifLFE,
				  AVS2DECODER self,
				  int lf_winseq_indexL,
				  unsigned char sampleData[],
				  int numOutBytes,
				  tianlai_block *vb,
				  tns_data *pTnsData,
                   int *sampleRate,            /*!< pointer to sample rate */
                   char frameOK,                /*!< indicates if current frame data is valid */
 				   float MdctSpectrum[],
				   int indexinelement,
				 int elementindex,
				 int nChannelsInEl,
				 int config_idx )
{
//	static int LL[5]={4096/8/2, 4096/4/2, 4096/2/2, 4096/2,4096/16/2};

	int ErrorStatus = frameOK, Seqmode;
	int avs2Channels = 0;
    int type = 0;
    char ElementInstanceTag;
	int count;

    int   i,j,k;
    int  n, W;
	int lg;

    int lf_winseq_dec[20];
    int bandWidth;

	int nNumOfSFB;
	int *pSfbWidthTable;
	int sfbStartPos[MAX_BAND_NUM];

	StAvs2BweDecMDFT *pstBweMDFT;
	StAvs2BweDecCommon *pstBweCommon;

    self->frameOK = 1;

/*
	memset(&vb->opb,0,sizeof(avs2audiopack_buffer));
	vb->opb.buffer = vb->opb.ptr = sampleData;
    vb->opb.storage = numOutBytes;

	if((elementindex==0)&&(indexinelement==0))
	{unsigned int codectype;
		codectype = avs2audiopack_read(&vb->opb, 1);
		if(codectype==1)
		{
			avs2audiopack_read(&vb->opb, 4);
		}
	}
*/
	

   // while ( (type != ID_END) && self->frameOK )
	{
        //type = avs2audiopack_read(&vb->opb, 4);		
		type=ID_PCA2;
		switch (type)
		{
		  case ID_SCE:
		  case ID_PCA2: 
	      case ID_PCA4:
		  case ID_PCA6:
			/*
			  Consistency check
			*/

			if(avs2Channels >= 2/*Channels*/){

			  self->frameOK = 0;
			  break;
			}

			//usePCAitemnum
			/*
			if((nChannelsInEl>1)&&(indexinelement==0))
				self->usePCAitemnum = avs2audiopack_read(&vb->opb, 3);
			*/
          
			self->pStreamBWE->bweElement[self->pStreamBWE->NrElements].ElementID = ID_SCE;

			if(self->frameOK) 
			{

				int mdftoffsetindex=0, mdftlen;
				int seqtow[4]={0,3,2,1};

				n = 1024 / 8;
				W = 0;
				
				/*
				ElementInstanceTag = avs2audiopack_read(&vb->opb, 4);
				self->lf_winseq_index = avs2audiopack_read(&vb->opb, 6);				
				*/
				ElementInstanceTag = self->ElementInstanceTag;

				
				//////////////////////
				//根据ElementInstanceTag来配置当前声道解码所用码本等
				//
				vb_ci_settable_reset(vb, ElementInstanceTag);
				//设置当前解码声道的bandWidth useBWE信息
				if(!((nChannelsInEl==2)&&(elementindex==1)))
					reset_bandWidth(*sampleRate,config_idx, &bandWidth, nChannels, bitRateIndex, ifLFE, useBWE,ElementInstanceTag);
				else
					reset_bandWidth(*sampleRate,config_idx, &bandWidth, nChannels, /*0*/bitRateIndex, ifLFE, useBWE,ElementInstanceTag);

				memcpy(lf_winseq_dec,lf_winseq_table[self->lf_winseq_index],20*4);


				if((indexinelement==0)&&(nChannelsInEl>1))
				{
					//decoding anaMatrixdata[]
					anaMatrixdata_decode(vb,self->usePCAitemnum,lf_winseq_dec,elementindex,nChannelsInEl);
				}


				for(j = 1; j < lf_winseq_dec[0]+1; j++)
				{ 
					if(lf_winseq_dec[j]>lf_winseq_dec[j+1])
						mdftlen= (1<<(lf_winseq_dec[j]-1));
					else
						mdftlen= (1<<(lf_winseq_dec[j+1]-1));
					
					if(lf_winseq_dec[j]>lf_winseq_dec[j+1])
						W = seqtow[lf_winseq_dec[j]-1];
				    else
						W = seqtow[lf_winseq_dec[j+1]-1];

					lg = n*mdftlen;


					vb->encLen = bandWidth / (1024 / lg);

					switch(lg) {
					case 1024:
						nNumOfSFB = srInfo.nCBNum;
						pSfbWidthTable = srInfo.anCBWidth;
						break;
					case 128:
						nNumOfSFB = srInfo.nCB128Num;
						pSfbWidthTable = srInfo.anCB128Width;
						break;
					case 256:
						nNumOfSFB = srInfo.nCB256Num;
						pSfbWidthTable = srInfo.anCB256Width;
						break;
					case 512:
						nNumOfSFB = srInfo.nCB512Num;
						pSfbWidthTable = srInfo.anCB512Width;
						break;
					}

					sfbStartPos[0] = 0;
					for (k = 0; k < nNumOfSFB; k++)
					{
						sfbStartPos[k+1] = sfbStartPos[k] + pSfbWidthTable[k];
					}

				 //decode the PCA principal components 
				    LFBlockDecoder_PCA(vb, pTnsData, 1, lg, mdftoffsetindex, W, &MdctSpectrum[mdftoffsetindex*n], 
						indexinelement, elementindex, nChannelsInEl, nNumOfSFB, sfbStartPos);
				  
					if(lf_winseq_dec[j]>lf_winseq_dec[j+1])
						mdftoffsetindex += (1<<(lf_winseq_dec[j]-1));
					else
						mdftoffsetindex += (1<<(lf_winseq_dec[j+1]-1));


				}
			}

		//	avs2audiopack_read(&vb->opb, 4);

		//	avs2audiopack_read(&vb->opb, (8 - vb->opb.endbit));

			break;

		 

			/*case ID_FIL:
			{
			int fill_type = 0;
			int fill_byte = 0;
			fill_type = avs2audiopack_read(&vb->opb, 4);

			if(fill_type == 0)
			{
			fill_byte = avs2audiopack_read(&vb->opb, 8);
			for (i=0; i < fill_byte; i++)
			{
			avs2audiopack_read(&vb->opb,8);
			}

			}
			}
			break;*/

		default:
			printf("error!");
			break;
		}

		if(useBWE)
		{
			int ch = 0;
			float Mdftout[4096] = {0};
	
			count = self->pStreamBWE->bweElement[self->pStreamBWE->NrElements].Payload = avs2audiopack_read(&vb->opb, 8);

			for(i=0; i<count/4; i++)
		    {
				self->pStreamBWE->bweElement [self->pStreamBWE->NrElements].Data[i] = (unsigned int) avs2audiopack_read(&vb->opb,32);
			}

			if (count % 4 != 0)
			{
				self->pStreamBWE->bweElement[self->pStreamBWE->NrElements].Data[i] = (unsigned int)avs2audiopack_read(&vb->opb, 8 * (count % 4));
			}

			self->pStreamBWE->NrElements += 1;

			for(i = 0; i < FRAME_SIZE; i++)
			{
				float tmp2;
				tmp2 = MdctSpectrum[i];
				MdctSpectrum[i] = vb->SpectralCoefficient[i];
				vb->SpectralCoefficient[i] = tmp2;
			}
			for(i = 0; i < 1024; i++)
			{
				Mdftout[2*i] = MdctSpectrum[i] * 8;
				Mdftout[2*i+1] = 0;
			}
			
			pstBweMDFT = (StAvs2BweDecMDFT*)(self->st1_decin);
			imdft_lowpassframe4096block_multi(&self->pTimeData[0], pstBweMDFT->lf_winseq_pre, Mdftout, 0, 4096/4/2, pstBweMDFT->Srstereo[0]);
			//imdft_lowpassframe4096block_multi(&self->pTimeData[0], lf_winseq_dec, Mdftout, 0, 4096/4/2, pstBweMDFT->Srstereo[0]);

			/*{
				int j;
				FILE *fp0 = fopen("samples0.txt","a");
				for(j=0; j<1024; j++)
					fprintf(fp0,"%f\n",self->pTimeData[j]);
				fclose(fp0);
			}*/

			/*wuchaogang mdft decoder analysis */
			Seqmode = Avs2BweDecMDFTana(self->st1_decin, self->st_deccommon, &self->pTimeData[0],ch);

			/* decode one bwe frame */
			if(Seqmode != -1)
			{
				int usePS = (self->type==ID_SCE)? 0: 1;
				Avs2BweDecoder(bitRate, self->pStreamBWE, self->st2_decin, self->st_deccommon, bitPerSample, usePS);
			
				pstBweCommon = (StAvs2BweDecCommon *)(self->st_deccommon);
				memcpy(MdctSpectrum, pstBweCommon->bweMdftSpectrum, (FRAME_SIZE * 4+2048) * sizeof(float));
			}
		}
	}
	
    return ErrorStatus;
}

/*  Stream Configuration and Information.

    This class holds configuration and information data for a stream to be decoded. It
    provides the calling application as well as the decoder with substantial information,
    e.g. profile, sampling rate, number of channels found in the bitstream etc.
*/
void CStreamInfoOpen(CStreamInfo **pStreamInfo)
{

  /* initialize CStreamInfo */
 // pStreamInfo[0] = &StreamInfo;
	pStreamInfo[0] = (CStreamInfo*)malloc(sizeof(CStreamInfo));

  pStreamInfo[0]->SamplingRateIndex = 0;
  pStreamInfo[0]->SamplingRate = 0;
  pStreamInfo[0]->Profile = 0;
  pStreamInfo[0]->ChannelConfig = 0;
  pStreamInfo[0]->Channels = 0;
  pStreamInfo[0]->BitRate = 0;
  pStreamInfo[0]->SamplesPerFrame = FRAME_SIZE;

}

/*
  The function initializes the pointers to AacDecoderChannelInfo for each channel,
  set the start values for window shape and window sequence of overlap&add to zero,
  set the overlap buffer to zero and initializes the pointers to the window coefficients.
*/
AVS2DECODER CAvs2DecoderOpen(//HANDLE_BIT_BUF pBs,       /*!< pointer to bitbuffer structure */
	BWEBITSTREAM *pStreamBWE, /*!< pointer to bwe bitstream structure */
	float *pTimeData,
	int idx)         /*!< pointer to time data */
{

	/* initialize bit counter for syncroutine */
	Avs2DecoderInstance[idx].bitCount = 0;

	/* initialize pointer to bit buffer structure */
  //  Avs2DecoderInstance.pBs = pBs;

	/* initialize pointer to time data */
	Avs2DecoderInstance[idx].pTimeData = pTimeData;

	/* initialize pointer to sbr bitstream structure */
	Avs2DecoderInstance[idx].pStreamBWE = pStreamBWE;

	/* initialize stream info */
	CStreamInfoOpen(&Avs2DecoderInstance[idx].pStreamInfo);

	return &Avs2DecoderInstance[idx];
}

AVS2DECODER CAvs2DecoderOpen_frame(//HANDLE_BIT_BUF pBs,       /*!< pointer to bitbuffer structure */
	BWEBITSTREAM *pStreamBWE, /*!< pointer to bwe bitstream structure */
	float *pTimeData,
	int idx)         /*!< pointer to time data */
{
	/* initialize bit counter for syncroutine */
	Avs2DecoderInstance_frame[idx].bitCount = 0;

	/* initialize pointer to bit buffer structure */
	//  Avs2DecoderInstance.pBs = pBs;

	/* initialize pointer to time data */
	Avs2DecoderInstance_frame[idx].pTimeData = pTimeData;

	/* initialize pointer to sbr bitstream structure */
	Avs2DecoderInstance_frame[idx].pStreamBWE = pStreamBWE;

	/* initialize stream info */
	CStreamInfoOpen(&Avs2DecoderInstance_frame[idx].pStreamInfo);

	return &Avs2DecoderInstance_frame[idx];
}


AVS2DECODER CAvs2DecoderOpen_objframe(//HANDLE_BIT_BUF pBs,       /*!< pointer to bitbuffer structure */
	BWEBITSTREAM *pStreamBWE, /*!< pointer to bwe bitstream structure */
	float *pTimeData,
	int idx
)         /*!< pointer to time data */
{
	/* initialize bit counter for syncroutine */
	Avs2DecoderInstance_objframe[index_obj][idx].bitCount = 0;

	/* initialize pointer to bit buffer structure */
	//  Avs2DecoderInstance.pBs = pBs;

	/* initialize pointer to time data */
	Avs2DecoderInstance_objframe[index_obj][idx].pTimeData = pTimeData;

	/* initialize pointer to sbr bitstream structure */
	Avs2DecoderInstance_objframe[index_obj][idx].pStreamBWE = pStreamBWE;

	/* initialize stream info */
	CStreamInfoOpen(&Avs2DecoderInstance_objframe[index_obj][idx].pStreamInfo);

	return &Avs2DecoderInstance_objframe[index_obj][idx];
}




int CAvs2DecoderInit(AVS2DECODER self,
	int samplingRate,
	int bitrate,
	int useBWE,
	int *bandWidth)
{
	int i;
	int idx;

	if (!self)
	{
		return -1;
	}

	if (!useBWE)
	{
		for (i = 0; i < 13; i++)
		{
			if (bitrate >= rate_mapping_44_un[i] && bitrate < rate_mapping_44_un[i + 1])
				idx = i;
		}

		if (idx >= 7 && samplingRate == 24000)
			*bandWidth = (int)(samplingRate * 1024 / samplingRate / 32 + 0.9) * 32;
		else if (samplingRate > 16000)
			*bandWidth = (int)(lowpass_44[idx] * 1000 * 1024 / samplingRate / 32 + 0.9) * 32;
		else
			*bandWidth = (int)(lowpass_32[idx] * 1000 * 1024 / samplingRate / 32 + 0.9) * 32;
	}

	self->pStreamInfo->SamplingRate = samplingRate;


	self->pStreamInfo->BitRate = bitrate;
	self->pStreamInfo->Channels = 1;
	self->pStreamInfo->SamplesPerFrame = FRAME_SIZE;

	return 0;
}


void _make_decode_ready(AVS2Dec_File *vf, int sampleRate, int bitRate)
{

	memset(vf, 0, sizeof(*vf));

	vf->vi = _avs2audio_calloc(1, sizeof(*vf->vi));

	tianlai_info_init(vf->vi);

	tianlai_synthesis_init(&vf->vd, vf->vi, bitRate);

	tianlai_block_init(&vf->vd, &vf->vb);

	vf->bittrack = 0.f;
	vf->samptrack = 0.f;

	if (sampleRate > 16000)
		InitTns(&vf->TnsData, sampleRate);

	return;
}

void _make_decode_reset(AVS2Dec_File *vf, int sampleRate, int bitRate)
{

	tianlai_synthesis_init(&vf->vd, vf->vi, bitRate);

	tianlai_block_init(&vf->vd, &vf->vb);

	vf->bittrack = 0.f;
	vf->samptrack = 0.f;

	if (sampleRate > 16000)
		InitTns(&vf->TnsData, sampleRate);

	return;
}