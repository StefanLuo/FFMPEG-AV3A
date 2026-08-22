/******************************************************************************
		        VM1.0 of Audio Video Coding Standard, 
			copyright  (2013)  All Rights Reserved
 
  This software module was originally developed by Beijing Angel Voice 
Digital Technology Co.,Ltd. 
 
  This work(including software and documentation) is provided by the copyright
hoder under the following license:By obtaining, using and/or copying this work, 
you (the licensee) agree that you have read, understood, and will comply with the
following terms and conditions. without permission from Beijing Angel Voice Digital
Technology Co.,Ltd, any forms of copy,modification and distribution are forbidden.
The name and trademarks of copyright holders may NOT be used in advertising or
publicity pertaining to the software without specific, written prior permission.
Title to copyright in this software and any associated documentation will at all 
times remain with copyright holders, and all right reserved.
*******************************************************************************/
#ifndef SUPERMDFTANALYSIS
#define SUPERMDFTANALYSIS

#include <math.h>
#include <stdio.h>

extern float superfftTwiddleTab[1024+1];


extern float superLongWindowSine[1024*2];


extern float superShortWindowSine[128*2];

extern float superShortWindowKBD[512];

extern float superLongWindowKBD[];
extern float Short128WindowSine[];

extern float Long1024WindowSine[];

extern float superdataframe[];


#endif