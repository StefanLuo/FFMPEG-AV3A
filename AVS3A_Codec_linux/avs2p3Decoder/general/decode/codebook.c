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
#include <string.h>
#include <math.h>
#include "avs2audio.h"
#include "codebook.h"
#include "avs2audio.h"

/* there might be a straightforward one-line way to do the below
   that's portable and totally safe against roundoff, but I haven't
   thought of it.  Therefore, we opt on the side of caution */
long _book_maptype1_quantvals(const static_codebook *b){
  long vals=floor(pow((float)b->entries,1.f/b->dim));

  /* the above *should* be reliable, but we'll not assume that FP is
     ever reliable when bitstream sync is at stake; verify via integer
     means that vals really is the greatest value of dim for which
     vals^b->bim <= b->entries */
  /* treat the above as an initial guess */
  while(1){
    long acc=1;
    long acc1=1;
    int i;
    for(i=0;i<b->dim;i++){
      acc*=vals;
      acc1*=vals+1;
    }
    if(acc<=b->entries && acc1>b->entries){
      return(vals);
    }else{
      if(acc>b->entries){
        vals--;
      }else{
        vals++;
      }
    }
  }
}

/**** _ilog ******************************************/
int _ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}



/* returns the number of bits ************************************************/
int tianlai_book_encode(codebook *book, int a, avs2audiopack_buffer *b){
  if(a<0 || a>=book->c->entries)return(0);
  //avs2audiopack_write(b,book->codelist[a],book->c->lengthlist[a]);
  return(book->c->lengthlist[a]);
}

/* the 'eliminate the decode tree' optimization actually requires the
   codewords to be MSb first, not LSb.  This is an annoying inelegancy
   (and one of the first places where carefully thought out design
   turned out to be wrong; tianlai II and future avs2audio codecs should go
   to an MSb bitpacker), but not actually the huge hit it appears to
   be.  The first-stage decode table catches most words so that
   bitreverse is not in the main execution path. */

static unsigned int bitreverse(unsigned int x){
  x=    ((x>>16)&0x0000ffff) | ((x<<16)&0xffff0000);
  x=    ((x>> 8)&0x00ff00ff) | ((x<< 8)&0xff00ff00);
  x=    ((x>> 4)&0x0f0f0f0f) | ((x<< 4)&0xf0f0f0f0);
  x=    ((x>> 2)&0x33333333) | ((x<< 2)&0xcccccccc);
  return((x>> 1)&0x55555555) | ((x<< 1)&0xaaaaaaaa);
}

static long decode_packed_entry_number(codebook *book, avs2audiopack_buffer *b){
  int  read=book->dec_maxlength;
  long lo,hi;
  long lok = avs2audiopack_look(b,book->dec_firsttablen);
  if (lok >= 0) {
    long entry = book->dec_firsttable[lok];
    if(entry&0x80000000UL){
      lo=(entry>>15)&0x7fff;
      hi=book->used_entries-(entry&0x7fff);
    }else{
      avs2audiopack_adv(b, book->dec_codelengths[entry-1]);
      return(entry-1);
    }
  }else{
    lo=0;
    hi=book->used_entries;
  }
  lok = avs2audiopack_look(b, read);
  while(lok<0 && read>1)
    lok = avs2audiopack_look(b, --read);
  if(lok<0)return -1;
  /* bisect search for the codeword in the ordered list */
  {
    unsigned int testword=bitreverse((unsigned int)lok);
    while(hi-lo>1){
      long p=(hi-lo)>>1;
      long test=book->codelist[lo+p]>testword;
      lo+=p&(test-1);
      hi-=p&(-test);
      }
    if(book->dec_codelengths[lo]<=read){
      avs2audiopack_adv(b, book->dec_codelengths[lo]);
      return(lo);
    }
  }
  avs2audiopack_adv(b, read);

  return(-1);
}

