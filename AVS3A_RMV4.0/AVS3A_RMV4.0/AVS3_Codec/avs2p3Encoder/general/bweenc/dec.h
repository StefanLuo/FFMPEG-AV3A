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

//BWE decoder frontend prototypes and definitions
#ifndef __DEC_H
#define __DEC_H


#define MAXNRELEMENTS 2


#define MAXSBRBYTES 269


typedef enum
{
  BWEDEC_OK = 0,
  BWEDEC_CONCEAL,
  BWEDEC_NOSYNCH,
  BWEDEC_ILLEGAL_PROGRAM,
  BWEDEC_ILLEGAL_TAG,
  BWEDEC_ILLEGAL_CHN_CONFIG,
  BWEDEC_ILLEGAL_SECTION,
  BWEDEC_ILLEGAL_SCFACTORS,
  BWEDEC_ILLEGAL_PULSE_DATA,
  BWEDEC_MAIN_PROFILE_NOT_IMPLEMENTED,
  BWEDEC_GC_NOT_IMPLEMENTED,
  BWEDEC_ILLEGAL_PLUS_ELE_ID,
  BWEDEC_CREATE_ERROR,
  BWEDEC_NOT_INITIALIZED
}
BWE_ERROR;

/*typedef enum
{
  BWE_ID_SCE = 0,
  BWE_ID_CPE,
  BWE_ID_CCE,
  BWE_ID_LFE,
  BWE_ID_DSE,
  BWE_ID_PCE,
  BWE_ID_FIL,
  BWE_ID_END
}
BWE_ELEMENT_ID;*/

typedef struct
{
  int ElementID;
  int ExtensionType;
  int Payload;
  unsigned char Data[MAXSBRBYTES];
}
BWE_ELEMENT_STREAM;

typedef struct
{
  int NrElements;
  int NrElementsCore;
  BWE_ELEMENT_STREAM bweElement[MAXNRELEMENTS]; /* for the delayed frame */
}
BWEBITSTREAM;



#endif
