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
#include <math.h>
#include <string.h>
#include "masking.h"
#include "psy.h"
#include "scales.h"
#include "lfenc.h"

#define NEGINF -9999.f


tianlai_look_psy_global *_vp_global_look(/*tianlai_info *vi*/) {
	//  codec_setup_info *ci=vi->codec_setup;
	 // tianlai_info_psy_global *gi=&ci->psy_g_param;
	tianlai_look_psy_global *look = calloc(1, sizeof(*look));

	//  look->channels=vi->channels;

	look->ampmax = -9999.;
	//  look->gi=gi;
	return(look);
}

void _vp_global_free(tianlai_look_psy_global *look) {
	if (look) {
		memset(look, 0, sizeof(*look));
		free(look);
	}
}

void _vi_gpsy_free(tianlai_info_psy_global *i) {
	if (i) {
		memset(i, 0, sizeof(*i));
		free(i);
	}
}

void _vi_psy_free(tianlai_info_psy *i) {
	if (i) {
		memset(i, 0, sizeof(*i));
		free(i);
	}
}

static void min_curve(float *c, float *c2) {
	int i;
	for (i = 0; i < EHMER_MAX; i++)if (c2[i] < c[i])c[i] = c2[i];
}

static void max_curve(float *c,	float *c2) {
	int i;
	for (i = 0; i < EHMER_MAX; i++)if (c2[i] > c[i])c[i] = c2[i];
}

static void attenuate_curve(float *c, float att) {
	int i;
	for (i = 0; i < EHMER_MAX; i++)
		c[i] += att;
}

