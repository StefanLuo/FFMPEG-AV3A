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
#include "mdftimdft4096.h"

#define NNN12 12
#define BITNNN12 (0x3FFF)
#define LLL4096 (1<<NNN12)

static double pi=3.14159265358979323846264338327950288;
static FLOAT costable4096[4096*4];
static FLOAT sintable4096[4096*4];
static int invbittableN4096[LLL4096];

/*
setting the Bit-Reverse table
*/
int invbitN4096()
{
int i,j;
int N=12;
for (i=0;i<pow(2,N);i++)
{int pos,invpos;
pos=i;
invpos=0;
    for(j=0;j<N;j++)
    {
        invpos  = invpos*2+ (pos&0x01);
        pos = pos>>1;
    }
    invbittableN4096[i] = invpos;


}//i

return 0;
}

/*
setting the cos/sin function table
*/
int bulidcossintable4096()
{
int i;
int N=12;
for(i=0;i<(1<<N)*4;i++)
{
costable4096[i] = (FLOAT)cos(pi/(1<<N)/2*i);
sintable4096[i] = (FLOAT)sin(pi/(1<<N)/2*i);
}
return 0;
}

/*
MDFT Butterfly
*/
int mdft4096(FLOAT *sinput, FLOAT *Sr, FLOAT *Si)
{ FLOAT cosdelta,sindelta;
int N=12;

int i,j,M;
int L = (int)pow(2,N);
 FLOAT SQRT2DIV2 = (FLOAT)sqrt(2.0)/2;


for(i=0;i<L;i++)
{   Sr[i]= sinput[invbittableN4096[i]];

   Si[i]=0;
}

M=1;
for(i=0;i<L;i+=2)
{ FLOAT sr1=Sr[i];

FLOAT sr2=Sr[i+1];


   FLOAT delta1 =pi/4;
FLOAT delta2 =pi/4*3;

   	Sr[i]= (FLOAT)(sr1*SQRT2DIV2+sr2*(-SQRT2DIV2));
	Si[i]=(FLOAT)(-sr1*SQRT2DIV2-sr2*SQRT2DIV2);
	delta1 =pi/4*(3);
	delta2 =pi/4;
    Sr[i+1]= (FLOAT)((sr1*(-SQRT2DIV2)+sr2*(SQRT2DIV2)));
	Si[i+1]=(FLOAT)(-sr1*SQRT2DIV2-sr2*SQRT2DIV2);

}

M=2;
for(i=0;i<L;i+=2*M)
  {

   for(j=0;j<M;j++)
   {
FLOAT sr1=Sr[i+j];
FLOAT si1=Si[i+j];
FLOAT sr2=Sr[i+j+M];
FLOAT si2=Si[i+j+M];

int index =(L/M*(2*j+1));
 index = index&BITNNN12;
cosdelta =costable4096[index];
sindelta =sintable4096[index];
Sr[i+j] = sr1 + sr2*cosdelta+si2*sindelta;
Si[i+j] = si1 - sr2*sindelta+si2*cosdelta;
Sr[i+j+M] = -sr1 +sr2*cosdelta+si2*sindelta;
Si[i+j+M] = -si1 -sr2*sindelta+si2*cosdelta;
   }//j

  }//i
 for(M=4;M<L;M=M*2)
 {

  for(i=0;i<L;i+=2*M)
  {

   for(j=0;j<M;j++)
   {
FLOAT sr1=Sr[i+j];
FLOAT si1=Si[i+j];
FLOAT sr2=Sr[i+j+M];
FLOAT si2=Si[i+j+M];

int index =(L/M*(2*j+1));
 index = index&BITNNN12;
cosdelta =costable4096[index];
sindelta =sintable4096[index];

Sr[i+j] = sr1 + sr2*cosdelta+si2*sindelta;
Si[i+j] = si1 - sr2*sindelta+si2*cosdelta;
Sr[i+j+M] = sr1 - sr2*cosdelta-si2*sindelta;
Si[i+j+M] = si1 + sr2*sindelta-si2*cosdelta;
   }//j

  }//i
}//M

for (i=0;i<L;i++)
{
FLOAT sr1=Sr[i];
FLOAT si1=Si[i];

int index =((2*i+1));
 index = index&BITNNN12;
cosdelta =costable4096[index];
sindelta =sintable4096[index];

Sr[i] = sr1*cosdelta+ si1*sindelta;
Si[i] = -sr1*sindelta+ si1*cosdelta;
}

return 0;
}

//----------------------------------------------
/*
IMDFT Butterfly

*/
int imdft4096(FLOAT *Sr, FLOAT *Si,FLOAT *sout)
{int N=12;
    FLOAT cosdelta,sindelta;
FLOAT srinvpos[1<<12];
FLOAT siinvpos[1<<12];
int i,j,M;
int L = (1<<N);


for(i=0;i<L;i++)
{   srinvpos[i]= Sr[invbittableN4096[i]];
siinvpos[i]= Si[invbittableN4096[i]];

}

 for(M=1;M<L;M=M*2)
 {

  for(i=0;i<L;i+=2*M)
  {

   for(j=0;j<M;j++)
   {
	   FLOAT sr1=srinvpos[i+j];
	   FLOAT si1=siinvpos[i+j];
	   FLOAT sr2=srinvpos[i+j+M];
	   FLOAT si2=siinvpos[i+j+M];

	   int index =(L/M*(2*j+1+L/2));
	   index = index&BITNNN12;
	   cosdelta =costable4096[index];
	   sindelta =sintable4096[index];
	   srinvpos[i+j] = sr1 + sr2*cosdelta-si2*sindelta;
	   siinvpos[i+j] = si1 + sr2*sindelta+si2*cosdelta;
	   srinvpos[i+j+M] = sr1 - sr2*cosdelta+si2*sindelta;
	   siinvpos[i+j+M] = si1 - sr2*sindelta-si2*cosdelta;

   }//j

  }//i

 }//M
for (i=0;i<L;i++)
{
FLOAT sr1=srinvpos[i];
FLOAT si1=siinvpos[i];

int index =((2*i+1+L/2));

 index = index&BITNNN12;
cosdelta =costable4096[index];
sindelta =sintable4096[index];
srinvpos[i] = sr1*cosdelta- si1*sindelta;
siinvpos[i] = sr1*sindelta+ si1*cosdelta;

sout[i]=srinvpos[i]/L;
}//i

return 0;
}


