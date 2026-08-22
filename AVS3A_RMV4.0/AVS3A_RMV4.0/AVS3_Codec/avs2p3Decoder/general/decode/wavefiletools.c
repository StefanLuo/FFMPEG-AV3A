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

//#include "encoder.h"
#define BAND_LENGTH 2048
static void ExtractFormat(char *FormatString, short *ReadFlag, short *AppendFlag);
/*____________________________________________________________________________
 |
 |  FUNCTION NAME: Wave_fopen
 |____________________________________________________________________________
*/
FILE *Wave_fopen(char *Filename)
{
	FILE *FilePtr;
	int i = 0;

	if ((FilePtr = fopen(Filename, "wb")) == NULL)
	{
		return FilePtr;
	}
	/* RIFF chunk - 12 bytes *//* ds64/JUNK chunk - 36 bytes *//* FORMAT chunk - 24 bytes *//* DATA chunk - 8 bytes */
	for (i = 0; i < 44; i++)
		fwrite(&i, 1, 1, FilePtr);	//jump wave head - 100 bytes, these data will  be finished by wavs_fclose_wb()

	return FilePtr;
}
/*____________________________________________________________________________
 |
 |  FUNCTION NAME: Wave_fclose
 |____________________________________________________________________________
*/
void Wave_fclose(FILE *FilePtr, int NumOfChannels, int SamplingRate, int BitsPerSample)
{
	short formatType = 0xFFFE; //WAVE_FORMAT_EXTENSIBLE = 0xFFFE
	short cbSize = 0;
	int dsSize = 28;
	int junkSize = 28;

	short shorttmp = 0;
	int   longtmp = 0;

	unsigned long long DataLen = 0;
	unsigned long long FileLen = 0L;
	unsigned long long StartPos = 0;
	unsigned long long EndPos = 0;
	unsigned long long SampleCount = 0L;

	if (BitsPerSample != 8 && BitsPerSample != 16 && BitsPerSample != 24 && BitsPerSample != 32) {
		fprintf(stderr, "\n\n");
		fprintf(stderr, "\n (%s)ERROR: Only values 32/24/16/8 allowed for parameter 'BitsPerSample\n", __FUNCTION__);
		fprintf(stderr, "\n\n");
		BitsPerSample = 24;
	}

	long  FormatSize = 16L;
	short FormatTag = 1;		//WAVE_FORMAT_PCM = 0x0001
	short BytesPerSample = 0;
	long  BytesPerSecond = 0;

	BytesPerSample = (short)(BitsPerSample / 8 * NumOfChannels);
	BytesPerSecond = SamplingRate * BytesPerSample;

#ifdef _MSC_VER
	EndPos = _ftelli64(FilePtr);
	_fseeki64(FilePtr, 0, SEEK_SET);
	StartPos = _ftelli64(FilePtr);
#else
	EndPos = ftello(FilePtr);
	fseeko(FilePtr, 0, SEEK_SET);
	StartPos = ftello(FilePtr);
#endif
	FileLen = (EndPos - StartPos) - 8;

	if (FileLen > 0xFFFFFFFF)
	{
		/* RIFF chunk - 12 bytes */
		fwrite("RF64", 1, 4, FilePtr);  //bw64
		longtmp = 0xFFFFFFFF;
		fwrite(&longtmp, 4, 1, FilePtr);
		fwrite("WAVE", 1, 4, FilePtr);

		/* ds64 chunk - 36 bytes */
		fwrite("ds64", 1, 4, FilePtr);
		fwrite(&dsSize, 4, 1, FilePtr);
		fwrite(&FileLen, 8, 1, FilePtr);
		DataLen = FileLen - 80 + 8;
		fwrite(&DataLen, 8, 1, FilePtr);
		fwrite(&SampleCount, 8, 1, FilePtr);
		longtmp = 0;
		fwrite(&longtmp, 4, 1, FilePtr);

		/* FORMAT chunk - 24 bytes */
		fwrite("fmt ", 1, 4, FilePtr);
		fwrite(&FormatSize, 4, 1, FilePtr);
		fwrite(&formatType, 2, 1, FilePtr);
		fwrite(&NumOfChannels, 2, 1, FilePtr);
		fwrite(&SamplingRate, 4, 1, FilePtr);
		fwrite(&BytesPerSecond, 4, 1, FilePtr);
		fwrite(&BytesPerSample, 2, 1, FilePtr); //blockAlignment
		fwrite(&BitsPerSample, 2, 1, FilePtr);
		//fwrite(&cbSize, 2, 1, FilePtr);

		/* data chunk - 8 bytes*/
		fwrite("data", 1, 4, FilePtr);
		longtmp = 0xFFFFFFFF;
		fwrite(&longtmp, 4, 1, FilePtr);
	}
	else {
		/* RIFF chunk - 12 bytes */
		fwrite("RIFF", 1, 4, FilePtr);
		fwrite(&FileLen, 1, 4, FilePtr);
		fwrite("WAVE", 1, 4, FilePtr);

		/* JUNK chunk - 36 bytes */
		/*fwrite("JUNK", 1, 4, FilePtr);
		fwrite(&junkSize, 4, 1, FilePtr);
		fseek(FilePtr, junkSize, SEEK_CUR);*/

		/* FORMAT chunk - 24 bytes */
		fwrite("fmt ", 1, 4, FilePtr);
		fwrite(&FormatSize, 4, 1, FilePtr);
		fwrite(&FormatTag, 2, 1, FilePtr);
		fwrite(&NumOfChannels, 2, 1, FilePtr);
		fwrite(&SamplingRate, 4, 1, FilePtr);
		fwrite(&BytesPerSecond, 4, 1, FilePtr);
		fwrite(&BytesPerSample, 2, 1, FilePtr);
		fwrite(&BitsPerSample, 2, 1, FilePtr);
		//fwrite(&cbSize, 2, 1, FilePtr);

		/* data chunk - 8 bytes*/
		fwrite("data", 1, 4, FilePtr);
		DataLen = FileLen - 44 + 8;
		fwrite(&DataLen, 1, 4, FilePtr);
	}

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

void write_data(
	float data[],  /* input : data              */
	int   size,    /* input : number of samples */
	int  bitsPerSample,    /* input : bitsPerSample */
	FILE  *fp      /* output: file pointer      */
)
{
	short data16[4 * BAND_LENGTH * 8];
	int data24[4 * BAND_LENGTH * 8];
	char * ptchar = &data24;
	int   i;
	float temp;

	if (bitsPerSample == 16)
	{
		for (i = 0; i < size; i++)
		{
			temp = data[i];

			if (temp >= 0.0)
				temp += 0.5;
			else
				temp -= 0.5;
			if (temp > 32767.0) temp = 32767.0;
			if (temp < -32767.0) temp = -32767.0;
			data16[i] = (short)temp;
		}
		fwrite(data16, sizeof(short), size, fp);
	}
	else if (bitsPerSample == 24)
	{
		for (i = 0; i < size; i++)
		{
			int tmpint;
			temp = data[i];

			if (temp >= 0.0)
				temp += 0.5;
			else
				temp -= 0.5;
			if (temp > 32767.0 * 256) temp = 32767.0 * 256;
			if (temp < -32767.0 * 256) temp = -32767.0 * 256;
			//data16[i] = (short) temp;
			tmpint = temp;
			//	tmpint =(tmpint>>8);
			memcpy(ptchar + i * (24 / 8), &tmpint, 3);
		}
		fwrite(data24, 1, size * 3, fp);
	}
	else if (bitsPerSample == 32)
	{
		for (i = 0; i < size; i++)
		{
			int tmpint;
			temp = data[i];

			if (temp >= 0.0)
				temp += 0.5;
			else
				temp -= 0.5;
			if (temp > 32767.0 * 256 * 256) temp = 32767.0 * 256 * 256;
			if (temp < -32767.0 * 256 * 256) temp = -32767.0 * 256 * 256;
			//data16[i] = (short) temp;
			tmpint = temp;
			//	tmpint =(tmpint>>8);
			memcpy(ptchar + i * (32 / 8), &tmpint, 4);
		}
		fwrite(data24, 1, size * 4, fp);
	}
	else if (bitsPerSample == 8)
	{
		ptchar = &data16;
		for (i = 0; i < size; i++)
		{
			temp = data[i];

			if (temp >= 0.0)
				temp += 0.5;
			else
				temp -= 0.5;
			if (temp > 127.0) temp = 127.0;
			if (temp < -128.0) temp = -128.0;
			ptchar[i] = (temp + 128);
		}
		fwrite(ptchar, sizeof(char), size, fp);

	}

	return;
}