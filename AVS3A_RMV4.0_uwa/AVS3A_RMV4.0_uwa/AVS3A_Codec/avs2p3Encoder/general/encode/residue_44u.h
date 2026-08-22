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

#include "lfenc.h"
#include "res_books_uncoupled.h"

/***** residue backends *********************************************/


static const tianlai_info_residue0 _residue_44_low_un={
  0,-1, -1, 8,-1,-1,
  {0},
  {-1},
  {  0,  1,  1,  2,  2,  4, 28},
  { -1, 25, -1, 45, -1, -1, -1}
};

static const tianlai_info_residue0 _residue_44_mid_un={
  0,-1, -1, 10,-1,-1,
  /* 0   1   2   3   4   5   6   7   8   9 */
  {0},
  {-1},
  {  0,  1,  1,  2,  2,  4,  4, 16, 60},
  { -1, 30, -1, 50, -1, 80, -1, -1, -1}
};

static const tianlai_info_residue0 _residue_44_hi_un={
  0,-1, -1, 10,-1,-1,
  /* 0   1   2   3   4   5   6   7   8   9 */
  {0},
  {-1},
  {  0,  1,  2,  4,  8, 16, 32, 71,157},
  { -1, -1, -1, -1, -1, -1, -1, -1, -1}
};

/* mapping conventions:
   only one submap (this would change for efficient 5.1 support for example)*/
/* Four psychoacoustic profiles are used, one for each blocktype */
static const tianlai_info_mapping0 _map_nominal_u[4/*2*/]={
  {1, {0,0,0,0,0,0}, {0}, {0}, 0,{0},{0}},
  {1, {0,0,0,0,0,0}, {1}, {1}, 0,{0},{0}},
  {1, {0,0,0,0,0,0}, {2}, {2}, 0,{0},{0}},
  {1, {0,0,0,0,0,0}, {3}, {3}, 0,{0},{0}}
};

