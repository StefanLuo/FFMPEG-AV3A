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

#ifndef _lfenc_h_
#define _lfenc_h_

#include <math.h>

#include "psy.h"
#include "highlevel.h"
#include "bitrate.h"
#include "avs2audio.h"
#include "codec.h"

/* here we distinguish between stereo and the mono only encoder */
#ifdef MONO_ONLY
#define MAX_CHANNELS        1
#else
#define MAX_CHANNELS        2
#endif


#define MAX_CHANNEL_BITS         6144

#define rint(x)   (floor((x)+0.5f))

typedef  struct {
  int   sampleRate;            /* audio file sample rate */
  int   bitRate;               /* encoder bit rate in bits/sec */
  int   nChannelsIn;           /* number of channels on input (1,2) */
  int   nChannelsOut;          /* number of channels on output (1,2) */
  int   bandWidth;             /* core coder audio bandwidth in Hz */
} ENC_CONFIG;

typedef enum {
	ID_SCE = 0,     /* Single Channel Element   */
	ID_CPE_F = 1,     /* Channel Pair Element     */
	ID_CPE_S = 2,		/* Channel_Pair_Element */
	ID_PCA2 = 3,		/* Multi_Channel_Element */
	ID_LFE = 4,		/* Low_Frequency_Element */
	ID_PCA4 = 5,
	ID_FIL = 6,
	ID_END = 7,
	ID_PCA6 = 8,
	ID_PCA3 = 9,
	ID_SCE_L = 10,
	ID_SCE_R = 11,
	ID_SCE_C = 12,
	ID_PCA5 = 13,
	ID_CPE_H = 14,
	ID_CPE_L = 15,
	ID_NULL = 0xFF
}ELEMENT_TYPE;

typedef struct {
	ELEMENT_TYPE elType;
	int instanceTag;
	int nChannelsInEl;
	int ChannelIndex[6];
}ELEMENT_INFO;


struct AVS2_ENCODER {
  ENC_CONFIG config;
  ELEMENT_INFO elInfo;
};


/* psychoacoustic setup ********************************************/
#define P_BANDS 17      /* 62Hz to 16kHz */
#define P_LEVELS 8      /* 30dB to 100dB */
#define P_LEVEL_0 30.    /* 30 dB */
#define P_NOISECURVES 3

#define NOISE_COMPAND_LEVELS 40

#define NEGINF -9999.f


#define VE_PRE    16
#define VE_WIN    4
#define VE_POST   2
#define VE_AMP    (VE_PRE+VE_POST-1)

#define VE_BANDS  7
#define VE_NEARDC 15

typedef struct tianlai_block_internal{
  float  **pcmdelay;  /* this is a pointer into local storage */
  float  ampmax;
  int    blocktype;

  avs2audiopack_buffer *packetblob[PACKETBLOBS]; /* initialized, must be freed;
                                              blob PACKETBLOBS/2] points to
                                              the avs2audiopack_buffer in the
                                              main tianlai_block */
} tianlai_block_internal;





/* mode ************************************************************/
typedef struct {
  int blockflag;
  int windowtype;
  int transformtype;
  int mapping;
} tianlai_info_mode;



typedef struct tianlai_info_mapping0{
  int   submaps;  /* <= 16 */
  int   chmuxlist[256];   /* up to 256 channels in a tianlai stream */

  int   floorsubmap[16];   /* [mux] submap to floors */
  int   residuesubmap[16]; /* [mux] submap to residue */

  int   coupling_steps;
  int   coupling_mag[256];
  int   coupling_ang[256];

} tianlai_info_mapping0;



/* This structure encapsulates huffman and VQ style encoding books; it
   doesn't do anything specific to either.

   valuelist/quantlist are nonNULL (and q_* significant) only if
   there's entry->value mapping to be done.

   If encode-side mapping must be done (and thus the entry needs to be
   hunted), the auxiliary encode pointer will point to a decision
   tree.  This is true of both VQ and huffman, but is mostly useful
   with VQ.

*/

