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
#ifndef DECODER_H
#define DECODER_H

#include <stdio.h>
#include "dec.h"

typedef struct
{
	short FileFormat;            /* File format */
	int bitRate;
	short bitsPerSample;
	long sampleRate;
	short nChannels;
	short headChannels;

	int ref_frame_interval;
	int use_mono_encode;		/* force to encode in mono */	
} ChanInfo;

FILE *Wave_fopen(char *Filename);

void Wave_fclose(FILE *FilePtr, int NumOfChannels, int SamplingRate, int BitsPerSample);


void write_data(
			   float data[],  /* input : data              */
			   int   size,    /* input : number of samples */
			   	int  bitsPerSample,    /* input : bitsPerSample */
			   FILE  *fp      /* output: file pointer      */
			   );

extern int Avs2BweDecoderOpen(unsigned int *st_in, int bitrate, int sampleRateCore, int numChannels, int *bandWidth,int *config_idx);
extern int Avs2BweDecoder(int bitRate, BWEBITSTREAM* pStreamBwe, unsigned int *pst_in, unsigned int *pst_common,int bitsPerSample, int usePS);

extern int Avs2BweDecoderClose(unsigned int *st_in);

#endif