static const static_bookblock _resbook_44u_n1={
  {
    {0},
    {0,0,&_44un1__p1_0},
    {0,0,&_44un1__p2_0},
    {0,0,&_44un1__p3_0},
    {0,0,&_44un1__p4_0},
    {0,0,&_44un1__p5_0},
    {&_44un1__p6_0,&_44un1__p6_1},
    {&_44un1__p7_0,&_44un1__p7_1,&_44un1__p7_2}
   }
};
static const static_bookblock _resbook_44u_0={
  {
    {0},
    {0,0,&_44u0__p1_0},
    {0,0,&_44u0__p2_0},
    {0,0,&_44u0__p3_0},
    {0,0,&_44u0__p4_0},
    {0,0,&_44u0__p5_0},
    {&_44u0__p6_0,&_44u0__p6_1},
    {&_44u0__p7_0,&_44u0__p7_1,&_44u0__p7_2}
   }
};
static const static_bookblock _resbook_44u_1={
  {
    {0},
    {0,0,&_44u1__p1_0},
    {0,0,&_44u1__p2_0},
    {0,0,&_44u1__p3_0},
    {0,0,&_44u1__p4_0},
    {0,0,&_44u1__p5_0},
    {&_44u1__p6_0,&_44u1__p6_1},
    {&_44u1__p7_0,&_44u1__p7_1,&_44u1__p7_2}
   }
};
static const static_bookblock _resbook_44u_2={
  {
    {0},
    {0,0,&_44u2__p1_0},
    {0,0,&_44u2__p2_0},
    {0,0,&_44u2__p3_0},
    {0,0,&_44u2__p4_0},
    {0,0,&_44u2__p5_0},
    {&_44u2__p6_0,&_44u2__p6_1},
    {&_44u2__p7_0,&_44u2__p7_1,&_44u2__p7_2}
   }
};
static const static_bookblock _resbook_44u_3={
  {
    {0},
    {0,0,&_44u3__p1_0},
    {0,0,&_44u3__p2_0},
    {0,0,&_44u3__p3_0},
    {0,0,&_44u3__p4_0},
    {0,0,&_44u3__p5_0},
    {&_44u3__p6_0,&_44u3__p6_1},
    {&_44u3__p7_0,&_44u3__p7_1,&_44u3__p7_2}
   }
};
static const static_bookblock _resbook_44u_4={
  {
    {0},
    {0,0,&_44u4__p1_0},
    {0,0,&_44u4__p2_0},
    {0,0,&_44u4__p3_0},
    {0,0,&_44u4__p4_0},
    {0,0,&_44u4__p5_0},
    {&_44u4__p6_0,&_44u4__p6_1},
    {&_44u4__p7_0,&_44u4__p7_1,&_44u4__p7_2}
   }
};
static const static_bookblock _resbook_44u_5={
  {
    {0},
    {0,0,&_44u5__p1_0},
    {0,0,&_44u5__p2_0},
    {0,0,&_44u5__p3_0},
    {0,0,&_44u5__p4_0},
    {0,0,&_44u5__p5_0},
    {0,0,&_44u5__p6_0},
    {&_44u5__p7_0,&_44u5__p7_1},
    {&_44u5__p8_0,&_44u5__p8_1},
    {&_44u5__p9_0,&_44u5__p9_1,&_44u5__p9_2}
   }
};
static const static_bookblock _resbook_44u_6={
  {
    {0},
    {0,0,&_44u6__p1_0},
    {0,0,&_44u6__p2_0},
    {0,0,&_44u6__p3_0},
    {0,0,&_44u6__p4_0},
    {0,0,&_44u6__p5_0},
    {0,0,&_44u6__p6_0},
    {&_44u6__p7_0,&_44u6__p7_1},
    {&_44u6__p8_0,&_44u6__p8_1},
    {&_44u6__p9_0,&_44u6__p9_1,&_44u6__p9_2}
   }
};
static const static_bookblock _resbook_44u_7={
  {
    {0},
    {0,0,&_44u7__p1_0},
    {0,0,&_44u7__p2_0},
    {0,0,&_44u7__p3_0},
    {0,0,&_44u7__p4_0},
    {0,0,&_44u7__p5_0},
    {0,0,&_44u7__p6_0},
    {&_44u7__p7_0,&_44u7__p7_1},
    {&_44u7__p8_0,&_44u7__p8_1},
    {&_44u7__p9_0,&_44u7__p9_1,&_44u7__p9_2}
   }
};
static const static_bookblock _resbook_44u_8={
  {
    {0},
    {0,0,&_44u8_p1_0},
    {0,0,&_44u8_p2_0},
    {0,0,&_44u8_p3_0},
    {0,0,&_44u8_p4_0},
    {&_44u8_p5_0,&_44u8_p5_1},
    {&_44u8_p6_0,&_44u8_p6_1},
    {&_44u8_p7_0,&_44u8_p7_1},
    {&_44u8_p8_0,&_44u8_p8_1},
    {&_44u8_p9_0,&_44u8_p9_1,&_44u8_p9_2}
   }
};
static const static_bookblock _resbook_44u_9={
  {
    {0},
    {0,0,&_44u9_p1_0},
    {0,0,&_44u9_p2_0},
    {0,0,&_44u9_p3_0},
    {0,0,&_44u9_p4_0},
    {&_44u9_p5_0,&_44u9_p5_1},
    {&_44u9_p6_0,&_44u9_p6_1},
    {&_44u9_p7_0,&_44u9_p7_1},
    {&_44u9_p8_0,&_44u9_p8_1},
    {&_44u9_p9_0,&_44u9_p9_1,&_44u9_p9_2}
   }
};

