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
#include <stdlib.h>
#include <libtsp.h>
#include <libtsp/AFpar.h>

#include "common.h"
#include "av3dec.h"
#include "i2r_decoder.h"

int		frameCount;

int		code_channels;
int     freqIdx;
float  *OutBuff;

int	lossless_pcm[MAX_CHANNELS][FRAME_LEN];
static	int AASFHeaderRead(int *nch, 
	int *SfreqIdx, int *resolution, int *Sfreq);
static  int AATFHeaderRead(
	AATFHeader* headerInfoPtr, int *Sfreq);
static 	void decode_raw_block(Decoder_t *pDec, int Sfreq, 
	int nch, FILE*	fpinfo, /*AFILE* AFpO,*/ float* OutBuff);
static void	/*aatf*/frame_error_check();

void sam_decode(/*char *fpIn, */int fformat, char *fpOut, 
				AV3DecFrameInfo* hDecoder, int header_type
				)
{
	int	nch;
	Decoder_t *pDec;
	int	Sfreq, SfreqIdx, rawFrameLength;
	AATFHeader AATFHeaderInfo;

	//for AATF
	//AATFHeader* AATFHeaderInfoP = &AATFHeaderInfo;	

	//AFILE			*AFpO;
	FILE    *AFpO = NULL;
	char	av3fileHeader[4];

	extern int		abits_extended;
	/* -1 = undetermined */
	int				resolution = -1; 
	

	if(header_type == 1){	// AASF
		
		if(AASFHeaderRead(&nch, &SfreqIdx,&resolution, &Sfreq)){
			fprintf(stderr,"\nerror in read av3 audio header.\n");
			return;
		}
	} else {
		///////////////////////////////////////////////////
		//			    AATF header                      //
		///////////////////////////////////////////////////
		if(AATFHeaderRead(&AATFHeaderInfo, &Sfreq)){
			fprintf(stderr, "\n AATF header read error.\n");
			exit(0);
		}
		SfreqIdx  = AATFHeaderInfo.sampling_frequency_index;
		resolution= AATFHeaderInfo.resolution;
		nch = AATFHeaderInfo.channel_config;				
	}

	if (SfreqIdx != FSIDX_EXT){
		Sfreq = samp_rate_info[SfreqIdx];
	}
//	AFpO = AFopenWrite(OutName, fformat, nch, (float)Sfreq, fpinfo);
	AFpO = Wave_fopen(fpOut);


	OutBuff = (float *)av3_malloc(FRAMESIZE*nch*sizeof(float));
	fprintf(stderr, "\nOutput file name: %s\n", fpOut);


	code_channels = nch;
	pDec = i2r_DecoderInit(code_channels, Sfreq, resolution);

	///////////////////////////////////////////////////////
	//                main decoding loop                 //
	///////////////////////////////////////////////////////
	while (!end_bs()) {
		if(header_type == 2){				
						
			decode_raw_block(pDec, Sfreq, 
				nch, AFpO, OutBuff);
			
			frame_error_check();

			if (!end_bs()) {
				if (AATFHeaderRead(&AATFHeaderInfo, &Sfreq, &rawFrameLength)) {
					fprintf(stderr, "\nAATF header read error.\n");
					break;
				}
			}
		} else{
			decode_raw_block(pDec, Sfreq,
				nch, AFpO, OutBuff);
		}
 	}
	free(OutBuff);	
	i2r_DecoderRelease(pDec);
	
	//if(header_type == 2){
	//	free(AATFHeaderInfoP);
 	//}	
	//	AFclose(AFpO);
	Wave_fclose(AFpO, nch, Sfreq, (resolution+1)*8);
	fprintf(stderr, "\n");
}

static void decode_raw_block(Decoder_t *pDec, 
							 int Sfreq, 
							 int nch, 
							 /*FILE* fpinfo,*/
							 FILE* AFpO, 
							 float* OutBuff
							 )
{
	int				i, j, ch;
	int				used_bits;
	int				stereo_mode;
	int				common_window;
	int				frameLength;
	int				samples[2][FRAMESIZE];
	int				frameStart;
	static int		FirstTime;
	static int		frameCount = 0;
	
	extern int		abits_extended;
	int				isLFE;
	int				LR_length;
	int             mc_present;

	int				SfreqIdx = 0;

	frameStart = i_sstell();
	frameLength = (getbits(16)<<3)+16;
	   
	/* lossless decoding */
	{
		int i, j = 0;

		const unsigned char *pucMappings = 
			defChMappings[nch - 1];
		for (i = 0; i < MAX_CHANNELS; )
		{
			unsigned char chPos;
			chPos	= *pucMappings++;
			if((FC == chPos) || (LF == chPos)){
				i2r_DecodeFrame(pDec, j, 1, lossless_pcm);
				i = i + 1;	j	= j + 1;
			}else if(0xFF != chPos){
				chPos	= *pucMappings;
				if(0xFF != chPos){
					i2r_DecodeFrame(pDec, j, 2, lossless_pcm);
					i = i + 2;	j	= j + 2; pucMappings++;
				}else{
					i2r_DecodeFrame(pDec, j, 1, lossless_pcm);
					i = i + 1;	j	= j + 1;
				}
			}else{
				i = i + 1;	
			}
		}
	}

	////////////////////////////////////////////////////////////////
	/*Write Output PCM data*/
	if (frameCount >= 0){
		j = 0;
		for (i = 0; i < FRAMESIZE; i++)
		{
			int jj;
			for (jj = 0; jj < code_channels; jj++)
			{
				//shy OutBuff[j++] = (float)pcmOut[jj][i]/pDec->pcm_scale;			
				if (pDec->pcm_scale > 0)
					OutBuff[j++] = (float)lossless_pcm[jj][i]/(2<<pDec->pcm_scale);
				else if (pDec->pcm_scale < 0)
					OutBuff[j++] = (float)lossless_pcm[jj][i]*(2<<(-pDec->pcm_scale));
				else
					OutBuff[j++] = (float)lossless_pcm[jj][i];
			}
		}
		/* write multi-channel data */
//		AFwriteData(AFpO, OutBuff, FRAMESIZE * code_channels);
		if (pDec->Res == 24) //shumin.xu 20211105
		{
			for (i = 0; i < FRAMESIZE*code_channels; i++)
				OutBuff[i] *= 256.0;
		}
		write_data(OutBuff, FRAMESIZE * code_channels, pDec->Res, AFpO);
	}

	used_bits = i_sstell() - frameStart;
	if (used_bits > frameLength)
		fprintf(stderr/*fpinfo*/, "Error: Decoding Bits(%d) > Framelength(%d)\n", used_bits, frameLength);

	/*Byte align*/
	while (used_bits < frameLength) {
		int  remain, read_bits;

		remain = frameLength - used_bits;
		read_bits = remain > 8 ? 8 : remain;
		i = getbits(read_bits);
		used_bits = i_sstell() - frameStart;
	}
	fprintf(stderr/*fpinfo*/, "\rFrame # : %5d\t%5d", frameCount++, i_sstell()-frameStart);
}