/* Decode side is specced and easier, because we don't need to find
   matches using different criteria; we simply read and map.  There are
   two things we need to do 'depending':

   We may need to support interleave.  We don't really, but it's
   convenient to do it here rather than rebuild the vector later.

   Cascades may be additive or multiplicitive; this is not inherent in
   the codebook, but set in the code using the codebook.  Like
   interleaving, it's easiest to do it here.
   addmul==0 -> declarative (set the value)
   addmul==1 -> additive
   addmul==2 -> multiplicitive */

/* returns the [original, not compacted] entry number or -1 on eof *********/
long tianlai_book_decode(codebook *book, avs2audiopack_buffer *b){
  if(book->used_entries>0){
    long packed_entry=decode_packed_entry_number(book,b);
    if(packed_entry>=0)
      return(book->dec_index[packed_entry]);
  }

  /* if there's no dec_index, the codebook unpacking isn't collapsed */
  return(-1);
}

/* returns 0 on OK or -1 on eof *************************************/
/* decode vector / dim granularity gaurding is done in the upper layer */
long tianlai_book_decodevs_add(codebook *book,float *a,avs2audiopack_buffer *b,int n){
  if(book->used_entries>0){
    int step=n/book->dim;
    long *entry = malloc(sizeof(*entry)*step);
    float **t = malloc(sizeof(*t)*step);
    int i,j,o;

    for (i = 0; i < step; i++) {
      entry[i]=decode_packed_entry_number(book,b);
      if(entry[i]==-1)return(-1);
      t[i] = book->valuelist+entry[i]*book->dim;
    }
    for(i=0,o=0;i<book->dim;i++,o+=step)
      for (j=0;j<step;j++)
        a[o+j]+=t[j][i];
  }
  return(0);
}

/* decode vector / dim granularity gaurding is done in the upper layer */
long tianlai_book_decodev_add(codebook *book,float *a,avs2audiopack_buffer *b,int n){
  if(book->used_entries>0){
    int i,j,entry;
    float *t;

    if(book->dim>8){
      for(i=0;i<n;){
        entry = decode_packed_entry_number(book,b);
        if(entry==-1)return(-1);
        t     = book->valuelist+entry*book->dim;
        for (j=0;j<book->dim;)
          a[i++]+=t[j++];
      }
    }else{
      for(i=0;i<n;){
        entry = decode_packed_entry_number(book,b);
        if(entry==-1)return(-1);
        t     = book->valuelist+entry*book->dim;
        j=0;
        switch((int)book->dim){
        case 8:
          a[i++]+=t[j++];
        case 7:
          a[i++]+=t[j++];
        case 6:
          a[i++]+=t[j++];
        case 5:
          a[i++]+=t[j++];
        case 4:
          a[i++]+=t[j++];
        case 3:
          a[i++]+=t[j++];
        case 2:
          a[i++]+=t[j++];
        case 1:
          a[i++]+=t[j++];
        case 0:
          break;
        }
      }
    }
  }
  return(0);
}

/* unlike the others, we guard against n not being an integer number
   of <dim> internally rather than in the upper layer (called only by
   floor0) */
long tianlai_book_decodev_set(codebook *book,float *a,avs2audiopack_buffer *b,int n){
  if(book->used_entries>0){
    int i,j,entry;
    float *t;

    for(i=0;i<n;){
      entry = decode_packed_entry_number(book,b);
      if(entry==-1)return(-1);
      t     = book->valuelist+entry*book->dim;
      for (j=0;i<n && j<book->dim;){
        a[i++]=t[j++];
      }
    }
  }else{
    int i;

    for(i=0;i<n;){
      a[i++]=0.f;
    }
  }
  return(0);
}

long tianlai_book_decodevv_add(codebook *book,float **a,long offset,int ch,
                              avs2audiopack_buffer *b,int n){

  long i,j,entry;
  int chptr=0;
  if(book->used_entries>0){
    for(i=offset/ch;i<(offset+n)/ch;){
      entry = decode_packed_entry_number(book,b);
      if(entry==-1)return(-1);
      {
        const float *t = book->valuelist+entry*book->dim;
        for (j=0;j<book->dim;j++){
          a[chptr++][i]+=t[j];
          if(chptr==ch){
            chptr=0;
            i++;
          }
        }
      }
    }
  }
  return(0);
}
