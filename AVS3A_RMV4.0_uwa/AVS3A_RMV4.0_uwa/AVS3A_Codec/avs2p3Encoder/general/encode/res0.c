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
#include "registry.h"
#include "codebook.h"



typedef struct {
  tianlai_info_residue0 *info;

  int         parts;
  int         stages;
  codebook   *fullbooks;
  codebook   *phrasebook;
  codebook ***partbooks;

  int         partvals;
  int       **decodemap;

  long      postbits;
  long      phrasebits;
  long      frames;


} tianlai_look_residue0;

void res0_free_info(tianlai_info_residue *i) {
	tianlai_info_residue0 *info = (tianlai_info_residue0 *)i;
	if (info) {
		memset(info, 0, sizeof(*info));
		free(info);
	}
}

void res0_free_look(tianlai_look_residue *i) {
	int j;
	if (i) {

		tianlai_look_residue0 *look = (tianlai_look_residue0 *)i;


		for (j = 0; j < look->parts; j++)
			if (look->partbooks[j])free(look->partbooks[j]);
		free(look->partbooks);
		for (j = 0; j < look->partvals; j++)
			free(look->decodemap[j]);
		free(look->decodemap);

		memset(look, 0, sizeof(*look));
		free(look);
	}
}

static int ilog(unsigned int v) {
	int ret = 0;
	while (v) {
		ret++;
		v >>= 1;
	}
	return(ret);
}

static int icount(unsigned int v) {
	int ret = 0;
	while (v) {
		ret += v & 1;
		v >>= 1;
	}
	return(ret);
}


tianlai_look_residue *res0_look(codec_setup_info     *ci,//tianlai_dsp_state *vd,
	tianlai_info_residue *vr) {
	tianlai_info_residue0 *info = (tianlai_info_residue0 *)vr;
	tianlai_look_residue0 *look = malloc(1 * sizeof(*look));
	// codec_setup_info     *ci=vd->vi->codec_setup;

	int j, k, acc = 0;
	int dim;
	int maxstage = 0;
	look->info = info;

	look->parts = info->partitions;
	look->fullbooks = ci->fullbooks;
	look->phrasebook = ci->fullbooks + info->groupbook;
	dim = look->phrasebook->dim;

	look->partbooks = malloc(look->parts * sizeof(*look->partbooks));

	for (j = 0; j < look->parts; j++) {
		int stages = ilog(info->secondstages[j]);
		if (stages) {
			if (stages > maxstage)maxstage = stages;
			look->partbooks[j] = malloc(stages * sizeof(*look->partbooks[j]));
			for (k = 0; k < stages; k++)
				if (info->secondstages[j] & (1 << k)) {
					look->partbooks[j][k] = ci->fullbooks + info->booklist[acc++];

				}
		}
	}

	look->partvals = 1;
	for (j = 0; j < dim; j++)
		look->partvals *= look->parts;

	look->stages = maxstage;
	look->decodemap = malloc(look->partvals * sizeof(*look->decodemap));
	for (j = 0; j < look->partvals; j++) {
		long val = j;
		long mult = look->partvals / look->parts;
		look->decodemap[j] = malloc(dim * sizeof(*look->decodemap[j]));
		for (k = 0; k < dim; k++) {
			long deco = val / mult;
			val -= deco * mult;
			mult /= look->parts;
			look->decodemap[j][k] = deco;
		}
	}

	look->postbits = 0;
	look->phrasebits = 0;
	return(look);
}

/* break an abstraction and copy some code for performance purposes */
static int local_book_besterror(
#if QUANTLIZEDVALUEOUTRANGE  
	avs2audiopack_buffer *opb,
#endif
	codebook *book, int *a) {
	int dim = book->dim;
	int i, j, o;
	int minval = book->minval;
	int del = book->delta;
	int qv = book->quantvals;
	int ze = (qv >> 1);
	int index = 0;
	/* assumes integer/centered encoder codebook maptype 1 no more than dim 8 */
	int p[8] = { 0,0,0,0,0,0,0,0 };

	if (del != 1) {
		for (i = 0, o = dim; i < dim; i++) {
			int v = (a[--o] - minval + (del >> 1)) / del;
			int m = (v < ze ? ((ze - v) << 1) - 1 : ((v - ze) << 1));
			index = index * qv + (m < 0 ? 0 : (m >= qv ? qv - 1 : m));
			p[o] = v * del + minval;
#if QUANTLIZEDVALUEOUTRANGE
			if ((m < 0) || (m >= qv))
			{
				opb->quantvalueoutrange_flag = 1;
			}
#endif
		}
	}
	else {
		for (i = 0, o = dim; i < dim; i++) {
			int v = a[--o] - minval;
			int m = (v < ze ? ((ze - v) << 1) - 1 : ((v - ze) << 1));
			index = index * qv + (m < 0 ? 0 : (m >= qv ? qv - 1 : m));
			p[o] = v * del + minval;
#if QUANTLIZEDVALUEOUTRANGE
			if ((m < 0) || (m >= qv))
			{
				opb->quantvalueoutrange_flag = 1;
			}
#endif
		}
	}

	if (book->c->lengthlist[index] <= 0) {
		const static_codebook *c = book->c;
		int best = -1;
		/* assumes integer/centered encoder codebook maptype 1 no more than dim 8 */
		int e[8] = { 0,0,0,0,0,0,0,0 };
		int maxval = book->minval + book->delta*(book->quantvals - 1);
		for (i = 0; i < book->entries; i++) {
			if (c->lengthlist[i] > 0) {
				int this = 0;
				for (j = 0; j < dim; j++) {
					int val = (e[j] - a[j]);
					this += val * val;
				}
				if (best == -1 || this < best) {
					memcpy(p, e, sizeof(p));
					best = this;
					index = i;
				}
			}
			/* assumes the value patterning created by the tools in vq/ */
			j = 0;
			while (e[j] >= maxval)
				e[j++] = 0;
			if (e[j] >= 0)
				e[j] += book->delta;
			e[j] = -e[j];
		}
	}

	if (index > -1) {
		for (i = 0; i < dim; i++)
			*a++ -= p[i];
	}

	return(index);
}