static int AASFHeaderRead(int *nch, 
						  int *SfreqIdx,
						  int *resolution, 
						  int *Sfreq
						  )
{
	int raw_stream_length;
	int audio_codec_id;
	int coding_profile;
	char av3fileID[4];
	int	 header_size;
	int	 i, j;
	int tmp;

	/* read AASF ID */
	av3fileID[0] = (char)getbits(8);
	av3fileID[1] = (char)getbits(8);
	av3fileID[2] = (char)getbits(8);
	av3fileID[3] = (char)getbits(8);

	if(av3fileID[0] != 'A' 
		&& av3fileID[1] != 'A' 
		&& av3fileID[2] != 'S'
		&& av3fileID[3] != 'F'){
		printf("\nstream not exist AASF flag");
		return 1;
	}
    printf("\nav3file ID is : ");
	for(i=0;i<4;i++){
		printf("%c",av3fileID[i]);
	}

	/* read AASF header size */
	header_size = getbits(24);
	printf("\nheader size is %d", header_size);

	/* read raw stream length */
	raw_stream_length = getbits(32);
	printf("\nraw stream length is %d bytes", 
		raw_stream_length);

	/* read audio code id */
	audio_codec_id = getbits(4);
	if( (1 != audio_codec_id) ){
		printf("\nit is not lossless stream");
		return 1;
	}

	/* read resolution */
	*resolution = getbits(2);
	if( (*resolution > 2) ){
		printf("\nunsupport resolution");
		return 1;
	}

	/* read coding profile */
	coding_profile = getbits(3);
	if( (coding_profile > 1) ){
		printf("\nunsupport coding profile");
		return 1;
	}

	/* read anc_data_index */
	tmp = getbits(1);
	if ((tmp > 0)) {
		printf("\nunsupport anc_data_index");
		return 1;
	}

	/* read channel number */
	*nch = getbits(4) + 1;
	if(16 == *nch){
		*nch += getbits(4);
	}

	/* read frequency index */
	*SfreqIdx = getbits(4);
	printf("\nfrequency index is %d", *SfreqIdx);
	if (FSIDX_EXT == *SfreqIdx){
		/* read frequency value */
		*Sfreq= getbits(24);
	}

	/* byte alignment */
	if (i_sstell() & 0x7){
		getbits(8 - i_sstell()&0x7);
	}
	return 0;
}

static int AATFHeaderRead(AATFHeader* headerInfoPtr, int *Sfreq)
{
	int i, coding_profile, audio_codec_id;
	int tmp, tmp2, raw_frame_len;
	
	/* aatf_decoding_header */
	if(getbits(12) != 0x07fe){
		printf("\nerror in read syncword.\n");
		return 1;
	}
	/* read audio code id */
	audio_codec_id = getbits(4);
	if( (audio_codec_id != 1) ){
		printf("\it is not lossless frame");
		return 1;
	}

	/* read anc data index*/
	tmp = getbits(1);

	/* read coding profile */
	coding_profile = getbits(3);
	if( (coding_profile > 1) ){
		printf("\nunsupport coding profile");
		return 1;
	}
	/* read frequency index */
	headerInfoPtr->sampling_frequency_index = getbits(4);
	if (FSIDX_EXT == headerInfoPtr->sampling_frequency_index){
		/* read frequency value */
		*Sfreq= getbits(24);
	}

	/* read raw frame length*/
	tmp = getbits(8);
	tmp2 = getbits(8);
	tmp2 = tmp2 << 8;
	raw_frame_len = tmp2 + tmp;

	/* read attf error check*/
	tmp = getbits(8);

	/* read channel numer */
	headerInfoPtr->channel_config = getbits(4) + 1;
	if(16 == headerInfoPtr->channel_config){
		headerInfoPtr->channel_config += getbits(4);
	}
	/* read resolution */
	headerInfoPtr->resolution = getbits(2);
	if( (headerInfoPtr->resolution > 2) ){
		printf("\nunsupport resolution");
		return 1;
	}
	
	/* byte alignment */
	if (i_sstell() & 0x7){
		getbits(8 - i_sstell()&0x7);
	}

	return 0;
}

static void frame_error_check()
{
	char crcCheck = getbits(8);
}