static float ***setup_tone_curves(float curveatt_dB[P_BANDS], float binHz, int n,
	float center_boost, float center_decay_rate) {
	int i, j, k, m;
	float ath[EHMER_MAX];
	float workc[P_BANDS][P_LEVELS][EHMER_MAX];
	float athc[P_LEVELS][EHMER_MAX];
	float *brute_buffer = malloc(n * sizeof(*brute_buffer));

	float ***ret = malloc(sizeof(*ret)*P_BANDS);

	memset(workc, 0, sizeof(workc));

	for (i = 0; i < P_BANDS; i++) {
		/* we add back in the ATH to avoid low level curves falling off to
		   -infinity and unnecessarily cutting off high level curves in the
		   curve limiting (last step). */

		   /* A half-band's settings must be valid over the whole band, and
			  it's better to mask too little than too much */
		int ath_offset = i * 4;
		for (j = 0; j < EHMER_MAX; j++) {
			float min = 999.;
			for (k = 0; k < 4; k++)
				if (j + k + ath_offset < MAX_ATH) {
					if (min > ATH[j + k + ath_offset])min = ATH[j + k + ath_offset];
				}
				else {
					if (min > ATH[MAX_ATH - 1])min = ATH[MAX_ATH - 1];
				}
				ath[j] = min;
		}

		/* copy curves into working space, replicate the 50dB curve to 30
		   and 40, replicate the 100dB curve to 110 */
		for (j = 0; j < 6; j++)
			memcpy(workc[i][j + 2], tonemasks[i][j], EHMER_MAX * sizeof(*tonemasks[i][j]));
		memcpy(workc[i][0], tonemasks[i][0], EHMER_MAX * sizeof(*tonemasks[i][0]));
		memcpy(workc[i][1], tonemasks[i][0], EHMER_MAX * sizeof(*tonemasks[i][0]));

		/* apply centered curve boost/decay */
		for (j = 0; j < P_LEVELS; j++) {
			for (k = 0; k < EHMER_MAX; k++) {
				float adj = center_boost + abs(EHMER_OFFSET - k)*center_decay_rate;
				if (adj < 0. && center_boost>0)adj = 0.;
				if (adj > 0. && center_boost < 0)adj = 0.;
				workc[i][j][k] += adj;
			}
		}

		/* normalize curves so the driving amplitude is 0dB */
		/* make temp curves with the ATH overlayed */
		for (j = 0; j < P_LEVELS; j++) {
			attenuate_curve(workc[i][j], curveatt_dB[i] + 100. - (j < 2 ? 2 : j)*10. - P_LEVEL_0);
			memcpy(athc[j], ath, EHMER_MAX * sizeof(**athc));
			attenuate_curve(athc[j], +100. - j * 10.f - P_LEVEL_0);
			max_curve(athc[j], workc[i][j]);
		}

		/* Now limit the louder curves.

		   the idea is this: We don't know what the playback attenuation
		   will be; 0dB SL moves every time the user twiddles the volume
		   knob. So that means we have to use a single 'most pessimal' curve
		   for all masking amplitudes, right?  Wrong.  The *loudest* sound
		   can be in (we assume) a range of ...+100dB] SL.  However, sounds
		   20dB down will be in a range ...+80], 40dB down is from ...+60],
		   etc... */

		for (j = 1; j < P_LEVELS; j++) {
			min_curve(athc[j], athc[j - 1]);
			min_curve(workc[i][j], athc[j]);
		}
	}

	for (i = 0; i < P_BANDS; i++) {
		int hi_curve, lo_curve, bin;
		ret[i] = malloc(sizeof(**ret)*P_LEVELS);

		/* low frequency curves are measured with greater resolution than
		   the MDCT/FFT will actually give us; we want the curve applied
		   to the tone data to be pessimistic and thus apply the minimum
		   masking possible for a given bin.  That means that a single bin
		   could span more than one octave and that the curve will be a
		   composite of multiple octaves.  It also may mean that a single
		   bin may span > an eighth of an octave and that the eighth
		   octave values may also be composited. */

		   /* which octave curves will we be compositing? */
		bin = floor(fromOC(i*.5) / binHz);
		lo_curve = ceil(toOC(bin*binHz + 1) * 2);
		hi_curve = floor(toOC((bin + 1)*binHz) * 2);
		if (lo_curve > i)lo_curve = i;
		if (lo_curve < 0)lo_curve = 0;
		if (hi_curve >= P_BANDS)hi_curve = P_BANDS - 1;

		for (m = 0; m < P_LEVELS; m++) {
			ret[i][m] = malloc(sizeof(***ret)*(EHMER_MAX + 2));

			for (j = 0; j < n; j++)brute_buffer[j] = 999.;

			/* render the curve into bins, then pull values back into curve.
			   The point is that any inherent subsampling aliasing results in
			   a safe minimum */
			for (k = lo_curve; k <= hi_curve; k++) {
				int l = 0;

				for (j = 0; j < EHMER_MAX; j++) {
					int lo_bin = fromOC(j*.125 + k * .5 - 2.0625) / binHz;
					int hi_bin = fromOC(j*.125 + k * .5 - 1.9375) / binHz + 1;

					if (lo_bin < 0)lo_bin = 0;
					if (lo_bin > n)lo_bin = n;
					if (lo_bin < l)l = lo_bin;
					if (hi_bin < 0)hi_bin = 0;
					if (hi_bin > n)hi_bin = n;

					for (; l < hi_bin && l < n; l++)
						if (brute_buffer[l] > workc[k][m][j])
							brute_buffer[l] = workc[k][m][j];
				}

				for (; l < n; l++)
					if (brute_buffer[l] > workc[k][m][EHMER_MAX - 1])
						brute_buffer[l] = workc[k][m][EHMER_MAX - 1];

			}

			/* be equally paranoid about being valid up to next half ocatve */
			if (i + 1 < P_BANDS) {
				int l = 0;
				k = i + 1;
				for (j = 0; j < EHMER_MAX; j++) {
					int lo_bin = fromOC(j*.125 + i * .5 - 2.0625) / binHz;
					int hi_bin = fromOC(j*.125 + i * .5 - 1.9375) / binHz + 1;

					if (lo_bin < 0)lo_bin = 0;
					if (lo_bin > n)lo_bin = n;
					if (lo_bin < l)l = lo_bin;
					if (hi_bin < 0)hi_bin = 0;
					if (hi_bin > n)hi_bin = n;

					for (; l < hi_bin && l < n; l++)
						if (brute_buffer[l] > workc[k][m][j])
							brute_buffer[l] = workc[k][m][j];
				}

				for (; l < n; l++)
					if (brute_buffer[l] > workc[k][m][EHMER_MAX - 1])
						brute_buffer[l] = workc[k][m][EHMER_MAX - 1];

			}


			for (j = 0; j < EHMER_MAX; j++) {
				int bin = fromOC(j*.125 + i * .5 - 2.) / binHz;
				if (bin < 0) {
					ret[i][m][j + 2] = -999.;
				}
				else {
					if (bin >= n) {
						ret[i][m][j + 2] = -999.;
					}
					else {
						ret[i][m][j + 2] = brute_buffer[bin];
					}
				}
			}

			/* add fenceposts */
			for (j = 0; j < EHMER_OFFSET; j++)
				if (ret[i][m][j + 2] > -200.f)break;
			ret[i][m][0] = j;

			for (j = EHMER_MAX - 1; j > EHMER_OFFSET + 1; j--)
				if (ret[i][m][j + 2] > -200.f)
					break;
			ret[i][m][1] = j;

		}
	}

	return(ret);
}