static const tianlai_residue_template _res_44u_n1[]={
  {1,32,  &_residue_44_low_un,
   &_huff_book__44un1__short,&_huff_book__44un1__short,
   &_resbook_44u_n1,&_resbook_44u_n1},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44un1__long,&_huff_book__44un1__long,
   &_resbook_44u_n1,&_resbook_44u_n1},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44un1__long,&_huff_book__44un1__long,
   &_resbook_44u_n1,&_resbook_44u_n1},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44un1__long,&_huff_book__44un1__long,
   &_resbook_44u_n1,&_resbook_44u_n1}
};
static const tianlai_residue_template _res_44u_0[]={
  {1,16,  &_residue_44_low_un,
   &_huff_book__44u0__short,&_huff_book__44u0__short,
   &_resbook_44u_0,&_resbook_44u_0},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u0__long,&_huff_book__44u0__long,
   &_resbook_44u_0,&_resbook_44u_0},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u0__long,&_huff_book__44u0__long,
   &_resbook_44u_0,&_resbook_44u_0},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u0__long,&_huff_book__44u0__long,
   &_resbook_44u_0,&_resbook_44u_0}
};
static const tianlai_residue_template _res_44u_1[]={
  {1,16,  &_residue_44_low_un,
   &_huff_book__44u1__short,&_huff_book__44u1__short,
   &_resbook_44u_1,&_resbook_44u_1},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u1__long,&_huff_book__44u1__long,
   &_resbook_44u_1,&_resbook_44u_1},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u1__long,&_huff_book__44u1__long,
   &_resbook_44u_1,&_resbook_44u_1},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u1__long,&_huff_book__44u1__long,
   &_resbook_44u_1,&_resbook_44u_1}
};
static const tianlai_residue_template _res_44u_2[]={
  {1,16,  &_residue_44_low_un,
   &_huff_book__44u2__short,&_huff_book__44u2__short,
   &_resbook_44u_2,&_resbook_44u_2},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u2__long,&_huff_book__44u2__long,
   &_resbook_44u_2,&_resbook_44u_2},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u2__long,&_huff_book__44u2__long,
   &_resbook_44u_2,&_resbook_44u_2},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u2__long,&_huff_book__44u2__long,
   &_resbook_44u_2,&_resbook_44u_2}
};
static const tianlai_residue_template _res_44u_3[]={
  {1,16,  &_residue_44_low_un,
   &_huff_book__44u3__short,&_huff_book__44u3__short,
   &_resbook_44u_3,&_resbook_44u_3},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u3__long,&_huff_book__44u3__long,
   &_resbook_44u_3,&_resbook_44u_3},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u3__long,&_huff_book__44u3__long,
   &_resbook_44u_3,&_resbook_44u_3},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u3__long,&_huff_book__44u3__long,
   &_resbook_44u_3,&_resbook_44u_3}
};

static const tianlai_residue_template _res_44u_4[]={
  {1,16,  &_residue_44_low_un,
   &_huff_book__44u4__short,&_huff_book__44u4__short,
   &_resbook_44u_4,&_resbook_44u_4},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u4__long,&_huff_book__44u4__long,
   &_resbook_44u_4,&_resbook_44u_4},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u4__long,&_huff_book__44u4__long,
   &_resbook_44u_4,&_resbook_44u_4},

  {1,32,  &_residue_44_low_un,
   &_huff_book__44u4__long,&_huff_book__44u4__long,
   &_resbook_44u_4,&_resbook_44u_4}
};

static const tianlai_residue_template _res_44u_5[]={
  {1,16,  &_residue_44_mid_un,
   &_huff_book__44u5__short,&_huff_book__44u5__short,
   &_resbook_44u_5,&_resbook_44u_5},

  {1,32,  &_residue_44_mid_un,
   &_huff_book__44u5__long,&_huff_book__44u5__long,
   &_resbook_44u_5,&_resbook_44u_5},

  {1,32,  &_residue_44_mid_un,
   &_huff_book__44u5__long,&_huff_book__44u5__long,
   &_resbook_44u_5,&_resbook_44u_5},

  {1,32,  &_residue_44_mid_un,
   &_huff_book__44u5__long,&_huff_book__44u5__long,
   &_resbook_44u_5,&_resbook_44u_5}
};

