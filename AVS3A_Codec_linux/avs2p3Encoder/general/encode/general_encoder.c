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
#include "iir32resample.h"
#include "resampler.h"
#include "../bweenc/avs2BweEncMDFT.h"
#include "../bweenc/encoder.h"
#include "lfenc.h"
#include "pca.h"
#include "maxcorr.h"
#include "mc_rom.h"
#include "crc_16.h"

#ifdef __unix__
	#define	bname(s)	(strrchr(s, '/')? strrchr(s, '/') + 1 : s)
	#define	C35		"\E[35m"
	#define C0		"\E[0m"
#else
	#define	bname(s)	s 
	#define	C35		""
	#define	C0		""
#endif

#define DOWN_DELAY (6)
#define MCR_DELAY (1024*2)
#define TOTAL_DELAY (CORE_FRAMESIZE*4+MCR_DELAY)
#define MAX_ALLCHANNEL 16                   //shumin.xu 20210510
#define CORE_DELAY   (1600)
#define INPUT_DELAY  ((CORE_DELAY)*2+1)     /* ((1600 (core codec)*2 (multi rate) + 6*64 (sbr dec delay) - 2048 (sbr enc delay) + magic*/
#define MAX_DS_FILTER_DELAY 16              /* the additional max resampler filter delay (source fs)*/
#define MAX_PAYLOAD_SIZE    256

static IIR21_RESAMPLER IIR21_reSampler[MAX_ALLCHANNEL]; 
static IIR21_RESAMPLER IIR21_bweSampler[MAX_ALLCHANNEL]; 
static float inputBuffer[MAX_ALLCHANNEL][AVS2ENC_BLOCKSIZE*2 + MAX_DS_FILTER_DELAY + INPUT_DELAY];
static float downmixBuffer[MAX_ALLCHANNEL][AVS2ENC_BLOCKSIZE*2 + MAX_DS_FILTER_DELAY + INPUT_DELAY];
struct MULTI_CHAN_MODE{
	int numofele; 	/* the number of element */
	ELEMENT_TYPE idType[MAX_ALLCHANNEL];	/* the type of the element */
	int ele_id[MAX_ALLCHANNEL];
};
typedef struct MULTI_CHAN_MODE MC_MODE;
codec_setup_info ci_table[13];
extern int inputchannelnum;
extern const double rate_mapping_44_multi[5];

// added by lumin 2014.11.21
MC_MODE	CoupleChannelTable[8+2+6+2] = {
	{1,	{ID_SCE,0,0,0,0,0},{0,0,0,0,0,0}},
	{2,	{ID_SCE,ID_SCE,0,0,0,0},{0,1,0,0,0,0}},
	{1,	{ID_CPE_F,0,0,0,0,0},{0,1,0,0,0,0}},
	{1,	{ID_CPE_L,0,0,0,0,0},{0,1,0,0,0,0}},//CPE_L
	{6,	{ID_SCE,ID_SCE,ID_SCE,ID_SCE,ID_SCE,ID_SCE},{0,1,2,3,4,5}},
	{5,	{ID_SCE,ID_SCE,ID_SCE,ID_SCE,ID_CPE_F,0},{0,1,2,3,4,5}},
	{4,	{ID_CPE_F,ID_SCE,ID_SCE,ID_CPE_F,0,0},{0,1,2,3,4,5}},
	{3,	{ID_CPE_F,ID_CPE_F,ID_CPE_F,0,0,0},{0,1,2,3,4,5}},
	{3, {ID_CPE_F,ID_CPE_F,ID_CPE_F,0,0,0},{0,4,2,3,1,5}},
	{4, {ID_CPE_F,ID_CPE_F,ID_CPE_F,ID_CPE_F,0,0},{0,1,2,3,4,5,6,7}},

	/*aditional channel configuration, shumin.xu 20210105*/                            
	{ 1, { ID_PCA4, 0, 0, 0, 0, 0 }, { 0, 1, 2, 3, 0, 0 } },			//4.0									//10
	{ 2, { ID_CPE_F, ID_CPE_F, 0, 0, 0, 0 }, { 0, 1, 2, 3, 0, 0 } },											//11

	{ 4, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, 0, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7 } },						 	//12          5.1.2
	{ 5, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, ID_CPE_H, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9} },				//13          5.1.4
	{ 5, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } },				//14          7.1.2
	{ 6, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_H, ID_CPE_H }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11} }, //15          7.1.4
	{ 8, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}}, //16 3rd HOA, shumin.xu 20210510
	{ 5, { ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_CPE_F, ID_SCE, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8 }} //9ch 2nd HOA, shumin.xu 211123
};
typedef struct {
  ELEMENT_TYPE elType;
  int nChannelsInEl;
  int ChannelIndex[MAX_ALLCHANNEL];
 } ELEMENTENCODE_INFO;

///////////////////////////////
avs2audio_packet       op;
MC_MODE *encoder_mode;
ELEMENTENCODE_INFO elementencode_info[/*8*/9];
MC_MODE PCAGroupmodeHeaderTable[36]={
/* mono */
	{1,	{ID_SCE, 0,0,0,0}},//0
	/* stereo */
	{1,	{ID_PCA2, 0,0,0,0}},//1
	/* 5.1*/
	{2,	{ID_PCA4,ID_PCA2, 0}}, //0x02
	/* 5.1*/
	{3,	{ID_PCA2,ID_PCA2,ID_PCA2}}, //0x03
	/* 5.1*/
	{3,	{ID_PCA2,ID_PCA2,ID_PCA2}}, //0x04
	/* 5.1*/
	{2,	{ID_PCA5,ID_LFE, 0}}, //0x05
	/* 5.1*/
	{3,	{ID_PCA4,ID_SCE_C, ID_LFE}}, //0x06
	/* 5.1*/
	{1,	{ID_PCA6, 0}}, //0x07
	/* reserved */
	{0, {0,0,0,0,0}},//0x08
	/* reserved */
	{0, {0,0,0,0,0}},//0x09
	/* reserved */
	{0, {0,0,0,0,0}},//0x0a
	/* reserved */
	{0, {0,0,0,0,0}},//0x0b
	/* reserved */
	{0, {0,0,0,0,0}},//0x0c
	/* reserved */
	{0, {0,0,0,0,0}},//0x0d
	/* reserved */
	{0, {0,0,0,0,0}},//0x0e
	/* reserved */
	{0, {0,0,0,0,0}},//0x0f
	/* 7.1 */
	{3,	{ID_PCA4,ID_PCA2,ID_PCA2, 0}} //0x10
};

MC_MODE *encoder_mode_tianlai51 =&PCAGroupmodeHeaderTable[2];
ELEMENTENCODE_INFO elementencode_info_tianlai51[8];

//double allchannelbasesetting[MAX_ALLCHANNEL]={4,4,4,2,2,2};
long Maxpcmvalue=(32768*256);
long datalencount[MAX_ALLCHANNEL]={0}; //各声道编码码流大小统计
long mcrlencount[MAX_ALLCHANNEL]={0}; //各声道编码码流大小统计
long PCAdatalencount[MAX_ALLCHANNEL]={0}; //各声道编码码流大小统计

int brate_mapping[16][16] = {
 {16,32,44,56,64,72,80,96,128,144,164,192,48}, //mono
 {24,32,48,64,80,96,128,144,192,256,320},   //stereo
 {128,192,256,320,384,448,512,640,720,144}, //5.1
 {192,480,256,384,576,640},                 //7.1
 {0},                                       //10.2
 {0},                                       //22.2
 {48,96,128,192,256},                       //4.0
 {152,320,480,576},                         //5.1.2
 {176,384,576,704,256,448},                 //5.1.4
 {216,480,576,384,768},                     //7.1.2 
 {240,608,384,512,832},                     //7.1.4
 {256,320,384,512,640,896},                 //16ch 3rd HOA, shumin.xu 211214
 {192,256,320,384,480,512,640},             //9ch  2nd HOA, shumin.xu 211214
};

//////////////////////////////
int blocktypeB;
int lf_winseqpre[20] = {1,4,4};
int wOffset, cOffset;
int coreOffset = CORE_FRAMESIZE/2 + CORE_FRAMESIZE/8/2;
float Mdftout[CORE_FRAMESIZE * 8] = {0};
int codetypemode=0; //0 ： all tsinghua; 1: all tianlai; 2 auto

void copyright(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "\n");
}

const char *phelpstr = "avs encoder verson 1.0\n\n\
usage: avs2enc -if <infile> -of <outfile> [options]\n\n\
RECOMMENDED:\n\
when input.wav is a mono wave:\n\
    avs2enc -if input.wav -of output.avs -b 20000\n\
when input.wav is a stereo wave:\n\
    avs2enc -if input.wav -of output.avs -b 24000\n\n\
OPTIONS:\n\
    -b bitrate\t\tset the bitrate, in bps, default 16000\n\
	-codec_id 0,1\tset audio codec id\n\
	             \t0:general audio encoder;1:lossless audio encoder; default is general audio encoder;\n\
	-f 1,2\tset output format\n\
	      \t1:AASF;2:AATF;default is AASF;\n\
    -h or --help\tshow this list of options\n";

static void parsecmdline(int argc,
                         char *argv[],
                         char **input_filename,
                         char **output_filename, 
                         //char **config_filename, 
                         ChanInfo * conf)
{
	conf->bitRate = 64000;
	conf->outputFormat = 2;
	conf->use_mono_encode = 0;
	conf->codec_id = 0;
	conf->coding_profile = 0;
	conf->anc_data_index = 0;
		
	argc--;
	argv++;

	if (argc == 0)
	{
		fprintf(stderr, phelpstr, *argv);
		exit(EXIT_SUCCESS);
	}
	
	while (argc > 0)
	{
		if (!strcmp(*argv, "-if"))
		{
		  argv++;
		  argc--;
		  *input_filename = *argv;
		}
		else if (!strcmp(*argv, "-of"))
		{
		  argv++;
		  argc--;
		  *output_filename = *argv;
		} 
		else if (!strcmp(*argv, "-b"))
		{
			argv++;
			argc--;
			conf->bitRate = atoi(*argv);
		}
		else if (!strcmp(*argv, "-f"))
		{
			argv++;
			argc--;
			conf->outputFormat = atoi(*argv);
		}
		else if (!strcmp(*argv, "-mono"))
		{
			conf->use_mono_encode = 1;
		}
		else if (!strcmp(*argv, "-codec_id"))
		{
			argv++;
			argc--;
			conf->codec_id = atoi(*argv);
		}
		else if (!strcmp(*argv, "-coding_profile"))
		{
			argv++;
			argc--;
			conf->coding_profile = atoi(*argv);
		}
		else if (!strcmp(*argv, "-head"))
		{
			argv++;
			argc--;
			conf->headflag = atoi(*argv);
		}
		else if (!strcmp(*argv, "-anc"))
		{
			argv++;
			argc--;
			conf->anc_data_index = atoi(*argv);
		}
		else if (!strcmp(*argv, "-h") || !strcmp(*argv, "--help"))
		{
			fprintf(stderr, phelpstr, *argv);
			exit(EXIT_SUCCESS);
		}
		else
		{
			fprintf(stderr, "Unknown option %s\n", *argv);
			exit(EXIT_FAILURE);
		}
		argv++;
		argc--;
	}		   
}

#define MASK      0x0001
void int2bin(
			 int   value,         /* input : value to be converted to binary      */
			 int   no_of_bits,    /* input : number of bits associated with value */
			 short *bitstream     /* output: address where bits are written       */
			 )
{
	short *pt_bitstream;
	int   i;
	pt_bitstream = bitstream + no_of_bits;
	for (i = 0; i < no_of_bits; i++)
	{
		*--pt_bitstream = (short)(value & MASK);
		value >>= 1;
		//printf("%d	%d\n",pt_bitstream,*pt_bitstream);
	}
}

void WriteBitstreamPlus(short length, short offset, short * serial, unsigned char* headbuffer)
{
	unsigned char byte;
	short j, k, nb_byte, *pt_serial, *ptr;

	pt_serial = (short *)serial;
	nb_byte = ((length)+7) / 8;
	ptr = &pt_serial[offset * (length / 4)];
	for (j = 0; j < nb_byte; j++)
	{
		byte = 0;
		for (k = 0; k < 8; k++, ptr++)
		{
			byte <<= 1;
			if (*ptr != 0)
				byte += 1;
		}
		memcpy(headbuffer + j, &byte, 1);
		//fwrite(&byte, sizeof(char), 1, f_serial);
	}
}