static int _encodepart(avs2audiopack_buffer *opb, int *vec, int n,
	codebook *book, long *acc) {
	int i, bits = 0;
	int dim = book->dim;
	int step = n / dim;

	for (i = 0; i < step; i++) {
		int entry = local_book_besterror(
#if QUANTLIZEDVALUEOUTRANGE
			opb,
#endif
			book, vec + i * dim);
		bits += tianlai_book_encode(book, entry, opb);
	}
	return(bits);
}


static long **_01class(tianlai_block *vb, tianlai_look_residue *vl,
	int **in, int ch) {
	long i, j, k;
	tianlai_look_residue0 *look = (tianlai_look_residue0 *)vl;
	tianlai_info_residue0 *info = look->info;

	/* move all this setup out later */
	int samples_per_partition = info->grouping;
	int possible_partitions = info->partitions;
	int n = vb->encLen;//info->end-info->begin;

	int partvals = (int)((float)(n + (samples_per_partition - 1)) / (float)samples_per_partition + 0.0);//n/samples_per_partition;  //wuchaogang 2014.11.12
	long **partword = _tianlai_block_alloc(vb, ch * sizeof(*partword));
	float scale = 100. / samples_per_partition;

	/* we find the partition type for each partition of each
	   channel.  We'll go back and do the interleaved encoding in a
	   bit.  For now, clarity */

	for (i = 0; i < ch; i++) {
		partword[i] = _tianlai_block_alloc(vb, (n + (samples_per_partition - 1)) / samples_per_partition * sizeof(*partword[i])); //wuchaogang 2014.11.12
		memset(partword[i], 0, (n + (samples_per_partition - 1)) / samples_per_partition * sizeof(*partword[i])); //wuchaogang 2014.11.12
	}

	for (i = 0; i < partvals; i++) {
		int offset = i * samples_per_partition + info->begin;
		for (j = 0; j < ch; j++) {
			int max = 0;
			int ent = 0;
			for (k = 0; k < samples_per_partition; k++) {
				if (abs(in[j][offset + k]) > max)max = abs(in[j][offset + k]);
				ent += abs(in[j][offset + k]);
			}
			ent *= scale;

			for (k = 0; k < possible_partitions - 1; k++)
				if (max <= info->classmetric1[k] &&
					(info->classmetric2[k] < 0 || ent < info->classmetric2[k]))
					break;


			partword[j][i] = k;
		}
	}

	look->frames++;

	return(partword);
}



