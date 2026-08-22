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

#include "..\bweenc\encoder.h"
#define BAND_LENGTH 2048
static void ExtractFormat(char *FormatString, short *ReadFlag, short *AppendFlag);
/*____________________________________________________________________________
 |
 |  FUNCTION NAME: Wave_fopen
 |____________________________________________________________________________
*/
FILE *Wave_fopen(char *Filename, char *Mode, short *NumOfChannels, long *SamplingRate, 
	short *BitsPerSample, long *SampleCount)
{
	FILE *FilePtr, *FilePtrAppend = NULL;
	short ReadFlag;
	short AppendFlag;
	char  WaveTag[4];
	long  PCM_Size = 0L;
	long  FileSize = 0L;		/* Actual size will be replaces in "Wave_fclose" -procedure */
	long  SampleCount1 = 0L;
	long  DataSize = 0L, DataSize1 = 0L;
	long  FormatSize = 16L;
	short FormatTag = 1;		/* Format Tag (PCM=1) */
	short BytesPerSample;
	long  BytesPerSecond;
	long  longtmp;
	short shorttmp;
	long chunkSize, dsSize, junkSize;
	long sRate;
	int i;
	ExtractFormat(Mode, &ReadFlag, &AppendFlag);
	if (!ReadFlag) {
		if ((FilePtr = fopen(Filename, Mode)) == NULL) 
		{
			return FilePtr;
		}
		if (!AppendFlag) {
			if (*BitsPerSample != 8 && *BitsPerSample != 16 && *BitsPerSample != 24 && *BitsPerSample != 32) {
				fprintf(stderr, "\n\n");
				fprintf(stderr, "\n ERROR: Only values 16 or 8 or 24allowed for parameter 'BitsPerSample' in function Wave_fopen");
				fprintf(stderr, "\n        [value: %d received]", *BitsPerSample);
				fprintf(stderr, "\n\n");
				exit(-1);
			}
			BytesPerSample = (short)(*BitsPerSample / 8 * *NumOfChannels);
			BytesPerSecond = *SamplingRate * BytesPerSample;
			/* RIFF chunk - 12 bytes */
			fwrite("RIFF", 1, 4, FilePtr);
			fwrite(&FileSize, 4, 1, FilePtr);
			fwrite("WAVE", 1, 4, FilePtr);
			/* FORMAT chunk - 24 bytes */
			fwrite("fmt ", 1, 4, FilePtr);
			fwrite(&FormatSize, 4, 1, FilePtr);
			fwrite(&FormatTag, 2, 1, FilePtr);
			fwrite(NumOfChannels, 2, 1, FilePtr);
			sRate = (*SamplingRate);
			fwrite(&sRate, 4, 1, FilePtr);
			fwrite(&BytesPerSecond, 4, 1, FilePtr);
			fwrite(&BytesPerSample, 2, 1, FilePtr);
			fwrite(BitsPerSample, 2, 1, FilePtr);
			/* DATA chunk - Data length + 8 bytes */
			fwrite("data", 1, 4, FilePtr);
			fwrite(&PCM_Size, 4, 1, FilePtr);
		}
		return FilePtr;
	}
	else {
		if ((FilePtr = fopen(Filename, Mode)) == NULL) 
		{
			return FilePtr;
		}
		if (AppendFlag) 
		{
			FilePtrAppend = FilePtr;
		}
		fseek(FilePtr, 0, SEEK_SET);
		/* RIFF chunk - 12 bytes */
		fread(&WaveTag, 4, 1, FilePtr);
		if (strncmp("RIFF", WaveTag, 4) != 0) {
			if (strncmp("RF64", WaveTag, 4) == 0);
			else if (strncmp("bw64", WaveTag, 4) == 0);
			else return NULL;
		}
		fread(&FileSize, 4, 1, FilePtr);
		fread(&WaveTag, 4, 1, FilePtr);
		if (strncmp("WAVE", WaveTag, 4) != 0) {
			return NULL;
		}

#if 1
		fread(&WaveTag, 4, 1, FilePtr);
		if (strncmp("JUNK", WaveTag, 4) == 0)
		{
			fread(&junkSize, 4, 1, FilePtr);
			fseek(FilePtr, junkSize, SEEK_CUR);
		}

		/* ds64 chunk - 36+ bytes */
		if (strncmp("ds64", WaveTag, 4) == 0)
		{
			fread(&dsSize, 4, 1, FilePtr);
			fread(&FileSize, 8, 1, FilePtr);
			fread(&DataSize, 8, 1, FilePtr);
			fread(&SampleCount1, 8, 1, FilePtr); //sample count
			fread(&longtmp, 4, 1, FilePtr); //tableLength
			if (dsSize > 28)
				fseek(FilePtr, (dsSize - 28), SEEK_CUR);
		}

		while (1)
		{
			/* FORMAT chunk - 24 bytes*/
			if (strncmp("fmt ", WaveTag, 4) == 0)
			{
				fread(&chunkSize, 4, 1, FilePtr); //0x28
				fread(&shorttmp, 2, 1, FilePtr);  //0xFFFE
				fread(NumOfChannels, 2, 1, FilePtr);
				fread(SamplingRate, 4, 1, FilePtr);
				fread(&longtmp, 4, 1, FilePtr); //bytesPerSecond
				fread(&BytesPerSample, 2, 1, FilePtr); //blockAlignment
				fread(BitsPerSample, 2, 1, FilePtr);
				//fread(&shorttmp, 2, 1, FilePtr); //cbSize
				/* DATA chunk - Data length + 8 bytes */
				if (chunkSize > 16)
					fseek(FilePtr, chunkSize - 16, SEEK_CUR);
			}
			else if (strncmp("data", WaveTag, 4) == 0)
				break;
			else if (strncmp("JUNK", WaveTag, 4) == 0 || strncmp("ds64", WaveTag, 4) == 0);
			else {
				fread(&junkSize, 4, 1, FilePtr);
				fseek(FilePtr, junkSize, SEEK_CUR);
			}

			fread(&WaveTag, 4, 1, FilePtr);

		}

		fread(&DataSize1, 4, 1, FilePtr);

		if (DataSize1 != 0xFFFFFFFF)
			*SampleCount = DataSize1 / BytesPerSample;
		else
			*SampleCount = DataSize / BytesPerSample;

#else
		/* FORMAT chunk - 24 bytes */
		fread(&WaveTag, 4, 1, FilePtr);
		if (strncmp("fmt ", WaveTag, 4) != 0) {
			return NULL;
		}
		fread(&chunk_size, 4, 1, FilePtr);
		fread(&shorttmp, 2, 1, FilePtr);
		fread(NumOfChannels, 2, 1, FilePtr);
		fread(SamplingRate, 4, 1, FilePtr);
		fread(&longtmp, 4, 1, FilePtr);
		fread(&BytesPerSample, 2, 1, FilePtr);
		fread(BitsPerSample, 2, 1, FilePtr);
		/* DATA chunk - Data length + 8 bytes */
		if (chunk_size - 16 > 0)
		{
			for (i = 0; i < chunk_size - 16; i++)
				fread(&shorttmp, 1, 1, FilePtr);
		}
		do {
			shorttmp = fread(&WaveTag, 4, 1, FilePtr);
			if (shorttmp == 0)
				return NULL;
			if (strncmp("data", WaveTag, 4) == 0) {
				break;
			}
			fread(&chunk_size, 4, 1, FilePtr);
			for (i = 0; i < chunk_size; i++)
			{
				if (fread(&shorttmp, 1, 1, FilePtr) != 0)
					;
			}
		} while (1);

		fread(DataSize, 4, 1, FilePtr);
		if (FileSize!=0)
			*DataSize = *DataSize / BytesPerSample;
#endif
		if (AppendFlag) return FilePtrAppend;
		return FilePtr;
	}
}
/*____________________________________________________________________________
 |
 |  FUNCTION NAME: Wave_fclose
 |____________________________________________________________________________
*/
void Wave_fclose(FILE *FilePtr, short BitsPerSample)
{
	long DataLen;
	long FileLen;
	long StartPos;
	long EndPos;
	if (BitsPerSample != 8 && BitsPerSample != 16 && BitsPerSample != 24 && BitsPerSample != 32) {
		fprintf(stderr, "\n\n");
		fprintf(stderr, "\n ERROR: Only values 16 or 8 or 24 allowed for parameter 'BitsPerSample' in function Wave_fclose");
		fprintf(stderr, "\n\n");
		exit(-1);
	}
	EndPos = ftell(FilePtr);
	fseek(FilePtr, 0, SEEK_SET);
	StartPos = ftell(FilePtr);
	FileLen = ((EndPos - StartPos) / (BitsPerSample / 16)) - 8;
	DataLen = FileLen - 44 + 8;
	fseek(FilePtr, 4, SEEK_SET);
	fwrite(&FileLen, 1, 4, FilePtr);
	fseek(FilePtr, 40, SEEK_SET);
	fwrite(&DataLen, 1, 4, FilePtr);
	fclose(FilePtr);
}
/*____________________________________________________________________________
 |
 |  FUNCTION NAME: ExtractFormat
 |____________________________________________________________________________
*/
static void ExtractFormat(char *FormatString, short *ReadFlag, short *AppendFlag)
{
	*ReadFlag = 0;
	*AppendFlag = 0;
	if(strchr(FormatString, (int)'a') != NULL) *AppendFlag = 1;
	if(strchr(FormatString, (int)'A') != NULL) *AppendFlag = 1;
	if(strchr(FormatString, (int)'r') != NULL) *ReadFlag = 1;
	if(strchr(FormatString, (int)'R') != NULL) *ReadFlag = 1;
}

int read_data(      /* return: number of data successfully read */
			  FILE  *fp,        /* input : data file (16-bit words)         */
			  float data[],     /* output: speech data                      */
			  int   size        /* input : number of samples                */
			  )
{
	int   i, n;
	short data16[4*BAND_LENGTH];
	n = fread((void *)data16, sizeof(short), size, fp);
	for (i = 0; i < n; i++)  
	{
		data[i] = (float) data16[i];
	}
	for (i = n; i < size; i++)  
	{
		data[i] = 0.0;
	}
	return n;
}

void writ_data(
			   float data[],  /* input : data              */
			   int   size,    /* input : number of samples */
			   FILE  *fp      /* output: file pointer      */
			   )
{
	short data16[4*BAND_LENGTH];
	int   i;
	float temp;
	for (i = 0; i < size; i++)
	{
		temp = data[i];
		if (temp >= 0.0)
			temp += 0.5;
		else
			temp -= 0.5;
		if (temp >  32767.0 ) temp =  32767.0;
		if (temp < -32767.0 ) temp = -32767.0;
		data16[i] = (short) temp;
	}
	fwrite(data16, sizeof(short), size, fp);
	return;
}