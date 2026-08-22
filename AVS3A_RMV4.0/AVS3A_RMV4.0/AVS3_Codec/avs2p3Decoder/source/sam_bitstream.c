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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>

#include "funcs.h"

#define TRUE  1
#define FALSE 0

#define  MINIMUM    4   
#define  MAX_LENGTH  3072		

static FILE		*bs_fp;
static unsigned char bs_buf[4096];  /* bit stream buffer */
static int  bs_buf_size;            /* size of buffer (in number of bytes) */
static long  bs_totbit;             /* bit counter of bit stream */
static int  bs_buf_byte_idx;        /* pointer to top byte in buffer */
static int  bs_buf_bit_idx;         /* pointer to top bit of top byte in buffer */
static int  bs_eob;                 /* end of buffer index */
static int  bs_eobs;                /* end of bit stream flag */
static long	ReadBitsPerFrame;

extern header_type;

void	refill_buffer();

/* refill the buffer from the input device when the buffer becomes empty    */
void	refill_buffer()
{
	register	int i=bs_buf_size-bs_buf_byte_idx+4;
	register	int n=1;

   bs_buf_byte_idx = 4; /* keep minimum 4 historical bytes in buffer for arithmetic code */
   if (!bs_eob) {
	 n = fread(&bs_buf[i], 1, bs_buf_size-i, bs_fp);
	 
	 if(n != (bs_buf_size-i))
	 {
		bs_eob = n+i;
		memset(&bs_buf[n+i],0,bs_buf_size-i-n); // zero padding
	 }
   }
}

/* open the device to read the bit stream from it */
void open_bitstream(char *bs_filenam)
{
	bs_fp = fopen(bs_filenam, "rb");
	if (bs_fp == NULL) {
		printf("Could not find input bs file %s.\n", bs_filenam);
		exit(1);
	}
	
  bs_buf_byte_idx=4095;
  bs_buf_bit_idx=0;
  bs_totbit=0;
  ReadBitsPerFrame = 0;
  bs_eob = FALSE;
  bs_eobs = FALSE;
  bs_buf_size = 4096;
}

/*close the device containing the bit stream after a read process*/
void close_bitstream()
{
	fclose(bs_fp);
}

static int i_putmask[9]={0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};

static int pbs = 0;
static int refill_bits = 0;
void putbits(unsigned int d,int N) /* bit return from arithmetic code */ 
{
	refill_bits=d;
	pbs = N;
}

/*read N bit from the bit stream */
unsigned int getbits(int N)
{
  unsigned long val=0;
  register int i;
  register int j = N;
  register int k, tmp;

  if(N <= 0) return 0;

  if (pbs)
  {
	  if (pbs >= N)
	  {
		  val = refill_bits>>(pbs-N);
		  pbs -= N;
		  return val;
	  }
	  else 
	  {
		  val = refill_bits<<(N-pbs);
		  pbs = 0;
		  N -= pbs;
	  }
  }

  if (N > MAX_LENGTH)
    printf("Cannot read or write more than %d bits at a time.\n", MAX_LENGTH);

  bs_totbit += N;
  ReadBitsPerFrame += N;
  while (j > 0) {
    if (!bs_buf_bit_idx) {
      bs_buf_bit_idx = 8;
      bs_buf_byte_idx++;
      if ((bs_buf_byte_idx >= (bs_buf_size - MINIMUM)) || (bs_buf_byte_idx == bs_eob)) {
        if (bs_eob) {
          bs_eobs = TRUE;
          //return 0;
        }
		else {
          for (i=bs_buf_byte_idx-4; i<bs_buf_size;i++)
            bs_buf[i-bs_buf_byte_idx+4] = bs_buf[i];
          refill_buffer();
        }
      }
    }
    k = (j < bs_buf_bit_idx) ? j : bs_buf_bit_idx;
    tmp = bs_buf[bs_buf_byte_idx]&i_putmask[bs_buf_bit_idx];
    val |= (tmp >> (bs_buf_bit_idx-k)) << (j-k);
    bs_buf_bit_idx -= k;
    j -= k;
  }
  return(val);
}

unsigned int byteAlign()
{
	int res;

	res = bs_buf_bit_idx % 8;
	if (res) getbits(res);
	return res;
}

void gobackNbytes(int N)
{
	bs_buf_byte_idx -= N;
	bs_totbit -= 8*N;
}

/*return the current bit stream length (in bits)*/
unsigned long i_sstell()
{
  return(bs_totbit);
}


/* return the status of the bit stream */
/* returns 1 if end of bit stream was reached */
/* returns 0 if end of bit stream was not reached */
int end_bs()
{
	ReadBitsPerFrame = 0;
	if (bs_eob && bs_buf_byte_idx+1 >= bs_eob)
		return TRUE;
	else
		return FALSE;
}



/* read data from file 'fp' and fill 'N' byte data de-interpolated into buffer 'buf' */
#define AATFFRAMESIZE 4096+2048

int fread_deinterpolate(unsigned char buf[], int N, FILE *fp)
{
	int length;
	static char aatfframe_buf[AATFFRAMESIZE];
	static lengthUsable = 0;

	long foffset;

	while (lengthUsable < N) {
		int K, LNew;
		int ByteRead;
		foffset = ftell(fp);
		ByteRead = fread(&aatfframe_buf[lengthUsable], 1, AATFFRAMESIZE-lengthUsable, fp);
			
		K = find_syncword(&aatfframe_buf[lengthUsable+1], ByteRead)+1;

		if (ByteRead != AATFFRAMESIZE-lengthUsable && K == 0) {
			LNew = (decode_scan(&aatfframe_buf[lengthUsable+1], (ByteRead-1)*8) >> 3)+1;
			lengthUsable += LNew;
			break;
		}

		fseek(fp, foffset+K, SEEK_SET);
		LNew = (decode_scan(&aatfframe_buf[lengthUsable+1], (K-1)*8) >> 3)+1;
		lengthUsable += LNew;
	}

	length = lengthUsable < N ? lengthUsable : N;
	memcpy(buf, aatfframe_buf, sizeof(unsigned char) * length);
	lengthUsable -= length;
	memmove(aatfframe_buf, &aatfframe_buf[length], sizeof(unsigned char)*lengthUsable);

	return length;
}

int find_syncword(unsigned char buf[], int RANGE)
{
	int i = 0;
	while (i < RANGE) {
		if (buf[i] == 0x00 && (buf[i+1]&0xF0) == 0x10) {
			return i;
		}
		i++;
	}
	return -1;
}

unsigned char mask1[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
unsigned char mask0[8] = {0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};
int decode_scan(unsigned char buffer[], int bitNum)
{
	int i;
	int zero_counter = 0;
	int ii = 0;
	

	unsigned char buffer2[AATFFRAMESIZE];

	for (i = 0; i < bitNum; i++) {
		if (i%8 == 3 && zero_counter >= 11 ) {
			zero_counter++;
			continue;
		}

		if (buffer[i/8]&mask1[i%8]) {	// '1'
			buffer2[ii/8] |= mask1[ii%8];
			ii++;
			zero_counter = 0;
		}else{									// '0'
			buffer2[ii/8] &= mask0[ii%8];
			ii++;
			zero_counter++;
		}		
	}

	memcpy(buffer, buffer2, sizeof(unsigned char) * (ii>>3));
	return ii;
}