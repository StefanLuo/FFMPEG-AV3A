/*------------ Telecommunications & Signal Processing Lab --------------
                         McGill University

Routine:
  UTpar.h

Description:
  Declarations for the TSP utility routines.

Author / revision:
  P. Kabal  Copyright (C) 1997
  $Revision: 1.1.1.1 $  $Date: 2005/12/22 09:54:42 $

----------------------------------------------------------------------*/

#ifndef UTpar_h_
#define UTpar_h_

/* Machine and data byte order codes */
enum {
  DS_UNDEF	= -1,	/* undefined */
  DS_EB		= 0,	/* big-endian */
  DS_EL		= 1,	/* little-endian */
  DS_NATIVE	= 2,	/* native */
  DS_SWAP       = 3	/* byte-swapped */
};

#endif	/* UTpar_h_ */
