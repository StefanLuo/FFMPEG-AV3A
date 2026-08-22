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
#include <assert.h>
#include <math.h>
#include <memory.h>

#include "av3enc.h"
#include "i2r_encoder.h"

#define NR_OF_BLOCKS 1
#define AATF_FLAG    0
#define AATF_BUFFER_FULNESS 0x7FF

/* Returns the sample rate index */
int GetSRIndex(unsigned int sampleRate)
{
	int fsIdx;
	switch(sampleRate)
	{
	case 96000:
		fsIdx = FSIDX_96;
		break;
	case 88200:
		fsIdx = FSIDX_88;
		break;
	case 64000:
		fsIdx = FSIDX_64;
		break;
	case 48000:
		fsIdx = FSIDX_48;
		break;
	case 44100:
		fsIdx = FSIDX_44;
		break;
	case 32000:
		fsIdx = FSIDX_32;
		break;
	case 24000:
		fsIdx = FSIDX_24;
		break;
	case 22050:
		fsIdx = FSIDX_22;
		break;
	case 16000:
		fsIdx = FSIDX_16;
		break;
	case 12000:
		fsIdx = FSIDX_12;
		break;	
	case 11025:
		fsIdx = FSIDX_11;
		break;
	case 8000:
		fsIdx = FSIDX_8;
		break;
	case 192000:
		fsIdx = FSIDX_192;
		break;
	case 176400:
		fsIdx = FSIDX_176;
		break;
	case 128000:
		fsIdx = FSIDX_128;
		break;
	default:
		fsIdx = FSIDX_EXT;
		break;
	}
	return fsIdx;

}


EncFramePtr EncOpen(unsigned long sampleRate,
                    unsigned int numChannels,
                    unsigned long *inputSamples,
                    unsigned long *maxOutputBytes)
{
    unsigned int channel;
    EncFramePtr hEncoder;

    *inputSamples = 1024*numChannels;
	*maxOutputBytes = 6144*numChannels;//(6144/8)*numChannels; //shumin.xu 20211022

    hEncoder = (EncFrame*)malloc(sizeof(EncFrame));
    memset(hEncoder, 0, sizeof(EncFrame));

    hEncoder->config.numChannels = numChannels;
    hEncoder->config.sampleRate = sampleRate;
    hEncoder->config.sampleRateIdx = GetSRIndex(sampleRate);

    /* Initialize variables to default values */
    hEncoder->frameNum = 0;

	/* default channel map is straight-through */
	for( channel = 0; channel <8; channel++ )
		hEncoder->config.channel_map[channel] = channel;
	
    /*
        by default we have to be compatible with all previous software
        which assumes that we will generate ADTS
        /AV
    */
    hEncoder->config.outputFormat = 0;

    for (channel = 0; channel < numChannels; channel++) 
	{
        hEncoder->sampleBuff[channel] = NULL;
    }

    /* Initialize coder functions */
	hEncoder->hLosslessEnc = 0; 
	
	/* Return handle */
    return hEncoder;
}

int EncClose(EncFramePtr hEncoder)
{
    unsigned int channel;

    /* Free remaining buffer memory */
    for (channel = 0; channel < hEncoder->config.numChannels; channel++) 
	{
		if (hEncoder->sampleBuff[channel])
			free(hEncoder->sampleBuff[channel]);
    }

	i2r_EncoderRelease(hEncoder->hLosslessEnc); 


    /* Free handle */
    if (hEncoder) 
		free(hEncoder);

    return 0;
}

void GetChannelInfo(ChannelInfo *channelInfo, int numChannels)
{
	int i, j = 0;

	const unsigned char *pucMappings = 
		defChMappings[numChannels - 1];
	for (i = 0; i < MAX_CHANNELS; )
	{
		unsigned char chPos;
		chPos	= *pucMappings++;
		if ((FC==chPos) || (LF==chPos)){
			channelInfo[j++].paired_ch     = -1;
			i = i + 1;
		}else if (0xFF != chPos){
			chPos	= *pucMappings;
			if (0xFF != chPos){
				channelInfo[j++].paired_ch =  1;
				channelInfo[j++].paired_ch =  1;
				i = i + 2;	pucMappings++;
			}else{
				channelInfo[j++].paired_ch = -1;
				i = i + 1;
			}
		}else{
			i = i + 1;
		}
	}
}

