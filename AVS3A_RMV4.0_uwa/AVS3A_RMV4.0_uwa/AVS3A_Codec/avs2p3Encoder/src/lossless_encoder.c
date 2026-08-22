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

#include <windows.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
 
//#include <libtsp.h>
//#include <libtsp/AFpar.h>
#include "av3enc.h"
#include "getcmdarg.h"
#include "../general/bweenc/encoder.h"

#undef stderr
#define stderr stdout

static const char *logo =
    "\n  AVS Lossless Audio encoder version 2.0 \n";
static const char *help =
	"\n%s\t[-of[outfilename]][-f<1,2>][-ms<+,->][-w<+,->]\n"
	"    \t[-lpc[1,127]][-e<0,1,2>][-h/-help] [-if[infilename]]\n\n"
	"  -if[inputfilename]:\t specify input file name\n"
	"  -of[outfilename]  :\t specify output file name\n"
	"  -f<1,2>          :\t set output format\n"
	"                   :\t 1: storage; 2: transport, defualt is transport format\n"
	"  -ms<+,->         :\t channel correlation encoding enable flag\n"
	"                   :\t enable ('+') or disable ('-'), default is enable\n"
	"  -w<+,->          :\t wavelet transform flag\n"
	"                   :\t enable ('+') or disable ('-'), default is enable\n"
	"  -lpcX            :\t the LPC max order, X is in (1, 127), default is 14\n"
	"  -eX              :\t the entropy encoding flag, X is in {0,1, 2},\n"
	"                   :\t 0: arithmetic coding 1: Golomb-rice coding,\n"
	"                   :\t 2: (mix coding), default is 2\n"
	"  -codec_idX       :\t the codec identity, X is in{0, 1}, default is 0\n"
	"                   :\t 0: general audio encoder 1: lossless audio encoder\n"
	"  -h/-help         :\t show this help info\n"
	"\n  for example: %s -codec_id 1 -of outfile.avsl -if infile.wav\n"
	"\n  IF ANY COMMENTS AND SUGGESTIONS, PLEASE CONTACT AVSaudio@jdl.ac.cn\n";

char* binSet[] = {"+", "-", "\0"};
char* finSet[] = {"0", "1", "2", "\0"};