int write_avs2file_header(int samplingRate, 
						   int bitstream_type, 
						   ChanInfo * conf, 
						   int useSuperMode, int cpe_config,
						   int PCAGroupmodeHeader,
						   unsigned char* headbuffer)
{
	short buffer[256];
	short *ptr = buffer;
	int nbits = 0;
	int i,j;
	int numOfChannels = conf->nChannels;
	int  bitsPerSample = conf->bitsPerSample;
	int index;
	
	printf("buffer:%d	\n",ptr);
	memset(buffer, 0, 256 * sizeof(short));
	// esaf id
	int2bin('A', 8, ptr);
	ptr += 8;
	int2bin('A', 8, ptr);
	ptr += 8;
	int2bin('S', 8, ptr);
	ptr += 8;
	int2bin('F', 8, ptr);
	ptr += 8;
	nbits += 32;
	
	// header length
	ptr += 24;
	nbits += 24;

	// raw stream length
	ptr += 32;
	nbits += 32;

	//audio_codec_id
	// 0: general audio codec
	int2bin(0, 4/*2*/, ptr);
	ptr += 4/*2*/;
	nbits += 4/*2*/;

	//resolution
	//0: 8bits/samples, 1: 16bits/samples,  2: 24bits/samples,	
	if( bitsPerSample==8)
	{
		int2bin(0, 2, ptr);
		ptr += 2;
		nbits += 2;
	}else if( bitsPerSample==16)
	{
		int2bin(1, 2, ptr);
		ptr += 2;
		nbits += 2;
	}else if( bitsPerSample==24)
	{
		int2bin(2, 2, ptr);
		ptr += 2;
		nbits += 2;
	}

	// coding profile
	//0: basic profile
	int2bin(conf->coding_profile, 3/*2*/, ptr);
	ptr += 3/*2*/;
	nbits += 3/*2*/;

	//anc_data_index
	int2bin(conf->anc_data_index, 1, ptr);
	ptr += 1;
	nbits += 1;

	// channel number
	switch(numOfChannels) {
    case 1:  //mono
		index = 0;
		break;
	case 2:  //stereo
		index = 1;
		break;
	case 6:  //5.1
		index = 2;
		break;
	case 8:  //7.1
		index = 3;
		break;
	case 12: //10.2
		index = 4;
		break;
	case 24: //22.2
		index = 5;
		break;
	case 9: //2nd HOA
		index = 12;
		break;
	case 16: //3rd HOA
		index = 11;
		break;
	default:
		printf("error channel numbers %d", numOfChannels);
		exit(1);
		break;
	}
	int2bin(index, 7/*3*/, ptr);
	ptr += 7/*3*/;
	nbits += 7/*3*/;

	// sampling_frequency_index
	switch(conf->sampleRate) {
	case 192000:
		samplingRate = 0;
		break;
	case 96000:
		samplingRate = 1;
		break;
	case 48000:
		samplingRate = 2;
		break;
	case 44100:
		samplingRate = 3;
		break;
	case 32000:
		samplingRate = 4;
		break;
	case 24000:
		samplingRate = 5;
		break;
	case 22050:
		samplingRate = 6;
		break;
	case 16000:
		samplingRate = 7;
		break;
	case 8000:
		samplingRate = 8;
		break;
	default:
		printf("error sampling rate %d", conf->sampleRate);
		exit(1);
		break;
	}
	int2bin(samplingRate, 4, ptr);
	ptr += 4;
	nbits += 4;

	// bit_rate_index
	/*if(numOfChannels==1) j = 0;
	else if(numOfChannels==2) j = 1;
	else if((numOfChannels==6)||(numOfChannels==5)) j = 2;
	else if(numOfChannels==8) j = 3;
	for(i = 0; i < 15; i++)
		if(conf->bitRate/1000 <= brate_mapping[j][i]) break;
	i += j * 16; 
	
	int2bin(i, 6, ptr);	
	ptr += 6;
	nbits += 6;*/
	//for (i = 0; i < 15; i++)  //shumin.xu 20190313
	//	if (conf->bitRate / 1000 <= brate_mapping[index][i]) break;
	//int2bin(i, 4, ptr);
	//ptr += 4;
	//nbits += 4;
	{ //wu.chaogang 20210510
		int bitratedelta = (1 << 30);
		int bitrateindex = 0;
		for (i = 0; i <= 15; i++)
		{
			if (bitratedelta > abs(conf->bitRate / 1000 - brate_mapping[index][i]))
			{
				bitrateindex = i;
				bitratedelta = abs(conf->bitRate / 1000 - brate_mapping[index][i]);
			}
		}

		int2bin(bitrateindex, 4, ptr);
		ptr += 4;
		nbits += 4;
	}
	
	// bitstream_type
	int2bin(bitstream_type, 1, ptr);
	ptr += 1;
	nbits += 1;

	// bwe_check_stream
	int2bin(0, 1, ptr);
	ptr += 1;
	nbits += 1;

	// bwe_check_obj
	int2bin(0, 1, ptr);
	ptr += 1;
	nbits += 1;

	// added by lumin 2014.11.17
	if(numOfChannels>=2)
	{
		//supermode_flag
		int2bin(useSuperMode, 2, ptr);
		ptr += 2;
		nbits += 2;
		//couple_channel_config
		int2bin(cpe_config, 8, ptr);
		ptr += 8;
		nbits += 8;
	}

	//PCAGroupmodeHeader 
	if(numOfChannels>2){
	
		int2bin(PCAGroupmodeHeader, 8, ptr);
		ptr += 8;
		nbits += 8;
	}
	
	// byte_alignment
	if (nbits % 8 != 0)
	{
		int2bin(0, 8 - nbits % 8, ptr);
		ptr += 8 - nbits % 8;
		nbits += 8 - nbits % 8;
	}

	// header_length
	ptr = buffer + 32;
	int2bin(nbits / 8, 24, ptr);

	WriteBitstreamPlus((short)nbits, (short)0, buffer, /*f_output*/headbuffer);
	return nbits;
}

#define	SYNCWORD  2046 //4095, shumin.xu 20200511

int avs2AATF_error_check()
{
	return 0;
}

void write_avs2AATF_header(int samplingRate, 
						   int bitstream_type, 
						   ChanInfo * conf, 
						   unsigned char* headbuffer)
{
	short buffer[256];
	short *ptr = buffer;
	int nbits = 0;
	int i,j;
	int numOfChannels = conf->nChannels;
	int bitsPerSample = conf->bitsPerSample;
	int bitsTmp;
	int index;
	
	memset(buffer, 0, 256 * sizeof(short));

	/* write syncword */
	int2bin(SYNCWORD, 12, ptr);
	ptr += 12;
	nbits += 12;

	//shumin.xu 20190410
	/* write audio codec id */
	int2bin(conf->codec_id, 4, ptr);
	ptr += 4;
	nbits += 4;

	/*anc_data_index*/
	int2bin(conf->anc_data_index, 1, ptr);
	ptr += 1;
	nbits += 1;

	// coding profile
	//0: basic profile
	int2bin(conf->coding_profile, 3, ptr);
	ptr += 3;
	nbits += 3;

	/* write sampling frequency index */
	switch(conf->sampleRate) {
	case 192000:
		samplingRate = 0;
		break;
	case 96000:
		samplingRate = 1;
		break;
	case 48000:
		samplingRate = 2;
		break;
	case 44100:
		samplingRate = 3;
		break;
	case 32000:
		samplingRate = 4;
		break;
	case 24000:
		samplingRate = 5;
		break;
	case 22050:
		samplingRate = 6;
		break;
	case 16000:
		samplingRate = 7;
		break;
	case 8000:
		samplingRate = 8;
		break;
	default:
		printf("error sampling rate %d", conf->sampleRate);
		exit(1);
		break;
	}
	int2bin(samplingRate, 4, ptr);
	ptr += 4;
	nbits += 4;	

	/*raw_stream_length*/
	int2bin(0, 16, ptr);
	ptr += 16;
	nbits += 16;

	/*aatf_crc_check*/
	int2bin(0, 8, ptr);
	ptr += 8;
	nbits += 8;
	
	// channel number
#if 0 //shumin.xu 20210105
	switch(numOfChannels) {
    case 1:  //mono
		index = 0;
		break;
	case 2:  //stereo
		index = 1;
		break;
	case 6:  //5.1
		index = 2;
		break;
	case 8:  //7.1
		index = 3;
		break;
	case 12: //10.2
		index = 4;
		break;
	case 24: //22.2
		index = 5;
		break;
	case 4:  //4.0
		index = 6;
		break;
	default:
		printf("error channel numbers %d", numOfChannels);
		exit(1);
		break;
	}
#endif
	index = conf->channel_number_index;
	int2bin(index, 7, ptr);
	ptr += 7;
	nbits += 7;

	//resolution
	//0: 8bits/samples, 1: 16bits/samples,  2: 24bits/samples,	
	int2bin((bitsPerSample / 8 - 1), 2, ptr);
	ptr += 2;
	nbits += 2;

	//bit_rate_index
	/*if(numOfChannels==1) j = 0;
	else if(numOfChannels==2) j = 1;
	else if((numOfChannels==6)||(numOfChannels==5)) j = 2;
	else if(numOfChannels==8) j = 3;
	for(i = 0; i < 15; i++)
		if(conf->bitRate/1000 <= brate_mapping[j][i]) break;
	i += j * 16; 
	
	int2bin(i, 6, ptr);	
	ptr += 6;
	nbits += 6;*/
	//for (i = 0; i < 15; i++)  //shumin.xu 20190313
	//	if (conf->bitRate / 1000 == brate_mapping[index][i]) break;
	//int2bin(i, 4, ptr);
	//ptr += 4;
	//nbits += 4;
	{ //wu.chaogang 20210510
		int bitratedelta = (1 << 30);
		int bitrateindex = 0;
		for (i = 0; i <= 15; i++)
		{
			if (bitratedelta > abs(conf->bitRate / 1000 - brate_mapping[index][i]))
			{
				bitrateindex = i;
				bitratedelta = abs(conf->bitRate / 1000 - brate_mapping[index][i]);
			}
		}

		int2bin(bitrateindex, 4, ptr);
		ptr += 4;
		nbits += 4;
	}
	
	// bitstream_type
	int2bin(bitstream_type, 1, ptr);
	ptr += 1;
	nbits += 1;

	/*write bwe_check_stream*/
	int2bin(0, 1, ptr);
	ptr += 1;
	nbits += 1;

	/*write bwe_check_obj*/
	int2bin(0, 1, ptr);
	ptr += 1;
	nbits += 1;

	/*write bwe_superframe*/
	int2bin(0, 1, ptr);
	ptr += 1;
	nbits += 1;
	
	// byte_alignment
	if (nbits % 8 != 0)
	{
		int2bin(0, 8 - nbits % 8, ptr);
		ptr += 8 - nbits % 8;
		nbits += 8 - nbits % 8;
	}
	WriteBitstreamPlus((short)nbits, (short)0, buffer, /*f_output*/headbuffer);
}


void Bitstream_fclose(FILE *f_output, int headsize, int total_len)                  //chenhan 修改了函数WriteBitstreamPlus的参数
{
	short buffer[32];
	unsigned char lenbuf[4];

	fseek(f_output, 4, 0);
	//write aasf head size
	int2bin(headsize, 24, buffer);
	WriteBitstreamPlus(24, 0, buffer, lenbuf);
	fwrite(lenbuf, 3, 1, f_output);
	//write aasf data size
	int2bin(total_len, 32, buffer);	
	WriteBitstreamPlus(32, 0, buffer, /*f_output*/lenbuf);
	fwrite(lenbuf, 4, 1, f_output);

	fclose(f_output);
}

void ZeroLFEHighFreq(struct AVS2_ENCODER *lfEnc, float *Mdftout)
{
	int LL[5] = { 4096 / 8 / 2, 4096 / 4 / 2, 4096 / 2 / 2, 4096 / 2, 4096 / 16 / 2 };

	int mdftoffset = 0;
	for (int index = 1; index<(lfEnc->lf_winseq[0] + 1); index++)
	{
		int lowbandoffset;
		int ll = max(LL[lfEnc->lf_winseq[index] - 1], LL[lfEnc->lf_winseq[index + 1] - 1]) / 2;

		switch (ll)
		{
		case 4096 / 8 / 2 / 2:			lowbandoffset = 2; break;
		case 4096 / 8 / 2:				lowbandoffset = 4; break;
		case 4096 / 4 / 2:				lowbandoffset = 8; break;
		case 4096 / 2 / 2:				lowbandoffset = 8; break;
		case 4096 / 16 / 2 / 2:			lowbandoffset = 2; break;
		default:		lowbandoffset = 8; break;
		}

		for (int kk = lowbandoffset; kk < ll; kk++)
		{
			Mdftout[mdftoffset + kk * 2] = 0;
			Mdftout[mdftoffset + kk * 2 + 1] = 0;
		}
		mdftoffset += (ll * 2);
	}
}

int calc_mcr_bits(int k, avs2audiopack_buffer *opb, int cid, avs2audiopack_buffer *data_mcr)
{
	int count = 0;

	if (data_mcr)
	{
		/*if (corr_4H != -1)*/
		if (data_mcr->endbit > 0)
			count += 8;

		for (int j = 0; j < data_mcr->endbyte; j++)
			count += 8;
	}

	return count;
}