static const tianlai_residue_template _res_44u_6[]={
  {1,16,  &_residue_44_mid_un,
   &_huff_book__44u6__short,&_huff_book__44u6__short,
   &_resbook_44u_6,&_resbook_44u_6},

  {1,32,  &_residue_44_mid_un,
   &_huff_book__44u6__long,&_huff_book__44u6__long,
   &_resbook_44u_6,&_resbook_44u_6},

  {1,32,  &_residue_44_mid_un,
   &_huff_book__44u6__long,&_huff_book__44u6__long,
   &_resbook_44u_6,&_resbook_44u_6},

  {1,32,  &_residue_44_mid_un,
   &_huff_book__44u6__long,&_huff_book__44u6__long,
   &_resbook_44u_6,&_resbook_44u_6}
};

static const tianlai_residue_template _res_44u_7[]={
  {1,16,  &_residue_44_mid_un,
   &_huff_book__44u7__short,&_huff_book__44u7__short,
   &_resbook_44u_7,&_resbook_44u_7},

  {1,32,  &_residue_44_mid_un,
   &_huff_book__44u7__long,&_huff_book__44u7__long,
   &_resbook_44u_7,&_resbook_44u_7},

  {1,32,  &_residue_44_mid_un,
   &_huff_book__44u7__long,&_huff_book__44u7__long,
   &_resbook_44u_7,&_resbook_44u_7},

  {1,32,  &_residue_44_mid_un,
   &_huff_book__44u7__long,&_huff_book__44u7__long,
   &_resbook_44u_7,&_resbook_44u_7}
};

static const tianlai_residue_template _res_44u_8[]={
  {1,16,  &_residue_44_hi_un,
   &_huff_book__44u8__short,&_huff_book__44u8__short,
   &_resbook_44u_8,&_resbook_44u_8},

  {1,32,  &_residue_44_hi_un,
   &_huff_book__44u8__long,&_huff_book__44u8__long,
   &_resbook_44u_8,&_resbook_44u_8},

  {1,32,  &_residue_44_hi_un,
   &_huff_book__44u8__long,&_huff_book__44u8__long,
   &_resbook_44u_8,&_resbook_44u_8},

  {1,32,  &_residue_44_hi_un,
   &_huff_book__44u8__long,&_huff_book__44u8__long,
   &_resbook_44u_8,&_resbook_44u_8}
};
static const tianlai_residue_template _res_44u_9[]={
  {1,16,  &_residue_44_hi_un,
   &_huff_book__44u9__short,&_huff_book__44u9__short,
   &_resbook_44u_9,&_resbook_44u_9},

  {1,32,  &_residue_44_hi_un,
   &_huff_book__44u9__long,&_huff_book__44u9__long,
   &_resbook_44u_9,&_resbook_44u_9},

  {1,32,  &_residue_44_hi_un,
   &_huff_book__44u9__long,&_huff_book__44u9__long,
   &_resbook_44u_9,&_resbook_44u_9},

  {1,32,  &_residue_44_hi_un,
   &_huff_book__44u9__long,&_huff_book__44u9__long,
   &_resbook_44u_9,&_resbook_44u_9}
};

static const tianlai_mapping_template _mapres_template_44_uncoupled[]={
  { _map_nominal_u, _res_44u_n1 }, /* -1 */
  { _map_nominal_u, _res_44u_0 }, /* 0 */
  { _map_nominal_u, _res_44u_1 }, /* 1 */
  { _map_nominal_u, _res_44u_2 }, /* 2 */
  { _map_nominal_u, _res_44u_3 }, /* 3 */
  { _map_nominal_u, _res_44u_4 }, /* 4 */
  { _map_nominal_u, _res_44u_5 }, /* 5 */
  { _map_nominal_u, _res_44u_6 }, /* 6 */
  { _map_nominal_u, _res_44u_7 }, /* 7 */
  { _map_nominal_u, _res_44u_8 }, /* 8 */
  { _map_nominal_u, _res_44u_9 }, /* 9 */
  { _map_nominal_u, _res_44u_9 }, /* 10 *///added
	{ _map_nominal_u, _res_44u_9 }, /* 10 *///added

};


