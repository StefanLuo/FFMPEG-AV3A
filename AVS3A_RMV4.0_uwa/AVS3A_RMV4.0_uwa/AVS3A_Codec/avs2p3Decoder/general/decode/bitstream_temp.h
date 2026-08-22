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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "avs2audio.h"

#define BUFFER_INCREMENT 1024
static const unsigned long mask[]=
{0x00000000,0x00000001,0x00000003,0x00000007,0x0000000f,
0x0000001f,0x0000003f,0x0000007f,0x000000ff,0x000001ff,
0x000003ff,0x000007ff,0x00000fff,0x00001fff,0x00003fff,
0x00007fff,0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
0x000fffff,0x001fffff,0x003fffff,0x007fffff,0x00ffffff,
0x01ffffff,0x03ffffff,0x07ffffff,0x0fffffff,0x1fffffff,
0x3fffffff,0x7fffffff,0xffffffff };



void avs2audiopack_reset(avs2audiopack_buffer *b);

void avs2audiopack_writeinit(avs2audiopack_buffer *b);

long avs2audiopack_bytes(avs2audiopack_buffer *b);

void avs2audiopack_writetrunc(avs2audiopack_buffer *b,long bits);

void avs2audiopack_writeclear(avs2audiopack_buffer *b);

/* Takes only up to 32 bits. */
void avs2audiopack_write(avs2audiopack_buffer *b,unsigned long value,int bits);

void avs2audiopack_adv(avs2audiopack_buffer *b,int bits);

/* Read in bits without advancing the bitptr; bits <= 32 */
long avs2audiopack_look(avs2audiopack_buffer *b,int bits);

void avs2audiopack_readinit(avs2audiopack_buffer *b,unsigned char *buf,int bytes);

/* bits <= 32 */
long avs2audiopack_read(avs2audiopack_buffer *b,int bits);

unsigned char *avs2audiopack_get_buffer(avs2audiopack_buffer *b);