void _vp_psy_init(tianlai_look_psy *p, tianlai_info_psy *vi,
	tianlai_info_psy_global *gi, int n, long rate) {
	long i, j, lo = -99, hi = 1;
	long maxoc;
	memset(p, 0, sizeof(*p));

	p->eighth_octave_lines = gi->eighth_octave_lines;
	p->shiftoc = rint(log(gi->eighth_octave_lines*8.f) / log(2.f)) - 1;

	p->firstoc = toOC(.25f*rate*.5 / n)*(1 << (p->shiftoc + 1)) - gi->eighth_octave_lines;
	maxoc = toOC((n + .25f)*rate*.5 / n)*(1 << (p->shiftoc + 1)) + .5f;
	p->total_octave_lines = maxoc - p->firstoc + 1;
	p->ath = malloc(n * sizeof(*p->ath));

	p->octave = malloc(n * sizeof(*p->octave));
	p->bark = malloc(n * sizeof(*p->bark));
	p->vi = vi;
	p->n = n;
	p->rate = rate;

	/* AoTuV HF weighting */
	p->m_val = 1.;
	if (rate < 26000) p->m_val = 0;
	else if (rate < 38000) p->m_val = .94;   /* 32kHz */
	else if (rate > 46000) p->m_val = 1.275; /* 48kHz */

	/* set up the lookups for a given blocksize and sample rate */

	for (i = 0, j = 0; i < MAX_ATH - 1; i++) {
		int endpos = rint(fromOC((i + 1)*.125 - 2.) * 2 * n / rate);
		float base = ATH[i];
		if (j < endpos) {
			float delta = (ATH[i + 1] - base) / (endpos - j);
			for (; j < endpos && j < n; j++) {
				p->ath[j] = base + 100.;
				base += delta;
			}
		}
	}

	for (; j < n; j++) {
		p->ath[j] = p->ath[j - 1];
	}

	for (i = 0; i < n; i++) {
		float bark = toBARK(rate / (2 * n)*i);

		for (; lo + vi->noisewindowlomin < i &&
			toBARK(rate / (2 * n)*lo) < (bark - vi->noisewindowlo); lo++);

		for (; hi <= n && (hi < i + vi->noisewindowhimin ||
			toBARK(rate / (2 * n)*hi) < (bark + vi->noisewindowhi)); hi++);

		p->bark[i] = ((lo - 1) << 16) + (hi - 1);

	}

	for (i = 0; i < n; i++)
		p->octave[i] = toOC((i + .25f)*.5*rate / n)*(1 << (p->shiftoc + 1)) + .5f;

	p->tonecurves = setup_tone_curves(vi->toneatt, rate*.5 / n, n,
		vi->tone_centerboost, vi->tone_decay);

	/* set up rolling noise median */
	p->noiseoffset = malloc(P_NOISECURVES * sizeof(*p->noiseoffset));
	for (i = 0; i < P_NOISECURVES; i++)
		p->noiseoffset[i] = malloc(n * sizeof(**p->noiseoffset));

	for (i = 0; i < n; i++) {
		float halfoc = toOC((i + .5)*rate / (2.*n))*2.;
		int inthalfoc;
		float del;

		if (halfoc < 0)halfoc = 0;
		if (halfoc >= P_BANDS - 1)halfoc = P_BANDS - 1;
		inthalfoc = (int)halfoc;
		del = halfoc - inthalfoc;

		for (j = 0; j < P_NOISECURVES; j++)
			p->noiseoffset[j][i] =
			p->vi->noiseoff[j][inthalfoc] * (1. - del) +
			p->vi->noiseoff[j][inthalfoc + 1] * del;

	}
}

