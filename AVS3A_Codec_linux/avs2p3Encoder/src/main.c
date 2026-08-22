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

#ifdef BUILD_APP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "..\general\encode\general.h"
#include "..\threeD\threeD.h"
#pragma comment(lib, "general.lib")
#pragma comment(lib, "threeD.lib")

const char *phelp = "avs encoder verson 2.0\n\n\
usage: avs2enc -if <infile> -of <outfile> [options]\n\n\
OPTIONS:\n\
    -codec_id 0,1\tset audio codec id\n\
	             \t0:general audio encoder;1:lossless audio encoder; defualt is general audio encoder;\n\
    -h or --help\tshow this list of options\n";
int objbitrate;

int main(int argc, char *argv[])
{
	int codec_id = 0;
	int coding_profile = 0;
	int argc_tmp;
	char **argv_tmp;

	argc_tmp = argc;
    argv_tmp = argv;
	argc_tmp--;
	argv_tmp++;

	if (argc_tmp == 0)
	{
		fprintf(stderr, phelp, *argv_tmp);
		exit(EXIT_SUCCESS);
	}
	
	while (argc_tmp > 0)
	{
		if (!strcmp(*argv_tmp, "-codec_id"))
		{
		  argv_tmp++;
		  argc_tmp--;
		  codec_id = atoi(*argv_tmp);
		}
		else if (!strcmp(*argv_tmp, "-coding_profile"))
		{
		  argv_tmp++;
		  argc_tmp--;
		  coding_profile = atoi(*argv_tmp);
		}
		else if (!strcmp(*argv_tmp, "-ob"))
		{
		  argv_tmp++;
		  argc_tmp--;
		  objbitrate = atoi(*argv_tmp);
		}
		else if (!strcmp(*argv_tmp, "-h") || !strcmp(*argv_tmp, "--help"))
		{
			fprintf(stderr, phelp, *argv_tmp);
			exit(EXIT_SUCCESS);
		}
		argv_tmp++;
		argc_tmp--;
	}

	if (coding_profile == 0)
	{
		if (codec_id == 0)
			general_encoder(argc, argv);
		if (codec_id == 1)
			lossless_encoder(argc, argv);
	}
	else if(coding_profile == 1)
	{
        threeD_encoder(argc, argv);
	}
	
    return 0;
	
}
#endif