const cmd_switch  swtArr[] = 
{   {"of",             2,    NULL,    0, 0},  /* output file name */
	{"f",              3,    finSet,  0, 1},  /* bitstream format option */  
    {"h",              0,    NULL,    0, 2},  /* show help */
    {"help",           0,    NULL,    0, 3},  /* show help */
	{"ms",	           2,    binSet,  0, 4},  /* channel correlation encoding */
	{"w",	           2,    binSet,  0, 5},  /* wavelet encoding */
	{"lpc",	           1,    NULL,    0, 6},  /* lpc order */
	{"e",	           1,    NULL,    0, 7},  /* entropy coding method        */
	{"codec_id",       1,    NULL,    0, 8},  /* codec identity               */
	//{"coding_profile", 1,    NULL,    0, 9},  /* coding profile identify */
    {"if",             2,    NULL,    0, 10}, /*input file name*/
    {"\0",             2,    NULL,    0, 11 } /* necessary for ending         */
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

static int getParam(cmd_params* param, cmd_option* option, int narg)
{
    int i;
    int f;
    
    i = 0;
    f = 0;
    while(i < narg){
        char* opt;
        opt = option[i].opt;
        switch(option[i].swIdx)
        {
            case 0:
                param->outFile = opt;
                break;                        
			case 1:
				if(opt && !strcmp(opt, "1"))
					param->format = 1;
				else if(opt && !strcmp(opt, "2"))
					param->format = 2;    
				break;
            case 2:
            case 3:
                param->showHelp = 1;
                break;
			case 4:
				if(opt && !strcmp(opt, "-"))
                    param->msenc   = 0;
                break;
			case 5:
				if(opt && !strcmp(opt, "-"))
                    param->wavelet = 0;
                break;
			case 6:
                param->maxLpcOrder=min(127, max(1, atol(opt)));
                break;
			case 7:
                param->entropy    =min(2,   max(0, atol(opt)));
                break;
			case 8:
				param->codecId= min(1,   max(0, atol(opt)));
				break;
			case 9:
				//param->codingProfile= min(1,   max(0, atol(opt)));
				//break;
			//case 10:
                param->inFile = opt;
                f++;
                break;
            default:
                break;             
       }
       i++;
   }
   return f;
}

int lossless_encoder(int argc, char *argv[])
{
    int frames, currentFrame,FrameCnt,samplesRead = 0;
	unsigned long samplesInput, maxBytesOutput, totalBytesWritten=0;
	unsigned int AVSVer = AVS1,useSquarePolar = 1; 
	EncFramePtr hEncoder;
	EncCfgPtr configPtr;
	int   TimeDataPcm[1024 * 2 * 8] = { 0 };
	char  TimeDataPcmBuffer[1024 * 2 * 8 * 4];
	float *pcmbuf = NULL;
    unsigned char *bitbuf = NULL;
	char *audioFileName = NULL;
    char *av3FileName = NULL;
    char *av3FileExt = NULL;
	FILE *outfile = NULL;
	FILE *fp = NULL;
	pcmfile_t *infile = NULL;
	unsigned char chRawStreamLength[7];

	int         narg;
    int         fn;
    cmd_params  param;
    cmd_option* option;
		
	option = malloc(argc*sizeof(cmd_option));
    memset(option, 0, argc*sizeof(cmd_option));    

	/*set default value*/
    param.outFile  = NULL;
    param.inFile   = NULL;
    param.showHelp = 0;
	param.format   = 2;
	
	param.msenc       = 1;
	param.wavelet     = 1;
	param.entropy     = 2;
	param.codecId     = 1;
	param.maxLpcOrder = 14;
    
    /* parse cmd line and get parameters*/    
    narg = parseCommandLine(option, swtArr, argc, argv);
    fn = getParam(&param, option, narg);

	fprintf(stderr, logo);   
    if(fn>1){
        fprintf(stderr, "\tMultiple input files are not supported\n");
		goto FAIL;
    }
	
	if(param.showHelp){
		fprintf(stderr, help, argv[0],argv[0],argv[0]);
		if(fn == 0){
			goto FAIL;
		}
	}else if(fn == 0){
		fprintf(stderr, "  No input file specified," 
			" for help type '%s -help' or '%s -h'\n", argv[0], argv[0]);
		goto FAIL;
    }
    
	audioFileName = param.inFile;
    if(param.outFile){
        if(strrchr(param.outFile, '.')){
            /* with extension */        
            av3FileName = param.outFile;
        }else
        {   
            /* without extension */
			int l = strlen(param.outFile); 
			av3FileExt = ".avsl";
			av3FileName = malloc(l+1+5);
			memcpy(av3FileName, param.outFile, l);
			memcpy(av3FileName + l,av3FileExt, 5);
			av3FileName[l+5] = '\0';
        }            
    }else{
        char *t = strrchr(audioFileName, '.');
	    int l = t ?  strlen(audioFileName) - strlen(t) : strlen(audioFileName);
		av3FileExt = ".avsl";
		av3FileName = malloc(l+1+5);
		memcpy(av3FileName, audioFileName, l);
		memcpy(av3FileName + l,av3FileExt, 5);
		av3FileName[l+5] = '\0';
    } 

    infile = malloc(sizeof(*infile));
	memset(infile, 0, sizeof(*infile));

	infile->fp = Wave_fopen(audioFileName, "rb", &infile->channels, &infile->samplerate,
		&infile->bps, &infile->samples);
	if (infile->fp == NULL)
	{
		fprintf(stderr, "Error opening the input file %s.\n", audioFileName);
		exit(EXIT_FAILURE);
	}

	//infile->f = AFopenRead(audioFileName, 
	//	&infile->samples, &infile->channels, &infile->samplerate, NULL);

	/* open the encoder library */
    hEncoder = EncOpen((int)infile->samplerate, infile->channels,
        &samplesInput, &maxBytesOutput);

	hEncoder->config.isLFE = 0;
	hEncoder->config.codecId = 1;
    pcmbuf = (float *)malloc(samplesInput*sizeof(float));
    bitbuf = (unsigned char*)malloc(maxBytesOutput*sizeof(unsigned char));

    /* put the options in the configuration struct */
    configPtr = &(hEncoder->config);
    configPtr->AVSVersion = AVSVer;

	switch (infile->bps)
	{
	case 8: 
		configPtr->resolution = 0;
		break;
	case 16:
		configPtr->resolution = 1;
		break;
	case 24:
		configPtr->resolution = 2;
		break;
	default:
		printf("unsupported PCM resolution %d \n",infile->bps);
		goto FAIL;
	}

	hEncoder->hLosslessEnc = i2r_EncoderInit(infile->channels, 
		configPtr->sampleRate, configPtr->resolution, param.maxLpcOrder);
	configPtr->msenc                 =param.msenc;
	hEncoder->hLosslessEnc->msenc    =param.msenc;
	hEncoder->hLosslessEnc->wavelet  =param.wavelet;
	hEncoder->hLosslessEnc->ucEntropy=param.entropy;
	configPtr->outputFormat = param.format;
	configPtr->codecId = param.codecId;
	hEncoder->headSize = 0;
	
    /* open the av3 output file */
    outfile = fopen(av3FileName, "wb");
    if (!outfile) {
        fprintf(stderr, "Couldn't create output file %s\n", av3FileName);
		goto FAIL;
    }
	BitstreamOpen(outfile);

    if (outfile)
    {
        int showcnt = 0;
        char* format_string;
		float ts;
        int hh;
        int mm;
        float ss;
        long begin = GetTickCount();
        
		if (infile->samples){
            frames = ((infile->samples + 1023) >> 10)+1 ;
        }else{
          frames = 0;
		}

		currentFrame = 0;
		FrameCnt = 0;

        if(!frames){
            fprintf(stderr, "  empty audio file\n");
			goto FAIL;
        }
		
        switch(configPtr->outputFormat) {
        case 1:
			format_string = "AASF";		
        	break;
        case 2:
			format_string = "AATF";
			break;
		default:
			fprintf(stderr,"unsupported format");
        }

		ts = (float) infile->samples / infile->samplerate;
        hh = (int) ts / 3600;
        mm = (int) (ts - hh*3600) / 60;
        ss = ts - (int)(ts / 60)*60;
        
        fprintf(stderr, "  input file name  : \t%s\n", audioFileName);
        fprintf(stderr, "  output file name : \t%s\n", av3FileName);
		fprintf(stderr, "  output format    : \t%s\n", format_string);
        fprintf(stderr, "  sampling rate    : \t%d Hz\n", (int)infile->samplerate);
        fprintf(stderr, "  channel number   : \t%d\n", infile->channels);
        fprintf(stderr, "  playback time    : \t%dh-%dm-%.2fs\n", hh, mm, ss);
        
        fprintf(stderr, 
            "+---------------+-------------+------------+--------------------+-----------+\n"
            "   FRAME        |  BITRATE    | PERCENTAGE | ELAPESED/ESTIMATED | PLAY/CPU \n");            

		int k = 0;
		int samplesCount = 0;
        /* encoding loop */
		for ( ; ;) 
		{
			 /*wavread by shumin.xu 20211105*/
			 int bytesWritten, pcmindex;
			 samplesRead = fread(TimeDataPcmBuffer, infile->bps / 8, samplesInput, infile->fp);

			 for (pcmindex = 0; pcmindex < samplesInput; pcmindex++)
			 {
				 int pcmtmp;
				 short pcmshorttmp;
				 char *ptchar = &pcmtmp;

				 if (infile->bps == 24)
				 {
					 memcpy(ptchar, TimeDataPcmBuffer + pcmindex * (infile->bps / 8), 3);
					 pcmtmp = (pcmtmp << 8);
					 pcmtmp = (pcmtmp / 256);
				 }
				 else if (infile->bps == 16)
				 {
					 ptchar = &pcmshorttmp;
					 memcpy(ptchar, TimeDataPcmBuffer + pcmindex * (infile->bps / 8), 2);
					 pcmtmp = pcmshorttmp;
				 }
				 TimeDataPcm[pcmindex] = pcmtmp;
			 }

			 for (k = 0; k < infile->channels; k++)
			 {
				 int i;
				 for (i = 0; i < samplesRead / infile->channels; i++)
					 pcmbuf[i*infile->channels + k] = (float)TimeDataPcm[i*infile->channels + k];
			 }

			 if (infile->bps == 24)
			 {
				 int i;
				 for (i = 0; i < samplesInput; i++)
					 pcmbuf[i] /= 256.0;
			 }

			 samplesCount += samplesRead;
			 if (samplesCount > infile->samples*infile->channels)
				 break;


			 /* call the actual encoding routine */
 			 bytesWritten = EncEncode(hEncoder,
										(int *)pcmbuf,
										samplesRead,
										bitbuf);				 
		
			 if (bytesWritten){
				 currentFrame++;
				 showcnt--;
				 totalBytesWritten += bytesWritten;
			 }
			 //FrameCnt++;

			 if ((showcnt <= 0) || !bytesWritten)
			 {
				 double timeused;
				 char percent[MAX_PATH + 20];
				 timeused = (GetTickCount() - begin) * 1e-3;

				 if (currentFrame && (timeused > 0.1))
				 {
					 showcnt += 1;
					 {
						 int percentage = currentFrame*100/frames;                        
						 float currentBitrate = (float)totalBytesWritten  / 125.0 /(ts * currentFrame / frames);
						 float estimated = timeused * frames / currentFrame;
						 float plx = (1024.0 * currentFrame / infile->samplerate) / timeused;

						 fprintf(stderr,   
								"\r%5d/%-5d     |  %5.1f kbps |  %3d%%      | %6.1f s/%6.1f s  | %.1f X ",
								currentFrame, frames, currentBitrate, percentage, timeused, estimated, plx);                                                
					 } 

					 fflush(stderr);
					 if (frames != 0)
					 {
						 sprintf(percent, "%.2f%% encoding %s",
								100.0 * currentFrame / frames, audioFileName);
						 SetConsoleTitle(percent);
					 }
				 }
			 }

			 /*renew aatf header*/ //shumin.xu 20211022
			 if (configPtr->outputFormat == 2)
			 {
				 fseek(outfile, -(bytesWritten+4), SEEK_CUR);
				 int totalFrameLength = 7/*aatfHeadSize*/ + bytesWritten + 1;
				 memcpy(&bitbuf[3], &totalFrameLength, sizeof(short));
				 fwrite(&totalFrameLength, sizeof(short), 1, outfile);

				 /*write frame error check*/
				 fseek(outfile, 0, SEEK_END); //fseek(outfile, (2+bytesWritten), SEEK_CUR);
				 unsigned short nCRCBitsB;
				 nCRCBitsB = CRC16(bitbuf, totalFrameLength - 1);
				 char crcB = nCRCBitsB ^ (nCRCBitsB >> 8);
				 fwrite(&crcB, sizeof(unsigned char), 1, outfile);
			 }

			 /* all done, bail out */
			 if (!samplesRead && !bytesWritten)
				 break ;
		}
		fprintf(stderr, "\n\n");

		// write AASF header (renew raw_stream_length)
		if (configPtr->outputFormat == 1)
		{
			fseek(outfile, 4L, SEEK_SET);
			chRawStreamLength[0] = (hEncoder->headSize>>16)&0xFF;
			chRawStreamLength[1] = (hEncoder->headSize>>8)&0xFF;
			chRawStreamLength[2] = hEncoder->headSize&0xFF;

			chRawStreamLength[3] = (totalBytesWritten>>24)&0xFF;
			chRawStreamLength[4] = (totalBytesWritten>>16)&0xFF;
			chRawStreamLength[5] = (totalBytesWritten>>8)&0xFF;
			chRawStreamLength[6] = totalBytesWritten&0xFF;
			
			fwrite(chRawStreamLength, 1, 7, outfile);
		}
		fclose(outfile);
	}
    EncClose(hEncoder);
    //AFclose(infile->f);
	Wave_fclose(infile->fp, infile->bps);

FAIL:
	if(option) free(option);
	if(infile) free(infile);
    if(pcmbuf) free(pcmbuf);
    if(bitbuf) free(bitbuf);

    return 0;
}




