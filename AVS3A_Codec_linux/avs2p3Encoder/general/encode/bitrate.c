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
#include <stdio.h>
#include "avs2audio.h"
#include "codec.h"
#include "bitrate.h"
#include "avs2audio.h"
#include "lfenc.h"

extern int inputchannelnum;

/* compute bitrate tracking setup  */
void tianlai_bitrate_init(tianlai_info *vi,bitrate_manager_state *bm,int useBWE){
  codec_setup_info *ci=vi->codec_setup;
  bitrate_manager_info *bi=&ci->bi;

  memset(bm,0,sizeof(*bm));

  if(bi && (bi->reservoir_bits>0)){
    long ratesamples=vi->rate;
    int  halfsamples=ci->blocksizes[0]>>1;

//   if(useBWE==1)
//		ratesamples /=2;//wchg

	if(inputchannelnum<2)
		bm->managed=1;//0;
	else
		bm->managed=1;

    bm->avg_bitsper= rint(1.*bi->avg_rate*halfsamples/ratesamples);
    bm->min_bitsper= rint(1.*bi->min_rate*halfsamples/ratesamples);
    bm->max_bitsper= rint(1.*bi->max_rate*halfsamples/ratesamples);//wuchaogang 2013.08.27

    bm->avgfloat=PACKETBLOBS/2;

    /* not a necessary fix, but one that leads to a more balanced
       typical initialization */
    {
      long desired_fill=bi->reservoir_bits*bi->reservoir_bias;
      bm->minmax_reservoir=desired_fill;
      bm->avg_reservoir=desired_fill;
    }

  }
}

void tianlai_bitrate_clear(bitrate_manager_state *bm){
  memset(bm,0,sizeof(*bm));
  return;
}

int tianlai_bitrate_managed(tianlai_block *vb)
{
  tianlai_dsp_state      *vd=vb->vd;
  private_state         *b=vd->backend_state;
  bitrate_manager_state *bm=&b->bms;

  if(bm && bm->managed)return(1);
  return(0);
}

