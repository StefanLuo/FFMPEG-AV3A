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

#ifndef __DEGETCMDARG_H
#define __DEGETCMDARG_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef struct {
    char*           swt;        /* switch word*/   
    int             optType;    /* 0, without option;       1, with a numeric option */
                                /* 2, with a string option; 3, finite set option */
    char**          optSet;     /* in case finite set option */
    int             setType;    /* 0, new set overwrites old one */
                                /* 1, all sets are accummulated */
    int             relative;   /* the idx of the next switch affects the same cmd_params members */                                    
} cmd_switch;

typedef struct {        
    int     swIdx;             /* the idx of a switch */
    char*   opt;               /* the pointer of an option for the corresponding switch */
} cmd_option;

typedef struct {
	int    codecId;
    int    outformat;
	int    sampleformat;
    char*  outFile;                 
    char*  inFile;                      
    int    showHelp;
	int	   profile;
} cmd_params;

/* command line parsing strategy:
 * 0. a string without a '/' or '-' immediately preceding it or the first char is 
      not '/' or '-' is taken as ordinary string whatever it looks like.
 * 1. a switch has at most one argument.
 * 2. a string immediately follows '/' or '-' is taken as a switch; a string excluding
      the first char is also a switch if the one is '/' or '-'.
 * 3. the part of a string (if not vacant) immediately follows a switch is taken as
      as the argument of the switch if the switch expects an argument.
 * 4. a switch expecting an argument but followed by an invalid argument is ignored
      with warning. 
 * 5. an argument following a switch not expecting any argument is ignored with warning.
 * 6. an unrecognized switch is ignored with warning
 * 7. only the last seting of a cmd_params member except inFile and outFile is active. 
      the preceding setting (if any) is overwritten by the current one with warning.
 * 8. command line input about Infile and outFile are accumlated.
 * 9. mutiple '/' or '-' in a row simply ignored.
 
 * NOTE: TO AVOID SWITCH WORD AMBIGUITY, A SWITCH EXPECTING AN ARGUMENTS SHOULD NOT HAVE 
         ITS SWITCH WORD CONCINCIDE WITH THE LEADING CHARACTERS OF ANY OTHER SWITCHES.
         (i.e. "abc" and "abcd" should not appear in the same cmd_switch set if "abc" is
          attached to a switch expecting an argument)
 */

/* parse 'argc' number of argv to set 'param' according to 'swts'.
 * parsed options are stored in 'option'. return the number(>=0) of all active cmd line inputs
 * NOTE: the caller roution has the responsibility to ensure there is enough space
 *       in 'option' for storage.
 */ 
int parseCommandLine(cmd_option*         option,  //out:
                     const cmd_switch*   swts,    //in:
                     const int           argc,    //in:
                     char*               argv[]); //in:

int getParam(cmd_params* param, cmd_option* option, int narg);
                            
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DEGETCMDARG_H
