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

#ifndef _OS_TYPES_H
#define _OS_TYPES_H

/* make it easy on the folks that want to compile the libs with a
   different malloc than stdlib */
#define _avs2audio_malloc  malloc
#define _avs2audio_calloc  calloc
#define _avs2audio_realloc realloc
#define _avs2audio_free    free

#if defined(_WIN32) 

#  if defined(__CYGWIN__)
#    include <stdint.h>
     typedef int16_t avs2audio_int16_t;
     typedef uint16_t avs2audio_uint16_t;
     typedef int32_t avs2audio_int32_t;
     typedef uint32_t avs2audio_uint32_t;
     typedef int64_t avs2audio_int64_t;
     typedef uint64_t avs2audio_uint64_t;
#  elif defined(__MINGW32__)
#    include <sys/types.h>
     typedef short avs2audio_int16_t;
     typedef unsigned short avs2audio_uint16_t;
     typedef int avs2audio_int32_t;
     typedef unsigned int avs2audio_uint32_t;
     typedef long long avs2audio_int64_t;
     typedef unsigned long long avs2audio_uint64_t;
#  elif defined(__MWERKS__)
     typedef long long avs2audio_int64_t;
     typedef int avs2audio_int32_t;
     typedef unsigned int avs2audio_uint32_t;
     typedef short avs2audio_int16_t;
     typedef unsigned short avs2audio_uint16_t;
#  else
     /* MSVC/Borland */
     typedef __int64 avs2audio_int64_t;
     typedef __int32 avs2audio_int32_t;
     typedef unsigned __int32 avs2audio_uint32_t;
     typedef __int16 avs2audio_int16_t;
     typedef unsigned __int16 avs2audio_uint16_t;
#  endif

#elif defined(__MACOS__)

#  include <sys/types.h>
   typedef SInt16 avs2audio_int16_t;
   typedef UInt16 avs2audio_uint16_t;
   typedef SInt32 avs2audio_int32_t;
   typedef UInt32 avs2audio_uint32_t;
   typedef SInt64 avs2audio_int64_t;

#elif (defined(__APPLE__) && defined(__MACH__)) /* MacOS X Framework build */

#  include <inttypes.h>
   typedef int16_t avs2audio_int16_t;
   typedef uint16_t avs2audio_uint16_t;
   typedef int32_t avs2audio_int32_t;
   typedef uint32_t avs2audio_uint32_t;
   typedef int64_t avs2audio_int64_t;

#elif defined(__HAIKU__)

  /* Haiku */
#  include <sys/types.h>
   typedef short avs2audio_int16_t;
   typedef unsigned short avs2audio_uint16_t;
   typedef int avs2audio_int32_t;
   typedef unsigned int avs2audio_uint32_t;
   typedef long long avs2audio_int64_t;

#elif defined(__BEOS__)

   /* Be */
#  include <inttypes.h>
   typedef int16_t avs2audio_int16_t;
   typedef uint16_t avs2audio_uint16_t;
   typedef int32_t avs2audio_int32_t;
   typedef uint32_t avs2audio_uint32_t;
   typedef int64_t avs2audio_int64_t;

#elif defined (__EMX__)

   /* OS/2 GCC */
   typedef short avs2audio_int16_t;
   typedef unsigned short avs2audio_uint16_t;
   typedef int avs2audio_int32_t;
   typedef unsigned int avs2audio_uint32_t;
   typedef long long avs2audio_int64_t;

#elif defined (DJGPP)

   /* DJGPP */
   typedef short avs2audio_int16_t;
   typedef int avs2audio_int32_t;
   typedef unsigned int avs2audio_uint32_t;
   typedef long long avs2audio_int64_t;

#elif defined(R5900)

   /* PS2 EE */
   typedef long avs2audio_int64_t;
   typedef int avs2audio_int32_t;
   typedef unsigned avs2audio_uint32_t;
   typedef short avs2audio_int16_t;

#elif defined(__SYMBIAN32__)

   /* Symbian GCC */
   typedef signed short avs2audio_int16_t;
   typedef unsigned short avs2audio_uint16_t;
   typedef signed int avs2audio_int32_t;
   typedef unsigned int avs2audio_uint32_t;
   typedef long long int avs2audio_int64_t;

#elif defined(__TMS320C6X__)

   /* TI C64x compiler */
   typedef signed short avs2audio_int16_t;
   typedef unsigned short avs2audio_uint16_t;
   typedef signed int avs2audio_int32_t;
   typedef unsigned int avs2audio_uint32_t;
   typedef long long int avs2audio_int64_t;

#else

#include <avs2audio/config_types.h>

#endif

#endif  /* _OS_TYPES_H */