void _vp_psy_clear(tianlai_look_psy *p) {
	int i, j;
	if (p) {
		if (p->ath)free(p->ath);
		if (p->octave)free(p->octave);
		if (p->bark)free(p->bark);
		if (p->tonecurves) {
			for (i = 0; i < P_BANDS; i++) {
				for (j = 0; j < P_LEVELS; j++) {
					free(p->tonecurves[i][j]);
				}
				free(p->tonecurves[i]);
			}
			free(p->tonecurves);
		}
		if (p->noiseoffset) {
			for (i = 0; i < P_NOISECURVES; i++) {
				free(p->noiseoffset[i]);
			}
			free(p->noiseoffset);
		}
		memset(p, 0, sizeof(*p));
	}
}

/* octave/(8*eighth_octave_lines) x scale and dB y scale */
static void seed_curve(float *seed,
	const float **curves,
	float amp,
	int oc, int n,
	int linesper, float dBoffset) {
	int i, post1;
	int seedptr;
	const float *posts, *curve;

	int choice = (int)((amp + dBoffset - P_LEVEL_0)*.1f);
	choice = max(choice, 0);
	choice = min(choice, P_LEVELS - 1);
	posts = curves[choice];
	curve = posts + 2;
	post1 = (int)posts[1];
	seedptr = oc + (posts[0] - EHMER_OFFSET)*linesper - (linesper >> 1);

	for (i = posts[0]; i < post1; i++) {
		if (seedptr > 0) {
			float lin = amp + curve[i];
			if (seed[seedptr] < lin)seed[seedptr] = lin;
		}
		seedptr += linesper;
		if (seedptr >= n)break;
	}
}

static void seed_loop(tianlai_look_psy *p,
	const float ***curves,
	const float *f,
	const float *flr,
	float *seed,
	float specmax) {
	tianlai_info_psy *vi = p->vi;
	long n = p->n, i;
	float dBoffset = vi->max_curve_dB - specmax;

	/* prime the working vector with peak values */

	for (i = 0; i < n; i++) {
		float max = f[i];
		long oc = p->octave[i];
		while (i + 1 < n && p->octave[i + 1] == oc) {
			i++;
			if (f[i] > max)max = f[i];
		}

		if (max + 6.f > flr[i]) {
			oc = oc >> p->shiftoc;

			if (oc >= P_BANDS)oc = P_BANDS - 1;
			if (oc < 0)oc = 0;

			seed_curve(seed,
				curves[oc],
				max,
				p->octave[i] - p->firstoc,
				p->total_octave_lines,
				p->eighth_octave_lines,
				dBoffset);
		}
	}
}

static void seed_chase(float *seeds, int linesper, long n) {
	long  *posstack = malloc(n * sizeof(*posstack));
	float *ampstack = malloc(n * sizeof(*ampstack));
	long   stack = 0;
	long   pos = 0;
	long   i;

	for (i = 0; i < n; i++) {
		if (stack < 2) {
			posstack[stack] = i;
			ampstack[stack++] = seeds[i];
		}
		else {
			while (1) {
				if (seeds[i] < ampstack[stack - 1]) {
					posstack[stack] = i;
					ampstack[stack++] = seeds[i];
					break;
				}
				else {
					if (i < posstack[stack - 1] + linesper) {
						if (stack > 1 && ampstack[stack - 1] <= ampstack[stack - 2] &&
							i < posstack[stack - 2] + linesper) {
							/* we completely overlap, making stack-1 irrelevant.  pop it */
							stack--;
							continue;
						}
					}
					posstack[stack] = i;
					ampstack[stack++] = seeds[i];
					break;

				}
			}
		}
	}

	/* the stack now contains only the positions that are relevant. Scan
	   'em straight through */

	for (i = 0; i < stack; i++) {
		long endpos;
		if (i<stack - 1 && ampstack[i + 1]>ampstack[i]) {
			endpos = posstack[i + 1];
		}
		else {
			endpos = posstack[i] + linesper + 1; /* +1 is important, else bin 0 is
											  discarded in short frames */
		}
		if (endpos > n)endpos = n;
		for (; pos < endpos; pos++)
			seeds[pos] = ampstack[i];
	}

	/* there.  Linear time.  I now remember this was on a problem set I
	   had in Grad Skool... I didn't solve it at the time ;-) */

}

