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
#include "../bwedec/avs2BweDecMDFT.h"
#include "../bwedec/decoder.h"
#include "avs2audio.h"
#include "lfdec.h"
#include "avs2decmain.h"

//////////////////////////////
int brate_mapping[16][16] = {
 {16,32,44,56,64,72,80,96,128,144,164,192,48}, //mono
 {24,32,48,64,80,96,128,144,192,256,320},      //stereo
 {128,192,256,320,384,448,512,640,720,144},    //5.1
 {192,480,256,384,576,640},                    //7.1              
 {0},
 {0},
 {48,96,128,192,256},          //quar4.0
 {152,320,480,576},            //5.1.2
 {176,384,576,704,256,448},    //5.1.4
 {216,480,576,384,768},        //7.1.2 
 {240,608,384,512,832},        //7.1.4
 {256,320,384,512,640,896},    //16ch 3rd HOA, shumin.xu 211214
 {192,256,320,384,480,512,640},//9ch  2nd HOA, shumin.xu 211214
 };

void copyright(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "\n");
}

const char *phelpstr = "avs decoder verson 2.0\n\n\
usage: avs2decoder -if <infile> -of <outfile> [options]\n\n\
RECOMMENDED:\n\
when input.wav is a mono wave:\n\
    avs2decoder -if input.avs -of output.wav\n\
when input.wav is a stereo wave:\n\
    avs2decoder -if input.avs -of output.wav\n\n\
OPTIONS:\n\
    -h or --help\tshow this list of options\n";

 void parsecmdline(int argc,
                   char *argv[],
                   char **input_filename,
                   char **output_filename,
	               char **filepath
)
{
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
		else if (!strcmp(*argv, "-fp"))
		{
			argv++;
			argc--;
			if (filepath)
				*filepath = *argv;
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

int bin2int(           /* output: recovered integer value              */
			int   no_of_bits,    /* input : number of bits associated with value */
			short *bitstream     /* input : address where bits are read          */
			)
{
	int   value, i;
	value = 0;
	for (i = 0; i < no_of_bits; i++)	
	{
		value <<= 1;
		value += (int)((*bitstream++) & MASK);
	}
	return(value);
}

short ReadBitstreamPlus(short nbits, short nb_byte, short *serial, FILE * f_serial, short offset)
{
  unsigned char byte;
  short j, k, n, *ptr;

  ptr = &serial[offset * (nbits / 4)];
  n = 0;
  for (j = 0; j < nb_byte; j++)
  {
    n += fread(&byte, sizeof(unsigned char), 1, f_serial);
    for (k = 0; k < 8; k++, ptr++)
    {
       *ptr = (byte & (short) 128) == (short) 128;
       byte <<= 1;
    }
  }
  
  return n;

}

int read_avs2file_header(long *bitstream_len, int *sampleRatecore, ChanInfo *inputInfo, int *useSuperMode, int *cpe_config,int *PCAGroupmodeHeader, FILE *f_input)
{
	short buffer[256];
	short *ptr = buffer;
	int nbits = 0;
	unsigned short audio_codec_id;
	unsigned short coding_profile;
	
	// aasf id
	int tmp = 0, i;
	fread(&tmp, 1, 4, f_input);

	if (tmp != (int)'FSAA')
	{
		printf("error aasf id %d", tmp);
	}

	//headsize
	ReadBitstreamPlus(24, 3, buffer, f_input, 0);
	tmp = bin2int(24, buffer) * 8;
	nbits = tmp;
	
	ReadBitstreamPlus((short)(tmp - 56), (short)(tmp / 8 - 7), buffer, f_input, 0);
	nbits -= 56;

	// raw stream length
	*bitstream_len = bin2int(32, ptr);
	ptr += 32;
	nbits -= 32;

    //audio_codec_id
	audio_codec_id = bin2int(4, ptr);
	ptr += 4;
	nbits -= 4;


	// resolution
	tmp = bin2int(2, ptr);
	ptr += 2;
	nbits -= 2;
	inputInfo->bitsPerSample = 8*(tmp+1);
	//	inputInfo->bitsPerSample=24;

	//coding_profile
	tmp = bin2int(3, ptr);
	ptr += 3;
	nbits -= 3;
	coding_profile = tmp;

	//anc_data_index
	tmp = bin2int(1, ptr);
	ptr += 1;
	nbits -= 1;

	// channel number index
	i = bin2int(7, ptr);
	ptr += 7;
	nbits -= 7;
	
	switch(i) {
    case 0:  //mono
		inputInfo->nChannels = 1;
		break;
	case 1:  //stereo
		inputInfo->nChannels = 2;
		break;
	case 2:  //5.1
		inputInfo->nChannels = 6;
		break;
	case 3:  //7.1
		inputInfo->nChannels = 8;
		break;
	case 4: //10.2
		inputInfo->nChannels = 12;
		break;
	case 5: //22.2
		inputInfo->nChannels = 24;
		break;
	case 11:
		inputInfo->nChannels = 16;
		break;
	case 12:
		inputInfo->nChannels = 9;
		break;
	default:
		printf("error channel numbers %d", tmp);
		exit(1);
		break;
	}


	// sampling_frequency_index
	tmp = bin2int(4, ptr);
	ptr += 4;
	nbits -= 4;

	switch(tmp) {
	case 0:
		inputInfo->sampleRate = 192000;
		break;
	case 1:
		inputInfo->sampleRate = 96000;
		break;
	case 2:
		inputInfo->sampleRate = 48000;
		break;
	case 3:
		inputInfo->sampleRate = 44100;
		break;
	case 4:
		inputInfo->sampleRate = 32000;;
		break;
	case 5:
		inputInfo->sampleRate = 24000;
		break;
	case 6:
		inputInfo->sampleRate = 22050;
		break;
	case 7:
		inputInfo->sampleRate = 16000;
		break;
	case 8:
		inputInfo->sampleRate = 8000;
		break;
	default:
		printf("error sampling rate index %d", tmp);
		exit(1);
		break;
	}

	
	*sampleRatecore = inputInfo->sampleRate/2;

	
	// bit_rate
	tmp = bin2int(4, ptr);
	ptr += 4;
	nbits -= 4;
	/*if(inputInfo->nChannels==1) i = 0;
	else if(inputInfo->nChannels==2) i = 1;
	else if(inputInfo->nChannels==6) i = 2;
	else if(inputInfo->nChannels==8) i = 3;
	tmp -= i * 16; */
	inputInfo->bitRate = brate_mapping[i][tmp] * 1000;

	// bitstream_type
	tmp = bin2int(1, ptr);
	ptr += 1;
	nbits -= 1;	

	// bwe_check_stream
	tmp = bin2int(1, ptr);
	ptr += 1;
	nbits -= 1;

	// bwe_check_obj
	tmp = bin2int(1, ptr);
	ptr += 1;
	nbits -= 1;

	//added by lumin 2014.11.18
	if(inputInfo->nChannels>=2)
	{
		//supermode_flag
		*useSuperMode = bin2int(2, ptr);
		ptr += 2;
		nbits -= 2;
		//couple_channel_config
		*cpe_config = bin2int(8, ptr);
		ptr += 8;
		nbits -= 8;
	}

	
	if(inputInfo->nChannels>2)
	{
		*PCAGroupmodeHeader = bin2int(8, ptr);
		ptr += 8;
		nbits -= 8;
	}
	
	
	return nbits;
}


int read_avs2AATF_header(int *frameLength, int *sampleRatecore, ChanInfo *inputInfo, FILE *f_input)
{
	short buffer[256];
	short *ptr = buffer;
	int nbits = 72;
	unsigned short audio_codec_id;
	unsigned short coding_profile;
	int i, tmp;

	ReadBitstreamPlus(72, 9, buffer, f_input, 0); //AATF frame header length

	//read syncword
	tmp = bin2int(12, buffer);
	ptr += 12;
	nbits -= 12;
	if(tmp != 2046){
		if (tmp != 4095)
		{
			printf("\nerror in read syncword.\n");
			return -1;
		}
	}

    //audio_codec_id
	tmp = bin2int(4, ptr);
	ptr += 4;
	nbits -= 4;
	audio_codec_id = tmp;

	//anc_data_index
	tmp = bin2int(1, ptr);
	ptr += 1;
	nbits -= 1;

	//coding_profile
	tmp = bin2int(3, ptr);
	ptr += 3;
	nbits -= 3;
	coding_profile = tmp;

	// sampling_frequency_index
	tmp = bin2int(4, ptr);
	ptr += 4;
	nbits -= 4;

	switch(tmp) {
	case 0:
		inputInfo->sampleRate = 192000;
		break;
	case 1:
		inputInfo->sampleRate = 96000;
		break;
	case 2:
		inputInfo->sampleRate = 48000;
		break;
	case 3:
		inputInfo->sampleRate = 44100;
		break;
	case 4:
		inputInfo->sampleRate = 32000;;
		break;
	case 5:
		inputInfo->sampleRate = 24000;
		break;
	case 6:
		inputInfo->sampleRate = 22050;
		break;
	case 7:
		inputInfo->sampleRate = 16000;
		break;
	case 8:
		inputInfo->sampleRate = 8000;
		break;
	default:
		printf("error sampling rate index %d", tmp);
		//exit(1);
		return -1;
		break;
	}
	
	*sampleRatecore = inputInfo->sampleRate/2;

	// raw_frame_length
	tmp = bin2int(8, ptr);
	ptr += 8;
	int tmp2 = bin2int(8, ptr);
	tmp2 = tmp2 << 8;
	*frameLength = tmp2 + tmp;
	ptr += 8;
	nbits -= 16;

	// aatf_crc_check
	tmp = bin2int(8, ptr);
	ptr += 8;
	nbits -= 8;

	// channel number index
	i = bin2int(7, ptr);
	ptr += 7;
	nbits -= 7;

	switch(i) {
    case 0:  //mono
		inputInfo->nChannels = 1;
		break;
	case 1:  //stereo
		inputInfo->nChannels = 2;
		break;
	case 2:  //5.1
		inputInfo->nChannels = 6;
		break;
	case 3:  //7.1
		inputInfo->nChannels = 8;
		break;
	case 4: //10.2
		inputInfo->nChannels = 12;
		break;
	case 5: //22.2
		inputInfo->nChannels = 24;
		break;
	case 6:
		inputInfo->nChannels = 4;
		break;
	case 7: /*5.1.2*/
		inputInfo->nChannels = 8;
		inputInfo->headChannels = 2;
		break;
	case 8: /*5.1.4*/
		inputInfo->nChannels = 10;
		inputInfo->headChannels = 4;
		break;
	case 9: /*7.1.2*/
		inputInfo->nChannels = 10;
		inputInfo->headChannels = 2;
		break;
	case 10: /*7.1.4*/
		inputInfo->nChannels = 12;
		inputInfo->headChannels = 4;
		break;
	case 11: /*16ch 3rd HOA*/
		inputInfo->nChannels = 16;
		break;
	case 12: /*9ch 2nd HOA*/
		inputInfo->nChannels = 9;
		break;
	default:
		printf("error channel numbers %d", tmp);
		//exit(1);
		return -1;
		break;
	}

	// resolution
	tmp = bin2int(2, ptr);
	ptr += 2;
	nbits -= 2;
	
	inputInfo->bitsPerSample = 8*(tmp+1);
	
	// bit_rate
	tmp = bin2int(4, ptr);
	ptr += 4;
	nbits -= 4;
	/*if(inputInfo->nChannels==1) i = 0;
	else if(inputInfo->nChannels==2) i = 1;
	else if(inputInfo->nChannels==6) i = 2;
	else if(inputInfo->nChannels==8) i = 3;
	tmp -= i * 16; */
	inputInfo->bitRate = brate_mapping[i][tmp] * 1000;

	// bitstream_type
	tmp = bin2int(1, ptr);
	ptr += 1;
	nbits -= 1;	

	// bwe_check_stream
	tmp = bin2int(1, ptr);
	ptr += 1;
	nbits -= 1;

	// bwe_check_obj
	tmp = bin2int(1, ptr);
	ptr += 1;
	nbits -= 1;

	// bwe_superframe
	tmp = bin2int(1, ptr);
	ptr += 1;
	nbits -= 1;
	
	return nbits;
}