typedef struct static_codebook{
  long   dim;            /* codebook dimensions (elements per vector) */
  long   entries;        /* codebook entries */
  long  *lengthlist;     /* codeword lengths in bits */

  /* mapping ***************************************************************/
  int    maptype;        /* 0=none
                            1=implicitly populated values from map column
                            2=listed arbitrary values */

  /* The below does a linear, single monotonic sequence mapping. */
  long     q_min;       /* packed 32 bit float; quant value 0 maps to minval */
  long     q_delta;     /* packed 32 bit float; val 1 - val 0 == delta */
  int      q_quant;     /* bits: 0 < quant <= 16 */
  int      q_sequencep; /* bitflag */

  long     *quantlist;  /* map == 1: (int)(entries^(1/dim)) element column map
                           map == 2: list of dim*entries quantized entry vals
                        */
  int allocedp;
} static_codebook;

typedef struct codebook{
  long dim;           /* codebook dimensions (elements per vector) */
  long entries;       /* codebook entries */
  long used_entries;  /* populated codebook entries */
  const static_codebook *c;

  /* for encode, the below are entry-ordered, fully populated */
  /* for decode, the below are ordered by bitreversed codeword and only
     used entries are populated */
  float        *valuelist;  /* list of dim*entries actual entry values */
  unsigned int *codelist;   /* list of bitstream codewords for each entry */

  int          *dec_index;  /* only used if sparseness collapsed */
  char         *dec_codelengths;
  unsigned int *dec_firsttable;
  int           dec_firsttablen;
  int           dec_maxlength;

  /* The current encoder uses only centered, integer-only lattice books. */
  int           quantvals;
  int           minval;
  int           delta;
} codebook;


typedef struct codec_setup_info {

  /* tianlai supports only short and long blocks, but allows the
     encoder to choose the sizes */

  long blocksizes[2*2];//wuchaogang 2013.8.28

  /* modes are the primary means of supporting on-the-fly different
     blocksizes, different channel mappings (LR or M/A),
     different residue backends, etc.  Each mode consists of a
     blocksize flag and a mapping (along with the mapping setup */

  int        modes;
  int        maps;
  int        floors;
  int        residues;
  int        books;
  int        psys;     /* encode only */

  tianlai_info_mode       *mode_param[64];
  int                     map_type[64];
  tianlai_info_mapping    *map_param[64];
  int                     floor_type[64];
  tianlai_info_floor      *floor_param[64];
  int                     residue_type[64];
  tianlai_info_residue    *residue_param[64];
  static_codebook        *book_param[256];
  codebook               *fullbooks;

  tianlai_info_psy        *psy_param[4*2]; /* encode only */
  tianlai_info_psy_global psy_g_param;

//  bitrate_manager_info   bi;
  highlevel_encode_setup hi; 
								/* used only by tianlaienc.c.  It's a
                                highly redundant structure, but
                                improves clarity of program flow. */
  int         halfrate_flag; /* painless downsample for decode */
} codec_setup_info;

#define VIF_POSIT 63
#define VIF_CLASS 16
#define VIF_PARTS 31
typedef struct{
  int   partitions;                /* 0 to 31 */
  int   partitionclass[VIF_PARTS]; /* 0 to 15 */

  int   class_dim[VIF_CLASS];        /* 1 to 8 */
  int   class_subs[VIF_CLASS];       /* 0,1,2,3 (bits: 1<<n poss) */
  int   class_book[VIF_CLASS];       /* subs ^ dim entries */
  int   class_subbook[VIF_CLASS][8]; /* [VIF_CLASS][subs] */


  int   mult;                      /* 1 2 3 or 4 */
  int   postlist[VIF_POSIT+2];    /* first two implicit */


  /* encode side analysis parameters */
  float maxover;
  float maxunder;
  float maxerr;

  float twofitweight;
  float twofitatten;

  int   n;

} tianlai_info_floor1;

typedef struct {
  int sorted_index[VIF_POSIT+2];
  int forward_index[VIF_POSIT+2];
  int reverse_index[VIF_POSIT+2];

  int hineighbor[VIF_POSIT];
  int loneighbor[VIF_POSIT];
  int posts;

  int n;
  int quant_q;
  tianlai_info_floor1 *vi;

  long phrasebits;
  long postbits;
  long frames;
} tianlai_look_floor1;

/* this would all be simpler/shorter with templates, but.... */
/* Floor backend generic *****************************************/
typedef struct{
  tianlai_look_floor     *(*look)  (tianlai_dsp_state *,tianlai_info_floor *);
  void (*free_info) (tianlai_info_floor *);
  void (*free_look) (tianlai_look_floor *);
 void *(*inverse1)  (struct tianlai_block *,tianlai_look_floor *,
	 int *,float *);
  int   (*inverse2)  (struct tianlai_block *,tianlai_look_floor *,
                     float *,float *);
} tianlai_func_floor;

