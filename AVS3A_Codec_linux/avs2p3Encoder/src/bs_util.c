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

#include "stdlib.h"
#include "stdio.h"
#include "av3enc.h"

static FILE *ptrBitstream;
/* Bits buffered for output */
static int buffer;
static int bits_to_go;
static unsigned char OutputBuffer[8191*50];
static int CurrentByte;

void output_byte(long byte,int len);
void start_outputing_bits();
void done_outputing_bits();
void FlushBuffer(/*void*/unsigned char *outbuf); //shumin.xu 20211022
void FlushBufferWithLength(/*void*/unsigned char *outbuf);
int BitstreamOpen(FILE *fname);
void BitstreamClose(void);
int GetBitstreamSize(void);
int ByteAlign(void);

unsigned char OutputBuffer_ext[8191*8];
int nOutput_ext;

unsigned char LosslessBuffer[8191*8];
unsigned char LosslessBuffer_ext[8191*8];
int nLossless;
int nLossless_ext;

unsigned char WaveletBuffer[2][1024*4];
static int CurPos[2], buf_wavelet[2], bits_to_go_wavelet[2];
void start_outputing_bits_wavelet(int bufno);
int PutByte_Wavelet(unsigned char c,int bufno);
void output_bit_wavelet(int bit,int bufno);
void output_byte_wavelet(long byte,int len,int bufno);
int GetBitstreamSize_Wavelet(int bufno);
int WaveletByteAlign(int bufno);

void FlushBufferInit();
void FlushBufferWithLength_ext(void);
void FlushFrame(int fill_bits);

void ScanAATFFrame(void);

// for AASF
void FlushAASFHeaderBuffer();

// for AATF        
void Register_buffer(void);
int	EncodeBSHCGetAATFSize();
void Restore_buffer(void);
void FlushAATFHeaderBuffer();

static int buffer_last;
static int bits_to_go_last;
static int CurrentByte_last;

int BitstreamOpen(FILE *fname)
{
	ptrBitstream = fname;
	nOutput_ext = 0;
	if(ptrBitstream==NULL) return 1;
	return 0;
}
void BitstreamClose(void)
{
	fclose(ptrBitstream);
}
int GetBitstreamSize(void)
{
	return CurrentByte*8+(8-bits_to_go);
}
void FlushBuffer(unsigned char* outbuf)
{
	memcpy(outbuf, OutputBuffer, CurrentByte); //shumin.xu 20211022
	fwrite(OutputBuffer,1,CurrentByte,ptrBitstream);
	CurrentByte = 0;
}

void FlushBufferWithLength(unsigned char* outbuf)
{
	memcpy(&outbuf[7], OutputBuffer, CurrentByte); //shumin.xu 20211022
	fwrite(OutputBuffer,1,CurrentByte,ptrBitstream);
	memcpy(&outbuf[9], LosslessBuffer, nLossless);
	fwrite(LosslessBuffer,1,nLossless,ptrBitstream);
}

void LosslessFlushBufferInit()
{
	nLossless = 0;
}

void LosslessFlushBufferWithLength()
{
	memcpy(LosslessBuffer+nLossless, OutputBuffer,CurrentByte);
	nLossless += CurrentByte;
}

void start_outputing_bits()
{   
	/* Buffer is empty to start with*/
    buffer = 0;
    bits_to_go= 8;				                   
	CurrentByte = 0;
}
int PutByte(unsigned char c)
{
	if(CurrentByte>8191)
	{
		fprintf(stderr,"\n\n\t\t\terr");
		return 1;
	}
	OutputBuffer[CurrentByte] = c;
	CurrentByte++;	
	return 0;
}
void output_bit(int bit)
{
	/* Put bit in top of buffer.*/
	buffer <<= 1; if (bit) buffer |= 0x1;
    bits_to_go -= 1;
	/* Output buffer if it is   */
    if (bits_to_go==0) {
		/* now full.            */
		PutByte(buffer);
        bits_to_go = 8;
    }
}

void done_outputing_bits()
{   
	PutByte(buffer<<bits_to_go);
	/* Buffer is empty to start */
	buffer = 0;
	/* with.                    */
    bits_to_go= 8;
}
void output_byte(long byte,int len)
{
	int i;
	int mask;
	/* MSB first */
	mask = 1<<(len-1);
	for(i=0;i<len;i++)
	{
		if(byte & mask)
			output_bit(1);
		else
			output_bit(0);		
		mask >>= 1;
	}
}
int ByteAlign(void)
{
	int ret = bits_to_go;
	if( (bits_to_go != 8) )
	{
		/* byte align */
		output_byte(0,bits_to_go);
		return ret;
	}else{
		return 0;
	}
}

