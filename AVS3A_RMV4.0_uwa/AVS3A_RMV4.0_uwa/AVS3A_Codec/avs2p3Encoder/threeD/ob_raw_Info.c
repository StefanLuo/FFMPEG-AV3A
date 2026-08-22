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

#include "..\general\encode\Bitstream.h"

void object_info(int ObjectNum, float a[][16], int Object_ID[], int*OutBytes, char*obInfo)
{
	int i;
	int n;
	int positionx[4];
	int positiony[4];
	int positionz[4];
	int ObjectAreaGamma[4];
	int pID[4];
	int obj;
	int sb;
	int PanInfoFlag = 1;
	int ObjectAreaControl;
	int ObjectType = 2;
	int ObjectEnvelope = 1;
	int ObjectDataGroupNum = ObjectNum;
	int Object_codec_id = 1;
	int channel_number_index = 2;
	int channel_number = 6;
	int resolution = 1;
	int bitrate_index = 22;
	int bitstream_type = 1;
	int numbit = 0;
	avs2audiopack_buffer *opb;
	opb = calloc(1, sizeof(avs2audiopack_buffer));
	avs2audiopack_writeinit(opb);

	ObjectType = ((int)(rand() / 5000) + 1) & 0x07; //add (&0x07) to fix bug, shumin.xu 20210105
	ObjectEnvelope = rand() & 0x7F;

	avs2audiopack_write(opb, 0, 4);   //THRDVersion, 20181122 shumin.xu
	numbit += 4;
	avs2audiopack_write(opb, ObjectNum, 7);
	numbit += 7;


	//3D info
	for (obj = 0; obj < ObjectNum; obj++)
	{

		n = Object_ID[obj];

		for (i = 3; i >= 0; i--)
			positionx[i] = (int)(a[n][4 * i] * 1000);
		for (i = 3; i >= 0; i--) {
			if (i > 0)
				positionx[i] = positionx[i] - positionx[i - 1];
		}


		for (i = 3; i >= 0; i--)
			positiony[i] = (int)(a[n][4 * i + 1] * 1000);
		for (i = 3; i >= 0; i--) {
			if (i > 0)
				positiony[i] = positiony[i] - positiony[i - 1];
		}


		for (i = 3; i >= 0; i--)
			positionz[i] = (int)(a[n][4 * i + 2] * 1000);
		for (i = 3; i >= 0; i--) {
			if (i > 0)
				positionz[i] = positionz[i] - positionz[i - 1];
		}


		for (i = 3; i >= 0; i--)
			ObjectAreaGamma[i] = (int)a[n][4 * i + 3];


		for (i = 0; i < 4; i++) {
			if ((positionx[i] >= 0) && (positiony[i] >= 0) && (positionz[i] >= 0))
				pID[i] = 0;
			else if ((positionx[i] >= 0) && (positiony[i] >= 0) && (positionz[i] < 0))
				pID[i] = 1;
			else if ((positionx[i] >= 0) && (positiony[i] < 0) && (positionz[i] >= 0))
				pID[i] = 2;
			else if ((positionx[i] >= 0) && (positiony[i] < 0) && (positionz[i] < 0))
				pID[i] = 3;
			else if ((positionx[i] < 0) && (positiony[i] >= 0) && (positionz[i] >= 0))
				pID[i] = 4;
			else if ((positionx[i] < 0) && (positiony[i] >= 0) && (positionz[i] > 0))
				pID[i] = 5;
			else if ((positionx[i] < 0) && (positiony[i] < 0) && (positionz[i] >= 0))
				pID[i] = 6;
			else if ((positionx[i] < 0) && (positiony[i] < 0) && (positionz[i] < 0))
				pID[i] = 7;
			positionx[i] = abs(positionx[i]);
			positiony[i] = abs(positiony[i]);
			positionz[i] = abs(positionz[i]);

		}


		avs2audiopack_write(opb, Object_ID[obj], 7);
		numbit += 7;

		for (sb = 0; sb < 4; sb++)
		{

			if (sb == 0) {
				PanInfoFlag = 1;
			}
			else {
				avs2audiopack_write(opb, PanInfoFlag, 1);
				numbit += 1;
			}
			if (PanInfoFlag == 1)
			{

				if (sb == 0) {
					avs2audiopack_write(opb, pID[sb], 3);
					numbit += 3;
					avs2audiopack_write(opb, positionx[sb], 10);
					numbit += 10;
					avs2audiopack_write(opb, positiony[sb], 10);
					numbit += 10;
					avs2audiopack_write(opb, positionz[sb], 10);
					numbit += 10;
				}

				else {
					avs2audiopack_write(opb, pID[sb], 3);
					numbit += 3;
					if (positionx[sb] < 15) {
						avs2audiopack_write(opb, positionx[sb], 4);
						numbit += 4;
					}
					else if ((positionx[sb] >= 15) && (positionx[sb] < 255)) {
						avs2audiopack_write(opb, 15, 4);
						numbit += 4;
						avs2audiopack_write(opb, positionx[sb], 8);
						numbit += 8;
					}
					else if (positionx[sb] >= 255) {
						avs2audiopack_write(opb, 4095, 12);
						numbit += 12;
						avs2audiopack_write(opb, positionx[sb], 12);
						numbit += 12;
					}

					if (positiony[sb] < 15) {
						avs2audiopack_write(opb, positiony[sb], 4);
						numbit += 4;
					}
					else if ((positiony[sb] >= 15) && (positiony[sb] < 255)) {
						avs2audiopack_write(opb, 15, 4);
						numbit += 4;
						avs2audiopack_write(opb, positiony[sb], 8);
						numbit += 8;
					}
					else if (positiony[sb] >= 255) {
						avs2audiopack_write(opb, 4095, 12);
						numbit += 12;
						avs2audiopack_write(opb, positiony[sb], 12);
						numbit += 12;
					}

					if (positionz[sb] < 15) {
						avs2audiopack_write(opb, positionz[sb], 4);
						numbit += 4;
					}
					else if ((positionz[sb] >= 15) && (positionz[sb] < 255)) {
						avs2audiopack_write(opb, 15, 4);
						numbit += 4;
						avs2audiopack_write(opb, positionz[sb], 8);
						numbit += 8;
					}
					else if (positionz[sb] >= 255) {
						avs2audiopack_write(opb, 4095, 12);
						numbit += 12;
						avs2audiopack_write(opb, positionz[sb], 12);
						numbit += 12;
					}
				}
				if (ObjectType == 0)
					ObjectAreaControl = 0;
				else
					ObjectAreaControl = 1;
				avs2audiopack_write(opb, ObjectAreaControl, 1);
				numbit += 1;
				if (ObjectAreaControl == 1) {
					avs2audiopack_write(opb, ObjectAreaGamma[sb], 4);
					numbit += 4;
				}

				avs2audiopack_write(opb, ObjectType, 3);
				numbit += 3;
				if (ObjectType > 0) {
					avs2audiopack_write(opb, ObjectEnvelope, 7);
					numbit += 7;
				}
			}
			avs2audiopack_write(opb, 0, 4);
			numbit += 4;

		}
		avs2audiopack_write(opb, 0, 16);
		numbit += 16;

	}

	if (numbit % 8 != 0)
	{
		avs2audiopack_write(opb, 0, (8 - numbit % 8));
		numbit += (8 - numbit % 8);
	}
	*OutBytes = numbit / 8;

	memcpy(obInfo, opb->buffer, opb->endbyte);
}

