/*
  BWE decoder frontend prototypes and definitions
*/

#ifndef __DEC_H
#define __DEC_H


#define MAXNRELEMENTS 2


#define MAXSBRBYTES 269


typedef enum
{
  BWEDEC_OK = 0,
  BWEDEC_CONCEAL,
  BWEDEC_NOSYNCH,
  BWEDEC_ILLEGAL_PROGRAM,
  BWEDEC_ILLEGAL_TAG,
  BWEDEC_ILLEGAL_CHN_CONFIG,
  BWEDEC_ILLEGAL_SECTION,
  BWEDEC_ILLEGAL_SCFACTORS,
  BWEDEC_ILLEGAL_PULSE_DATA,
  BWEDEC_MAIN_PROFILE_NOT_IMPLEMENTED,
  BWEDEC_GC_NOT_IMPLEMENTED,
  BWEDEC_ILLEGAL_PLUS_ELE_ID,
  BWEDEC_CREATE_ERROR,
  BWEDEC_NOT_INITIALIZED
}
BWE_ERROR;

/*typedef enum
{
  BWE_ID_SCE = 0,
  BWE_ID_CPE,
  BWE_ID_CCE,
  BWE_ID_LFE,
  BWE_ID_DSE,
  BWE_ID_PCE,
  BWE_ID_FIL,
  BWE_ID_END
}
*/
BWE_ELEMENT_ID;

typedef struct
{
  int ElementID;
  int ExtensionType;
  int Payload;
  unsigned int Data[MAXSBRBYTES];
}
BWE_ELEMENT_STREAM;

typedef struct
{
  int NrElements;
  int NrElementsCore;
  BWE_ELEMENT_STREAM bweElement[MAXNRELEMENTS]; /* for the delayed frame */
}
BWEBITSTREAM;



#endif