/* finish taking in the block we just processed */
int tianlai_bitrate_addblock(tianlai_block *vb, int blocknum, int n, int tnsdatasize)
{
	tianlai_block_internal *vbi = vb->internal;
	tianlai_dsp_state      *vd = vb->vd;
	private_state          *b = vd->backend_state;
	bitrate_manager_state  *bm = &b->bms;
	tianlai_info           *vi = vd->vi;
	codec_setup_info       *ci = vi->codec_setup;
	bitrate_manager_info   *bi = &ci->bi;

	int  choice = rint(bm->avgfloat);
	long this_bits = avs2audiopack_bytes(vbi->packetblob[choice]) * 8;

	long min_target_bits = (vb->W ? bm->min_bitsper*(ci->blocksizes[vb->W] / ci->blocksizes[0]) : bm->min_bitsper);
	long max_target_bits = (vb->W ? bm->max_bitsper*(ci->blocksizes[vb->W] / ci->blocksizes[0]) : bm->max_bitsper);

	int  samples = ci->blocksizes[vb->W] >> 1;
	long desired_fill = bi->reservoir_bits*bi->reservoir_bias;
	static int countall = 0;
#if CONSTANT_BITRATE_CONTROL
	//if (opencbr)
	{
		bm->avg_reservoir = desired_fill;
	}
#endif
	if (!bm->managed)
	{
		/* not a bitrate managed stream, but for API simplicity, we'll
		   buffer the packet to keep the code path clean */

		getfloorout(vb, blocknum, PACKETBLOBSSELECT, n);
		this_bits = avs2audiopack_bytes(vbi->packetblob[PACKETBLOBSSELECT]) * 8;
		countall += this_bits;

		if (bm->vb) return(-1); /* one has been submitted without being claimed */
		bm->vb = vb;
		return(0);
	}

	bm->vb = vb;

	/* look ahead for avg floater */
	if (bm->avg_bitsper > 0)
	{
		double slew = 0.;
		long avg_target_bits = (vb->W ? bm->avg_bitsper*(ci->blocksizes[vb->W] / ci->blocksizes[0]) : bm->avg_bitsper) - tnsdatasize;
		double slewlimit = 15. / bi->slew_damp;

		/* choosing a new floater:
		   if we're over target, we slew down
		   if we're under target, we slew up

		   choose slew as follows: look through packetblobs of this frame
		   and set slew as the first in the appropriate direction that
		   gives us the slew we want.  This may mean no slew if delta is
		   already favorable.

		   Then limit slew to slew max */

		if (bm->avg_reservoir + (this_bits - avg_target_bits) > desired_fill)
		{
			while (choice > 0 && this_bits > avg_target_bits && bm->avg_reservoir + (this_bits - avg_target_bits) > desired_fill)
			{
				choice--;
				this_bits = avs2audiopack_bytes(vbi->packetblob[choice]) * 8;
			}
		}
		else if (bm->avg_reservoir + (this_bits - avg_target_bits) < desired_fill)
		{
#if QUANTLIZEDVALUEOUTRANGE
			while ((choice + 1 < PACKETBLOBS && this_bits < avg_target_bits && bm->avg_reservoir + (this_bits - avg_target_bits) < desired_fill)&&(vbi->packetblob[choice+1]->quantvalueoutrange_flag==0))
#else
			while (choice + 1 < PACKETBLOBS && this_bits < avg_target_bits && bm->avg_reservoir + (this_bits - avg_target_bits) < desired_fill)
#endif
			{
				choice++;
				this_bits = avs2audiopack_bytes(vbi->packetblob[choice]) * 8;
			}
		}
#if CONSTANT_BITRATE_CONTROL
		if (bm->avg_reservoir + (this_bits - avg_target_bits) > desired_fill)
		{
			while (choice > 0 && this_bits > avg_target_bits && bm->avg_reservoir + (this_bits - avg_target_bits) > desired_fill)
			{
				choice--;
				this_bits = avs2audiopack_bytes(vbi->packetblob[choice]) * 8;
			}
		}
#else
		slew = rint(choice - bm->avgfloat) / samples * vi->rate;
		if (slew < -slewlimit)
			slew = -slewlimit;
		if (slew > slewlimit)
			slew = slewlimit;
		choice = rint(bm->avgfloat += slew / vi->rate*samples);
		this_bits = avs2audiopack_bytes(vbi->packetblob[choice]) * 8;
#endif
		getfloorout(vb, blocknum, choice, n);

	}



	/* enforce min(if used) on the current floater (if used) */
	if (bm->min_bitsper > 0) {
		/* do we need to force the bitrate up? */
		if (this_bits < min_target_bits) {
#if QUANTLIZEDVALUEOUTRANGE
			while ((bm->minmax_reservoir - (min_target_bits - this_bits) < 0) && (vbi->packetblob[choice + 1]->quantvalueoutrange_flag == 0)) {
#else
			while (bm->minmax_reservoir - (min_target_bits - this_bits) < 0) {
#endif
				choice++;
				if (choice >= PACKETBLOBS)break;
				this_bits = avs2audiopack_bytes(vbi->packetblob[choice]) * 8;
			}
		}
	}

	/* enforce max (if used) on the current floater (if used) */
	if (bm->max_bitsper > 0) {
		/* do we need to force the bitrate down? */
		if (this_bits > max_target_bits) {
			while (bm->minmax_reservoir + (this_bits - max_target_bits) > bi->reservoir_bits) {
				choice--;
				if (choice < 0)break;
				this_bits = avs2audiopack_bytes(vbi->packetblob[choice]) * 8;
			}
		}
	}

	/* Choice of packetblobs now made based on floater, and min/max
	   requirements. Now boundary check extreme choices */

	if (choice < 0) {
		/* choosing a smaller packetblob is insufficient to trim bitrate.
		   frame will need to be truncated */
		long maxsize = (max_target_bits + (bi->reservoir_bits - bm->minmax_reservoir)) / 8;
		bm->choice = choice = 0;

		if (avs2audiopack_bytes(vbi->packetblob[choice]) > maxsize) {

			avs2audiopack_writetrunc(vbi->packetblob[choice], maxsize * 8);
			this_bits = avs2audiopack_bytes(vbi->packetblob[choice]) * 8;
		}
	}
	else {
		long minsize = (min_target_bits - bm->minmax_reservoir + 7) / 8;
		if (choice >= PACKETBLOBS)
			choice = PACKETBLOBS - 1;

		bm->choice = choice;

		/* prop up bitrate according to demand. pad this frame out with zeroes */
		minsize -= avs2audiopack_bytes(vbi->packetblob[choice]);
		while (minsize-- > 0)avs2audiopack_write(vbi->packetblob[choice], 0, 8);
		this_bits = avs2audiopack_bytes(vbi->packetblob[choice]) * 8;

	}

	/* now we have the final packet and the final packet size.  Update statistics */
	/* min and max reservoir */
	if (bm->min_bitsper > 0 || bm->max_bitsper > 0) {

		if (max_target_bits > 0 && this_bits > max_target_bits) {
			bm->minmax_reservoir += (this_bits - max_target_bits);
		}
		else if (min_target_bits > 0 && this_bits < min_target_bits) {
			bm->minmax_reservoir += (this_bits - min_target_bits);
		}
		else {
			/* inbetween; we want to take reservoir toward but not past desired_fill */
			if (bm->minmax_reservoir > desired_fill) {
				if (max_target_bits > 0) { /* logical bulletproofing against initialization state */
					bm->minmax_reservoir += (this_bits - max_target_bits);
					if (bm->minmax_reservoir < desired_fill)bm->minmax_reservoir = desired_fill;
				}
				else {
					bm->minmax_reservoir = desired_fill;
				}
			}
			else {
				if (min_target_bits > 0) { /* logical bulletproofing against initialization state */
					bm->minmax_reservoir += (this_bits - min_target_bits);
					if (bm->minmax_reservoir > desired_fill)bm->minmax_reservoir = desired_fill;
				}
				else {
					bm->minmax_reservoir = desired_fill;
				}
			}
		}
	}
	long avg_target_bits;
	/* avg reservoir */
	if (bm->avg_bitsper > 0) {
		avg_target_bits = (vb->W ? bm->avg_bitsper*(ci->blocksizes[vb->W] / ci->blocksizes[0]) : bm->avg_bitsper) - tnsdatasize;
		bm->avg_reservoir += this_bits - avg_target_bits;
	}
	countall += this_bits;
#if 	INNERLOOP_DEBUG
	printf("this_bits %d avg_target_bits %d	%d	%d %d	%d	%d\n", this_bits, avg_target_bits, avs2audiopack_bytes(vbi->packetblob[0]), avs2audiopack_bytes(vbi->packetblob[1]), avs2audiopack_bytes(vbi->packetblob[2]), avs2audiopack_bytes(vbi->packetblob[3]), avs2audiopack_bytes(vbi->packetblob[7]));
#endif
	return(0);
}

#if 1
int tianlai_bitrate_flushpacket(tianlai_dsp_state *vd, avs2audio_packet *op)
{
	private_state         *b = vd->backend_state;
	bitrate_manager_state *bm = &b->bms;
	tianlai_block         *vb = bm->vb;
	int                   choice = PACKETBLOBSSELECT;//PACKETBLOBS-1;
	
	if(!vb)return 0;

	if(op)
	{
		tianlai_block_internal *vbi = vb->internal;

		if(tianlai_bitrate_managed(vb)) choice = bm->choice;
		//printf("bm->choice %d\n", bm->choice);

		/*{
			int t;
			FILE *fp0 = fopen("bitchoice.txt","a");
			fprintf(fp0, "\n[choice][%d]\n", choice);
			fclose(fp0);
		}
		{
			int t;
			FILE *fp0 = fopen("bitchoice0.txt","a");
			fprintf(fp0, "\n[choice][%d]\n", choice);
			fclose(fp0);
		}*/

		op->packet = avs2audiopack_get_buffer(vbi->packetblob[choice]);
		op->bytes = vbi->packetblob[choice]->endbyte;// avs2audiopack_bytes(vbi->packetblob[choice]);   //chenhan  20180423  byte Alignment
		op->bit = vbi->packetblob[choice]->endbit; //chenhan  20180423  byte Alignment
		op->b_o_s = 0;
		op->e_o_s = vb->eofflag;
		op->granulepos = vb->granulepos;
		op->packetno = vb->sequence; /* for sake of completeness */

		/*opb->buffer = avs2audiopack_get_buffer(vbi->packetblob[choice]);
		opb->endbit = vbi->packetblob[choice]->endbit;
		opb->endbyte = vbi->packetblob[choice]->endbyte;
		opb->ptr = vbi->packetblob[choice]->ptr;
		opb->storage = vbi->packetblob[choice]->storage;
		*/
	}
	
	bm->vb = 0;
	return(1);
}
#endif
int tianlai_bitrate_checkblock0(tianlai_block *vb, int numbwebitsinblock)
{
	tianlai_block_internal *vbi = vb->internal;
	tianlai_dsp_state      *vd = vb->vd;
	private_state          *b = vd->backend_state;
	bitrate_manager_state  *bm = &b->bms;
	tianlai_info           *vi = vd->vi;
	codec_setup_info       *ci = vi->codec_setup;
	bitrate_manager_info   *bi = &ci->bi;


	long this_bits = avs2audiopack_bytes(vbi->packetblob[0]) * 8;

	/* look ahead for avg floater */
	if (bm->avg_bitsper > 0)
	{
		double slew = 0.;
		long avg_target_bits = (vb->W ? bm->avg_bitsper*(ci->blocksizes[vb->W] / ci->blocksizes[0]) : bm->avg_bitsper);

		this_bits = avs2audiopack_bytes(vbi->packetblob[0]) * 8;
		//printf("this_bits %d > avg_target_bits %d\n", this_bits, avg_target_bits);
		//if (vbi->packetblob[0]->quantvalueoutrange_flag == 1)
		//	return 1;
#if 	INNERLOOP_DEBUG
		printf("this_bits %d	 avg_target_bits	%d\n", this_bits, (vb->W ? bm->avg_bitsper*(vi->blocksizes[vb->W] / vi->blocksizes[0]) : bm->avg_bitsper));
#endif
		return (this_bits >= avg_target_bits - numbwebitsinblock);
	}

	return 0;
}