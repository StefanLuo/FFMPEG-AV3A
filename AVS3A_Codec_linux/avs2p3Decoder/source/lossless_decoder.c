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

#ifdef _WIN32
#include <windows.h>
#define off_t __int64
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "AFpar.h"

#include "av3dec.h"
#include "degetcmdarg.h"
#include "funcs.h"

int header_type;
/* globals */
char *progName;

char *file_ext[] =
{
    NULL,
    ".wav",
    ".aif",
    NULL
};

const unsigned char defChMappings[MAX_CHANNELS][MAX_CHANNELS]={
  /*FL	  FR     FC    LF    LS    RS    BL    BR*/
  { 0xFF, 0xFF,  FC,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, /* mono    */
  { FL,   FR,    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, /* stereo  */
  { FL,   FR,    0xFF, LF,   0xFF, 0xFF, 0xFF, 0xFF}, /* 2.1     */
  { FL,   FR,    FC,   LF,   0xFF, 0xFF, 0xFF, 0xFF}, /* 3.1     */
  { FL,   FR,    FC,   LF,   LS,   0xFF, 0xFF, 0xFF}, /* 4.1     */
  { FL,   FR,    FC,   LF,   LS,   RS,   0xFF, 0xFF}, /* 5.1     */
  { FL,   FR,    FC,   LF,   LS,   RS,   BL,   0xFF}, /* 6.1     */
  { FL,   FR,    FC,   LF,   LS,   RS,   BL,   BR  }, /* 7.1     */
};

void usage(void)
{
    fprintf(stdout, "\nUsage:\n");
    fprintf(stdout, "%s [options]\n", progName);
    fprintf(stdout, "Options(- or /):\n");
    fprintf(stdout, " -h or -help Show this help screen.\n");
	fprintf(stdout, " -ifX        Set input filename(X input file name, .avsl).\n");
    fprintf(stdout, " -ofX        Set output filename(X output file name).\n");
    fprintf(stdout, " -bX         Set output sample format. Valid values for X are:\n");
	fprintf(stdout, "             0:   8 bit PCM data.\n");
    fprintf(stdout, "             1:  16 bit PCM data (default).\n");
    fprintf(stdout, "             2:  24 bit PCM data.\n");
	fprintf(stdout, " -codec_idX  Set codec identity. Valid values for X are:\n");
	fprintf(stdout, "             0:  general audio encoder(default).\n");
    fprintf(stdout, "             1:  lossless audio encoder.\n");
    fprintf(stdout, "Example:\n");    
    fprintf(stdout, "       %s -codec_id1 -if infile.avsl\n", progName);
    fprintf(stdout, "       %s -codec_id1 -of outfile.wav -if infile.avsl\n", progName);
    return;
}

//////////////////////////////////////////////////////////////////// 
//
//  decodeAVSfile():  the main av3 decoding function.
//  input parameter:
//		av3file:   input coded audio file(*.avsl) name
//		sndfile:   output file name
//		object_type: object type, now only basic level supported
//		outputFormat: the output file format(*.wav or *.aif)
//		SampleFormat: output PCM data format, 8bits/16bits/24bits integer
//		header_type: header type, now just AV3_HEAD_DEFAULT supported
//  output parameter:
//		Zero
//    
////////////////////////////////////////////////////////////////////
int decodeAVSfile(char *av3file, char *sndfile, int object_type,
				  int outputFormat, int SampleFormat, int header_type)
{
    AV3DecFrameInfoPtr hDecoder;
    AV3DecCfgPtr config;
	int bread;
	int normal;
	int samplefmt = 5;

    av3_buffer b;
    memset(&b, 0, sizeof(av3_buffer));

    b.infile = fopen(av3file, "rb");
    if (b.infile == NULL)
    {
        /* unable to open file */
        fprintf(stderr, "Error opening file: %s\n", av3file);
        return 1;
    }

    if (!(b.buffer = (unsigned char*)av3_malloc(AV3DEC_MIN_IBUFSIZE*MAX_CHANNELS)))
    {
        fprintf(stderr, "Memory allocation error\n");
        return 0;
    }
    memset(b.buffer, 0, AV3DEC_MIN_IBUFSIZE*MAX_CHANNELS);

    bread = fread(b.buffer, 1, AV3DEC_MIN_IBUFSIZE*MAX_CHANNELS, b.infile);
    b.bytes_into_buffer = bread;
    b.bytes_consumed = 0;
    b.file_offset = 0;

    if (bread != AV3DEC_MIN_IBUFSIZE*MAX_CHANNELS)
        b.at_eof = 1;

    hDecoder = AV3DecOpen();

    /* Set the default object type and samplerate */
    /* This is useful for RAW av3 files */
    config = AV3DecGetCurrentConfiguration(hDecoder);
    config->av3ObjectType = object_type;
    config->outputFormat = outputFormat;
	config->pcmFormat = SampleFormat;
    normal=AV3DecSetConfiguration(hDecoder, config);    

    fill_buffer(&b);
	/* Check if some error initializing occured */
	if ((hDecoder == NULL)||(!normal))
	{        
        fprintf(stderr, "Error initializing decoder library.\n");
        if (b.buffer)
            free(b.buffer);
        AV3DecClose(hDecoder);
        fclose(b.infile);
        return 1;
    }

    /* print av3 file info */
	fprintf(stderr, "input file name: \t%s\n", av3file);
    fprintf(stderr, "%s file info:\n", av3file);
    switch (header_type)
    {
		case 1:
			fprintf(stderr, "input file format:\t AVS2P3_AASF\n");
			break;
		case 2:
			fprintf(stderr, "input file format:\t AVS2P3_AATF\n");
			break;
		default:
			fprintf(stderr, "input file format:\t unknown format\n");
			break;
    }
	fclose(b.infile);

	/* Open input bitstream file */
	open_bitstream(av3file);

	switch(SampleFormat){
	case AV3_PCM_8BIT:
		samplefmt = FD_UINT8   + 256*2;
		break;		
	case AV3_PCM_16BIT:
		samplefmt = FD_INT16   + 256*2;
		break;
	case AV3_PCM_24BIT:
		samplefmt = FD_INT24   + 256*2;
		break;
	case AV3_PCM_32BIT:
		samplefmt = FD_INT32   + 256*2;
		break;
	case AV3_PCM_FLOAT:
		samplefmt = FD_FLOAT32 + 256*2;
		break;	
	default:
		samplefmt = FD_INT16   + 256*2;
		break;
	}

	sam_decode(/*av3file,*/samplefmt, 
		sndfile, hDecoder, header_type);

	close_bitstream();

    AV3DecClose(hDecoder);

    fclose(b.infile);

    if (b.buffer)
        free(b.buffer);

	return 0;
}

char  *cdecid_param[]={"0","1","\0"};
char  *format_param[]={"0","1","2","\0"};
char  *pcmfmt_param[]={"0","1","2","3","4","\0"};

static const cmd_switch  swtArr[] = 
{                                   
    {"f",     3, format_param,   0, 0},   /* output format */
    {"b",     3, pcmfmt_param,   0, 1},   /* sample bit format */
	{"codec_id", 3, cdecid_param,0, 2},   /* codec identity */
    {"of",     2, NULL,			 0, 3},   /* output file name */
    {"h",     0, NULL,			 0, 4},   /* show help */
    {"help",  0, NULL,			 0, 5},   /* show help */
    {"if",     2, NULL,			 0, 6},   /* input file name */
	{"\0",    0, NULL,			 0, 7}    /* necessary for ending */
};

int lossless_decoder(int argc, char *argv[])   
{
    int result;
	int begin;
    int infoOnly = 0;
    int object_type = AVS_LOSSLESS;
    int def_srate = 0;
    char *fnp;
	
	char  header_tag[4];			//for head judgement
	FILE* av3file;
	int   nch;
	int	  Sfreq, SfreqIdx;

    int         outfile_set = 0;
    int         narg;
    cmd_params  param;
    cmd_option* option;
	char        output_filename[80];	
	int         temp;

	progName = argv[0];
    fprintf(stderr, "\nAVS Audio decoder release ver2.0.\n");

    /* begin process command line */
	option = av3_malloc(argc*sizeof(cmd_option));
    memset(option, 0, argc*sizeof(cmd_option));    
    
    /* set default value */
	param.codecId   = 1;
    param.outformat = 1;
    param.sampleformat = 1; //16bit
    param.outFile   = NULL;
    param.inFile    = NULL;
    param.showHelp  = 0;
	param.profile   = -1; /* up to bit-stream */
    
    /*parse cmd line and get parameters*/    
    narg = parseCommandLine(option, swtArr, argc, argv);
    outfile_set = getParam(&param, option, narg);

    /* Print help if requested or no infile */
    if ((param.inFile==NULL)||param.showHelp){
        usage();
        return 1;
    }

	/* recognize header type */
	if ((av3file = fopen(param.inFile, "rb"))==NULL){
		usage();
		return 0;
		
	}
	fread(header_tag, 4, 1, av3file);
	
	if(memcmp(header_tag, "AASF", 4) == 0)
		header_type = 1;
	else if(header_tag[0] == (char)0xff 
		&& ((header_tag[1]&0xf0) == 0xf0))
		header_type = 2;
	else if (header_tag[0] == (char)0x7f
		&& ((header_tag[1] & 0xf0) == 0xe0))
		header_type = 2;
	else
		header_type = -1;

	fclose(av3file);
#ifdef _WINDOWS_
	begin = GetTickCount();
#endif

    /* set default outfile name */
    if(!outfile_set)
    {
		strcpy(output_filename, param.inFile);  

        fnp = (char *)strrchr(output_filename,'.');

        if (fnp)
            fnp[0] = '\0';

        strcat(output_filename, file_ext[param.outformat]);
		param.outFile = output_filename;
    }
	/* decoding bit stream */
	result = decodeAVSfile(param.inFile, param.outFile, 
		object_type, param.outformat, param.sampleformat, 
		header_type);

	if ( (!result) )
    {
#ifdef _WINDOWS_
		float dec_length = 
			(float)(GetTickCount()-begin)/1000.0;
        SetConsoleTitle("AV3decoder");
#endif
    }
    return 0;
}