int EncEncode(EncFramePtr hEncoder,
              int *inputBuffer,
              unsigned int samplesInput,
              unsigned char *outputBuffer)
{
    unsigned int channel, i;
    int sb, frameBytes, temp;
	static int frameStart = 1;

    /* local copy's of parameters */
    ChannelInfo *channelInfo = hEncoder->channelInfo;
	unsigned int sampleRate  = hEncoder->config.sampleRate;
    unsigned int numChannels = hEncoder->config.numChannels;
	unsigned int outputformat = hEncoder->config.outputFormat;

	// for AATFHeader
	Encoder_t *pEnc = hEncoder->hLosslessEnc;

    /* Increase frame number */
    hEncoder->frameNum++;

    if (samplesInput == 0)
        return 0; 

    /* Determine the channel configuration */
    GetChannelInfo(channelInfo, numChannels);

    /* Update current sample buffers & remap channels*/
    for (channel = 0; channel < numChannels; channel++) 
	{
		int samples_per_channel = samplesInput/numChannels;
		float *input_channel;

		if (!hEncoder->sampleBuff[channel])
			hEncoder->sampleBuff[channel] = (double*)malloc(FRAME_LEN*sizeof(double));

		input_channel = (float*)inputBuffer + hEncoder->config.channel_map[channel];

		for (i = 0; i < samples_per_channel; i++)
		{
			hEncoder->sampleBuff[channel][i] = (double)*input_channel;
			input_channel += numChannels;
		}

        for (i = (int)(samplesInput/numChannels); i < FRAME_LEN; i++)
	        hEncoder->sampleBuff[channel][i] = 0.0; // zero padding
    }

	for (channel = 0; channel < numChannels; channel++)
	{
		for (i = 0; i < FRAME_LEN; i++)
		{
			if (pEnc->pcm_scale > 0)
				pEnc->x[channel][i] = (int)(hEncoder->sampleBuff[channel][i] * (2 << pEnc->pcm_scale));
			else if (pEnc->pcm_scale < 0)
				pEnc->x[channel][i] = (int)(hEncoder->sampleBuff[channel][i] / (2 << (-pEnc->pcm_scale)));
			else
				pEnc->x[channel][i] = (int)(hEncoder->sampleBuff[channel][i]);
		}
	}

	{
		/////////////////////////////////////////////////////////////////////
		start_outputing_bits();	
		if(outputformat == 1 && frameStart) {
			int headSize = 0;
			frameStart = 0;
			if((headSize = WriteAASFHeader(
				numChannels,hEncoder->config.sampleRateIdx,
				hEncoder->config.resolution, hEncoder->hLosslessEnc->Freq,
				hEncoder->config.codecId, 0)) == 0)
			{
				fprintf(stderr,"\nerror in write aasf audio header.\n");
				return 0;
			}
			hEncoder->headSize = headSize;
		}

		if(outputformat == 2){
			WriteAATFHeader(hEncoder->config.sampleRateIdx,
				numChannels, hEncoder->config.resolution, 
				hEncoder->hLosslessEnc->Freq,
				hEncoder->config.codecId,
				0/*hEncoder->config.codingProfile*/);
			FlushBuffer(outputBuffer);
		}
		/////////////////////////////////////////////////////////////////////
	}

	{
		int numChannelsLeft;
		int numChannelsCoded;

		ChannelInfo *localChannelInfo;
		frameBytes   =0;

		pEnc->ll_bits=0;
		LosslessFlushBufferInit();

		for(i = 0; i < numChannels; i += numChannelsCoded)
		{
			int ll_byte;
			localChannelInfo = &channelInfo[i];
			/* current encoded channel number */
			if(localChannelInfo->paired_ch == -1){
				/* no pair channel */
				numChannelsCoded = 1;
			}else{
				numChannelsCoded = 2;
			}
			i2r_EncodeFrame(pEnc, i, numChannelsCoded);
			pEnc->ll_bits += GetBitstreamSize();
			LosslessFlushBufferWithLength();
		}
	}

	{/* write to bit-stream */
		int ll_bytes = pEnc->ll_bits / 8;
		start_outputing_bits();
		output_byte(ll_bytes,16);
		pEnc->ll_bits += 16;
		FlushBufferWithLength(outputBuffer);
	}
	frameBytes = pEnc->ll_bits/8;
	
    return frameBytes;
}