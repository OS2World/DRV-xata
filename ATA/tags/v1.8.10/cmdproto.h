/**************************************************************************
 *
 * SOURCE FILE NAME =  CMDPROTO.H
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : C prototypes for Command Parser internal functions
 *
 ****************************************************************************/


/*  Command_Parser local function - PROTOTYPES	*/

#ifndef STATIC
  #define STATIC static
#endif

STATIC VOID   NEAR Insert_End_Token(VOID);
STATIC BOOL   NEAR Locate_First_Slash(VOID);
STATIC BOOL   NEAR Parse_Option_Value(VOID);
STATIC VOID   NEAR Skip_Over_Blanks(VOID);
STATIC VOID   NEAR char_parser(VOID);
STATIC VOID   NEAR d_parser(VOID);
STATIC VOID   NEAR dd_parser(VOID);
STATIC VOID   NEAR dddd_parser(VOID);
STATIC VOID   NEAR dddddd_parser(VOID);
STATIC ULONG  NEAR dd_parsersub(VOID);
STATIC VOID   NEAR hh_parser(VOID);
STATIC VOID   NEAR hhhh_parser(VOID);
STATIC VOID   NEAR location_parser(VOID);
STATIC BOOL   NEAR Insert_Token(VOID);
STATIC BOOL   NEAR Locate_Next_Slash(VOID);
STATIC BOOL   NEAR Validate_State_Index(USHORT);
STATIC BOOL   NEAR HH_Char_To_Byte(VOID);
#ifdef BBR
STATIC VOID   NEAR bad_block_parser(VOID);
STATIC VOID   NEAR hhhhhhhh_parser(VOID);

#pragma alloc_text( FCode, bad_block_parser )
#pragma alloc_text( FCode, hhhhhhhh_parser )
#endif
#if 1
#pragma alloc_text( FCode, Validate_State_Index )
#pragma alloc_text( FCode, Locate_Next_Slash )
#pragma alloc_text( FCode, Insert_Token )
#pragma alloc_text( FCode, location_parser )
#pragma alloc_text( FCode, hhhh_parser )
#pragma alloc_text( FCode, HH_Char_To_Byte )
#pragma alloc_text( FCode, hh_parser )
#pragma alloc_text( FCode, dd_parsersub )
#pragma alloc_text( FCode, dddd_parser )
#pragma alloc_text( FCode, dddddd_parser )
#pragma alloc_text( FCode, dd_parser )
#pragma alloc_text( FCode, d_parser )
#pragma alloc_text( FCode, char_parser )
#pragma alloc_text( FCode, Skip_Over_Blanks )
#pragma alloc_text( FCode, Parse_Option_Value )
#pragma alloc_text( FCode, Locate_First_Slash )
#pragma alloc_text( FCode, Insert_End_Token )
#endif
