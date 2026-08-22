#include "avs2p3_decoder.h"

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
#include "..\general\decode\general.h"
#include "..\threeD\general_decoder_3D.h"

const char *phelp = "avs decoder verson 2.0\n\n\
usage: avs2dec -if <infile> -of <outfile> [options]\n\n\
OPTIONS:\n\
    -codec_id 0,1\tset audio codec id\n\
	             \t0:general audio encoder;1:lossless audio encoder; defualt is general audio encoder;\n\
    -h or --help\tshow this list of options\n";

int avs2p3_decoder(int argc, char *argv[])
{
    FILE* inputfile;
    unsigned char p;
    int codec_id = 0;
    int argc_tmp;
    char **argv_tmp;
    char *inputfilename = NULL;
    char *outputfilename = NULL;
    char *filepath = NULL;
    int coding_profile = 0;
    char  header_tag[4];
    int   isAASF = 2;

    argc_tmp = argc;
    argv_tmp = argv;
    argc_tmp--;
    argv_tmp++;
    /*获得命令行中的字符串*/
    if (argc_tmp == 0)
    {
        fprintf(stderr, phelp, *argv_tmp);
        exit(EXIT_SUCCESS);
    }
    while (argc_tmp > 0)
    {
        if (!strcmp(*argv_tmp, "-if"))
        {
            argv_tmp++;
            argc_tmp--;
            inputfilename = *argv_tmp;
        }
        if (!strcmp(*argv_tmp, "-of"))
        {
            argv_tmp++;
            argc_tmp--;
            outputfilename = *argv_tmp;
        }
        else if (!strcmp(*argv_tmp, "-fp"))
        {
            argv_tmp++;
            argc_tmp--;
            filepath = *argv_tmp;
        }
        else if (!strcmp(*argv_tmp, "-h") || !strcmp(*argv_tmp, "--help"))
        {
            fprintf(stderr, phelp, *argv_tmp);
            exit(EXIT_SUCCESS);
        }
        argv_tmp++;
        argc_tmp--;
    }
    /*读取头中控制字决定解码通道*/
    inputfile = fopen(inputfilename, "rb");
    fread(header_tag, 4, 1, inputfile);

    if (memcmp(header_tag, "AASF", 4) == 0)
        isAASF = 1;
    else if (header_tag[0] == (char)0x7f
        && ((header_tag[1] & 0xf0) == 0xe0))
        isAASF = 2;
    else if (header_tag[0] == (char)0xff
        && ((header_tag[1] & 0xf0) == 0xf0))
        isAASF = 2;
    else
        isAASF = -1;

    fseek(inputfile, -sizeof(char) * 4, 1);

    if (isAASF == 1)
    {
        fseek(inputfile, 11, 0);
        fread(&p, 1, 1, inputfile);
        codec_id = (int)(p >> 4);
        //  p=p<<4;
        fread(&p, 1, 1, inputfile);
        coding_profile = (int)(p >> 7);
        fclose(inputfile);
    }
    else if (isAASF == 2)
    {
        fseek(inputfile, 1, 0);
        fread(&p, 1, 1, inputfile);
        p = p << 4;
        codec_id = (int)(p >> 4);
        fread(&p, 1, 1, inputfile);
        p = p >> 4;
        coding_profile = (int)(p);
        fclose(inputfile);
    }
    /*选择要使用的解码器*/
    if (coding_profile == 0) {
        if (codec_id == 0)
            general_decoder(argc, argv);  //通用解码
        if (codec_id == 1)
        {
            lossless_decoder(argc, argv);  //无损解码
        }
    }
    else if (coding_profile == 1)
    {
        decoder_3D(argc, argv);  //3D解码
    }
    return 0;
}