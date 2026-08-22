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

#include "common.h"
#include "av3dec.h"
#include "funcs.h"

int fill_buffer(av3_buffer *b)
{
    int bread;

    if (b->bytes_consumed > 0)
    {
        if (b->bytes_into_buffer)
        {
            memmove((void*)b->buffer, (void*)(b->buffer + b->bytes_consumed),
                b->bytes_into_buffer*sizeof(unsigned char));
        }

        if (!b->at_eof)
        {
            bread = fread((void*)(b->buffer + b->bytes_into_buffer), 1,
                b->bytes_consumed, b->infile);

            if (bread != b->bytes_consumed)
                b->at_eof = 1;

            b->bytes_into_buffer += bread;
        }

        b->bytes_consumed = 0;

        if (b->bytes_into_buffer > 3)
        {
            if (memcmp(b->buffer, "TAG", 3) == 0)
                b->bytes_into_buffer = 0;
        }
        if (b->bytes_into_buffer > 11)
        {
            if (memcmp(b->buffer, "LYRICSBEGIN", 11) == 0)
                b->bytes_into_buffer = 0;
        }
        if (b->bytes_into_buffer > 8)
        {
            if (memcmp(b->buffer, "APETAGEX", 8) == 0)
                b->bytes_into_buffer = 0;
        }
    }

    return 1;
}

void advance_buffer(av3_buffer *b, int bytes)
{
    b->file_offset += bytes;
    b->bytes_consumed = bytes;
    b->bytes_into_buffer -= bytes;
}

AV3DecFrameInfoPtr  AV3DecOpen(void)
{
    AV3DecFrameInfoPtr hDecoder = NULL;

    if ((hDecoder = (AV3DecFrameInfoPtr)av3_malloc(sizeof(AV3DecFrameInfo))) == NULL)
        return NULL;

    memset(hDecoder, 0, sizeof(AV3DecFrameInfo));

    hDecoder->config.outputFormat  = AV3_PCM_16BIT;
	hDecoder->config.av3ObjectType = AVS_LOSSLESS;	/* now only basic level*/
	hDecoder->config.header_type = AV3_HEAD_DEFAULT;
    hDecoder->config.defSampleRate = 44100;             /* Default: 44.1kHz */ 
    hDecoder->frameLength = 1024;
    hDecoder->frame = 0;
    hDecoder->sample_buffer = NULL;
    return hDecoder;
}

AV3DecCfgPtr  AV3DecGetCurrentConfiguration(AV3DecFrameInfoPtr hDecoder)
{
    if (hDecoder)
    {
        AV3DecCfgPtr config = &(hDecoder->config);
        return config;
    }
    return NULL;
}

uint8_t  AV3DecSetConfiguration(AV3DecFrameInfoPtr hDecoder,AV3DecCfgPtr config)
{
    if (hDecoder && config)
    {
        /* check if we can decode this object type */
		if(config->av3ObjectType!=AVS_LOSSLESS)
		{
			fprintf(stdout,"Object type error!\n");
			return 0;
		}
        hDecoder->config.av3ObjectType = config->av3ObjectType;
		hDecoder->object_type = hDecoder->config.av3ObjectType;

        /* samplerate: anything but 0 should be possible */
        if (config->defSampleRate == 0)
		{
			fprintf(stdout,"Object type error!\n");
			return 0;
		}
        hDecoder->config.defSampleRate = config->defSampleRate;

        /* check output file format */
        if ((config->outputFormat < 1) || (config->outputFormat > 2))
   		{
			fprintf(stdout,"Output file format error!\n");
			return 0;
		}
        hDecoder->config.outputFormat = config->outputFormat;

		/* check output pcm data format */
        if ((config->pcmFormat < 0) || (config->pcmFormat > 4))
   		{
			fprintf(stdout,"Output pcm data format error!\n");
			return 0;
		}
        hDecoder->config.pcmFormat = config->pcmFormat;

		/* check header type */
		if( config->header_type != AV3_HEAD_DEFAULT)
		{
			fprintf(stdout,"Header type error!\n");
			return 0;
		}
		hDecoder->config.header_type = config->header_type;

        /* OK */
        return 1;
    }
    return 0;
}

void  AV3DecClose(AV3DecFrameInfoPtr hDecoder)
{
    if (hDecoder == NULL)
        return;

    if (hDecoder->sample_buffer) av3_free(hDecoder->sample_buffer);
    if (hDecoder) av3_free(hDecoder);
}


