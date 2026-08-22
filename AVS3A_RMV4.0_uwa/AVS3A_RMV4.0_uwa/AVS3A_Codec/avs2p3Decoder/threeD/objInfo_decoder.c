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
#include <string.h>
#include <math.h>
#include "..\general\decode\avs2audio.h"

void objInfo_decoder(avs2audiopack_buffer *opb, float objpx[][4], float objpy[][4], float objpz[][4], int area[][4], int*count, int Object_ID[], int txt)
{
	int buffer[100];
	int n = 0;
	int*p1 = buffer;
	int sb;
	//int Object_ID;
	int pId[4];
	int px[4];
	int py[4];
	int pz[4];
	int parea[4];
	int*a1 = pId;
	int*a2 = px;
	int*a3 = py;
	int*a4 = pz;
	int*a5 = parea;
	int NumPanSubBlocks = 4;
	int ObjectAreaControl;
	int PanInfoFlag;
	int ObjectType;
	int ObjectEnvelope;
	//3d info
	*p1 = avs2audiopack_read(opb, 7);
	Object_ID[txt] = *p1;
	*count += 7;
	p1 += 1;

	*a1 = avs2audiopack_read(opb, 3);

	*count += 3;
	a1 += 1;
	*a2 = avs2audiopack_read(opb, 10);

	*count += 10;
	a2 += 1;
	*a3 = avs2audiopack_read(opb, 10);

	*count += 10;
	a3 += 1;
	*a4 = avs2audiopack_read(opb, 10);

	*count += 10;
	a4 += 1;
	*p1 = avs2audiopack_read(opb, 1);
	ObjectAreaControl = *p1;
	*count += 1;
	p1 += 1;
	if (ObjectAreaControl == 1) {
		*a5 = avs2audiopack_read(opb, 4);//area
		*count += 4;
		a5 += 1;
	}
	*p1 = avs2audiopack_read(opb, 3);
	ObjectType = *p1;
	*count += 3;
	p1 += 1;


	if (ObjectType > 0)
	{
		*p1 = avs2audiopack_read(opb, 7);
		ObjectEnvelope = *p1;
		*count += 7;
		p1 += 1;

	}
	*p1 = avs2audiopack_read(opb, 4);

	*count += 4;
	p1 += 1;

	for (sb = 1; sb < 4; sb++) {
		*p1 = avs2audiopack_read(opb, 1);
		PanInfoFlag = *p1;
		*count += 1;
		p1 += 1;
		if (PanInfoFlag == 1) {
			*a1 = avs2audiopack_read(opb, 3);

			*count += 3;
			a1 += 1;
			*a2 = avs2audiopack_read(opb, 4);
			*count += 4;
			if (*a2 == 15) {
				*a2 = avs2audiopack_read(opb, 8);
				*count += 8;
				if (*a2 == 255) {
					*a2 = avs2audiopack_read(opb, 12);
					*count += 12;
				}
			}
			a2 += 1;

			*a3 = avs2audiopack_read(opb, 4);
			*count += 4;
			if (*a3 == 15) {
				*a3 = avs2audiopack_read(opb, 8);
				*count += 8;
				if (*a3 == 255) {
					*a3 = avs2audiopack_read(opb, 12);
					*count += 12;
				}
			}
			a3 += 1;

			*a4 = avs2audiopack_read(opb, 4);
			*count += 4;
			if (*a4 == 15) {
				*a4 = avs2audiopack_read(opb, 8);
				*count += 8;
				if (*a4 == 255) {
					*a4 = avs2audiopack_read(opb, 12);
					*count += 12;
				}
			}
			a4 += 1;

			*p1 = avs2audiopack_read(opb, 1);
			ObjectAreaControl = *p1;
			*count += 1;
			p1 += 1;
			if (ObjectAreaControl == 1)
			{
				*a5 = avs2audiopack_read(opb, 4);//area
				*count += 4;
				a5 += 1;
			}
			*p1 = avs2audiopack_read(opb, 3);
			ObjectType = *p1;
			*count += 3;
			p1 += 1;

			if (ObjectType > 0)
			{
				*p1 = avs2audiopack_read(opb, 7);
				ObjectEnvelope = *p1;
				*count += 7;
				p1 += 1;

			}
		}
		*p1 = avs2audiopack_read(opb, 4);
		*count += 4;
		p1 += 1;
	}
	*p1 = avs2audiopack_read(opb, 16);

	*count += 16;
	p1 += 1;



	for (n = 0; n < 4; n++) {
		switch (pId[n]) {
		case 0:
			px[n] = px[n];
			py[n] = py[n];
			pz[n] = pz[n];
			break;
		case 1:
			px[n] = px[n];
			py[n] = py[n];
			pz[n] = (-1)*pz[n];
			break;
		case 2:
			px[n] = px[n];
			py[n] = (-1)*py[n];
			pz[n] = pz[n];
			break;
		case 3:
			px[n] = px[n];
			py[n] = (-1)*py[n];
			pz[n] = (-1)*pz[n];
			break;
		case 4:
			px[n] = (-1)*px[n];
			py[n] = py[n];
			pz[n] = pz[n];
			break;
		case 5:
			px[n] = (-1)*px[n];
			py[n] = py[n];
			pz[n] = (-1)*pz[n];
			break;
		case 6:
			px[n] = (-1)*px[n];
			py[n] = (-1)*py[n];
			pz[n] = pz[n];
			break;
		case 7:
			px[n] = (-1)*px[n];
			py[n] = (-1)*py[n];
			pz[n] = (-1)*pz[n];
			break;
		}
	}

	objpx[Object_ID[txt]][0] = ((float)px[0]) / 1000;

	for (n = 1; n < 4; n++) {
		px[n] = px[n] + px[n - 1];
		objpx[Object_ID[txt]][n] = (float)px[n] / 1000;
	}

	objpy[Object_ID[txt]][0] = (float)py[0] / 1000;
	for (n = 1; n < 4; n++) {
		py[n] = py[n] + py[n - 1];
		objpy[Object_ID[txt]][n] = (float)py[n] / 1000;
	}

	objpz[Object_ID[txt]][0] = (float)pz[0] / 1000;
	for (n = 1; n < 4; n++) {
		pz[n] = pz[n] + pz[n - 1];
		objpz[Object_ID[txt]][n] = (float)pz[n] / 1000;
	}


	for (n = 0; n < 4; n++) {
		area[Object_ID[txt]][n] = (float)parea[n];
	}

}