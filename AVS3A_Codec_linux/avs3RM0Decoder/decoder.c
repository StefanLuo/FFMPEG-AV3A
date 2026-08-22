/* ====================================================================================================================

  The copyright in this software is being made available under the License included below.
  No express or implied licenses to any party's patent rights are granted by this license.

  Copyright (c) 2022, HUAWEI TECHNOLOGIES CO., LTD. All rights reserved.
  Copyright (c) 2022, XIAOMI COMMUNICATIONS CO., LTD. All rights reserved.
  Copyright (c) 2022, BEIJING ZITIAO NETWORK TECHNOLOGY CO., LTD. All rights reserved.
  Copyright (c) 2022, BEIJING SINECORE MICROSEMI TECHNOLOGY CO., LTD. All rights reserved.
  Copyright (c) 2022, WAVARTS TECHNOLOGIES CO., LTD. All rights reserved.
  Copyright (c) 2022, PEKING UNIVERSITY. All rights reserved.
  Copyright (c) 2022, TSINGHUA UNIVERSITY. All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted only for
  the purpose of developing standards within Audio and Video Coding Standard Workgroup of China (AVS) and for testing and
  promoting such standards. The following conditions are required to be met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
      the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
      the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name of the above copyright owners may not be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

==================================================================================================================== */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "avs3_extern_decoder.h"
#include "avs2p3_decoder.h"

#define AVS2P3_GENERAL  0xF0FF
#define AVS2P3_LOSSNESS 0xF1FF
#define AVS3_GENERAL    0xF2FF
#define ERROR_CODECID   -1

static short GetCodecId(FILE  **f_Bitstream, const int argc, char* argv[])
{
    short i = 1;
    char* bitstreamName = NULL;
    unsigned short codecId = 0;
    short retCode = -1;

    if (*f_Bitstream != NULL) {
        return -1;
    }

    while (i < argc - 1)
    {
        if (strncmp(argv[i], "-if", strlen("-if")) == 0)
        {
            i++;
            bitstreamName = argv[i];

            break;
        }

        i++;
    }

    if ((*f_Bitstream = fopen(bitstreamName, "rb")) == NULL)
    {
        fprintf(stderr, "Error: Bitstream file %s can not be opened!\n", bitstreamName);
        exit(-1);
    }

    fread(&codecId, sizeof(unsigned short), 1, *f_Bitstream);

    switch (codecId)
    {
    case AVS2P3_GENERAL:
        retCode = 0;
        break;
    case AVS2P3_LOSSNESS:
        retCode = 1;
        break;;
    case AVS3_GENERAL:
        retCode = 2;
        break;
    default:
        retCode = -1;
        break;
    }

    return retCode;
}


int main(int argc, char* argv[])
{
    FILE* f_bitstream = NULL;

    short codecId = GetCodecId(&f_bitstream, argc, argv);

    if (codecId == 2)
    {
        avs3_decoder(argc, argv);
    }
    else
    {
        avs2p3_decoder(argc, argv);
    }

    if (f_bitstream != NULL) {
        fclose(f_bitstream);
    }

    return 0;
}