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
#include <string.h>
#include "enc.h"
#include "..\general\bweenc\encoder.h"

#define AVS2ENC_BLOCKSIZE  1024


const char *phelpstr3D = "avs encoder verson 2.0\n\n\
usage: avs2enc -if <infile> -of <outfile> [options]\n\n\
RECOMMENDED:\n\
when input.wav is a mono wave:\n\
    avs2enc -if input.wav -of output.avs -b 20000\n\n\
OPTIONS:\n\
    -b bitrate\t\tset the bitrate, in bps, default 16000\n\
	-codec_id 0,1\tset audio codec id\n\
	             \t0:general audio encoder;1:lossless audio encoder; default is general audio encoder;\n\
	-f 1,2\tset output format\n\
	      \t1:AASF;2:AATF;default is AASF;\n\
    -h or --help\tshow this list of options\n";

extern int objbitrate;

static void parsecmdline(int argc,
                         char *argv[],
                         char **input_filename,
                         char **output_filename,
						 char **filepath,
                         //char **config_filename, 
                         ChanInfo * conf)
{
	conf->bitRate = 48000;
	conf->outputFormat = 2;
	conf->use_mono_encode = 0;
	conf->codec_id = 0;
	conf->coding_profile = 1;
	conf->anc_data_index = 0;
		
	argc--;
	argv++;

	if (argc == 0)
	{
		fprintf(stderr, phelpstr3D, *argv);
		exit(EXIT_SUCCESS);
	}
	
	while (argc > 0)
	{
		if (!strcmp(*argv, "-if"))
		{
		  argv++;
		  argc--;
		  *input_filename = *argv;
		}
		else if (!strcmp(*argv, "-of"))
		{
		  argv++;
		  argc--;
		  *output_filename = *argv;
		} 
		else if (!strcmp(*argv, "-fp"))
		{
			argv++;
			argc--;
			*filepath = *argv;
		}
		else if (!strcmp(*argv, "-b"))
		{
			argv++;
			argc--;
			conf->bitRate = atoi(*argv);
		}
		else if (!strcmp(*argv, "-f"))
		{
			argv++;
			argc--;
			conf->outputFormat = atoi(*argv);
		}
		else if (!strcmp(*argv, "-mono"))
		{
			conf->use_mono_encode = 1;
		}
		else if (!strcmp(*argv, "-codec_id"))
		{
			argv++;
			argc--;
			conf->codec_id = atoi(*argv);
		}
		else if (!strcmp(*argv, "-coding_profile"))
		{
			argv++;
			argc--;
			conf->coding_profile = atoi(*argv);
		}
		else if (!strcmp(*argv, "-head"))
		{
			argv++;
			argc--;
			conf->headflag = atoi(*argv);
		}
		else if (!strcmp(*argv, "-anc"))
		{
			argv++;
			argc--;
			conf->anc_data_index = atoi(*argv);
		}
		else if (!strcmp(*argv, "-h") || !strcmp(*argv, "--help"))
		{
		fprintf(stderr, phelpstr3D, *argv);
		exit(EXIT_SUCCESS);
		}

		argv++;
		argc--;
	}
}