/* bleaugh, this is more complicated than it needs to be */
#include<stdio.h>
static void max_seeds(tianlai_look_psy *p,
	float *seed,
	float *flr) {
	long   n = p->total_octave_lines;
	int    linesper = p->eighth_octave_lines;
	long   linpos = 0;
	long   pos;
	float seedmaskcur[4096], octaveseedmaskcur[1000]; //wuchaogang 
	static int count3 = 0, endpos;
	count3++;
	seed_chase(seed, linesper, n); /* for masking */

	pos = p->octave[0] - p->firstoc - (linesper >> 1);

	while (linpos + 1 < p->n) {
		float minV = seed[pos];
		long end = ((p->octave[linpos] + p->octave[linpos + 1]) >> 1) - p->firstoc;
		int startpos = pos;
		if (minV > p->vi->tone_abs_limit)minV = p->vi->tone_abs_limit;
		while (pos + 1 <= end) {
			pos++;
			if ((seed[pos] > NEGINF && seed[pos] < minV) || minV == NEGINF)
				minV = seed[pos];
		}

		endpos = end;
		{int tt;
		for (tt = startpos; tt <= end; tt++)
			octaveseedmaskcur[tt] = minV;
		}
		end = pos + p->firstoc;

		for (; linpos < p->n && p->octave[linpos] <= end; linpos++)
		{
			if (flr[linpos] < minV)flr[linpos] = minV;
			seedmaskcur[linpos] = minV;
		}
	}

	{
		float minV = seed[p->total_octave_lines - 1];
		for (; linpos < p->n; linpos++)
		{
			if (flr[linpos] < minV)flr[linpos] = minV;
			seedmaskcur[linpos] = minV;
		}
	}
}

static void bark_noise_hybridmp(int n, const long *b,
	const float *f,
	float *noise,
	const float offset,
	const int fixed) {

	float N[1024 * 2];
	float X[1024 * 2];
	float XX[1024 * 2];
	float Y[1024 * 2];
	float XY[1024 * 2];

	float tN, tX, tXX, tY, tXY;
	int i;

	int lo, hi;
	float R = 0.f;
	float A = 0.f;
	float B = 0.f;
	float D = 1.f;
	float w, x, y;

	tN = tX = tXX = tY = tXY = 0.f;

	y = f[0] + offset;
	if (y < 1.f) y = 1.f;

	w = y * y * .5;

	tN += w;
	tX += w;
	tY += w * y;

	N[0] = tN;
	X[0] = tX;
	XX[0] = tXX;
	Y[0] = tY;
	XY[0] = tXY;

	for (i = 1, x = 1.f; i < n; i++, x += 1.f) {

		y = f[i] + offset;
		if (y < 1.f) y = 1.f;

		w = y * y;

		tN += w;
		tX += w * x;
		tXX += w * x * x;
		tY += w * y;
		tXY += w * x * y;

		N[i] = tN;
		X[i] = tX;
		XX[i] = tXX;
		Y[i] = tY;
		XY[i] = tXY;
	}

	for (i = 0, x = 0.f;; i++, x += 1.f) {

		lo = b[i] >> 16;
		if (lo >= 0) break;
		hi = b[i] & 0xffff;

		tN = N[hi] + N[-lo];
		tX = X[hi] - X[-lo];
		tXX = XX[hi] + XX[-lo];
		tY = Y[hi] + Y[-lo];
		tXY = XY[hi] - XY[-lo];

		A = tY * tXX - tX * tXY;
		B = tN * tXY - tX * tY;
		D = tN * tXX - tX * tX;
		R = (A + x * B) / D;
		if (R < 0.f)
			R = 0.f;

		noise[i] = R - offset;
	}

	for (;; i++, x += 1.f) {

		lo = b[i] >> 16;
		hi = b[i] & 0xffff;
		if (hi >= n)break;

		tN = N[hi] - N[lo];
		tX = X[hi] - X[lo];
		tXX = XX[hi] - XX[lo];
		tY = Y[hi] - Y[lo];
		tXY = XY[hi] - XY[lo];

		A = tY * tXX - tX * tXY;
		B = tN * tXY - tX * tY;
		D = tN * tXX - tX * tX;
		R = (A + x * B) / D;
		if (R < 0.f) R = 0.f;

		noise[i] = R - offset;
	}
	for (; i < n; i++, x += 1.f) {

		R = (A + x * B) / D;
		if (R < 0.f) R = 0.f;

		noise[i] = R - offset;
	}

	if (fixed <= 0) return;

	for (i = 0, x = 0.f;; i++, x += 1.f) {
		hi = i + fixed / 2;
		lo = hi - fixed;
		if (lo >= 0)break;

		tN = N[hi] + N[-lo];
		tX = X[hi] - X[-lo];
		tXX = XX[hi] + XX[-lo];
		tY = Y[hi] + Y[-lo];
		tXY = XY[hi] - XY[-lo];


		A = tY * tXX - tX * tXY;
		B = tN * tXY - tX * tY;
		D = tN * tXX - tX * tX;
		R = (A + x * B) / D;

		if (R - offset < noise[i]) noise[i] = R - offset;
	}
	for (;; i++, x += 1.f) {

		hi = i + fixed / 2;
		lo = hi - fixed;
		if (hi >= n)break;

		tN = N[hi] - N[lo];
		tX = X[hi] - X[lo];
		tXX = XX[hi] - XX[lo];
		tY = Y[hi] - Y[lo];
		tXY = XY[hi] - XY[lo];

		A = tY * tXX - tX * tXY;
		B = tN * tXY - tX * tY;
		D = tN * tXX - tX * tX;
		R = (A + x * B) / D;

		if (R - offset < noise[i]) noise[i] = R - offset;
	}
	for (; i < n; i++, x += 1.f) {
		R = (A + x * B) / D;
		if (R - offset < noise[i]) noise[i] = R - offset;
	}
}

