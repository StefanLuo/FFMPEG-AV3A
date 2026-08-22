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

void avs2audiopack_writeinit(avs2audiopack_buffer *b){
  memset(b,0,sizeof(*b));
  b->ptr=b->buffer=malloc(BUFFER_INCREMENT);
  b->buffer[0]='\0';
  b->storage=BUFFER_INCREMENT;
}

/**/void avs2audiopack_writeclear(avs2audiopack_buffer *b){
  if(b->buffer)free(b->buffer);
  memset(b,0,sizeof(*b));
}

/* Takes only up to 32 bits. */
void avs2audiopack_write(avs2audiopack_buffer *b,unsigned long value,int bits){
  if(bits<0 || bits>32) goto err;
  if(b->endbyte>=b->storage-4){
    void *ret;
    if(!b->ptr)return;
    if(b->storage>LONG_MAX-BUFFER_INCREMENT) goto err;
    ret= realloc(b->buffer,b->storage+BUFFER_INCREMENT);
    if(!ret) goto err;
    b->buffer=ret;
    b->storage+=BUFFER_INCREMENT;
    b->ptr=b->buffer+b->endbyte;
  }

  value&=mask[bits];
  bits+=b->endbit;

  b->ptr[0]|=value<<b->endbit;

  if(bits>=8){
    b->ptr[1]=(unsigned char)(value>>(8-b->endbit));
    if(bits>=16){
      b->ptr[2]=(unsigned char)(value>>(16-b->endbit));
      if(bits>=24){
        b->ptr[3]=(unsigned char)(value>>(24-b->endbit));
        if(bits>=32){
          if(b->endbit)
            b->ptr[4]=(unsigned char)(value>>(32-b->endbit));
          else
            b->ptr[4]=0;
        }
      }
    }
  }

  b->endbyte+=bits/8;
  b->ptr+=bits/8;
  b->endbit=bits&7;
  return;
 err:
  avs2audiopack_writeclear(b);
}

void avs2audiopack_adv(avs2audiopack_buffer *b,int bits){
  bits+=b->endbit;

  if(b->endbyte > b->storage-((bits+7)>>3)) goto overflow;

  b->ptr+=bits/8;
  b->endbyte+=bits/8;
  b->endbit=bits&7;
  return;

 overflow:
  b->ptr=NULL;
  b->endbyte=b->storage;
  b->endbit=1;
}

/* Read in bits without advancing the bitptr; bits <= 32 */
long avs2audiopack_look(avs2audiopack_buffer *b,int bits){
  unsigned long ret;
  unsigned long m;

  if(bits<0 || bits>32) return -1;
  m=mask[bits];
  bits+=b->endbit;

  if(b->endbyte >= b->storage-4){
    /* not the main path */
    if(b->endbyte > b->storage-((bits+7)>>3)) return -1;
    /* special case to avoid reading b->ptr[0], which might be past the end of
        the buffer; also skips some useless accounting */
    else if(!bits)return(0L);
  }

  ret=b->ptr[0]>>b->endbit;
  if(bits>8){
    ret|=b->ptr[1]<<(8-b->endbit);
    if(bits>16){
      ret|=b->ptr[2]<<(16-b->endbit);
      if(bits>24){
        ret|=b->ptr[3]<<(24-b->endbit);
        if(bits>32 && b->endbit)
          ret|=b->ptr[4]<<(32-b->endbit);
      }
    }
  }
  return(m&ret);
}

/* bits <= 32 */
long avs2audiopack_read(avs2audiopack_buffer *b,int bits){
  long ret;
  unsigned long m;

  if(bits<0 || bits>32) goto err;
  m=mask[bits];
  bits+=b->endbit;

  if(b->endbyte >= b->storage-4){
    /* not the main path */
    if(b->endbyte > b->storage-((bits+7)>>3)) goto overflow;
    /* special case to avoid reading b->ptr[0], which might be past the end of
        the buffer; also skips some useless accounting */
    else if(!bits)return(0L);
  }

  ret=b->ptr[0]>>b->endbit;
  if(bits>8){
    ret|=b->ptr[1]<<(8-b->endbit);
    if(bits>16){
      ret|=b->ptr[2]<<(16-b->endbit);
      if(bits>24){
        ret|=b->ptr[3]<<(24-b->endbit);
        if(bits>32 && b->endbit){
          ret|=b->ptr[4]<<(32-b->endbit);
        }
      }
    }
  }
  ret&=m;
  b->ptr+=bits/8;
  b->endbyte+=bits/8;
  b->endbit=bits&7;
  return ret;

 overflow:
 err:
  b->ptr=NULL;
  b->endbyte=b->storage;
  b->endbit=1;
  return -1L;
}