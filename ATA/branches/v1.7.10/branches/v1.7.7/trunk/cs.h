/**************************************************************************
 *
 * SOURCE FILE NAME = CS.H
 *
 * DESCRIPTIVE NAME = Card Services Header File
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2006
 *
 ****************************************************************************/

    /*
    ** constant definitions
    */
#ifndef SUCCESS
#define SUCCESS 0
#endif
    /*
    ** type definitions
    */
typedef struct _IDC_PACKET {
  UCHAR  function;
  USHORT handle;
  PVOID  pointer;
  USHORT arglength;
  PVOID  argpointer;
} IDC_PACKET;