void _vp_noisemask(tianlai_look_psy *p,
	float *logmdct,
	float *logmask) {

	int i, n = p->n;
	float work[2048];

	bark_noise_hybridmp(n, p->bark, logmdct, logmask,
		140., -1);//wuchaogang +10 2013.7.22

	for (i = 0; i < n; i++) work[i] = logmdct[i] - logmask[i];

	bark_noise_hybridmp(n, p->bark, work, logmask, 0.0 + 0,//wuchaogang +10 2013.7.22
		p->vi->noisewindowfixed);

	for (i = 0; i < n; i++)work[i] = logmdct[i] - work[i];



	for (i = 0; i < n; i++) {
		int dB = logmask[i] + .5;
		if (dB >= NOISE_COMPAND_LEVELS)dB = NOISE_COMPAND_LEVELS - 1;
		if (dB < 0)dB = 0;
		logmask[i] = work[i] + p->vi->noisecompand[dB];
	}
}

void _vp_tonemask(tianlai_look_psy *p,
	float *logfft,
	float *logmask,
	float global_specmax,
	float local_specmax) {

	int i, n = p->n;
	float seed[1024];
	float att = local_specmax + p->vi->ath_adjatt;
	for (i = 0; i < p->total_octave_lines; i++)seed[i] = NEGINF;

	/* set the ATH (floating below localmax, not global max by a
	   specified att) */
	if (att < p->vi->ath_maxatt)att = p->vi->ath_maxatt;

	for (i = 0; i < n; i++)
		logmask[i] = p->ath[i] + att;
#if OUTPUT_TONE_NOISE
	{FILE *fp_out = fopen("ath.txt", "a");
	fprintf(fp_out, "global_specmax %f %f:	", global_specmax, local_specmax);
	for (int j = 0; j < n; j++)
	{
		fprintf(fp_out, "%f	", p->ath[j]);
	}
	fprintf(fp_out, "\n");
	fclose(fp_out);
	}

	{FILE *fp_out = fopen("ath.txt", "a");
	fprintf(fp_out, "logfft:	");
	for (int j = 0; j < n; j++)
	{
		fprintf(fp_out, "%f	", logfft[j]);
	}
	fprintf(fp_out, "\n");
	fclose(fp_out);
	}
#endif
	/* tone masking */
	seed_loop(p, (const float ***)p->tonecurves, logfft, logmask, seed, global_specmax);

	max_seeds(p, seed, logmask);
#if OUTPUT_TONE_NOISE
	{FILE *fp_out = fopen("ath.txt", "a");
	fprintf(fp_out, "logmask:	");
	for (int j = 0; j < n; j++)
	{
		fprintf(fp_out, "%f	", logmask[j]);
	}
	fprintf(fp_out, "\n");
	fclose(fp_out);
	}
#endif
}