static int _01forward(avs2audiopack_buffer *opb,
	tianlai_block *vb,
	tianlai_look_residue *vl,
	int **in,
	int ch,
	long **partword,
	int(*encode)(avs2audiopack_buffer *,
		int *,
		int,
		codebook *,
		long *),
	int submap)
{
	long i, j, k, s;
	tianlai_look_residue0 *look = (tianlai_look_residue0 *)vl;
	tianlai_info_residue0 *info = look->info;

	/* move all this setup out later */
	int samples_per_partition = info->grouping;
	int possible_partitions = info->partitions;
	int partitions_per_word = look->phrasebook->dim;
	int n = vb->encLen;//info->end-info->begin;

	int partvals = (int)((float)(n + (samples_per_partition - 1)) / (float)samples_per_partition + 0.0);//n/samples_per_partition;  //wuchaogang 2014.11.12
	long resbits[128];
	long resvals[128];

	memset(resbits, 0, sizeof(resbits));
	memset(resvals, 0, sizeof(resvals));

	/* we code the partition words for each channel, then the residual
	   words for a partition per channel until we've written all the
	   residual words for that partition word.  Then write the next
	   partition channel words... */

	for (s = 0; s < look->stages; s++)
	{
		for (i = 0; i < partvals; )
		{
			/* first we encode a partition codeword for each channel */
			if (s == 0)
			{
				for (j = 0; j < ch; j++)
				{
					long val = partword[j][i];

					for (k = 1; k < partitions_per_word; k++)
					{
						val *= possible_partitions;
						if (i + k < partvals)
							val += partword[j][i + k];
					}

					/* training hack */
					if (val < look->phrasebook->entries)
						look->phrasebits += tianlai_book_encode(look->phrasebook, val, opb);
				}
			}

			/* now we encode interleaved residual values for the partitions */
			for (k = 0; k < partitions_per_word && i < partvals; k++, i++)
			{
				long offset = i * samples_per_partition + info->begin;

				for (j = 0; j < ch; j++)
				{
					if (s == 0)
						resvals[partword[j][i]] += samples_per_partition;

					if (info->secondstages[partword[j][i]] & (1 << s))
					{

						codebook *statebook = look->partbooks[partword[j][i]][s];

						if (statebook)
						{
							int ret;
							long *accumulator = NULL;

							ret = encode(opb, in[j] + offset, samples_per_partition, statebook, accumulator);

							look->postbits += ret;
							resbits[partword[j][i]] += ret;

						}//if(statebook)
					}//if(info->secondstages[partword[j][i]]&(1<<s))
				}//for(j=0;j<ch;j++)
			}// for(k=0;k<partitions_per_word && i<partvals;k++,i++)
		}//for(i=0;i<partvals;)
	}//for(s=0;s<look->stages;s++)

	return(0);
}

/* a truncated packet here just means 'stop working'; it's not an error */
static int _01inverse(tianlai_block *vb, tianlai_look_residue *vl,
	float **in, int ch,
	long(*decodepart)(codebook *, float *,
		avs2audiopack_buffer *, int)) {

	long i, j, k, l, s;
	tianlai_look_residue0 *look = (tianlai_look_residue0 *)vl;
	tianlai_info_residue0 *info = look->info;

	/* move all this setup out later */
	int samples_per_partition = info->grouping;
	int partitions_per_word = look->phrasebook->dim;
	int n = vb->encLen;//lijing

	if (n > 0) {
		int partvals = (int)((float)(n + (samples_per_partition - 1)) / (float)samples_per_partition + 0.0); //wuchaogang 2014.11.12
		int partwords = (partvals + partitions_per_word - 1) / partitions_per_word;
		int ***partword = malloc(ch * sizeof(*partword));

		for (j = 0; j < ch; j++)
			partword[j] = _tianlai_block_alloc(vb, partwords * sizeof(*partword[j]));

		for (s = 0; s < look->stages; s++) {

			/* each loop decodes on partition codeword containing
			   partitions_per_word partitions */
			for (i = 0, l = 0; i < partvals; l++) {
				if (s == 0) {
					/* fetch the partition word for each channel */
					for (j = 0; j < ch; j++) {
						int temp = tianlai_book_decode(look->phrasebook, &vb->opb);

						if (temp == -1 /*|| temp>=64*/)goto eopbreak;
						partword[j][l] = look->decodemap[temp];
						if (partword[j][l] == NULL)goto errout;
					}
				}

				/* now we decode residual values for the partitions */
				for (k = 0; k < partitions_per_word && i < partvals; k++, i++)
					for (j = 0; j < ch; j++) {
						long offset = info->begin + i * samples_per_partition;
						if (info->secondstages[partword[j][l][k]] & (1 << s)) {
							codebook *stagebook = look->partbooks[partword[j][l][k]][s];
							if (stagebook) {
								if (decodepart(stagebook, in[j] + offset, &vb->opb,
									samples_per_partition) == -1)goto eopbreak;
							}
						}
					}
			}
		}
	}
errout:
eopbreak:
	return(0);
}



int res1_forward(avs2audiopack_buffer *opb, tianlai_block *vb, tianlai_look_residue *vl,
	int **in, int *nonzero, int ch, long **partword, int submap) {
	int i, used = 0;
	for (i = 0; i < ch; i++)
		if (nonzero[i])
			in[used++] = in[i];

	if (used) {
		return _01forward(opb, vb, vl, in, used, partword, _encodepart, submap);
	}
	else {
		return(0);
	}
}

long **res1_class(tianlai_block *vb, tianlai_look_residue *vl,
	int **in, int *nonzero, int ch) {
	int i, used = 0;
	for (i = 0; i < ch; i++)
		if (nonzero[i])
			in[used++] = in[i];
	if (used)
		return(_01class(vb, vl, in, used));
	else
		return(0);
}

int res1_inverse(tianlai_block *vb, tianlai_look_residue *vl,
	float **in, int *nonzero, int ch) {
	int i, used = 0;
	for (i = 0; i < ch; i++)
		if (nonzero[i])
			in[used++] = in[i];
	if (used)
		return(_01inverse(vb, vl, in, used, tianlai_book_decodev_add));
	else
		return(0);
}

const tianlai_func_residue residue1_exportbundle = {

  &res0_look,
  &res0_free_info,
  &res0_free_look,
  &res1_class,
  &res1_forward,
  &res1_inverse
};

