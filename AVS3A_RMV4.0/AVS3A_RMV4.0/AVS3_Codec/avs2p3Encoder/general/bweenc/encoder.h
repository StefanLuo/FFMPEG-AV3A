/******************************************************************************
		        VM1.0 of Audio Video Coding Standard, 
			copyright  (2013)  All Rights Reserved
 
  This software module was originally developed by Beijing Angel Voice 
Digital Technology Co.,Ltd. 
 
  This work(including software and documentation) is provided by the copyright
hoder under the following license:By obtaining, using and/or copying this work, 
you (the licensee) agree that you have read, understood, and will comply with the
following terms and conditions. without permission from Beijing Angel Voice Digital
Technology Co.,Ltd, any forms of copy,modification and distribution are forbidden.
The name and trademarks of copyright holders may NOT be used in advertising or
publicity pertaining to the software without specific, written prior permission.
Title to copyright in this software and any associated documentation will at all 
times remain with copyright holders, and all right reserved.
*******************************************************************************/
#ifndef ENCODER_HHH
#define ENCODER_HHH
#include <stdio.h>



typedef struct
{
	//short FileFormat;            /* File format */
	int bitRate;
	short bitsPerSample;
	long sampleRate;
	short nChannels;

	int outputFormat;
	int use_mono_encode;		/* force to encode in mono */	
	int codec_id;
	int coding_profile;

	//shumin.xu 20210105
	int channel_number_index;
	int headflag;            

	int anc_data_index;      
	int inSamples;

	int objBitRate;
	int objNum;
	
} ChanInfo;

FILE *Wave_fopen(char *Filename, char *Mode, short *NumOfChannels, long *SamplingRate, short *BitsPerSample,
				 long *DataSize);

void Wave_fclose(FILE *FilePtr, short BitsPerSample);

extern int setBWEbandWidth(int *bandWidth,int idx);
extern int Avs2BweEncoderOpen(unsigned int *st_in, int bitrate, int sampleRateCore, int numChannels, int *bandWidth, int *config_idx);
extern int Avs2BweEncoder(
			  unsigned char *ancBytes,    /*!< pointer to ancillary data bytes */
			  int *numAncDataBytes,
			  int *st_in,
			  int *st_common,
			  int bitRate,
			  int bitsPerSample);

extern int Avs2BweEncoderClose(unsigned int *st_in);


#endif