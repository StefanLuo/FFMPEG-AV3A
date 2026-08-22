/*------------ Telecommunications & Signal Processing Lab --------------
                         McGill University

Routine:
  STmsg.h

Description:
  Message texts for the TSP ST routines.

Author / revision:
  P. Kabal  Copyright (C) 1997
  $Revision: 1.1.1.1 $  $Date: 2005/12/22 09:54:42 $

----------------------------------------------------------------------*/

#ifndef STmsg_h_
#define STmsg_h_

/* Error messages */

/* Warning messages */
#define STM_DataErr	"Data format error"
#define STM_BadIdent	"Invalid identifier"
#define STM_EmptyData	"Empty data string"
#define STM_ExtraChars	"Extra characters follow parameter"
#define STM_StrTrunc	"String truncated"
#define STM_TooFewData	"Too few data values"

/* Informational messages */
#define STM_BigVal	"Value too large"
#define STM_ExtraData	"Extra data ignored"
#define STM_SmallVal	"Value too small"

#endif	/* STmsg_h_ */