//FILE *fp_lars;
int general_encoder(int argc, char *argv[])
{
	int 		i,k;
	int			pmode = 0;
	int			cpe_config = 0;
	int			chIndex[MAX_ALLCHANNEL];
	float		time_dmix[MAX_ALLCHANNEL][(TOTAL_DELAY*2+DOWN_DELAY)*2]; //下混声道
	int writeOffset  = MCR_DELAY/2;
	int frameOffset = AVS2ENC_BLOCKSIZE/2 + AVS2ENC_BLOCKSIZE/16;

	avs2audiopack_buffer *data_mcr[MAX_ALLCHANNEL/2];

	//	ChanInfo coreInfo;
	long frame = 0, samplingRate, dataSize;
	int frameLength = 0, cbrLength = 0;

	int error, inSamples;
	int bDoIIR2Downsample = 0;
	//int sfbnum;
	int numOutBytes = 0, numBytes;
	int nChannelsCore = 1;
	int sampleRateCore;
	int bEncodeMono = 0;
	int useBWE = 0;
	int usePS = 1;
	int useSuperMode = 0;
	PS_DATA	 *psData[MAX_ALLCHANNEL/2], *psData_pre[MAX_ALLCHANNEL/2];
	PS_BWE_DATA	 *psBweData[MAX_ALLCHANNEL/2], *psBweData_pre[MAX_ALLCHANNEL/2];

	unsigned int DataLength_mcr[MAX_ALLCHANNEL] = {0};
	unsigned int DataLength_tsinghua[MAX_ALLCHANNEL] = {0};
	unsigned int ancDataLength = 0;
	unsigned char sampleData[(6144/8)*6];

	////////////////////////////////////////////////
	
	char *input_filename;
	char *output_filename;
	//char *config_filename;
	ChanInfo inputInfo;
	
	
	FILE *fInputFile;               /* File of sound data                   */
	FILE *fOutputFile;              /* File of serial bits for transmission  */
	short numOfChannels, bitsPerSample;

	int useParametricStereo = 0;
	int bDoUpsample = 0;
	int bDingleRate = 0;

    int coreReadOffset = 0;
    int envWriteOffset = 0;
    int envReadOffset = 0;
    int upsampleReadOffset = 0;

    int TimeDataPcm[AVS2ENC_BLOCKSIZE*2*MAX_ALLCHANNEL];
	char  TimeDataPcmBuffer[AVS2ENC_BLOCKSIZE*2*MAX_ALLCHANNEL*4];

	float resamplerScratch[AVS2ENC_BLOCKSIZE*2];

    int nSamplesPerChannel;
	int numSamplesRead;
	int aasfHeadSize;
	
	unsigned char ancDataBytes[MAX_ALLCHANNEL][MAX_PAYLOAD_SIZE];

	unsigned int DataLength = 0;
	unsigned long PCMDataLength = 0;
	unsigned int numAncDataBytes[MAX_ALLCHANNEL+1] = {0};

	struct AVS2_ENCODER *lfEncset[MAX_ALLCHANNEL];

	ENC_CONFIG config;

	int cid = 0;
	int elementnum=1;
	int elementnum_tianlai51=1;
	double basesetting, channelbasesetting;

	int eid;
	int bitRateIndex=0;
	int coreBitrate;
	double lowpass_kHz;
	float mdftSpectrum[MAX_ALLCHANNEL][FRAME_LEN_LONG * 2] = {0};
	float Mdftout[4096*2];

	ST_RATE_CONFIG srateInfo;
	MCR_INFO mcr_info;

	char codectype=0; // Tsinghua or Tianlai

	int PCAcorebitpershort=150;

	unsigned int PCAGroupmode =0;
	int PCAGroupmodeHeader=0xFF;

	int config_idx=0;
	int fill_element_num =  1;

	unsigned char headbuffer[32];
	unsigned char objextbuffer[32];
	unsigned char loudnessbuffer[32];
	int readframecount = 0;
	input_filename = NULL;
	output_filename = NULL;
	//config_filename = NULL;

	for(i=0; i<MAX_ALLCHANNEL; i++)
	{
		memset(inputBuffer[i],0,sizeof(float)*(AVS2ENC_BLOCKSIZE*2 + MAX_DS_FILTER_DELAY + INPUT_DELAY));
		memset(downmixBuffer[i/2],0,sizeof(float)*(AVS2ENC_BLOCKSIZE*2 + MAX_DS_FILTER_DELAY + INPUT_DELAY));
	}
	
	copyright();
	parsecmdline(argc, argv, &input_filename, &output_filename, /*&config_filename,*/ &inputInfo);
	
	/* Open input wave file */
	if ((fInputFile = Wave_fopen(input_filename, "rb", &numOfChannels, &samplingRate,
		&bitsPerSample, &dataSize)) == NULL)
	{
		fprintf(stderr, "Error opening the input file %s.\n", input_filename);
		exit(EXIT_FAILURE);
	}

	if ((fOutputFile = fopen(output_filename, "wb")) == NULL)
	{
		fprintf(stderr, "Error opening the output file %s.\n", output_filename);
		exit(EXIT_FAILURE);		
	}

	dataSize *= numOfChannels;

	inputInfo.nChannels = numOfChannels;
	inputInfo.sampleRate = samplingRate;
	inputInfo.bitsPerSample = bitsPerSample;

	//shumin.xu 20210105
	switch (inputInfo.nChannels) {
	case 1:  //mono
		inputInfo.channel_number_index = 0;
		break;
	case 2:  //stereo
		inputInfo.channel_number_index = 1;
		break;
	case 6:  //5.1
		inputInfo.channel_number_index = 2;
		break;
	case 8:  //7.1
		inputInfo.channel_number_index = 3;
		break;
	case 10:
		break;
	case 12:  //10.2
		inputInfo.channel_number_index = 4;
		break;
	case 24:  //22.2
		inputInfo.channel_number_index = 5;
		break;
	case 4:
		inputInfo.channel_number_index = 6;
		break;
	case 16: //16ch 3rd HOA, shumin.xu 20210510
		inputInfo.channel_number_index = 11;
		break;
	case 9:
		inputInfo.channel_number_index = 12;
		break;
	default:
		fprintf(stderr, "(%s):  (%d) encoding has not been supported yet!", __FUNCTION__, inputInfo.nChannels);
		return -1;
	}

	//head
	if (inputInfo.headflag == 2)  //chenhan 20180328
	{
		//if (inputInfo.nChannels == 0)
		//	inputInfo.channel_number_index = 12;
		if (inputInfo.nChannels == 8)
			inputInfo.channel_number_index = 7;
		if (inputInfo.nChannels == 10)
			inputInfo.channel_number_index = 9;
	}
	else if (inputInfo.headflag == 4)
	{
		//if (inputInfo.nChannels == 0)
		//	inputInfo.channel_number_index = 13;
		if (inputInfo.nChannels == 10)
			inputInfo.channel_number_index = 8;
		if (inputInfo.nChannels == 12)
			inputInfo.channel_number_index = 10;
	}

	if(inputInfo.nChannels >= 5)
	{
		for(i =0; i< 5; i++)
			if(inputInfo.bitRate >= rate_mapping_44_multi[i])
				bitRateIndex = i;
	}

	inputchannelnum = numOfChannels;

    /* set up basic parameters for avs codec */
	InitDefaultConfig(&config);

	nChannelsCore  = 1;

	if( ((inputInfo.nChannels == 1) && (inputInfo.bitRate <= 48000/*32000*/)) 
	|| ((inputInfo.nChannels >= 2) && (inputInfo.bitRate <= 64000/*48000*/)) 
	|| ((inputInfo.nChannels >= 5) && (inputInfo.bitRate <= 144000/*128000*/))
	|| ((inputInfo.nChannels >= 7) && (inputInfo.bitRate <= 192000))
	|| ((inputInfo.nChannels >= 9) && (inputInfo.bitRate <= 216000/*256000*/))
	|| ((inputInfo.nChannels >= 11) && (inputInfo.bitRate <= 240000))
	|| ((inputInfo.nChannels >= 13) && (inputInfo.bitRate <= 320000))) //16ch 3rd HOA, shumin.xu 20210510
	{
		useBWE = 1;
	}
	if((PCAGroupmodeHeader!=0xFF)&&(inputInfo.nChannels >2)&&((inputInfo.bitRate <= 192000)))
		useBWE = 1;
	printf("inputInfo.bitRate = %d\n",inputInfo.bitRate);
	//decide ps and core bitrates
	if(numOfChannels == 1)
	{
		usePS = 0;
	}
	else if(numOfChannels >= 2)
	{
		//this version do not support BWE now
		if(bitrate_init(&srateInfo, inputInfo.bitRate/1000, inputInfo.channel_number_index))
		{
			usePS = 1;
			mcr_init(&mcr_info, srateInfo.mcr_brate[0], useBWE);
		}
		else
		{
			fprintf(stderr, "Error support the bitrate.\n");
			exit(EXIT_FAILURE);	
		}

	}

	// added by lumin 2014.12.18
	if(numOfChannels == 1) //mono
		cpe_config = 0;
	else if(numOfChannels >= 2)  //stereo
		cpe_config = srateInfo.couple_config;

	//if((usePS==1)&&(srateInfo.mcr_brate[0]<12))
	//{
		//useSuperMode = 1;
	//}

	if (inputInfo.sampleRate == 16000) 
	{
		bDoUpsample = 1;
		inputInfo.sampleRate = 32000;
		bDingleRate = 1;
	}

	sampleRateCore = inputInfo.sampleRate;

	/* set IIR 2:1 downsampling */
	if(useBWE)
		bDoIIR2Downsample = (bDoUpsample) ? 0 : 1;

	/* set up 1:2 upsampling */
    if (bDoUpsample) 
	{
		if (inputInfo.nChannels>1) 
		{
		  fprintf( stderr, "\n Stereo @ 16kHz input sample rate is not supported\n");
		  return -1;
		}
		for(cid = 0; cid < MAX_ALLCHANNEL; cid++)
		{
			InitIIR21_Resampler(&(IIR21_reSampler[cid]));
			InitIIR21_Resampler(&(IIR21_bweSampler[cid/2]));
		}

		if (useParametricStereo)
		{
			writeOffset += AVS2ENC_BLOCKSIZE;

			upsampleReadOffset = writeOffset;
			envWriteOffset = envReadOffset;
		}
		else
		{
			writeOffset += AVS2ENC_BLOCKSIZE;

			coreReadOffset = writeOffset;

			upsampleReadOffset = writeOffset - (((INPUT_DELAY - IIR21_reSampler[0].delay) >> 1));

			envWriteOffset = ((INPUT_DELAY - IIR21_reSampler[0].delay) & 0x1);

			envReadOffset = 0;
		}
	}
	else
	{
		/* set up 2:1 downsampling */
		if (bDoIIR2Downsample)
		{
			for(cid = 0; cid < MAX_ALLCHANNEL; cid++)
			{
				InitIIR21_Resampler(&(IIR21_reSampler[cid]));
				InitIIR21_Resampler(&(IIR21_bweSampler[cid/2]));
				writeOffset += IIR21_reSampler[cid].delay;
			}
	    }
	}
	
	/* set up encoder */
	if (sampleRateCore < 32000)
	{
		fprintf(stderr, "\n sampleRateCore is less than 32000bps\n");
		return -1;
	}

	sampleRateCore = sampleRateCore/2;
	config.sampleRate = sampleRateCore;

	//2014.04.22 bitstream_type<----- bitsPerSample
	/*
	bitsPerSample	2bit，音频采样大小的标志
	-- '00', 8bit采样
	-- '01', 166bit采样
	-- '10', 24bit采样
	-- '11', 32bit采样

	*/
	if (inputInfo.outputFormat == 1)
	{
		int nbits;
		nbits = write_avs2file_header(sampleRateCore, 0, &inputInfo, useSuperMode, cpe_config, PCAGroupmodeHeader,/* fOutputFile*/headbuffer);
		aasfHeadSize = nbits / 8;
		fwrite(headbuffer, 1, aasfHeadSize, fOutputFile);
	}
	//maximum pcm value
	//bitsPerSample = 24;// 内部处理固定为24比特精度
	Maxpcmvalue = 1<<(bitsPerSample-1);

	//channel information
	if(inputInfo.nChannels==1)
	{
		encoder_mode = &CoupleChannelTable[0];
	}
	// added by lumin 2014.11.21
	else if(inputInfo.nChannels>=2)
	{
		encoder_mode = &CoupleChannelTable[srateInfo.couple_config];
	}
	memcpy(chIndex, encoder_mode->ele_id, MAX_ALLCHANNEL * sizeof(int));
	elementnum =  encoder_mode->numofele;

	cid = 0;
	for(i = 0; i < elementnum; i++)
	{
		  elementencode_info[i].elType = encoder_mode->idType[i];

		  switch(elementencode_info[i].elType){

			case ID_SCE:      /* single channel */
				elementencode_info[i].nChannelsInEl = 1;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid++;				
			break;

			case ID_CPE_F:      /* channel pair */
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 2;				
			break;
			
			case ID_CPE_L:      /* channel pair */
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 2;				
			break;

			//shumin.xu 20210105
			case ID_CPE_H:      /* channel pair */
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 2;
			break;

			case ID_LFE:     /*LFE channel */
				elementencode_info[i].nChannelsInEl = 1;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid++;
			break;
			
			case ID_PCA2:     /*PCA2 channel */
				elementencode_info[i].nChannelsInEl = 2;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 2;
			break;

			case ID_PCA4:     /*PCA4 channel */
				elementencode_info[i].nChannelsInEl = 4;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 4;
			break;

			case ID_PCA6:     /*PCA6 channel */
				elementencode_info[i].nChannelsInEl = 6;
				elementencode_info[i].ChannelIndex[0] = cid;
				cid += 6;
			break;
		  }

	}

	for(i = elementnum; i < elementnum + fill_element_num; i++)                  //chenhan, to complete the fill element
	{
		elementencode_info[i].elType = ID_FIL;
	}

	for(i = 0; i < elementnum; i++)
	{
		psData[i] = (PS_DATA *)malloc(sizeof(PS_DATA));
		memset(psData[i], 0, sizeof(PS_DATA));
		
		psData_pre[i] = (PS_DATA *)malloc(sizeof(PS_DATA));
		memset(psData_pre[i], 0, sizeof(PS_DATA));

		psBweData[i] = (PS_BWE_DATA *)malloc(sizeof(PS_BWE_DATA));
		memset(psBweData[i], 0, sizeof(PS_BWE_DATA));
		psBweData[i]->bandwidth = AVS2ENC_BLOCKSIZE;
		
		psBweData_pre[i] = (PS_BWE_DATA *)malloc(sizeof(PS_BWE_DATA));
		memset(psBweData_pre[i], 0, sizeof(PS_BWE_DATA));
		psBweData_pre[i]->bandwidth = AVS2ENC_BLOCKSIZE;
	}

   // tianlai channel inf
	//encoder_mode_tianlai51 =&MC51ModeTianlai[numOfChannels-1];
	if(numOfChannels<=2)
		encoder_mode_tianlai51 =&PCAGroupmodeHeaderTable[numOfChannels-1];
	else if(numOfChannels==6)
		encoder_mode_tianlai51 =&PCAGroupmodeHeaderTable[2];
	else if(numOfChannels==8)
		encoder_mode_tianlai51 =&PCAGroupmodeHeaderTable[16];
	//shumin.xu 20210105
	else if (numOfChannels==4)
		encoder_mode_tianlai51 = NULL;  
	else if(numOfChannels<=16)
		encoder_mode_tianlai51 = NULL;
	else
	{
		printf("not support %d channel",numOfChannels);
		return -1;
	}

	if (encoder_mode_tianlai51 != NULL)  //shumin.xu 20210105
	{
		cid = 0;

		elementnum_tianlai51 = encoder_mode_tianlai51->numofele;

		for (i = 0; i < elementnum_tianlai51; i++)
		{
			elementencode_info_tianlai51[i].elType = encoder_mode_tianlai51->idType[i];

			switch (elementencode_info_tianlai51[i].elType) {

			case ID_SCE:      /* single channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 1;
				cid++;
				break;

			case ID_CPE_F:      /* channel pair */
				elementencode_info_tianlai51[i].nChannelsInEl = 2;
				cid++;
				cid++;
				break;

			case ID_LFE:     /*LFE channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 1;
				cid++;
				break;

			case ID_PCA2:     /*PCA2 channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 2;
				cid++;
				cid++;
				break;

			case ID_PCA4:     /*PCA2 channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 4;
				cid += 4;
				break;

			case ID_PCA6:     /*PCA2 channel */
				elementencode_info_tianlai51[i].nChannelsInEl = 6;
				cid += 6;
				break;
			}

		}
		for (i = elementnum_tianlai51; i < elementnum_tianlai51 + fill_element_num; i++)
		{
			elementencode_info_tianlai51[i].elType = ID_FIL;
		}
	}
	//0~13各级ci对应的码表的设置
	//ci_settable_init(ci_table, config);
	ci_settable_init0(ci_table, config, bDoIIR2Downsample);

	//initialize every encoder (lfEncset[])
	//MCR
	k = 0;
	for(eid = 0; eid < elementnum; eid++)
	{
		int iid;
		for(iid = 0; iid < elementencode_info[eid].nChannelsInEl; iid++)
		{
			if((numOfChannels==1)||(numOfChannels==2&&bEncodeMono))
				coreBitrate = inputInfo.bitRate;
			else
			{
				coreBitrate = srateInfo.core_brate[k]*1000;
			}
			if(coreBitrate == 0) 
			{
				if (useBWE)
					config.bitRate = srateInfo.core_brate[k-1]*1000;
				else
					config.bitRate = 48000;
			}
			else
				config.bitRate = coreBitrate;

			config.nChannelsIn = 1;
			config.nChannelsOut = 1;

			error = LFEncOpen(&lfEncset[k], &config, useBWE);

			tianlai_info_init(&(lfEncset[k]->vi));

			lfEncset[k]->vi.channels = config.nChannelsIn;
			lfEncset[k]->vi.rate = config.sampleRate;

			lfEncset[k]->elInfo.elType = elementencode_info[eid].elType;
			lfEncset[k]->elInfo.nChannelsInEl = elementencode_info[eid].nChannelsInEl;

			gettableindex(1, config.sampleRate*2, -1, config.bitRate, -1, &basesetting);

			ci_set(&(lfEncset[k]->vi), basesetting);
			tianlai_encode_setup_setting(&(lfEncset[k]->vi), lfEncset[k]->vi.channels, config.sampleRate*2);

			lowpass_kHz = freqbeginend_setting(&(lfEncset[k]->vi), elementencode_info[eid].nChannelsInEl, bitRateIndex, 0);
			reset_bandWidth(config.sampleRate, /*basesetting*/config_idx, lowpass_kHz, &(lfEncset[k]->config.bandWidth),useBWE);

			tianlai_analysis_init(&(lfEncset[k]->vd), &(lfEncset[k]->vi), useBWE);

			_vds_flr_res_set(&(lfEncset[k]->vd), &(lfEncset[k]->vi), min(12,basesetting+3));

			tianlai_block_init(&(lfEncset[k]->vd), &(lfEncset[k]->vb));

			lfEncset[k]->vb.lW = 1;
			lfEncset[k]->vb.W = 1 ;
			lfEncset[k]->vb.nW = 1 ;

			Avs2EncMDFTfunOpen();
			Avs2BweMDFTOpen((unsigned int*)&(lfEncset[k]->st1_in), (unsigned int*)&(lfEncset[k]->st_common));

			if(useBWE)
			{
				Avs2BweEncoderOpen((unsigned int*)&(lfEncset[k]->st2_in), config.bitRate, sampleRateCore, nChannelsCore, &(config.bandWidth),&config_idx);
			}			
	
			lfEncset[k]->elInfo.elType  = elementencode_info[eid].elType;

			///wu add 2015.2.6
			lowpass_kHz = freqbeginend_setting(&(lfEncset[k]->vi), elementencode_info[eid].nChannelsInEl, bitRateIndex, 0);
			reset_bandWidth(config.sampleRate, /*basesetting*/config_idx, lowpass_kHz, &(lfEncset[k]->config.bandWidth),useBWE);

			if(config.sampleRate > 32000)
			init_tns_configuration(config.bitRate, config.sampleRate, config.nChannelsIn, lfEncset[k]->config.bandWidth, &(lfEncset[k]->vb.tnsConf[0]), 1);

			if(coreBitrate > 0)
				reset_bitrate_copy0(&(lfEncset[k]->vd), coreBitrate);

			k++;
		}
	}

	//getbitpershort
	PCAcorebitpershort = getbitpershort(&(lfEncset[0]->vd));
	//重新配置目标码率


	lf_winseqpre[19] = 0;
	/* set up input samples block size feed */
	if(useBWE)
		inSamples = AVS2ENC_BLOCKSIZE * inputInfo.nChannels * 2;
	else
		inSamples = AVS2ENC_BLOCKSIZE * inputInfo.nChannels;
	inputInfo.inSamples = inSamples;

	cbrLength = (float)(inputInfo.bitRate / 8) / inputInfo.sampleRate * 1024 * (useBWE + 1) + 1;

	if (bDoUpsample) 
	{
		inSamples = inSamples>>1;
	}
	memset(time_dmix, 0, MAX_ALLCHANNEL*(TOTAL_DELAY*2+DOWN_DELAY)*sizeof(float)*2);

	//添加声道对中核心编码器的配置和初始化
	//InitDefaultConfig(&config);
	config.nChannelsIn = nChannelsCore;
	config.nChannelsOut = nChannelsCore;
	config.sampleRate = sampleRateCore;

	memset(sampleData, 0, (6144/8));
	////////////////////////////////////////////////////
	/* the frame loop */
	fprintf(stderr, "\n --- Running ---\n");

	//int totalSamples = 0;
	while(1)
	{	
		int n;
	    int i,  outSamples, numOutBytes = 0;
		int numMcrBytes = 0;
		
	
	//	fprintf(stderr, " Frames processed: %ld    \r", frame);
	
		//printf( " *********************Frames processed: %ld    ******************\n", frame);	
//		fprintf(fp_lars, " *********************Frames processed: %ld    ******************\n", frame);	
		/* File input read, resample and downmix */
		//////////////////////   File input read, resample and downmix start//////////////////////////
		{
			int pcmindex;

			/* no resampling prior to encoding required */
			/* read from file */
			numSamplesRead = fread(TimeDataPcmBuffer, inputInfo.bitsPerSample/8, inSamples, fInputFile);

			for(pcmindex=0; pcmindex<inSamples; pcmindex++)
			{
				int pcmtmp;
				short pcmshorttmp;
				char *ptchar = &pcmtmp;

				if(bitsPerSample==24)
				{
					memcpy(ptchar,TimeDataPcmBuffer +pcmindex*(bitsPerSample/8),3);
					pcmtmp =(pcmtmp<<8);
					pcmtmp =(pcmtmp/256);
				}else if(bitsPerSample==16)
				{
					ptchar = &pcmshorttmp;
					memcpy(ptchar,TimeDataPcmBuffer +pcmindex*(bitsPerSample/8),2);
					pcmtmp = pcmshorttmp;
				}else if(bitsPerSample==32)
				{
					memcpy(ptchar,TimeDataPcmBuffer +pcmindex*(bitsPerSample/8),4);
					pcmtmp =(pcmtmp);
				}
				TimeDataPcm[pcmindex] =pcmtmp;
			}

			switch (inputInfo.nChannels) 
			{
				case 1:
				  nSamplesPerChannel = numSamplesRead;
				  break;
				case 2:
				  nSamplesPerChannel = numSamplesRead >> 1;
				  break;
				default:
				  nSamplesPerChannel = numSamplesRead / inputInfo.nChannels;
		    }

			if(numSamplesRead < inSamples)
			{
				break;
			}
	
			for(k = 0; k < inputInfo.nChannels; k++)
			{
				int i;
				for(i = 0; i < numSamplesRead/inputInfo.nChannels; i++)
					inputBuffer[k][writeOffset+i] = (float) TimeDataPcm[i*inputInfo.nChannels+k];

				/* copy from short to float input buffer, downmix stereo input signal to mono, reordering necessary since the encoder takes interleaved data */
				if((inputInfo.nChannels==2) && bEncodeMono)
				{
					int i;
					for(i = 0; i < numSamplesRead/2; i++)	
					   inputBuffer[k][writeOffset+i] = ((float)TimeDataPcm[2*i] + (float)TimeDataPcm[2*i+1])*0.5f;   
				}
			}

			PCMDataLength += numSamplesRead;
			readframecount++;
			if (PCMDataLength > dataSize)
				break;

		}	//end (bDoIIR32Resample) end
		//////////////////////   File input read, resample and downmix end//////////////////////////

		/////////////////////////////////////////////////////////////////////////////////////////////////
        if(inputInfo.outputFormat == 2)
		{
			write_avs2AATF_header(sampleRateCore, 0, &inputInfo,/* fOutputFile*/headbuffer);
			//fwrite(headbuffer, 9, 1, fOutputFile);
			//DataLength += 9;
		}
		//////////////////////    mode select, mdct and BWE_encoding (start)//////////////////////////
		cid = 0;
		k = 0;
		for(eid = 0; eid < elementnum; eid++)
		{
			switch(elementencode_info[eid].elType) 
			{
			case ID_SCE:      /* single channel */			
			{
				int ch;
				struct AVS2_ENCODER *lfEnc = lfEncset[cid];

				numAncDataBytes[cid] = 0;
				/* 2:1 downsampling for core */
				if (bDoIIR2Downsample) 
				{
					//select the Low frequency's MDCT/MDFT mode lf_winseq
					Avs2LFmodeselect(lfEnc->st1_in, inputBuffer[cid]+0+writeOffset, lfEnc->config.bitRate,&(lfEnc->lf_winseq_ptr),nSamplesPerChannel,Maxpcmvalue);
					memcpy(lfEnc->lf_winseq,lfEnc->lf_winseq_ptr,20*4);
				
					if(useBWE)
					{
							//MDFT,high frequency 
							Avs2BweMDFTTransform(lfEnc->st1_in, lfEnc->st_common,  0/*lfEnc->config.nChannelsIn*/, lfEnc->config.bitRate,&(lfEnc->blocktypeB));						
					}
					//update the mdft status
					Avs2LFMDFTupdate(lfEnc->st1_in);
					
					IIR21_Downsample(&(IIR21_reSampler[cid]), inputBuffer[cid]+writeOffset, nSamplesPerChannel, 1, inputBuffer[cid], &outSamples, 1);

				}
				else
				{
					//select MDCT/MDFT mode lf_winseq
					Avs2modeselect0(lfEnc->st1_in, inputBuffer[cid]+coreReadOffset, &(lfEnc->lf_winseq_ptr), nSamplesPerChannel,Maxpcmvalue);
					memcpy(lfEnc->lf_winseq,lfEnc->lf_winseq_ptr,20*4);
					
					//update the mdft status
					Avs2MDFTupdate(lfEnc->st1_in);
				}

				/* encode one hf BWE frame */
				if(useBWE)
				{
						Avs2BweEncoder(ancDataBytes[cid], &numAncDataBytes[cid], lfEnc->st2_in, lfEnc->st_common, lfEnc->config.bitRate,bitsPerSample);
				}
					
				for(i = 0; i < CORE_FRAMESIZE; i++)
				    lfEnc->pcm_buffer0[CORE_FRAMESIZE+i] = *(inputBuffer[cid]+coreReadOffset+i)/Maxpcmvalue;

			    mdft_lowpassframeblock_multi(lfEnc->pcm_buffer0-frameOffset, lfEnc->lf_winseq, Mdftout, CORE_FRAMESIZE/2);

			    for(i = 0; i < CORE_FRAMESIZE; i++)
					mdftSpectrum[cid][i] = Mdftout[i*2] * Maxpcmvalue;
			
			}			
			cid++;				
			break;
		
				
			case ID_PCA2:      /* PCAx channel */			
			case ID_PCA4:
			case ID_PCA6:
			case ID_CPE_F:
            case ID_CPE_L:
			case ID_CPE_H:
			{
				int iid;
				int sid = cid;
				for(iid = 0; iid < elementencode_info[eid].nChannelsInEl; iid++)
				{
					int ch;
					struct AVS2_ENCODER *lfEnc = lfEncset[cid];
					numAncDataBytes[cid + iid] = 0;
					/* 2:1 downsampling for core */
					if(bDoIIR2Downsample) 
					{
						//select the Low frequency's MDCT/MDFT mode lf_winseq
						Avs2LFmodeselect(lfEnc->st1_in, inputBuffer[cid]+writeOffset, lfEnc->config.bitRate,&(lfEnc->lf_winseq_ptr),nSamplesPerChannel,Maxpcmvalue);
						
						memcpy(lfEnc->lf_winseq,lfEnc->lf_winseq_ptr,20*4);

						if(iid != 0)
						{
							StAvs2BweCommon *pstBweCommon = (StAvs2BweCommon *)lfEnc->st_common; 
							StAvs2BweCommon *pstBweCommon0 = (StAvs2BweCommon *)lfEncset[sid]->st_common; 

							memcpy(lfEnc->lf_winseq, lfEncset[sid]->lf_winseq, 20*4);

							pstBweCommon->nSeqmode = pstBweCommon0->nSeqmode;
							pstBweCommon->nGroupmode = pstBweCommon0->nGroupmode;
						}
				
						if(useBWE)
						{							
							//MDFT,high frequency 
							Avs2BweMDFTTransform(lfEnc->st1_in, lfEnc->st_common,  iid, lfEnc->config.bitRate,&(lfEnc->blocktypeB));

						}

						if (cid == 3)
						{
							if (inputInfo.channel_number_index == 6 || inputInfo.channel_number_index == 11 || inputInfo.channel_number_index == 12);//do nothing
							else  ZeroLFEHighFreq_BWE(lfEnc->st_common);
						}
						//update the mdft status
						Avs2LFMDFTupdate(lfEnc->st1_in);
						
						IIR21_Downsample(&(IIR21_reSampler[cid]), inputBuffer[cid]+writeOffset, nSamplesPerChannel, 1, inputBuffer[cid], &outSamples, 1);
						
					}
					else
					{
						//select  MDCT/MDFT mode lf_winseq
						Avs2modeselect0(lfEnc->st1_in, inputBuffer[cid]+coreReadOffset, &(lfEnc->lf_winseq_ptr), nSamplesPerChannel,Maxpcmvalue);

						memcpy(lfEnc->lf_winseq, lfEnc->lf_winseq_ptr, 20*4);



						if(iid != 0)
							memcpy(lfEnc->lf_winseq, lfEncset[sid]->lf_winseq, 20*4);
					
						//update the mdft status
						Avs2MDFTupdate(lfEnc->st1_in);
					}
					
					// encode one hf BWE frame
					if(useBWE)
					{
						//Avs2BweEncoder(ancDataBytes[cid], /*hBweBitBuf[cid],*/ &numAncDataBytes[cid], lfEnc->st2_in, lfEnc->st_common, lfEnc->config.bitRate,bitsPerSample);
					}
				
					for(i = 0; i < CORE_FRAMESIZE; i++)
						lfEnc->pcm_buffer0[CORE_FRAMESIZE+i] = *(inputBuffer[cid]+coreReadOffset+i)/Maxpcmvalue;

					mdft_lowpassframeblock_multi(lfEnc->pcm_buffer0-frameOffset, lfEnc->lf_winseq, Mdftout, CORE_FRAMESIZE/2);

					if (cid == 3)
					{
						if (inputInfo.channel_number_index == 6 || inputInfo.channel_number_index == 11 || inputInfo.channel_number_index == 12);//do nothing
						else  ZeroLFEHighFreq_BWE(lfEnc->st_common);
					}

					for(i = 0; i < CORE_FRAMESIZE; i++)
						mdftSpectrum[cid][i] = Mdftout[i*2] * Maxpcmvalue;
	
					cid++;	
				}//for(iid=0;iid<elementencode_info[eid].nChannelsInEl;iid++)

				if(useBWE)
				{
					StAvs2BweCommon *pstBweCommon; 

					pstBweCommon = (StAvs2BweCommon *)(lfEncset[sid]->st_common);
					psBweData[k]->Seqmode = pstBweCommon->nSeqmode;
					psBweData[k]->Groupmode = pstBweCommon->nGroupmode;
					memcpy(psBweData[k]->left_data, pstBweCommon->mdft4096block_complex[0], (CORE_FRAMESIZE*4+2048)*sizeof(float));

					pstBweCommon = (StAvs2BweCommon *)(lfEncset[sid+1]->st_common); 
					memcpy(psBweData[k]->right_data, pstBweCommon->mdft4096block_complex[0], (CORE_FRAMESIZE*4+2048)*sizeof(float));

					data_mcr[eid] = (avs2audiopack_buffer *)calloc(1, sizeof(avs2audiopack_buffer));
					numMcrBytes = MCR_BWE_Encoder(inputInfo.bitRate, downmixBuffer[sid] + writeOffset, data_mcr[eid], psBweData[k], mcr_info, psBweData_pre[k]);
				   
					pstBweCommon = (StAvs2BweCommon *)(lfEncset[sid]->st_common); 
				     memcpy(pstBweCommon->mdft4096block_complex[0], psBweData[k]->sum_data, (CORE_FRAMESIZE*4+2048)*sizeof(float));
				 //    memcpy(pstBweCommon->mdft4096block_2048complex[0], psBweData[k]->sum_data, (CORE_FRAMESIZE*4+2048) * sizeof(float));
				     pstBweCommon = (StAvs2BweCommon *)(lfEncset[sid+1]->st_common); 
				     memcpy(pstBweCommon->mdft4096block_complex[0], psBweData[k]->dif_data, (CORE_FRAMESIZE*4+2048)*sizeof(float));
				  //   memcpy(pstBweCommon->mdft4096block_2048complex[0], psBweData[k]->dif_data, (CORE_FRAMESIZE*4+2048) * sizeof(float));

//					{ //added in 2015.09.10 (to ignore the influence of the core mono coding)
//						int t;		
//						FILE *fp0 = fopen("MdftSpectrum_1.txt","a");
//						for(t=0; t<CORE_FRAMESIZE*4; t++)
//							fprintf(fp0, "%f,", psBweData[k]->sum_data[t]);	
//						fprintf(fp0,"\n");
//						fclose(fp0);
//					}
				
					IIR21_Downsample(&(IIR21_bweSampler[k]), downmixBuffer[sid]+writeOffset, nSamplesPerChannel, 1, downmixBuffer[sid], &outSamples, 1);

					Avs2BweEncoder(ancDataBytes[sid],  &numAncDataBytes[sid], lfEncset[sid]->st2_in, lfEncset[sid]->st_common, lfEncset[sid]->config.bitRate,bitsPerSample);

// 					fprintf(fp_lars,"numAncDataBytes[%d]=%d\n",sid,numAncDataBytes[sid]);
					 
//					if(numAncDataBytes[sid])
//					{
//						int j;
//						for(j = 0; j < numAncDataBytes[sid]; j++)
//							 fprintf(fp_lars,"numAncDataBytes[%d][%d]=%d\n",sid,j,ancDataBytes[sid][j]);
//					}
					

					for(i = 0; i < CORE_FRAMESIZE; i++)
						lfEncset[sid]->pcm_buffer0[CORE_FRAMESIZE-frameOffset+i] = *(downmixBuffer[sid]+coreReadOffset+i)/Maxpcmvalue;

					mdft_lowpassframeblock_multi(lfEncset[sid]->pcm_buffer0-frameOffset, lfEncset[sid]->lf_winseq, Mdftout, CORE_FRAMESIZE/2);
					
					for(i = 0; i < CORE_FRAMESIZE; i++)
						mdftSpectrum[sid][i] = Mdftout[i*2] * Maxpcmvalue;

				}
			}
			k++;
			
			break;
			}//switch (elementencode_info[eid].elType) 		
		}//for( eid=0;eid<elementnum;eid++)

		//////////////////////    mode select and mdct (end)//////////////////////////
		///////////////////copy inputdata for Tsinghua superframe MCR//////////////////////////
		cid = 0;
		for(k = 0; k < elementnum; k++)
		{
			memcpy(&time_dmix[cid][0], &time_dmix[cid][CORE_FRAMESIZE*2], CORE_FRAMESIZE*2*sizeof(float));

			if(elementencode_info[k].nChannelsInEl == 2)
			{
				memcpy(&time_dmix[cid+1][0], &time_dmix[cid+1][CORE_FRAMESIZE*2], CORE_FRAMESIZE*2*sizeof(float));

				if(useBWE)
				{
					for(i = 0; i < (numSamplesRead/inputInfo.nChannels); i++)
					{
						time_dmix[cid][CORE_FRAMESIZE*2-frameOffset+i] = downmixBuffer[cid][coreReadOffset+i]/Maxpcmvalue;
						time_dmix[cid+1][CORE_FRAMESIZE*2-frameOffset+i] = downmixBuffer[cid+1][coreReadOffset+i]/Maxpcmvalue;
					}
				}
				else
				{
					for(i = 0; i < (numSamplesRead/inputInfo.nChannels); i++)
					{
						time_dmix[cid][CORE_FRAMESIZE*2+i] = (inputBuffer[cid][coreReadOffset+i]+inputBuffer[cid+1][coreReadOffset+i])/Maxpcmvalue * 0.5f;
						time_dmix[cid+1][CORE_FRAMESIZE*2+i] = (inputBuffer[cid][coreReadOffset+i]-inputBuffer[cid+1][coreReadOffset+i])/Maxpcmvalue * 0.5f;
					}
				}
				cid++;
				cid++;
			}
			else
			{
				for(i = 0; i < (numSamplesRead/inputInfo.nChannels); i++)
				{
					time_dmix[cid][CORE_FRAMESIZE*2+i] = inputBuffer[cid][coreReadOffset+i]/Maxpcmvalue;
				}
				cid++;
			}
		}


		///////////codectype, 编码方式选择，tsinghua_MCR (0) or tianlai_PCA (1)///////////
		PCAGroupmode = 0;
		if (inputInfo.nChannels == 6)
			codectype = 0;// codectypeselect(lfEncset, inputInfo.nChannels, mdftSpectrum, &PCAGroupmode);//1;//(codectype+1)%2;//rand()%2;
		else
			codectype = 0;
		if(inputInfo.nChannels==1)
			codectype = 1;
		if(inputInfo.nChannels==2)
			codectype = 0;

		//printf("codectype = %d\n",codectype);
		////////////////////////////////////////////////
		if (codectype == 0)
		{
			avs2audiopack_buffer *opb;
			opb = calloc(1, sizeof(avs2audiopack_buffer));
			avs2audiopack_writeinit(opb);
			avs2audiopack_write(opb, codectype, 1);

			if (useBWE == 1)
				avs2audiopack_write(opb, 1, 1);
			else if (useBWE == 0)
				avs2audiopack_write(opb, 0, 1);

			if (fill_element_num == 1)
				avs2audiopack_write(opb, 1, 1);
			else if (fill_element_num == 0)
				avs2audiopack_write(opb, 0, 1);


			cid = 0;
			for (k = 0; k < elementnum + fill_element_num; k++)
			{
				if (elementencode_info[k].elType == ID_CPE_F 
					|| elementencode_info[k].elType == ID_CPE_L
					|| elementencode_info[k].elType == ID_CPE_H)
				{
					if (useBWE == 0)
					{
						psData[k]->bandwidth = lfEncset[cid]->config.bandWidth;
						memcpy(psData[k]->winseq, lfEncset[cid]->lf_winseq, 20 * sizeof(int));
						memcpy(psData[k]->left_data, &mdftSpectrum[cid][0], CORE_FRAMESIZE * sizeof(float));
						memcpy(psData[k]->right_data, &mdftSpectrum[cid + 1][0], CORE_FRAMESIZE * sizeof(float));

						data_mcr[k] = (avs2audiopack_buffer *)calloc(1, sizeof(avs2audiopack_buffer));
						numMcrBytes = MCR_Encoder(data_mcr[k], psData[k], mcr_info, psData_pre[k]);

						memcpy(&mdftSpectrum[cid][0], psData[k]->sum_data, CORE_FRAMESIZE * sizeof(float));
						memcpy(&mdftSpectrum[cid + 1][0], psData[k]->dif_data, CORE_FRAMESIZE * sizeof(float));

						if (psData[k]->sflag == 1)
						{
							lfEncset[cid]->elInfo.elType = ID_CPE_S;
							lfEncset[cid + 1]->elInfo.elType = ID_CPE_S;
						}
						else
						{
							lfEncset[cid]->elInfo.elType = ID_CPE_F;
							lfEncset[cid + 1]->elInfo.elType = ID_CPE_F;
						}
					}
					else
					{
						if (psBweData[k]->sflag == 1)
						{
							lfEncset[cid]->elInfo.elType = ID_CPE_S;
							lfEncset[cid + 1]->elInfo.elType = ID_CPE_S;
						}
						else
						{
							lfEncset[cid]->elInfo.elType = ID_CPE_L;
							lfEncset[cid + 1]->elInfo.elType = ID_CPE_L;
						}
					}
					//fprintf(fp_lars,"numMcrBytes=%d\n",numMcrBytes);

					avs2audiopack_write(opb, lfEncset[cid]->elInfo.elType, 4);

					// write core channel number of each pair element
					if (srateInfo.core_brate[cid + 1] == 0)
						avs2audiopack_write(opb, 0, 1);
					else
						avs2audiopack_write(opb, 1, 1);

#if CONSTANT_BITRATE_CONTROL
					//stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
					if (1==1)//(opencbr)
					{
						if (useBWE)
						{
							if (k == 0)
								lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1 + OUTERLOOP_reservedBYTEforHEADER + 2;
							else
								lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1;
						}
						else
						{
							if (k == 0)
								lfEncset[cid]->numbwebytes = OUTERLOOP_reservedBYTEforHEADER + 2;
							else
								lfEncset[cid]->numbwebytes = 0;
						}
						if ((fill_element_num > 0) && (k == 0))
							lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
						if ((elementnum - 1 == k))
							lfEncset[cid]->numbwebytes += 1;
						lfEncset[cid]->numbwebytes -= (srateInfo.mcr_brate[cid] * 1000.0) * inputInfo.inSamples / (inputInfo.nChannels) / (lfEncset[cid]->config.sampleRate * 2) / 8;
						lfEncset[cid]->numbwebytes += calc_mcr_bits(k, opb, cid, data_mcr[k]) / 8;
					}
					else
						lfEncset[cid]->numbwebytes = 0;
#endif
					lfEncset[cid]->pcm_buffer = &time_dmix[cid][CORE_FRAMESIZE];
					Avs2LFEncoder(useBWE, lfEncset[cid], mdftSpectrum[cid],
						ancDataBytes[cid], numAncDataBytes + cid,
						sampleData, &numOutBytes, k, codectype, opb, Maxpcmvalue);
					//				   fprintf(fp_lars,"numOutBytes=%d\n",numOutBytes);
				   
					if (srateInfo.core_brate[cid + 1] > 0)
					{
#if CONSTANT_BITRATE_CONTROL
						if (1==1)//(opencbr)
						{
							if (k == 0)
								lfEncset[cid + 1]->numbwebytes = (numAncDataBytes[cid + 1] + 1)*(useBWE) + OUTERLOOP_reservedBYTEforHEADER + 2;
							else
								lfEncset[cid + 1]->numbwebytes = (numAncDataBytes[cid + 1] + 1)*(useBWE);
							if ((fill_element_num > 0) && (k == 0))
								lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
							if ((elementnum - 1 == k))
								lfEncset[cid]->numbwebytes += 1;
							lfEncset[cid]->numbwebytes -= (srateInfo.mcr_brate[cid] * 1000.0) * inputInfo.inSamples / (inputInfo.nChannels) / (lfEncset[cid]->config.sampleRate * 2) / 8;
							lfEncset[cid]->numbwebytes += calc_mcr_bits(k, opb, cid, data_mcr[k]) / 8;
						}
						else
							lfEncset[cid + 1]->numbwebytes = 0;
#endif
						lfEncset[cid + 1]->pcm_buffer = &time_dmix[cid + 1][CORE_FRAMESIZE];

						Avs2LFEncoder(useBWE, lfEncset[cid + 1], mdftSpectrum[cid + 1],
							ancDataBytes[cid + 1], numAncDataBytes + cid + 1,
							sampleData, &numOutBytes, 1, 0, opb, Maxpcmvalue);

					}

					memcpy(lfEncset[cid]->pcm_buffer0 - CORE_FRAMESIZE, lfEncset[cid]->pcm_buffer0, CORE_FRAMESIZE * 2 * sizeof(float));
					memcpy(lfEncset[cid + 1]->pcm_buffer0 - CORE_FRAMESIZE, lfEncset[cid + 1]->pcm_buffer0, CORE_FRAMESIZE * 2 * sizeof(float));

				}
				else if (elementencode_info[k].elType == ID_SCE)
				{
					//配置每个声道所用ci和编码码本
					int nChannelstmp = 1;
					bitRateIndex = 0;
					if (nChannelstmp == 1)
						channelbasesetting = basesetting;
					else
					{
						//double averageBitrate;
						//averageBitrate = inputInfo.bitRate * 0.2;

						channelbasesetting = basesetting;
					}

					ci_set(&(lfEncset[cid]->vi), channelbasesetting);

					_vds_flr_res_set(&(lfEncset[cid]->vd), &(lfEncset[cid]->vi), min(12,channelbasesetting+3));

					lowpass_kHz = freqbeginend_setting(&(lfEncset[cid]->vi), nChannelstmp, bitRateIndex, 0);

					reset_bandWidth(config.sampleRate, config_idx, lowpass_kHz, &(lfEncset[cid]->config.bandWidth), useBWE);

					{

						struct AVS2_ENCODER *lfEnc = lfEncset[cid];

						{
							int lf_winseq_index = lfEnc->lf_winseq[19];
							int s = min(12,basesetting+3);

							avs2audiopack_write(opb, elementencode_info_tianlai51[eid].elType, 4);
							avs2audiopack_write(opb, s, 4);
							avs2audiopack_write(opb, lf_winseq_index, 6);
						}


						//reset_bitrate(&(lfEnc->vd),PCAcorebitpershort*1.0,coreBitrate);
#if CONSTANT_BITRATE_CONTROL
						//stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
						if (1 == 1)//(opencbr)
						{
							if (useBWE)
							{
								if (eid == 0)
									lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1 + OUTERLOOP_reservedBYTEforHEADER + 2;
								else
									lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1;
							}
							else
							{
								if (eid == 0)
									lfEncset[cid]->numbwebytes = OUTERLOOP_reservedBYTEforHEADER + 2;
								else
									lfEncset[cid]->numbwebytes = 0;
							}
							if ((fill_element_num > 0) && (eid == 0))
								lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
							if ((elementnum - 1 == eid))
								lfEncset[cid]->numbwebytes += 1;
						}
						else
							lfEncset[cid]->numbwebytes = 0;
#endif
						/* encode lf frame using a  lf encoder */
						Avs2LFEncoder_PCA(useBWE,
							lfEnc,
							&(lfEnc->vb),
							inputBuffer[cid] + coreReadOffset,
							mdftSpectrum[cid],
							1,
							ancDataBytes[cid],
							&numAncDataBytes[cid],
							sampleData,
							&numOutBytes,
							0,
							eid,
							1,
							1,
							PCAGroupmode,
							opb, Maxpcmvalue
							);

						//{
						//		FILE *fp0 = fopen("bitnumber.txt","a");
						//		fprintf(fp0, "[core][%d]\n", numOutBytes);				
						//		if (numMcrBytes) fprintf(fp0, "[mcr][%d]\n", numMcrBytes);
						//		fclose(fp0);
						//}

						//update pcmbuffer
						for (i = -2048 / 2; i < 2048 / 2; i++)
							lfEnc->pcm_buffer0[i] = lfEnc->pcm_buffer0[2048 / 2 + i];

						if (bDoUpsample)
						{
							memmove(&inputBuffer[cid][envReadOffset],
								&inputBuffer[cid][envReadOffset + AVS2ENC_BLOCKSIZE * 2],
								(envWriteOffset - envReadOffset) * sizeof(float));

							memmove(&inputBuffer[cid][upsampleReadOffset],
								&inputBuffer[cid][upsampleReadOffset + AVS2ENC_BLOCKSIZE],
								(writeOffset - upsampleReadOffset) * sizeof(float));
						}
						else
						{
							memmove(inputBuffer[cid], inputBuffer[cid] + nSamplesPerChannel, writeOffset * sizeof(float));
						}
					}

					numMcrBytes = 0;
				}
				else if (elementencode_info[k].elType == ID_FIL)
				{
					int fill_type = 0;//frame % 4; //for test, shumin.xu
					int fill_byte = 0;// 0xff;
					char fill_dft_bits[2048] = { 0 };

					int leftbytes = inputInfo.bitRate * 1024 / inputInfo.sampleRate / 8 * 2;
					////if (objflag)
					leftbytes = inputInfo.bitRate * 1.0*inputInfo.inSamples / inputInfo.sampleRate / 8 / (inputInfo.nChannels) - OUTERLOOP_reservedBYTEforHEADER - 2;
					//else
					//	leftbytes = inputInfo.bitRate * 1.0*inputInfo.inSamples / inputInfo.sampleRate / 8 / (inputInfo.nChannels) - OUTERLOOP_reservedBYTEforHEADER - 2;
					leftbytes -= 1 + opb->endbyte +1;
					if (opb->endbit > 0) leftbytes--;

					fill_byte = leftbytes;
					if (fill_byte < 0)
					{
						printf("fill_byte %d < 0\n", fill_byte);
						fill_byte = 0;
					}

					avs2audiopack_write(opb, 6, 4); //ID_FIL = 6;
					avs2audiopack_write(opb, fill_type, 4);

					switch (fill_type)
					{

					case 0:
					{
						avs2audiopack_write(opb, fill_byte, 16);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_write(opb, fill_dft_bits[0], 1);
						}
					}
					break;
					case 1:
					{
						avs2audiopack_write(opb, fill_byte, 15);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_write(opb, fill_dft_bits[0], 2);
						}
					}
					break;
					case 2:
					{
						avs2audiopack_write(opb, fill_byte, 14);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_write(opb, fill_dft_bits[0], 4);
						}
					}
					break;
					case 3:
					{
						avs2audiopack_write(opb, fill_byte, 13);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_write(opb, fill_dft_bits[0], 8);
						}
					}
					break;
					default:
						printf("the value of fill_type is illegal");
						break;
					}
					numMcrBytes = 0;

				}
				else //elementencode_info[k].elType == ID_CPE_S
				{
#if CONSTANT_BITRATE_CONTROL
					//stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
					if (1 == 1)//(opencbr)
					{
						if (useBWE)
						{
							if (k == 0)
								lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1 + OUTERLOOP_reservedBYTEforHEADER + 2;
							else
								lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1;
						}
						else
						{
							if (k == 0)
								lfEncset[cid]->numbwebytes = OUTERLOOP_reservedBYTEforHEADER + 2;
							else
								lfEncset[cid]->numbwebytes = 0;
						}
						if ((fill_element_num > 0) && (k == 0))
							lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
						if ((elementnum - 1 == k))
							lfEncset[cid]->numbwebytes += 1;
						lfEncset[cid]->numbwebytes -= (srateInfo.mcr_brate[cid] * 1000.0) * inputInfo.inSamples / (inputInfo.nChannels) / (lfEncset[cid]->config.sampleRate * 2) / 8;
						lfEncset[cid]->numbwebytes += calc_mcr_bits(k, opb, cid, data_mcr[k]) / 8;
					}
					else
						lfEncset[cid]->numbwebytes = 0;
#endif
					lfEncset[cid]->pcm_buffer = &time_dmix[cid][CORE_FRAMESIZE];
					Avs2LFEncoder(useBWE, lfEncset[cid], mdftSpectrum[cid], ancDataBytes[cid],
						numAncDataBytes + cid, sampleData, &numOutBytes, k, codectype, opb, Maxpcmvalue);

					memcpy(lfEncset[cid]->pcm_buffer0 - CORE_FRAMESIZE, lfEncset[cid]->pcm_buffer0, CORE_FRAMESIZE * 2 * sizeof(float));

				}
				
				if (useBWE && elementencode_info[k].elType != ID_FIL)
				{
					int j;
					avs2audiopack_write(opb, numAncDataBytes[cid], 8);
					for (j = 0; j < numAncDataBytes[cid]; j++)
						avs2audiopack_write(opb, ancDataBytes[cid][j], 8);

					mcrlencount[cid + 1] += numAncDataBytes[cid] + 2;
				}
				if (numMcrBytes)
				{
					int j, tmp;
					if (data_mcr[k]->endbit > 0)
						avs2audiopack_write(data_mcr[k], 0, 8 - data_mcr[k]->endbit);

					for (j = 0; j < data_mcr[k]->endbyte; j++)
						avs2audiopack_write(opb, data_mcr[k]->buffer[j], 8);

					//  if (data_mcr[k]->endbit > 0)
					   //{
					   //   tmp = data_mcr[k]->buffer[data_mcr[k]->endbyte] >> (8 - data_mcr[k]->endbit);
					   //   avs2audiopack_write(opb, tmp, data_mcr[k]->endbit);
					   //}

					mcrlencount[cid] += avs2audiopack_bytes(data_mcr[k]);
				}

				cid += elementencode_info[k].nChannelsInEl;

			}

			for (cid = 0; cid < inputInfo.nChannels; cid++)
			{
				if (bDoUpsample)
				{
					memmove(&inputBuffer[cid][envReadOffset], &inputBuffer[cid][envReadOffset + AVS2ENC_BLOCKSIZE * 2], (envWriteOffset - envReadOffset) * sizeof(float));

					memmove(&inputBuffer[cid][upsampleReadOffset], &inputBuffer[cid][upsampleReadOffset + AVS2ENC_BLOCKSIZE], (writeOffset - upsampleReadOffset) * sizeof(float));
				}
				else
				{
					memmove(inputBuffer[cid], inputBuffer[cid] + nSamplesPerChannel, writeOffset * sizeof(float));
					memmove(downmixBuffer[cid], downmixBuffer[cid] + nSamplesPerChannel, writeOffset * sizeof(float));
				}
			}

			if (opb->endbit > 0)
				avs2audiopack_write(opb, 0, (8 - opb->endbit));

			/* write out the bitstream */
			memcpy(sampleData, opb->buffer, opb->endbyte);
			numOutBytes = avs2audiopack_bytes(opb);
			/*  {
			  FILE *fp0 = fopen("bitnumber.txt","a");
			  fprintf(fp0, "[total][%d]\n", numOutBytes);
			  fclose(fp0);
			  }*/

			free(opb);
		}

		else //tianlai PCA codec
		{

			////////////////////////tianlai_PCAcodec  start /////////////////////////
			avs2audiopack_buffer *opb, *tmp_opb;
			{
				opb = calloc(1, sizeof(avs2audiopack_buffer));
				avs2audiopack_writeinit(opb);


				if (numOfChannels > 1)
				{
					if (codectype == 0)
						avs2audiopack_write(opb, 0, 1);
					else
						avs2audiopack_write(opb, 1, 1);

					if (codectype == 1)
					{
						//PCAGroupmode
						avs2audiopack_write(opb, PCAGroupmode, 4);
					}


				}

				if (useBWE == 1)
					avs2audiopack_write(opb, 1, 1);
				else if (useBWE == 0)
					avs2audiopack_write(opb, 0, 1);
				if (fill_element_num == 1)
					avs2audiopack_write(opb, 1, 1);
				else if (fill_element_num == 0)
					avs2audiopack_write(opb, 0, 1);
			}


			coreBitrate = inputInfo.bitRate / (numOfChannels / 2.0) - 10000;
			//	reset_bitrate(&(lfEncset[0]->vd),128,coreBitrate);

					///////////////////copy inputdata for psy analysis//////////////////////////
			cid = 0;
			for (k = 0; k < elementnum; k++)
			{
				memcpy(&time_dmix[cid][0], &time_dmix[cid][CORE_FRAMESIZE * 2], CORE_FRAMESIZE * 2 * sizeof(float));

				if (elementencode_info[k].nChannelsInEl == 2)
				{
					for (i = 0; i < (numSamplesRead / inputInfo.nChannels); i++)
					{
						time_dmix[cid][CORE_FRAMESIZE * 2 + i] = (inputBuffer[cid][coreReadOffset + i] + inputBuffer[cid + 1][coreReadOffset + i]) / Maxpcmvalue * 0.5f;
						time_dmix[cid + 1][CORE_FRAMESIZE * 2 + i] = 0;//(inputBuffer[cid][coreReadOffset+i]-inputBuffer[cid+1][coreReadOffset+i])/Maxpcmvalue * 0.5f;
					}
					cid++;
					cid++;
				}
				else
				{
					for (i = 0; i < (numSamplesRead / inputInfo.nChannels); i++)
					{
						time_dmix[cid][CORE_FRAMESIZE * 2 + i] = inputBuffer[cid][coreReadOffset + i] / Maxpcmvalue;
					}
					cid++;
				}
			}

			//重新配置PCA组合
			if (numOfChannels == 8)
			{
				//	PCAGroupmode =1;
				encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[0x10 + PCAGroupmode];
				elementnum_tianlai51 = encoder_mode_tianlai51->numofele;
				cid = 0;
				for (i = 0; i < elementnum_tianlai51; i++)
				{
					elementencode_info_tianlai51[i].elType = encoder_mode_tianlai51->idType[i];

					switch (elementencode_info_tianlai51[i].elType) {

					case ID_SCE:      /* single channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 1;
						cid++;
						break;

					case ID_CPE_F:      /* channel pair */
						elementencode_info_tianlai51[i].nChannelsInEl = 2;
						cid++;
						cid++;
						break;

					case ID_LFE:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 1;
						cid++;
						break;

					case ID_PCA2:     /*PCA2 channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 2;
						cid++;
						cid++;
						break;

					case ID_PCA4:     /*PCA4 channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 4;
						cid += 4;
						break;

					case ID_PCA6:     /*PCA6 channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 6;
						cid += 6;
						break;
					}

				}
				for (i = elementnum_tianlai51; i < elementnum_tianlai51 + fill_element_num; i++)
				{
					elementencode_info_tianlai51[i].elType = ID_FIL;
				}

			}


			if (numOfChannels == 6)
			{
				//	PCAGroupmode =1;
				encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[0x02 + PCAGroupmode];
				elementnum_tianlai51 = encoder_mode_tianlai51->numofele;
				cid = 0;
				for (i = 0; i < elementnum_tianlai51; i++)
				{
					elementencode_info_tianlai51[i].elType = encoder_mode_tianlai51->idType[i];

					switch (elementencode_info_tianlai51[i].elType) {

					case ID_SCE:      /* single channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 1;

						cid++;
						break;

					case ID_CPE_F:      /* channel pair */
						elementencode_info_tianlai51[i].nChannelsInEl = 2;

						cid++;
						cid++;
						break;

					case ID_LFE:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 1;

						cid++;
						break;

					case ID_PCA2:     /*PCA2 channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 2;

						cid++;
						cid++;
						break;

					case ID_PCA4:     /*PCA4 channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 4;

						cid += 4;
						break;

					case ID_PCA6:     /*PCA6 channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 6;

						cid += 6;
						break;
					}

				}
				for (i = elementnum_tianlai51; i < elementnum_tianlai51 + fill_element_num; i++)
				{
					elementencode_info_tianlai51[i].elType = ID_FIL;
				}

			}

			if (numOfChannels == 2)
			{
				//PCAGroupmode =1;
				encoder_mode_tianlai51 = &PCAGroupmodeHeaderTable[0x01];
				elementnum_tianlai51 = encoder_mode_tianlai51->numofele;
				cid = 0;
				for (i = 0; i < elementnum_tianlai51; i++)
				{
					elementencode_info_tianlai51[i].elType = encoder_mode_tianlai51->idType[i];

					switch (elementencode_info_tianlai51[i].elType) {

					case ID_SCE:      /* single channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 1;

						cid++;
						break;

					case ID_CPE_F:      /* channel pair */
						elementencode_info_tianlai51[i].nChannelsInEl = 2;

						cid++;
						cid++;
						break;

					case ID_LFE:     /*LFE channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 1;

						cid++;
						break;

					case ID_PCA2:     /*PCA2 channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 2;

						cid++;
						cid++;
						break;

					case ID_PCA4:     /*PCA4 channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 4;

						cid += 4;
						break;

					case ID_PCA6:     /*PCA6 channel */
						elementencode_info_tianlai51[i].nChannelsInEl = 6;

						cid += 6;
						break;
					}

				}
				for (i = elementnum_tianlai51; i < elementnum_tianlai51 + fill_element_num; i++)
				{
					elementencode_info_tianlai51[i].elType = ID_FIL;
				}

			}

			cid = 0;

			for (eid = 0; eid < elementnum_tianlai51; eid++)
			{
				switch (elementencode_info_tianlai51[eid].elType)
				{

				case ID_SCE:      /* single channel */
				{

					struct AVS2_ENCODER *lfEnc = lfEncset[cid];

					for (i = 0; i < 2048 / 2; i++)
						lfEnc->pcm_buffer0[2048 / 2 + i] = *(inputBuffer[cid] + coreReadOffset + i) / Maxpcmvalue;

					mdft_lowpassframeblock_multi(lfEnc->pcm_buffer0 - 1024 + 448, lfEnc->lf_winseq, Mdftout, (4096 / 4) / 2);

					for (i = 0; i < 1024; i++)
						mdftSpectrum[cid][i] = Mdftout[i * 2] * Maxpcmvalue;

				}

				cid++;
				break;

				case ID_PCA2:      /* PCAx channel */
				case ID_PCA4:
				case ID_PCA6:
				{
					int iid;
					int sid = cid;


					for (iid = 0; iid < elementencode_info_tianlai51[eid].nChannelsInEl; iid++)
					{

						struct AVS2_ENCODER *lfEnc = lfEncset[cid];

						if (iid > 0)
							memcpy(lfEnc->lf_winseq, lfEncset[sid]->lf_winseq, 20 * sizeof(int));

						for (i = 0; i < 2048 / 2; i++)
							lfEnc->pcm_buffer0[2048 / 2 + i] = *(inputBuffer[cid] + coreReadOffset + i) / Maxpcmvalue;

						mdft_lowpassframeblock_multi(lfEnc->pcm_buffer0 - 1024 + 448, lfEnc->lf_winseq, Mdftout, (4096 / 4) / 2);

						//LFE, lowpass filter			
						if (cid == 3)
						{
							int index, tt, jj, kk, ll, mdftoffset;
							int LL[5] = { 4096 / 8 / 2, 4096 / 4 / 2, 4096 / 2 / 2, 4096 / 2,4096 / 16 / 2 };

							tt = lfEnc->lf_winseq[1];
							jj = lfEnc->lf_winseq[1 + 1];

							if (LL[tt - 1] > LL[jj - 1])
								ll = LL[tt - 1] / 2;
							else
								ll = LL[jj - 1] / 2;


							mdftoffset = 0;

							for (index = 1; index < (lfEnc->lf_winseq[0] + 1); index++)
							{
								int lowbandoffset;

								tt = lfEnc->lf_winseq[index];
								jj = lfEnc->lf_winseq[index + 1];

								if (LL[tt - 1] > LL[jj - 1])
									ll = LL[tt - 1] / 2;
								else
									ll = LL[jj - 1] / 2;

								lowbandoffset = 8;
								if (ll == 4096 / 8 / 2 / 2)
									lowbandoffset = 2;
								if (ll == 4096 / 8 / 2)
									lowbandoffset = 4;
								if (ll == 4096 / 4 / 2)
									lowbandoffset = 8;
								if (ll == 4096 / 2 / 2)
									lowbandoffset = 8;
								if (ll == 4096 / 16 / 2 / 2)
									lowbandoffset = 2;
								for (kk = lowbandoffset; kk < ll; kk++)
								{
									Mdftout[mdftoffset + kk * 2] = 0;
									Mdftout[mdftoffset + kk * 2 + 1] = 0;
								}
								mdftoffset += (ll * 2);

							}
						}

						for (i = 0; i < 1024; i++)
							mdftSpectrum[cid][i] = Mdftout[i * 2] * Maxpcmvalue;

						cid++;
					}//for(iid=0;iid<elementencode_info[eid].nChannelsInEl;iid++)

				//PCA analysis,get the usePCAitemnum principal components and PCA Matrix
					multichannelMDCT_PCA_2(&mdftSpectrum[sid], lfEncset[sid]->lf_winseq, elementencode_info_tianlai51[eid].nChannelsInEl, &mdftSpectrum[sid], eid);
				}
				break;
				default:
					break;
				}//switch (elementencode_info[eid].elType) {

			}//for( eid=0;eid<elementnum;eid++)

	////////////////////////////////////////////////


	////////////////////////////////////////////////
			cid = 0;

			for (eid = 0; eid < elementnum_tianlai51 + fill_element_num; eid++)
			{
				switch (elementencode_info_tianlai51[eid].elType) {

				case ID_SCE:      /* single channel */

					//配置每个声道所用ci和编码码本
					if (inputInfo.nChannels == 1)
						channelbasesetting = basesetting;
					else
					{
						double averageBitrate;
						averageBitrate = inputInfo.bitRate * 0.2;

						channelbasesetting = basesetting;
					}

					ci_set(&(lfEncset[cid]->vi), channelbasesetting);

					_vds_flr_res_set(&(lfEncset[cid]->vd), &(lfEncset[cid]->vi), min(12,channelbasesetting+3));

					lowpass_kHz = freqbeginend_setting(&(lfEncset[cid]->vi), inputInfo.nChannels, bitRateIndex, 0);

					reset_bandWidth(config.sampleRate, config_idx, lowpass_kHz, &(lfEncset[cid]->config.bandWidth), useBWE);

					{
						struct AVS2_ENCODER *lfEnc = lfEncset[cid];
						{
							int lf_winseq_index = lfEnc->lf_winseq[19];
							int s = min(12, basesetting + 3);

							avs2audiopack_write(opb, elementencode_info_tianlai51[eid].elType, 4);
							avs2audiopack_write(opb, s, 4);
							avs2audiopack_write(opb, lf_winseq_index, 6);
						}

						//reset_bitrate(&(lfEnc->vd),PCAcorebitpershort*1.0,coreBitrate);
#if CONSTANT_BITRATE_CONTROL
					//stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
						if (1 == 1)//(opencbr)
						{
							if (useBWE)
							{
								if (eid == 0)
									lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1 + OUTERLOOP_reservedBYTEforHEADER + 2;
								else
									lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1;
							}
							else
							{
								if (eid == 0)
									lfEncset[cid]->numbwebytes = OUTERLOOP_reservedBYTEforHEADER + 2;
								else
									lfEncset[cid]->numbwebytes = 0;
							}
							if ((fill_element_num > 0) && (eid == 0))
								lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
							if ((elementnum - 1 == eid))
								lfEncset[cid]->numbwebytes += 1;
						}
						else
							lfEncset[cid]->numbwebytes = 0;
#endif
						/* encode lf frame using a  lf encoder */
						Avs2LFEncoder_PCA(useBWE,

							lfEnc,
							&(lfEnc->vb),
							inputBuffer[cid] + coreReadOffset,
							mdftSpectrum[cid],
							1,
							ancDataBytes[cid],
							&numAncDataBytes[cid],
							sampleData,
							&numOutBytes,
							0,
							eid,
							1,
							1,
							PCAGroupmode,
							opb, Maxpcmvalue
						);

						//{
						//			FILE *fp0 = fopen("bitnumber.txt","a");
						//			fprintf(fp0, "[core][%d]\n", numOutBytes);				
						//			if (numMcrBytes) fprintf(fp0, "[mcr][%d]\n", numMcrBytes);
						//			fclose(fp0);
						//		}
						//updatae pcmbuffer
						for (i = -2048 / 2; i < 2048 / 2; i++)
							lfEnc->pcm_buffer0[i] = lfEnc->pcm_buffer0[2048 / 2 + i];

						if (bDoUpsample)
						{
							memmove(&inputBuffer[cid][envReadOffset],
								&inputBuffer[cid][envReadOffset + AVS2ENC_BLOCKSIZE * 2],
								(envWriteOffset - envReadOffset) * sizeof(float));

							memmove(&inputBuffer[cid][upsampleReadOffset],
								&inputBuffer[cid][upsampleReadOffset + AVS2ENC_BLOCKSIZE],
								(writeOffset - upsampleReadOffset) * sizeof(float));
						}
						else
						{
							memmove(inputBuffer[cid], inputBuffer[cid] + nSamplesPerChannel, writeOffset * sizeof(float));
						}
					}
					cid++;
					break;

				case ID_PCA2:      /* PCAx channel */
				case ID_PCA4:
				case ID_PCA6:
				{int iid;
				int sid = cid;



				//选择合适的usePCAitemnum
				usePCAitemnum_Optimization(elementencode_info_tianlai51[eid].nChannelsInEl,
					inputInfo.bitRate, inputInfo.nChannels, &(lfEncset[sid]->usePCAitemnum));

				if (numOfChannels >= 2)
				{
					if (elementencode_info_tianlai51[eid].nChannelsInEl == 4)
					{
						if (lfEncset[sid]->usePCAitemnum == 2)
						{
							reset_bitrate(&(lfEncset[sid]->vd), PCAcorebitpershort*0.9, coreBitrate);
							reset_bitrate(&(lfEncset[sid + 1]->vd), PCAcorebitpershort*0.65, coreBitrate);
						}
						else
						{
							reset_bitrate(&(lfEncset[sid]->vd), PCAcorebitpershort*0.9, coreBitrate);
							reset_bitrate(&(lfEncset[sid + 1]->vd), PCAcorebitpershort*0.65, coreBitrate);
							reset_bitrate(&(lfEncset[sid + 2]->vd), PCAcorebitpershort*0.45, coreBitrate);
						}

					}
					else if (elementencode_info_tianlai51[eid].nChannelsInEl == 2)
					{
						if (lfEncset[sid]->usePCAitemnum == 1)
						{
							if (eid == 0)
								reset_bitrate(&(lfEncset[sid]->vd), PCAcorebitpershort*1.1, coreBitrate);
							else
								reset_bitrate(&(lfEncset[sid]->vd), PCAcorebitpershort*0.9, coreBitrate);

						}
						else
						{
							if (eid == 0)
							{
								reset_bitrate(&(lfEncset[sid + 0]->vd), PCAcorebitpershort*0.75, coreBitrate);
								reset_bitrate(&(lfEncset[sid + 1]->vd), PCAcorebitpershort*0.35, coreBitrate);
							}
							else if (eid == 1)
							{
								reset_bitrate(&(lfEncset[sid + 0]->vd), PCAcorebitpershort*0.95, coreBitrate);
								reset_bitrate(&(lfEncset[sid + 1]->vd), PCAcorebitpershort*0.5, coreBitrate);
							}
							else
							{
								reset_bitrate(&(lfEncset[sid + 0]->vd), PCAcorebitpershort*0.6, coreBitrate);
								reset_bitrate(&(lfEncset[sid + 1]->vd), PCAcorebitpershort*0.35, coreBitrate);
							}

						}
					}
				}

				{	struct AVS2_ENCODER *lfEnc = lfEncset[cid];
				int lf_winseq_index = lfEnc->lf_winseq[19];
				int s = min(12, basesetting + 3);
				//	if((indexinelement==0)&&(nChannelsInEl>1))

				avs2audiopack_write(opb, elementencode_info_tianlai51[eid].elType, 4);

				{
					if (codectype == 1)
					{

						avs2audiopack_write(opb, lfEnc->usePCAitemnum, 3);
					}
				}

				//avs2audiopack_write(opb, lfEnc->elInfo.instanceTag, 4);
				avs2audiopack_write(opb, s, 4);
				avs2audiopack_write(opb, lf_winseq_index, 6);
				}


				for (iid = 0; iid < elementencode_info_tianlai51[eid].nChannelsInEl; iid++)
				{



					//配置每个声道所用ci和编码码本
					if (inputInfo.nChannels == 1)
						channelbasesetting = basesetting;
					else if ((inputInfo.nChannels == 2))
					{
						channelbasesetting = basesetting;
					}
					else
					{
						double averageBitrate;
						averageBitrate = inputInfo.bitRate * 0.2;


						channelbasesetting = basesetting;
					}

					ci_set(&(lfEncset[cid]->vi), channelbasesetting);

					_vds_flr_res_set(&(lfEncset[cid]->vd), &(lfEncset[cid]->vi), min(12,channelbasesetting+3));

					if (!((elementencode_info_tianlai51[eid].elType == ID_PCA2) && (eid == 1)))
						lowpass_kHz = freqbeginend_setting(&(lfEncset[cid]->vi), inputInfo.nChannels, bitRateIndex, 0);
					else
						lowpass_kHz = freqbeginend_setting(&(lfEncset[cid]->vi), inputInfo.nChannels, /*0*/bitRateIndex, 0);



					reset_bandWidth(config.sampleRate, config_idx, lowpass_kHz, &(lfEncset[cid]->config.bandWidth), useBWE);

					{
						//				unsigned int numAncDataBytes=0;
						struct AVS2_ENCODER *lfEnc = lfEncset[cid];

						//numOutBytes =0;
						/* encode lf frame using a new lf encoder */
						//encode the first usePCAitemnum principal components,  PCA Matrix
						lfEnc->elInfo.elType = 0;

						lfEnc->pcm_buffer = &time_dmix[cid][CORE_FRAMESIZE];
#if CONSTANT_BITRATE_CONTROL
						//stAvs2Enc->lfEncset[cid]->numbwebytes = 0;
						if (1 == 1)//(opencbr)
						{
							if (useBWE)
							{
								if (eid == 0)
									lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1 + OUTERLOOP_reservedBYTEforHEADER + 2;
								else
									lfEncset[cid]->numbwebytes = numAncDataBytes[cid] + 1;
							}
							else
							{
								if (eid == 0)
									lfEncset[cid]->numbwebytes = OUTERLOOP_reservedBYTEforHEADER + 2;
								else
									lfEncset[cid]->numbwebytes = 0;
							}
							if ((fill_element_num > 0) && (eid == 0))
								lfEncset[cid]->numbwebytes += 4; //填充块必要的比特数
							if ((elementnum - 1 == eid))
								lfEncset[cid]->numbwebytes += 1;
							lfEncset[cid]->numbwebytes -= (srateInfo.mcr_brate[cid] * 1000.0) * inputInfo.inSamples / (inputInfo.nChannels) / (lfEncset[cid]->config.sampleRate * 2) / 8;
							lfEncset[cid]->numbwebytes += calc_mcr_bits(eid, opb, cid, data_mcr[eid]) / 8;
						}
						else
							lfEncset[cid]->numbwebytes = 0;
#endif
						if ((lfEncset[sid]->usePCAitemnum) > iid)
							Avs2LFEncoder_PCA(useBWE,
								lfEnc,
								&(lfEnc->vb),
								inputBuffer[cid] + coreReadOffset,
								mdftSpectrum[cid],
								1,
								ancDataBytes[cid],
								&numAncDataBytes[cid],
								sampleData,
								&numOutBytes,
								iid,
								eid,
								elementencode_info_tianlai51[eid].nChannelsInEl,
								1,
								PCAGroupmode,
								opb, Maxpcmvalue
							);

						//updatae pcmbuffer
						for (i = -2048 / 2; i < 2048 / 2; i++)
							lfEnc->pcm_buffer0[i] = lfEnc->pcm_buffer0[2048 / 2 + i];

						if (bDoUpsample)
						{
							memmove(&inputBuffer[cid][envReadOffset],
								&inputBuffer[cid][envReadOffset + AVS2ENC_BLOCKSIZE * 2],
								(envWriteOffset - envReadOffset) * sizeof(float));

							memmove(&inputBuffer[cid][upsampleReadOffset],
								&inputBuffer[cid][upsampleReadOffset + AVS2ENC_BLOCKSIZE],
								(writeOffset - upsampleReadOffset) * sizeof(float));
						}
						else
						{
							memmove(inputBuffer[cid], inputBuffer[cid] + nSamplesPerChannel, writeOffset * sizeof(float));
						}

					}




					cid++;
				}


				{
					/* write out the bitstream */
					memcpy(sampleData, opb->buffer, opb->endbyte);
					numOutBytes = avs2audiopack_bytes(opb);
				}


				}
				break;

				case ID_FIL:
				{
					int fill_type = 0; //frame % 4;
					int fill_byte = 0;// 0xff;
					char fill_dft_bits[2048] = { 0 };
					int leftbytes = inputInfo.bitRate * 1024 / inputInfo.sampleRate / 8 * 2;
					////if (objflag)
					leftbytes = inputInfo.bitRate * 1.0*inputInfo.inSamples / inputInfo.sampleRate / 8 / (inputInfo.nChannels) - OUTERLOOP_reservedBYTEforHEADER - 2;
					//else
					//	leftbytes = inputInfo.bitRate * 1.0*inputInfo.inSamples / inputInfo.sampleRate / 8 / (inputInfo.nChannels) - OUTERLOOP_reservedBYTEforHEADER - 2;
					leftbytes -= 1 + opb->endbyte + 1;
					if (opb->endbit > 0) leftbytes--;
					fill_byte = leftbytes;
					if (fill_byte < 0)
					{
						printf("fill_byte %d < 0\n", fill_byte);
						fill_byte = 0;
					}

					avs2audiopack_write(opb, 6, 4);    //ID_FIL=6
					avs2audiopack_write(opb, fill_type, 4);
					switch (fill_type)
					{
					case 0:
					{
						avs2audiopack_write(opb, fill_byte, 8);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_write(opb, fill_dft_bits[0], 1);
						}
					}
					break;
					case 1:
					{
						avs2audiopack_write(opb, fill_byte, 15);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_write(opb, fill_dft_bits[0], 2);
						}
					}
					break;
					case 2:
					{
						avs2audiopack_write(opb, fill_byte, 14);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_write(opb, fill_dft_bits[0], 4);
						}
					}
					break;
					case 3:
					{
						avs2audiopack_write(opb, fill_byte, 13);
						for (i = 0; i < 8 * fill_byte; i++)
						{
							avs2audiopack_write(opb, fill_dft_bits[0], 8);
						}
					}
					break;
					default:
						printf("the value of fill_type is illegal");
						break;
					}
				}
				break;
				}//switch

				if (useBWE && elementencode_info_tianlai51[eid].elType != ID_FIL)
				{
					int channelindex;
					for (channelindex = 0; channelindex < numOfChannels; channelindex++)
					{
						int j;

						if (channelindex == 3)
							continue;

						avs2audiopack_write(opb, numAncDataBytes[channelindex], 8);

						for (j = 0; j < numAncDataBytes[channelindex]; j++)
							avs2audiopack_write(opb, ancDataBytes[channelindex][j], 8);

					}
				}

			}//for( eid=0;eid<elementnum;eid++)

			if (opb->endbit > 0)
				avs2audiopack_write(opb, 0, (8 - opb->endbit));

			/* write out the bitstream */
			memcpy(sampleData, opb->buffer, opb->endbyte);
			numOutBytes = avs2audiopack_bytes(opb);


			free(opb);


		}//if(codectype == 0)

		

		/* write one frame of encoded audio to file */
		if (numOutBytes)
		{
			if (inputInfo.outputFormat == 1)
			{
				DataLength += 2;
				fwrite(&numOutBytes, sizeof(short), 1, fOutputFile);
				DataLength += numOutBytes;
				fwrite(sampleData, sizeof(unsigned char), numOutBytes, fOutputFile);
			}
			else if (inputInfo.outputFormat == 2)
			{
				frameLength = 2 + numOutBytes + 9 + 1;
				unsigned short nCRCBitsA, nCRCBitsB;
				unsigned char CRCbuf[65536];
				memcpy(&CRCbuf[0], headbuffer, 3);
				memcpy(&CRCbuf[3], &frameLength, 2);
				memcpy(&CRCbuf[5], &headbuffer[6], 3);

				int halfLength = frameLength / 2 - 9;

				memcpy(&CRCbuf[8], &numOutBytes, 2);
				memcpy(&CRCbuf[10], sampleData, halfLength);

				nCRCBitsA = CRC16(&CRCbuf, frameLength / 2 - 1); /*calculate CRC16,6 bytes for header, 2 bytes for CRC*/

				char crcA = nCRCBitsA ^ (nCRCBitsA >> 8);
				memcpy(&headbuffer[3], &frameLength, 2);
				memcpy(&headbuffer[5], &crcA, 1);
				fwrite(headbuffer, sizeof(unsigned char), 9, fOutputFile);
				DataLength += 9;

				memcpy(&CRCbuf[0], headbuffer, 9);
				memcpy(&CRCbuf[9], &numOutBytes, 2);
				memcpy(&CRCbuf[11], sampleData, numOutBytes);

				fwrite(&numOutBytes, sizeof(short), 1, fOutputFile);
				DataLength += 2;

				fwrite(sampleData, sizeof(unsigned char), numOutBytes, fOutputFile);
				DataLength += numOutBytes;

				nCRCBitsB = CRC16(&CRCbuf, frameLength - 1);
				char crcB = nCRCBitsB ^ (nCRCBitsB >> 8);
				fwrite(&crcB, sizeof(unsigned char), 1, fOutputFile);
				DataLength += 1;
			}

			/*fwrite(&numOutBytes, sizeof(short), 1, fOutputFile);
			DataLength += 2;*/
			datalencount[eid] += numOutBytes;
		}
		//printf("numOutBytes %d\n", numOutBytes);
		frame++;

		if (inputInfo.outputFormat == 2)
		{
			fprintf(stderr, "[%d]: %d <---> %d\r", frame, frameLength, cbrLength);

			if ((cbrLength - frameLength) > 1)
				fprintf(stderr, "frame %d frameLength != targetLength\n", frame);
		}
		//if (frame > 100) break;
	}

	
	/* close encoder */	
	for(cid=0;cid<numOfChannels;cid++)
	{
		Avs2BweMDFTClose((unsigned int*)&(lfEncset[cid]->st1_in), (unsigned int*)&(lfEncset[cid]->st_common));
		if(useBWE)
		{	
			Avs2BweEncoderClose((unsigned int*)&(lfEncset[cid]->st2_in));
		}
	}

	Wave_fclose(fInputFile, bitsPerSample);

	if(inputInfo.outputFormat == 1)
		Bitstream_fclose(fOutputFile, aasfHeadSize, DataLength);
	fprintf(stderr, "encoding end!\n");

	return 0;
}