/************************************************************************/
/*                              AASF                                    */
/************************************************************************/

void FlushAASFHeaderBuffer()
{
	OutputBuffer[4] = (CurrentByte>>16)&0xFF;
	OutputBuffer[5] = (CurrentByte>>8)&0xFF;
	OutputBuffer[6] = CurrentByte&0xFF;
	
	fwrite(OutputBuffer,1,CurrentByte,ptrBitstream);
}

void Register_buffer(void)
{
	buffer_last = buffer;					
	bits_to_go_last = bits_to_go;				                   
	CurrentByte_last = CurrentByte;
}

int	EncodeBSHCGetAATFSize()
{
	return (CurrentByte - CurrentByte_last)<<3;
}

void Restore_buffer(void)
{
	buffer = buffer_last;					
	bits_to_go = bits_to_go_last;				                   
	CurrentByte = CurrentByte_last;

}

void FlushAATFHeaderBuffer()
{
	memcpy(&OutputBuffer[CurrentByte], 
		LosslessBuffer, sizeof(unsigned char) * nLossless);
	CurrentByte += nLossless;
	ScanAATFFrame();
	fwrite(OutputBuffer,1,CurrentByte,ptrBitstream);
}

unsigned char mask1[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
unsigned char mask0[8] = {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};

void ScanAATFFrame(void)
{
	// Byte number after scan and interpolating.
	int NewCurrentByte;
	int TotalBits;
	// number of the bit being scanned
	int NewBits;
	int AlignBits;
	unsigned char NewOutputBuffer[2048];
	int i;
	int zeroCounter = 0;
	int intCounter = 0;

	/* syncword which is not scanned */
	NewOutputBuffer[0] = 0x00;
	NewOutputBuffer[1] = 0x10;

	// number of bits need to scan.
	TotalBits = (CurrentByte<<3);
	NewBits = 12;

	/* scan */
	for (i = 12; i < TotalBits; i ++) {
		if (NewBits%8 == 3 && zeroCounter >= 11) {
			NewOutputBuffer[NewBits/8] &= 0xEF;
			NewBits ++;
			intCounter++;
			zeroCounter++;
		}

		if (OutputBuffer[i/8]&mask1[i%8]) {
			NewOutputBuffer[NewBits/8] |= mask1[NewBits%8];
			NewBits++;
			zeroCounter = 0;
		}else{
			NewOutputBuffer[NewBits/8] &= mask0[NewBits%8];
			NewBits++;
			zeroCounter++;
		}
	}
	
	NewCurrentByte = (NewBits+7)/8;
	/* align */
	AlignBits = NewBits%8 == 0 ? 0 : 8-NewBits%8;
    if (AlignBits == 5 && zeroCounter >= 11) {
		NewOutputBuffer[NewCurrentByte-1] &= 0xe0;
		NewOutputBuffer[NewCurrentByte-1] |= 0x0f;
    }else if(AlignBits != 0)
		NewOutputBuffer[NewCurrentByte-1] |= (1<<AlignBits)-1;
	
	CurrentByte = NewCurrentByte;
	memcpy(OutputBuffer, NewOutputBuffer, sizeof(unsigned char)*CurrentByte);

	return;
}

void start_outputing_bits_wavelet(int bufno)
{
	CurPos[bufno] = 0;
	bits_to_go_wavelet[bufno] = 8;
	buf_wavelet[bufno] = 0;
}
int PutByte_Wavelet(unsigned char c,int bufno)
{
	if(CurPos[bufno]>4096)
	{
		fprintf(stderr,"\n\n\t\t\terr");
		return 1;
	}
	WaveletBuffer[bufno][CurPos[bufno]] = c;
	CurPos[bufno]++;	
	return 0;
}
void output_bit_wavelet(int bit,int bufno)
{
	/* Put bit in top of buffer.*/
	buf_wavelet[bufno] <<= 1; if (bit) buf_wavelet[bufno] |= 0x1;
	bits_to_go_wavelet[bufno] -= 1;
	/* Output buffer if it is   */
	if (bits_to_go_wavelet[bufno]==0) {
		/* now full.                */
		PutByte_Wavelet(buf_wavelet[bufno],bufno);
		bits_to_go_wavelet[bufno] = 8;
	}
}
void output_byte_wavelet(long byte,int len,int bufno)
{
	int i;
	int mask;
	/* MSB first */
	mask = 1<<(len-1);
	for(i=0;i<len;i++)
	{
		if(byte & mask)
			output_bit_wavelet(1,bufno);
		else
			output_bit_wavelet(0,bufno);		
		mask >>= 1;
	}
}
int GetBitstreamSize_Wavelet(int bufno)
{
	return CurPos[bufno]*8+(8-bits_to_go_wavelet[bufno]);
}
int WaveletByteAlign(int bufno)
{
	if(bits_to_go_wavelet[bufno]!=8)
	{
		/* byte align */
		output_byte_wavelet(0,bits_to_go_wavelet[bufno],bufno);
		return bits_to_go_wavelet[bufno];
	}
	else
		return 0;
}
void FlushWaveletBuffer(int bufno)
{
	memcpy(OutputBuffer+CurrentByte,WaveletBuffer[bufno],CurPos[bufno]);
	CurrentByte += CurPos[bufno];
}

void EncodeWaveletStart(int bufno)
{
	start_outputing_bits_wavelet(bufno);
}
void EncodeWaveletPutBits(int val,int len, int bufferno)
{
	output_byte_wavelet((long)val,len,bufferno);
}
int EncodeWaveletGetSize(int bufno)
{
	//in bits
	return GetBitstreamSize_Wavelet(bufno);

}
int EncodeWaveletByteAlign(int bufno)
{
	return WaveletByteAlign(bufno);
}
void EncodeWaveletFlush(int bufno)
{
	FlushWaveletBuffer(bufno);
}

/************************************************************************/
/*                          AASF header                                 */
/************************************************************************/
int WriteAASFHeader(int ch, int freq, 
	int resolution, int sampleRate, int codecId, int codingProfile)
{
	int bitused = 0;	
	int anc_data_index = 0;
	
	/* write AVS AASF ID */
	output_byte('A',8);
	output_byte('A',8);
	output_byte('S',8);
	output_byte('F',8);
	bitused += 32;

	/* write AASF header size */
	output_byte(0, 24);
	bitused += 24;

	/* write raw stream length */
	output_byte(0, 32);
	bitused += 32;

	/* write audio codec id */
	output_byte(codecId, 4);
	bitused += 2;

	/* write resolution */
	output_byte(resolution, 2);
	bitused += 2;

	/* write coding profile */
	output_byte(codingProfile, 3/*2*/);
	bitused += 3/*2*/;

	/* write ancillary data index */
	output_byte(anc_data_index, 1);
	bitused += 1;

	/* write channel number */
	if (ch >= 16){
		output_byte(15, 4);
		output_byte(ch-16, 4);
		bitused += 8;
	}else{
		output_byte(ch-1,  4);
		bitused += 4;
	}

	/* write frequency index */
	output_byte(freq, 4);
	bitused += 4;
	if (FSIDX_EXT==freq){
		/* write frequency value */
		output_byte(sampleRate, 24);
		bitused += 24;
	}

	/* decoding information alignment */
	ByteAlign();
	bitused = ((bitused + 7) >> 3) << 3;
    FlushAASFHeaderBuffer();

	return (bitused >> 3);
}



/*+----------------------------------------------------+
|													   |
|			AATF header write						   |	
|													   | 	
+----------------------------------------------------+*/
#define	SYNCWORD 				2046/*4095*/
/* vacant function */
int aatf_error_check()
{
	return 0;
}

int aatf_decoding_header(int freqIdx, 
						 int ch,
						 int resolution, 
						 int sampleRate,
						 int codecId,
						 int codingProfile)
{
	int chbit = 0;

	/* write syncword */
	output_byte(SYNCWORD, 12);
	/* write audio codec id */
	output_byte(codecId,   4);
	/* write anc data index */
	output_byte(0, 1);
	/* write coding profile */
	output_byte(codingProfile, 3);
	/* write frequency index */
	output_byte(freqIdx, 4);
	if (FSIDX_EXT==freqIdx){
		/* write frequency value */
		output_byte(sampleRate, 24);
		chbit += 24;
	}
	/* write raw frame length */
	output_byte(0, 16); //to be filled

	/* write aatf error check */
	output_byte(0, 8);  //to be filled

	/* write channel number */
	if (ch >= 16){
		output_byte(15, 4);
		output_byte(ch-16, 4);
		chbit += 8;
	}else{
		output_byte(ch-1,  4);
		chbit += 4;
	}
	/* write resolution */
	output_byte(resolution, 2);

	return (12+8+4/*+chbit*/+16+8+4+2);
}

int WriteAATFHeader(int freqIdx,
					int	nch,
					int resolution,
					int sampleRate,
					int codecId,
					int codingProfile)
{
	int bit_count;
	int channel_config;

	channel_config = nch;
	bit_count = 0;
	bit_count += aatf_decoding_header(
		freqIdx, channel_config, 
		resolution, sampleRate, codecId, codingProfile);

	bit_count += ByteAlign();
	//not implement
	//bit_count += aatf_error_check();
	return bit_count;
}
