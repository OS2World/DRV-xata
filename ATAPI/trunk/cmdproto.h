/**************************************************************************
 *
 * SOURCE FILE NAME =  CMDPROTO.H
 *
 * DESCRIPTIVE NAME =  DaniATAPI.FLT - Filter Driver for ATAPI
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DATE
 *
 * DESCRIPTION : C prototypes for Command Parser internal functions
 *
 ****************************************************************************/


/*  Command_Parser local function - PROTOTYPES	*/

#ifndef STATIC
  #define STATIC static
#endif

 VOID	NEAR Insert_End_Token(void);
 BOOL	NEAR Locate_First_Slash(void);
 BOOL	NEAR Parse_Option_Value(void);
 VOID	NEAR Skip_Over_Blanks(void);
 VOID	NEAR char_parser(void);
 VOID	NEAR d_parser(void);
 VOID	NEAR dd_parser(void);
 VOID	NEAR dddd_parser(void);
 USHORT NEAR dd_parsersub(void);
 VOID	NEAR hh_parser(void);
 VOID	NEAR hhhh_parser(void);
 VOID	NEAR atapi_parser(void);
 VOID	NEAR format_parser(void);
 VOID	NEAR scsi_id_parser(void);
 VOID	NEAR geometry_parser(void);
 VOID	NEAR chgline_parser(void);
 BOOL	NEAR Insert_Token(void);
 BOOL	NEAR Locate_Next_Slash(void);
 BOOL	NEAR Validate_State_Index( USHORT maxstates );
 BOOL	NEAR HH_Char_To_Byte(void);