void _vp_offset_and_mix(tianlai_look_psy *p,
	float *noise,
	float *tone,
	int offset_select,
	float *logmask,
	float *mdct,
	float *logmdct,
	int useBWE,
	int innerloopcount) {
	int i, n = p->n;
	float de, coeffi, cx;/* AoTuV */
	float toneatt = 21;//p->vi->tone_masteratt[offset_select];

	cx = p->m_val;

	for (i = 0; i < n; i++) {
		float val = noise[i] + p->noiseoffset[offset_select][i] + innerloopcount;
		if (val > p->vi->noisemaxsupp)val = p->vi->noisemaxsupp;
		logmask[i] = max(val, tone[i] + toneatt) - 3;//

		if (useBWE == 1)
			logmask[i] = max(val, tone[i] + toneatt) - 3 + innerloopcount - 15;//
		else
			logmask[i] = max(val, tone[i] + toneatt) - 3 + innerloopcount - 15;//

		/* AoTuV */
		/** @ M1 **
			The following codes improve a noise problem.
			A fundamental idea uses the value of masking and carries out
			the relative compensation of the MDCT.
			However, this code is not perfect and all noise problems cannot be solved.
			by Aoyumi @ 2004/04/18
		*/

		if ((offset_select == 1) && (useBWE == 0)) {
			coeffi = -17.2;       /* coeffi is a -17.2dB threshold */
			val = val - logmdct[i];  /* val == mdct line value relative to floor in dB */

			if (val > coeffi) {
				/* mdct value is > -17.2 dB below floor */

				de = 1.0 - ((val - coeffi)*0.005*cx);
				/* pro-rated attenuation:
				   -0.00 dB boost if mdct value is -17.2dB (relative to floor)
				   -0.77 dB boost if mdct value is 0dB (relative to floor)
				   -1.64 dB boost if mdct value is +17.2dB (relative to floor)
				   etc... */

				if (de < 0) de = 0.0001;
			}
			else
				/* mdct value is <= -17.2 dB below floor */

				de = 1.0 - ((val - coeffi)*0.0003*cx);
			/* pro-rated attenuation:
			   +0.00 dB atten if mdct value is -17.2dB (relative to floor)
			   +0.45 dB atten if mdct value is -34.4dB (relative to floor)
			   etc... */

			mdct[i] *= de;

		}
	}
}

float _vp_ampmax_decay(float amp, void *p) {
	tianlai_dsp_state *vd = (tianlai_dsp_state *)p;
	tianlai_info *vi = vd->vi;
	codec_setup_info *ci = vi->codec_setup;
	tianlai_info_psy_global *gi = &ci->psy_g_param;

	int n = ci->blocksizes[vd->W] / 2;
	float secs = (float)n / vi->rate * 1; //wuchaogang 2013.7.22 *2

	amp += secs * gi->ampmax_att_per_sec;
	if (amp < -9999)amp = -9999;
	return(amp);
}



/* this is for per-channel noise normalization */
static int apsort(const void *a, const void *b) {
	float f1 = **(float**)a;
	float f2 = **(float**)b;
	return (f1 < f2) - (f1 > f2);
}

/*static void flag_lossless(int limit, float prepoint, float postpoint, float *mdct,
                         float *floor, int *flag, int i, int jn){
  int j;
  for(j=0;j<jn;j++){
    float point = j>=limit-i ? postpoint : prepoint;
    float r = fabs(mdct[j])/floor[j];
    if(r<point)
      flag[j]=0;
    else
      flag[j]=1;
  }
}*/

/* Overload/Side effect: On input, the *q vector holds either the
   quantized energy (for elements with the flag set) or the absolute
   values of the *r vector (for elements with flag unset).  On output,
   *q holds the quantized energy for all elements */
