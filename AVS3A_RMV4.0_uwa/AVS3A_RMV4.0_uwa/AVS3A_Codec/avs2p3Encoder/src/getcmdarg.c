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
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "getcmdarg.h"

static int isNum(char* opt)
{
    /* check if normal numeric expression: (+/-)xxxx.xxxx; (+/-).xxxx 
       or scientific numeric expression: (+/-)xxxx.xxxx(E/e)(+/-)xxxx
       or xxxx.xxxx% */    

    int     i;
    int     needSign;
    int     needPoint;
    int     needExp;
    int     needPer;
    
    i = 0;
    needSign = 1;
    needPoint = 1;
    needExp = 1;
    needPer = 0;
            
    while(opt[i] != '\0'){
        if((opt[i] == '+' || opt[i] == '-') && needSign)
            needSign = 0;
        else if(isdigit(opt[i])){
            needSign = 0;
            needPer = 1;
        }else if((opt[i] == 'E' || opt[i] == 'e') && needExp){
            needSign = 1;
            needExp = 0;
            needPer = 0;
        }else if(opt[i] == '.' && needPoint){
            needSign = 0;
            needPoint = 0;
            needPer = 0;    
        }else if(opt[i] == '%' && needPer){
            needSign = 0;
            needPoint = 0;
            needExp = 0;
            needPer = 0;            
        }else
            return 0;
        
        i++;
    }
    
    return 1;
}

static int isOpt(char* opt, char** optSet)
{
    /* check if 'opt' is an elegible option for switch indexed 'swIdx' 
       in case finite set option */
     
    if(opt == NULL)
        return 1;
    else
    {
        int i;
        
        i = 0;
        while(strcmp("\0", optSet[i])){
            if(!strcmp(optSet[i], opt))
                return 1;
            i++;
        }
        return 0;
    }
}

    
int parseCommandLine(cmd_option*         option,
                     const cmd_switch*   swts,
                     const int           argc, 
                     char*               argv[])
{
    int i;    
    int opIdx;
    
    int     numSwitch;
    char*   haveSet;
    
    numSwitch = 0;
    while(strcmp("\0", swts[numSwitch].swt)) numSwitch++;
    haveSet = malloc(numSwitch*sizeof(char));
    memset(haveSet, 0, numSwitch*sizeof(char));               
    
    i = 1;
    opIdx = 0;     
    while(i < argc){
        char mark;
        
        mark = argv[i][0];
        if(mark != '/' && mark != '-') {
            /* input files */
            option[opIdx].swIdx = numSwitch;
            option[opIdx].opt = argv[i++];
            opIdx++;
        }else{
            char* swt;
            char* opt;
            
            int j;
            int len;
            int s;
            int nst;
 
            /* go to the start of a new switch, all '/' or '-' between igored */
            swt = argv[i];  
            while(swt[0] == '/' || swt[0] == '-'){
                if(strlen(swt) == 1){
                    if(++i == argc){
                        /* come to the end of the cmd line */
                        printf("command line warning: \tno switch after '%c', '%c' ignore!\n", mark, mark);
                        return 0;
                    }else{
                        swt = argv[i];
                    }
                }else{
                    swt += 1;
                }
            }
            
            /* get switch idx and the start of the option for this switch 
               if it is contained in the current argment */
            
            s = -1;
            j = 0;
            nst = 1;
            opt = NULL;
            len = strlen(swt);            
            while(j < numSwitch){
                if(swts[j].optType == 0){
                    if(!strcmp(swts[j].swt, swt)){
                        s = j;
                        break;
                    }
                }else{
                    int slen;

                    slen = strlen(swts[j].swt);
                    if(!memcmp(swts[j].swt, swt, slen)){
                        s = j;
                        if(slen < len){
                            opt = swt + slen;
                            nst = 0;
                        }
                        break;
                    }
                }
                j++;
            }

            if(s < 0){
                /* no elegible switch found, ignore */
                printf("command line warning: \tunrecognized switch '%c%s', command line parsing restarts at the next argument!\n", mark, swt);
                i++;
                continue;
            }
            
            /* get the start of the option for this switch if it starts at a new argv */ 
            if(swts[s].optType){ 
                if(opt == NULL){
                    /* option starts at the next argv */
                    if(++i < argc)
                        opt = argv[i];
                    else if(swts[s].optType != 3)
                    {
                        /* come to end of cmd line while expecting an argument */
                        printf("command line warning: \t'%c%s' needs an argument, '%c%s' ignored!\n", mark, swts[s].swt, mark, swts[s].swt);
                        return opIdx;
                    }
                }
            }else if(opt != NULL){
                /* switch without option */
                printf("command line warning: \t'%c%s' does not need any argument, '%s' ignored!\n", mark,swts[s].swt, opt);
                opt = NULL;
            }
 
            /* check if argument valid */
            if(swts[s].optType == 1 && !isNum(opt)){
                /* no valid input found */
                printf("command line warning: \t'%c%s' need a numeric arguament, default value taken!\n", mark, swts[s].swt);
                if(!nst)
                    i++;
                continue;
            }
            if(swts[s].optType == 3 && !isOpt(opt, swts[s].optSet)){
                /* no valid input found */
                if(!nst)
                    printf("command line warning: \t'%s' is not an elegible argument for '%c%s', '%s' ignored!\n", opt, mark, swts[s].swt, opt);                 
                opt = NULL;
                i -= nst;
            }
            
            /* check if repeating setting the same nonaccumlative option */ 
            if(!swts[s].setType){
                if(haveSet[s])
                    printf("command line warning: \tcommand line parameter corresponding '%c%s' has already been set, \n\t\t\tthe new value will overwrite the old one!\n", mark, swts[s].swt);
                else
                {
                    int m;
                    
                    m = s;
                    while(!haveSet[m]){
                        haveSet[m] = 1;
                        m = swts[m].relative;
                    }
                }
            }  
            
            option[opIdx].swIdx = s;
            option[opIdx].opt = opt;
            opIdx++;
            i++;
        }
    }
    
    free(haveSet);
    return opIdx;
}

                                                                              