/* Residue backend generic *****************************************/
typedef struct{
  tianlai_look_residue *(*look)  (tianlai_dsp_state *,
                                 tianlai_info_residue *);
  void (*free_info)    (tianlai_info_residue *);
  void (*free_look)    (tianlai_look_residue *);
  long **(*class)      (struct tianlai_block *,tianlai_look_residue *,
                        int **,int *,int);
  int  (*forward)      (avs2audiopack_buffer *,struct tianlai_block *,
                        tianlai_look_residue *,
                        int **,int *,int,long **,int);
  int  (*inverse)      (struct tianlai_block *,tianlai_look_residue *,
                        float **,int *,int);
} tianlai_func_residue;


typedef struct {
  int att[P_NOISECURVES];
  float boost;
  float decay;
} att3;

typedef struct vp_adjblock{
  int block[P_BANDS];
} vp_adjblock;

typedef struct {
  int lo;
  int hi;
  int fixed;
} noiseguard;

typedef struct {
  int data[P_NOISECURVES][17];
} noise3;

typedef struct {
  int data[NOISE_COMPAND_LEVELS];
} compandblock;

typedef struct tianlai_info_residue0{
/* block-partitioned VQ coded straight residue */
  long  begin;
  long  end;

  /* first stage (lossless partitioning) */
  int    grouping;         /* group n vectors per partition */
  int    partitions;       /* possible codebooks for a partition */
  int    partvals;         /* partitions ^ groupbook dim */
  int    groupbook;        /* huffbook for partitioning */
  int    secondstages[64]; /* expanded out to pointers in lookup */
  int    booklist[512];    /* list of second stage books */

  const int classmetric1[64];
  const int classmetric2[64];
} tianlai_info_residue0;

typedef struct {
  const static_codebook *books[12+2][4];
} static_bookblock;

typedef struct {
  int res_type;

  int grouping;
  const tianlai_info_residue0 *res;
  const static_codebook  *book_aux;
  const static_codebook  *book_aux_managed;
  const static_bookblock *books_base;
  const static_bookblock *books_base_managed;
} tianlai_residue_template;

typedef struct {
  const tianlai_info_mapping0    *map;
  const tianlai_residue_template *res;
} tianlai_mapping_template;

typedef struct {
  int      mappings;
  const double  *rate_mapping;
  int      coupling_restriction;
  long     samplerate_min_restriction;
  long     samplerate_max_restriction;


  const int     *blocksize_short;
  const int     *blocksize_long;

  const att3    *psy_tone_masteratt;
  const int     *psy_tone_0dB;
  const int     *psy_tone_dBsuppress;

  const vp_adjblock *psy_tone_adj_impulse;
  const vp_adjblock *psy_tone_adj_long;
  const vp_adjblock *psy_tone_adj_other;

  const noiseguard  *psy_noiseguards;
  const noise3      *psy_noise_bias_impulse;
  const noise3      *psy_noise_bias_padding;
  const noise3      *psy_noise_bias_trans;
  const noise3      *psy_noise_bias_long;
  const int         *psy_noise_dBsuppress;

  const compandblock  *psy_noise_compand;
  const double        *psy_noise_compand_short_mapping;
  const double        *psy_noise_compand_long_mapping;

  const int      *psy_noise_normal_start[2];
  const int      *psy_noise_normal_partition[2];
  const double   *psy_noise_normal_thresh;

  const int      *psy_ath_float;
  const int      *psy_ath_abs;

  const double   *psy_lowpass;

  const tianlai_info_psy_global *global_params;
  const double     *global_mapping;

  const static_codebook *const *const *const floor_books;
  const tianlai_info_floor1 *floor_params;
  const int floor_mappings;
  const int **floor_mapping_list;

  const tianlai_mapping_template *maps;
} ve_setup_data_template;



void tianlai_info_init(tianlai_info *vi);
int tianlai_block_init(tianlai_dsp_state *v, tianlai_block *vb);
void *_tianlai_block_alloc(tianlai_block *vb,long bytes);

//void avs2audiopack_write(avs2audiopack_buffer *b,unsigned long value,int bits);

#endif