static float noise_normalize(tianlai_look_psy *p,/*, int limit*/ float *r, float *q, float *f, /*int *flags,*/ float acc, int i, int n, int *out) {

	tianlai_info_psy *vi = p->vi;
	float *sort[32];
	int j, count = 0;
	int start = (vi->normal_p ? vi->normal_start - i : n);
	if (start > n)start = n;


	/* force classic behavior where only energy in the current band is considered */
	acc = 0.f;

	/* still responsible for populating *out where noise norm not in
	   effect.  There's no need to [re]populate *q in these areas */
	for (j = 0; j < start; j++) {
		//    if(!flags || !flags[j]){ /* lossless coupling already quantized.
									//    Don't touch; requantizing based on
									//    energy would be incorrect. */
		float ve = q[j] / f[j];
		if (r[j] < 0)
			out[j] = -rint(sqrt(ve));
		else
			out[j] = rint(sqrt(ve));
		// }
	}

	/* sort magnitudes for noise norm portion of partition */
	for (; j < n; j++) {
		//if(!flags || !flags[j])
		{ /* can't noise norm elements that have
									already been loslessly coupled; we can
									only account for their energy error */
			float ve = q[j] / f[j];
			/* Despite all the new, more capable coupling code, for now we
			   implement noise norm as it has been up to this point. Only
			   consider promotions to unit magnitude from 0.  In addition
			   the only energy error counted is quantizations to zero. */
			   /* also-- the original point code only applied noise norm at > pointlimit */
			if (ve < .25f/* && (!flags || j>=limit-i)*/) {
				acc += ve;
				sort[count++] = q + j; /* q is fabs(r) for unflagged element */
			}
			else {
				/* For now: no acc adjustment for nonzero quantization.  populate *out and q as this value is final. */
				if (r[j] < 0)
					out[j] = -rint(sqrt(ve));
				else
					out[j] = rint(sqrt(ve));
				q[j] = out[j] * out[j] * f[j];
			}
		}/* else{
			again, no energy adjustment for error in nonzero quant-- for now
			}*/
	}

	if (count) {
		/* noise norm to do */
		qsort(sort, count, sizeof(*sort), apsort);
		for (j = 0; j < count; j++) {
			int k = sort[j] - q;
			if (acc >= vi->normal_thresh) {
				out[k] = unitnorm(r[k]);
				acc -= 1.f;
				q[k] = f[k];
			}
			else {
				out[k] = 0;
				q[k] = 0.f;
			}
		}
	}

	return acc;
}

/* Noise normalization, quantization and coupling are not wholly
   seperable processes in depth>1 coupling. */
void _vp_couple_quantize_normalize(//int blobno,
								  // tianlai_info_psy_global *g,
	tianlai_look_psy *p,
	//tianlai_info_mapping0 *vi,
	float mdct[],
	int   iwork[],
	float *quantmask,
	int    nonzero,
	// int     sliding_lowpass,
	int     ch) {

	int i;
	int n = p->n;
	int partition = (p->vi->normal_p ? p->vi->normal_partition : 16);

#if 0
	float de = 0.1*p->m_val; /* a blend of the AoTuV M2 and M3 code here and below */
#endif


  /* mdct is our raw mdct output, floor not removed. */
  /* inout passes in the ifloor, passes back quantized result */

  /* unquantized energy (negative indicates amplitude has negative sign) */
	float raw[1024 * 2];

	/* dual pupose; quantized energy (if flag set), othersize fabs(raw) */
	float quant[1024 * 2];

	/* floor energy */
	float floor[1024 * 2];

	/* flags indicating raw/quantized status of elements in raw vector */
	int   flag[1024 * 2];

	/* non-zero flag working vector */
	int    nz;

	/* energy surplus/defecit tracking */
	float  acc[1024 * 2];


	for (i = 0; i < ch; i++)
		acc[i] = 0.f;

	for (i = 0; i < n; i += partition) {
		int j, jn = partition > n - i ? n - i : partition;
		int track = 0;

		nz = nonzero;

		/* prefill */
		memset(flag, 0, ch*partition * sizeof(*flag));

		{
			int *iout = &iwork[i];
			if (nz) {

				for (j = 0; j < jn; j++)
					floor[j] = quantmask[i + j];


				for (j = 0; j < jn; j++) {
					quant[j] = raw[j] = mdct[i + j] * mdct[i + j];
					if (mdct[i + j] < 0.f) raw[j] *= -1.f;
					floor[j] *= floor[j];
				}

				acc[track] = noise_normalize(p, raw, quant, floor, acc[track], i, jn, iout);

			}
			else {
				for (j = 0; j < jn; j++) {
					floor[j] = 1e-10f;
					raw[j] = 0.f;
					quant[j] = 0.f;
					flag[j] = 0;
					iout[j] = 0;
				}
				acc[track] = 0.f;
			}
			track++;
		}
	}
}