int threeD_encoder(int argc, char *argv[])
{
	int fill_element_num = 0;
	int useSuperMode = 0;
	int cpe_config = 0;
	int PCAGroupmodeHeader = 0xFF;
	int sampleRateCore;
	int objsampleRateCore;
	char *input_filename;
	char *output_filename;
	char *filepath = NULL;
	//char *config_filename;
	ChanInfo inputInfo;
	ChanInfo objinputInfo;
	FILE *fInputFile;               /* File of sound data  */
	FILE *fOutputFile;              /* File of serial bits for transmission  */
	short numOfChannels, objnumOfChannels, bitsPerSample, objbitsPerSample;
	long frame = 0, samplingRate, objsamplingRate, dataSize, objdataSize;
	HANDLE_STAvs2Enc stAvs2Init;
	HANDLE_STAvs2Enc objstAvs2Init[128];
	int inLen;
	char  TimeDataPcmBuffer[AVS2ENC_BLOCKSIZE * 2 * MAX_ALLCHANNEL * 4];
	int OutBytes, objOutbytes;
	unsigned char sampleData[1024 * 64];
	unsigned char sampleDataObj[1024 * 64];
	
	int numSamplesRead;
	int inSamples;
	int objinSamples;
	int cid;
	long frameLength = 0;

	char obInfo[1024];
	char ptrpos[128][100];
	int cnt = 0;
	int txtNum;
	int ObjectNum;
	int Object_ID[128];
	float a[128][16];
	int txt;
	long objdataSizeSet[128];

	parsecmdline(argc, argv, &input_filename, &output_filename, &filepath, /*&config_filename,*/&inputInfo);

	FILE* posInfo[128];
	char tmp[128];
	if (filepath != NULL)
		sprintf(tmp, "%s%s", filepath, "\\position.txt");
	else
		sprintf(tmp, "%s", "position.txt");
	FILE* poscontain = fopen(tmp, "rb");//打开记录对象信息位置的TXT

	unsigned char headbuffer[9];

	char ptrobj[128][100];
	FILE* objInfo[128];
	char tmp1[128];
	if (filepath != NULL)
		sprintf(tmp1, "%s%s", filepath, "\\object.txt");
	else sprintf(tmp1, "%s", "object.txt");
	FILE* objcontain = fopen(tmp1, "rb");

	while (!feof(poscontain))
	{
		fscanf(poscontain, "%s", ptrpos[cnt]);

		printf("%s\n", ptrpos[cnt]);
		cnt++;
	}
	for (txt = 0; txt < cnt; txt++)
		posInfo[txt] = fopen(ptrpos[txt], "rb");
	txtNum = cnt;

	cnt = 0;
	while (!feof(objcontain))
	{
		fscanf(objcontain, "%s", ptrobj[cnt]);

		printf("%s\n", ptrobj[cnt]);
		cnt++;
	}


	for (txt = 0; txt < cnt; txt++)
	{
		objInfo[txt] = Wave_fopen(ptrobj[txt], "rb", &objnumOfChannels, &objsamplingRate, &objbitsPerSample, &objdataSize);
		objdataSizeSet[txt] = objdataSize;
		if (objInfo[txt] == NULL)
		{
			printf("error open the obj file!");
			exit(EXIT_FAILURE);
		}
	}
	objinputInfo.outputFormat = 2;
	objinputInfo.use_mono_encode = 0;
	objinputInfo.codec_id = 0;
	objinputInfo.coding_profile = 1;
	objinputInfo.bitRate = objbitrate;
	objdataSize *= objnumOfChannels;
	objinputInfo.nChannels = objnumOfChannels;
	objinputInfo.sampleRate = objsamplingRate;
	objinputInfo.bitsPerSample = objbitsPerSample;
	objsampleRateCore = objinputInfo.sampleRate;


	if ((fInputFile = Wave_fopen(input_filename, "rb", &numOfChannels, &samplingRate,
		&bitsPerSample, &dataSize)) == NULL)
	{
		fprintf(stderr, "Error opening the input file %s.\n", input_filename);
		exit(EXIT_FAILURE);
	}

	if ((fOutputFile = fopen(output_filename, "wb")) == NULL)
	{
		fprintf(stderr, "Error opening the output file %s.\n", output_filename);
		exit(EXIT_FAILURE);
	}


	dataSize *= numOfChannels;
	inputInfo.nChannels = numOfChannels;
	inputInfo.sampleRate = samplingRate;
	inputInfo.bitsPerSample = bitsPerSample;
	sampleRateCore = inputInfo.sampleRate;


	if (sampleRateCore < 32000)
	{
		return 0;
	}


	sampleRateCore = sampleRateCore / 2;

	printf("%d\n", numOfChannels);

	if (inputInfo.codec_id == 0)
	{
		//initialization
		for (txt = 0; txt < txtNum; txt++)
			init_avs2_general_encoder_obj(&objinputInfo, &objstAvs2Init[txt], &inLen, txt, fill_element_num);
		objinSamples = inLen;

 		init_avs2_general_encoder(&inputInfo, &stAvs2Init, &inLen, &cpe_config, &PCAGroupmodeHeader, &useSuperMode, fill_element_num);
		inSamples = inLen;
		inputInfo.inSamples = inSamples;

		//AASF
		if (inputInfo.outputFormat == 1)
		{
			int nbits;
			nbits = write_avs2file_header(sampleRateCore, 0, &inputInfo, useSuperMode, cpe_config, PCAGroupmodeHeader,/* fOutputFile*/headbuffer);
			fwrite(headbuffer, 1, nbits / 8, fOutputFile);
		}
		if (inputInfo.outputFormat == 2)
		{
			write_avs2AATF_header(sampleRateCore, 0, &inputInfo,/* fOutputFile*/headbuffer);
			//fwrite(headbuffer, 1, 9, fOutputFile);
		}

		int totalSamples = 0;

		while (1)
		{
			numSamplesRead = fread(TimeDataPcmBuffer, bitsPerSample / 8, inSamples, fInputFile);
			if (numSamplesRead < inSamples) break;

			totalSamples += numSamplesRead;
			if (totalSamples > dataSize) break;

			//AATF
			if ((inputInfo.outputFormat == 2) && (frame > 0)) {
				write_avs2AATF_header(samplingRate, 0, &inputInfo,/*fOutputFile*/headbuffer);
				//fwrite(headbuffer, 9, 1, fOutputFile);
				//DataLength += 9;
			}

			unsigned char *p_bed = sampleData;
			unsigned char *p_obj = sampleDataObj;

			//encoding a frame
			p_bed += 2;
			general_encoder_frame(stAvs2Init, TimeDataPcmBuffer, inSamples, &inputInfo, &OutBytes, p_bed, fill_element_num);
			memcpy(p_bed - 2, &OutBytes, 2);
			p_bed += OutBytes;

			if (inputInfo.coding_profile == 1)
			{
				//object information
				ObjectNum = 0;
				for (txt = 0; txt < txtNum; txt++)
				{
					for (cnt = 0; cnt < 16; cnt++)
						a[txt][cnt] = 2;
					cnt = 0;
					if (!posInfo[txt]) {
						printf("The file is not exist!");
					}
					while ((!feof(posInfo[txt])) && (cnt < 16)) { 
						fscanf(posInfo[txt], "%f", &a[txt][cnt]);
						cnt++;
					}
					if (objstAvs2Init[txt]->useBWE == 1 && (frame % 2) == 0)
						for (cnt = 0; cnt < 16; cnt++)
							a[txt][cnt] = 2;
					if ((a[txt][0] != 2) && (a[txt][4] != 2) && (a[txt][8] != 2) && (a[txt][12] != 2))
					{
						Object_ID[ObjectNum] = txt;
						ObjectNum++;
					}
				}

				object_info(ObjectNum, a, Object_ID, &objOutbytes, obInfo);
				memcpy(p_obj, obInfo, objOutbytes); 
				p_obj += objOutbytes;
				for (txt = 0; txt < ObjectNum; txt++)
				{
					object_data(ObjectNum, Object_ID, &OutBytes, obInfo, txt, &objinputInfo);
					memcpy(p_obj, obInfo, OutBytes);
					p_obj += OutBytes;
					if (objdataSizeSet[Object_ID[txt]] >= objinSamples)
					{
						numSamplesRead = fread(TimeDataPcmBuffer, objbitsPerSample / 8, objinSamples, objInfo[Object_ID[txt]]);
						objdataSizeSet[Object_ID[txt]] -= objinSamples;
					}
					else if (objdataSizeSet[Object_ID[txt]] > 0)
					{
						memset(TimeDataPcmBuffer, 0, objbitsPerSample / 8 * objinSamples);
						numSamplesRead = fread(TimeDataPcmBuffer, objbitsPerSample / 8, objdataSizeSet[Object_ID[txt]], objInfo[Object_ID[txt]]);
						objdataSizeSet[Object_ID[txt]] = 0;
					}

					p_obj += 2;
					general_encoder_frame_obj(objstAvs2Init[Object_ID[txt]], TimeDataPcmBuffer, objinSamples, &objinputInfo, &OutBytes, p_obj, fill_element_num);
					memcpy(p_obj - 2, &OutBytes, 2);  //datalength;
					p_obj += OutBytes;
				}//for(txt=0;txt<ObjectNum;txt++)

			    //对象文件尾，无足够数据
				//if(numSamplesRead < objinSamples)
				//{
				//    printf("numSamplesRead < objinSamples\n");
				//	break;
				//}
			}//if(inputInfo.coding_profile == 1)


			if (1) //shumin.xu 20210105
			{
				int bedLength = p_bed - sampleData;
				int objLength = 0;
				frameLength = 9 + 1 + bedLength;

				if (inputInfo.coding_profile == 1)
				{
					objLength = p_obj - sampleDataObj;
					frameLength += objLength;
				}

				unsigned short nCRCBitsA, nCRCBitsB;
				unsigned char CRCbuf[65536 * 2];
				memcpy(&CRCbuf[0], headbuffer, 3);
				memcpy(&CRCbuf[3], &frameLength, 2);
				memcpy(&CRCbuf[5], &headbuffer[6], 3);
				//memcpy(&CRCbuf[8], &bedLength, 2);

				int halfLength = frameLength / 2 - 9;

				if (halfLength < bedLength)
				{
					memcpy(&CRCbuf[8], sampleData, halfLength);
				}
				else
				{
					memcpy(&CRCbuf[8], sampleData, bedLength);
				}

				if (inputInfo.coding_profile == 1)
				{
					if (halfLength > bedLength)
					{
						memcpy(&CRCbuf[8 + bedLength], sampleDataObj, halfLength - bedLength);
					}
				}

				nCRCBitsA = CRC16(&CRCbuf, frameLength / 2 - 1); /*calculate CRC16,6 bytes for header, 2 bytes for CRC*/

				char crcA = nCRCBitsA ^ (nCRCBitsA >> 8);
				memcpy(&headbuffer[3], &frameLength, 2);
				memcpy(&headbuffer[5], &crcA, 1);

				memcpy(&CRCbuf[0], headbuffer, 9);
				//memcpy(&CRCbuf[9], &bedLength, 2);
				memcpy(&CRCbuf[9], sampleData, bedLength);

				if (inputInfo.coding_profile == 1)
					memcpy(&CRCbuf[9 + bedLength], sampleDataObj, objLength);

				nCRCBitsB = CRC16(&CRCbuf, frameLength - 1);
				char crcB = nCRCBitsB ^ (nCRCBitsB >> 8);
				memcpy(&CRCbuf[frameLength - 1], &crcB, sizeof(char));
				fwrite(CRCbuf, sizeof(unsigned char), frameLength, fOutputFile);
				//DataLength += 1;
			}

			*p_bed = NULL;
			*p_obj = NULL;

			frame++;
			printf("%d\r", frame);
			//if (frame > 100) break;

		}//while(1)


		for (cid = 0; cid < numOfChannels; cid++)
		{
			Avs2BweMDFTClose((unsigned int*)&(stAvs2Init->lfEncset[cid]->st1_in), (unsigned int*)&(stAvs2Init->lfEncset[cid]->st_common));
			if (stAvs2Init->useBWE)
			{
				Avs2BweEncoderClose((unsigned int*)&(stAvs2Init->lfEncset[cid]->st2_in));
			}

		}

		Wave_fclose(fInputFile, bitsPerSample);

		if (inputInfo.outputFormat == 1)
			Bitstream_fclose(fOutputFile, frameLength);
		else //shumin.xu 20210105
			fclose(fOutputFile);

		if (inputInfo.coding_profile == 1)
		{
			if (poscontain != NULL)
				fclose(poscontain);
			if (objcontain != NULL)
				fclose(objcontain);
			for (txt = 0; txt < txtNum; txt++)
			{
				if (objInfo[txt] != NULL)
					fclose(objInfo[txt]);
			}
		}

	}//if(inputInfo.codec_id == 0)

	else if (inputInfo.codec_id == 1)
	{
		lossless_encoder(argc, argv);
	}

	return 0;

}