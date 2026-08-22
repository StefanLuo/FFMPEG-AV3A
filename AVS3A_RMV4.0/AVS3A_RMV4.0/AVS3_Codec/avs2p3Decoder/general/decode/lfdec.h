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

#ifndef _lfdec_h_
#define _lfdec_h_

#include "lfenc.h"
#include "..\bwedec\dec.h"
#include "streamInfo.h"

#define MAX_ALLCHANNEL 16//8

#define FRAME_SIZE 1024

#define MAX_SFB_SHORT   15
#define MAX_SFB_LONG    51
#define MAX_SFB         (MAX_SFB_SHORT > MAX_SFB_LONG ? MAX_SFB_SHORT : MAX_SFB_LONG)   /* = MAX_SFB_LONG */



#define  MaximumWindows2  8
#define  MaximumBands2   49
#define  MaximumOrder2   31
#define  MaximumFilters2  3

#define MAX_BAND_NUM				128 //100

typedef struct
{
    int		nIndex;
    int		nLen;
    unsigned long	ulCodeWord; 
} HuffmanWord;

typedef	struct
{
    int		nTableSize;
    int		nPackedNumber;
    int		nLargestAbsoluteValue;
    int		nLenCodingRange;
    int		nOffset;
    int		nBeSigned;
    HuffmanWord	*hcw;
} HuffmanTableStruc;

extern	HuffmanTableStruc	huffmanbook;
extern	HuffmanWord		huffmantablescl[];
extern codebook huffmanDecodeBook;

typedef struct { 
  int    nCBNum;
  int    nCB128Num;
  int    nCB256Num;
  int    nCB512Num;

  int    anCBWidth[MAX_BAND_NUM];
  int    anCB128Width[MAX_BAND_NUM];
  int    anCB256Width[MAX_BAND_NUM];
  int    anCB512Width[MAX_BAND_NUM];
} SR_INFO;

typedef struct
{
  char StartBand;
  char StopBand;

  char Direction;
  char Resolution;

  char Order;
  char Coeff[MaximumOrder2];
} TnsFilter;

typedef struct
{
  char TnsDataPresent[MaximumWindows2];
  char NumberOfFilters[MaximumWindows2];
  TnsFilter Filter[MaximumWindows2][MaximumFilters2];
  
  char TotalSfBands[4];
  char tnsMaxSfb[4];
  int sfbCnt[4];
  int sfbOffset[4][MAX_SFB+1];

} tns_data;

/* The structure CStreamInfo contains the streaming information. */
CStreamInfo StreamInfo;
typedef struct AVS2Dec_File {
  tianlai_info     *vi;
  double           bittrack;
  double           samptrack;
  tianlai_dsp_state vd; /* central working state for the packet->PCM decoder */
  tianlai_block     vb; /* local working space for packet->PCM decode */


  tns_data TnsData;

} AVS2Dec_File;

struct AVS2_DECODER_INSTANCE {
	int type;		// added by Lu Min
  unsigned char frameOK;   /*!< Will be unset if the CRC, a consistency check etc. fails */
  unsigned long bitCount;
  long byteAlignBits;

  float *pTimeData;
  BWEBITSTREAM *pStreamBWE;
  CStreamInfo *pStreamInfo;

  
  //////
  AVS2Dec_File vf;

  int *st1_decin;
  int *st2_decin;
  int *st_deccommon;

  int lf_winseq_index; 

  int usePCAitemnum;
  char ElementInstanceTag;

};// Avs2DecoderInstance;

typedef struct AVS2_DECODER_INSTANCE *AVS2DECODER;





int CAvs2DecoderInit(AVS2DECODER self,
                    int samplingRate,
                    int bitrate,
					int useBWE,
					int *bandWidth);

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
				   int config_idx);

void Avs2Decoder_syn(AVS2DECODER self, 
				int useBWE,
				float MdctSpectrum[],
				float *pTimeData,
				int bitRate,
				int bitsPerSample);

long avs2audiopack_read(avs2audiopack_buffer *b,int bits);

void avs2audiopack_adv(avs2audiopack_buffer *b,int bits);
long avs2audiopack_look(avs2audiopack_buffer *b,int bits);

void _make_decode_ready(AVS2Dec_File *vf, int sampleRate, int bitRate);


void InitTns(tns_data *TnsData, int sampleRate);
void decodeTnsData(avs2audiopack_buffer *opb,  /*!< pointer to bitstream */
				   	int blocknum,
  				    int blocktype,
					int W,
                    tns_data *pTnsData) ;
void ApplyTns (tns_data *pTnsData, float *pSpectrum, int blocknum, int W, int encLen);


AVS2DECODER CAvs2DecoderOpen(//HANDLE_BIT_BUF pBs,       /*!< pointer to bitbuffer structure */
                           BWEBITSTREAM *pStreamBWE, /*!< pointer to bwe bitstream structure */
                           float *pTimeData,
						   int idx);

AVS2DECODER CAvs2DecoderOpen_frame(//HANDLE_BIT_BUF pBs,       /*!< pointer to bitbuffer structure */
                           BWEBITSTREAM *pStreamBWE, /*!< pointer to bwe bitstream structure */
                           float *pTimeData,
						   int idx);


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
				 int config_idx);
int ci_settable_init(codec_setup_info *ci_table);
void inithuffmantable(HuffmanTableStruc* phtable, int bookList[][121], HuffmanWord *phword, int nPackedNumber, int nLargestAbsoluteValue, int nBeSigned);

#endif