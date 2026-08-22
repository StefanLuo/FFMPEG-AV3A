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

#include "floor_all.h"
#include "psych_44.h"


static const int blocksize_short_44[11+2]={//lijing added +1
  512, 512,256,256,256,256,256,256,256,256,256,256,256//lijing added 512
};
static const int blocksize_long_44[11+2]={//lijing added +1
  4096, 4096,2048,2048,2048,2048,2048,2048,2048,2048,2048,2048,2048//lijing added 4096
};

static const double _psy_compand_short_mapping[12+1]={//lijing added +1
  0.5, 0.5, 1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.//lijing added 0.5
};
static const double _psy_compand_long_mapping[12+1]={//lijing added +1
  3.5, 3.5, 4., 4., 4.3, 4.6, 5., 5., 5., 5., 5., 5., 5.//lijing added 3.5
};

static const double _global_mapping_44[12+1]={//lijing added +1
  /* 1., 1., 1.5, 2., 2., 2.5, 2.7, 3.0, 3.5, 4., 4. */
 0., 0., 1., 1., 1.5, 2., 2., 2.5, 2.7, 3.0, 3.7, 4., 4.//lijing added 0.
};

static const int _floor_mapping_44a[11+2]={//lijing added +1
  0, 0/*0/*1*/,0,0,2,2,4,5,5,5,5,5,5	//wuchaogang static const tianlai_info_floor1 _floor[11]={ÖÐµÄÐòºÅ
};//lijing added 0

static const int _floor_mapping_44b[11+2]={//lijing added +1
  7, 7/*7/*8*/,7,7,7,7,7,7,7,7,7,7,7	//wuchaogang  static const tianlai_info_floor1 _floor[11]={
};//lijing added 7

static const int _floor_mapping_44c[11+2]={//lijing added +1
  10, 10,10,10,10,10,10,10,10,10,10,10,10
};//lijing added 10

static const int _floor_mapping_44d[11+2]={//lijing added +1
  9, 9,9,9,9,9,9,9,9,9,9,9,9
};//lijing added 9

static const int _floor_mapping_44e[11+2]={//lijing added +1
//  1, 1,3,6,6,6, 6,6,6,6,6,6
	1, 1, 3, 3,3,3, 3,3,3,3,3,3,3
};//lijing added 1

static const int *_floor_mapping_44[]={
  _floor_mapping_44a,
  _floor_mapping_44b,
  _floor_mapping_44d,//
  _floor_mapping_44e,//
  _floor_mapping_44c,
};
