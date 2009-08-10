/**************************************************************************
 *
 * SOURCE FILE NAME =  CMDPARSE.H
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Basic data types for Command Parser
 *
 ****************************************************************************/


/*  Typedefs  */

typedef unsigned char BYTE;

typedef struct _tbytes {
    BYTE   byte1;
    BYTE   byte2;
} TBYTES;

typedef union _number {
    USHORT n;
    TBYTES two_bytes;
} NUMBER;

#ifdef BBR
typedef struct _fbytes {
    BYTE   byte1;
    BYTE   byte2;
    BYTE   byte3;
    BYTE   byte4;
} FBYTES;

typedef union _lnumber {
    ULONG n;
    FBYTES four_bytes;
} LNUMBER;
#endif

typedef union _charbyte {
    BYTE   byte_value;
    CHAR   char_value;
} CHARBYTE;

typedef struct _cc {
  USHORT     ret_code;
  USHORT     err_index;
} CC;


/* list of Command_Parser Completion Codes (ret_code) */

#define NO_ERR		    0
#define SYNTAX_ERR	    1
#define BUF_TOO_SMALL_ERR   2
#define UNDEFINED_TYPE_ERR  3
#define UNDEFINED_STATE_ERR 4
#define NO_OPTIONS_FND_ERR  5
#define REQ_OPT_ERR	    6
#define INVALID_OPT_ERR     7



/*----------------------------------------------------------------------------*/
/*									      */
/*   Description of OPTIONTABLE Structure				      */
/*   ------------------------------------				      */
/*									      */
/*   The OPTIONTABLE allows the ADD to define the rules to be followed	      */
/*   by the Command_Parser.  In general, the table contains an entry	      */
/*   for each valid command line option.  Each entry defines the	      */
/*   command line option's syntax, unique id, and output buffer format.       */
/*   The command line option syntax is made up of:			      */
/*									      */
/*   1)  A character string field, which defines the option.		      */
/*   2)  A type field - which defines the format of the value assigned to     */
/*	 an option.							      */
/*   3)  A state table - which defines an option's positional relationship,   */
/*	 relative to the other options. 				      */
/*									      */
/*									      */
/*   Specificly, the OPTIONTABLE consists of the following fields:	      */
/*									      */
/*     entry_state   = This field tells the Command_Parser which element      */
/*		       in the poption -> opt.state[] contains the entry       */
/*		       position into the state table.			      */
/*									      */
/*     max_state     = This field tells the Command_Parser the		      */
/*		       number of elements in the poption -> opt.state[]       */
/*		       state table.					      */
/*									      */
/*	poption[] =  This field contains an array of OPT pointers.	      */
/*		     An entry exists for each valid command line option.      */
/*		     Please note that the order of the elements in the	      */
/*		     array is important.  For example: if you have 2 options  */
/*		     with similar option strings, say, "/DM" and "/DM:"       */
/*		     then a pointer to "/DM:" OPT structure must appear       */
/*		     before the "/DM" option pointer.                         */
/*									      */
/*		     The end of the array is denoted by 		      */
/*		     poption -> opt.id = TOK_ID_END			      */
/*									      */
/*									      */
/*   Description of OPT Structure					      */
/*   ----------------------------					      */
/*   The OPT structure defines the rules associated with a specific	      */
/*   command line option.  It consists of the following fields: 	      */
/*									      */
/*	      id  =  This field contains a unique id for its associated       */
/*		     command line option.  It is used by the parser to	      */
/*		     identify the option in the output buffer.		      */
/*		     (TOK_ID_END denotes the end of the table)		      */
/*									      */
/*	  string  =  This field contains a pointer to the valid command       */
/*		     line option string. This field is not case sensitive.    */
/*		     The option string must start with the "/" char and,      */
/*		     if a value can be assigned to the command opt, then      */
/*		     the option string must end with the ":" char,            */
/*		     followed by the assigned value.			      */
/*		     The parser uses this field to identify the option in     */
/*		     the command line.					      */
/*									      */
/*	    type  =  This field defines the format of the options assigned    */
/*		     value, for both parsing and setting up the output	      */
/*		     buffer. There are 9 pre-defined formats that the	      */
/*		     parser accepts, all of which are not case sensitive      */
/*		     (exception: TYPE_CHAR.)  The predefined formats are:     */
/*									      */
/*OutBuf Value								      */
/*Field Length Type	 Description					      */
/*------------ ------	 ------------------------------ 		      */
/*	  0    TYPE_0	 no associated valued allowed			      */
/*									      */
/*			In the output buffer the TYPE_0 token		      */
/*			contains a 0 byte field value.			      */
/*									      */
/*   varies    TYPE_CHAR characters until / char, new line,		      */
/*			 carriage return or null string detected	      */
/*									      */
/*			In the output buffer the TYPE_CHAR token	      */
/*			contains a varying number of byte field value.	      */
/*			Each byte is in char format and consists of	      */
/*			char as entered on the command line.		      */
/*									      */
/*			To determine the number of 1 byte char		      */
/*			fields, in the token string -- subtract 	      */
/*			TOK_MIN_LENGTH from the contents of the token	      */
/*			length field.					      */
/*									      */
/*	  1    TYPE_D	 one decimal digit (d)				      */
/*									      */
/*			In the output buffer the TYPE_D token		      */
/*			contains a 1 byte field value,			      */
/*			containing the char to integer conversion	      */
/*			of the d decimal characters.			      */
/*									      */
/*	  1    TYPE_O	 one optional decimal digit (d) 			       */
/*									      */
/*			In the output buffer the TYPE_D token		      */
/*			contains a 1 byte field value,			      */
/*			containing the char to integer conversion	      */
/*			of the d decimal characters or -1 if no value given   */
/*									      */
/*	  2    TYPE_DD	 two decimal digits (dd)			      */
/*									      */
/*			In the output buffer the TYPE_DD token		      */
/*			contains a 1 byte field value,			      */
/*			containing the char to integer conversion	      */
/*			of the dd decimal characters.			      */
/*									      */
/*	  2    TYPE_HH	 hh,hh hexidecimal digits			      */
/*									      */
/*			In the output buffer the TYPE_HH token		      */
/*			contains 2, 1 byte field values.  Each		      */
/*			containing the char to hex conversion		      */
/*			of the hh hex characters.			      */
/*									      */
/*	  2    TYPE_HHHH hhhh hexidecimal digits			      */
/*									      */
/*			In the output buffer the TYPE_HHHH token	      */
/*			contains a 2 byte field value consisting	      */
/*			of 1 unsign short field set to char to hex	      */
/*			conversion of the hhhh hex characters.		      */
/*									      */
/*	  3    TYPE_PCILOC  dd:dd.d#d	PCI bus:device.function#channel       */
/*									      */
/*			In the output buffer the TYPE_PCILOC token	      */
/*			contains a 3 byte field value consisting	      */
/*			of 1 byte bus, 1 byte device/function, 1 byte channel */
/*									      */
/*	  4    TYPE_HHHHHHHH hhhhhhhh hexidecimal digits		      */
/*									      */
/*			In the output buffer the TYPE_HHHHHHHH token	      */
/*			contains a 4 byte field value consisting	      */
/*			of 1 unsign long field set to char to hex	      */
/*			conversion of the hhhhhhhh hex characters.	      */
/*									      */
/*	  2    TYPE_FORMAT valid command line format strings are:	      */
/*			    360,260K,360KB				      */
/*			    720,720K,720KB				      */
/*			    1200,1200K,1200KB,1.2,1.2M,1.2MB		      */
/*			    1440,1440K,1440KB,1.44,1.44M,1.44MB 	      */
/*			    2880,2880K,2880KB,2.88,2.88M,2.88MB 	      */
/*									      */
/*			In the output buffer the TYPE_FORMAT token	      */
/*			contains a 2 byte field value consisting	      */
/*			of 1 unsign short field set to either		      */
/*			360, 720, 1200, 1440 or 2880.			      */
/*									      */
/*   varies    TYPE_GEOMETRY  dd or (dddd,dddd,dddd) physical geometry	      */
/*									      */
/*			In the output buffer the TYPE_GEOMETRY token	      */
/*			contains either a 1 byte field value or 6 byte	      */
/*			field value, consisting of 3 unsign short fields.     */
/*									      */
/*			To determine the format of this field (1 or 6),       */
/*			in the token string -- subtract 		      */
/*			TOK_MIN_LENGTH from the contents of the token	      */
/*			length field (results will be either 1 or 6).	      */
/*									      */
/*   varies    TYPE_SCSI      d,... and (d,d),... scsi target id	      */
/*				     where d = 0-7			      */
/*									      */
/*			In the output buffer the TYPE_SCSI token	      */
/*			contains varying number of 2 byte field values.       */
/*			The first contains the SCSI Target Id and the	      */
/*			second contains the LUN id (0 if the d format	      */
/*			is used.)					      */
/*									      */
/*			To determine the number of 2 byte fields, in	      */
/*			the token string -- subtract TOK_MIN_LENGTH	      */
/*			from the contents of the token length field and       */
/*			then divide by 2.				      */
/*									      */
/*	   state[1] =  This field defines a command line option's syntax,     */
/*		       relative to the other command line options.	      */
/*		       It is designed as a state table with the initial       */
/*		       state (entry_state) as the starting point and the      */
/*		       size of the table (array) defined by max_states.       */
/*		       The parser uses this table to syntax check the	      */
/*		       command line option.				      */
/*									      */
/*		       Based on the valid options (field != E) within a given */
/*		       state the parser locates the next option in the	      */
/*		       command line.  Once located, the next state (col2)     */
/*		       assigned is specified in the option (row) (col1)       */
/*		       field.  Then col1 = col2, and the parsing continues    */
/*		       until either an error or the end of command line is    */
/*		       detected.					      */
/*									      */
/*		       NOTE:  The last row (id = TOK_ID_END), is uniquely     */
/*			      defined.	Its state[] array, contains:	      */
/*									      */
/*			      -- R - if one of the valid options within the   */
/*				     state (col) is a required option	      */
/*									      */
/*			      -- O - if none of the options within the	      */
/*				     state (col) are required option.	      */
/*									      */
/*									      */
/*----------------------------------------------------------------------------*/

typedef struct _opt {
  CHAR	     id;	   /* user defined		     */
  NPSZ	     string;	   /* user defined		     */
  BYTE	     type;	   /* user selected from list below  */
  CHAR	     state[];	   /* user selected		     */
} OPT, FAR *POPT, NEAR *NPOPT;

typedef struct _optiontable {
  UCHAR      entry_state;
  UCHAR      max_states;
  NPOPT      poption[];
} OPTIONTABLE, FAR *POPTIONTABLE, NEAR *NPOPTIONTABLE;


/* SPECIAL TOKEN ID - OPT.id value */

#define TOK_END 	  -1	   /* denotes end of token string */
#define TOKL_ID_END	   2	   /* length of end token	  */



/*  OPT.type values and associate token length and output info	*/

#define TYPE_0		  1	 /* no associates values allowed      0      */
#define TYPE_CHAR	  2	 /* chars till '/' char/cr/nl/null   varies  */
#define TYPE_D		  3	 /* d digit			      1      */
#define TYPE_DD 	  4	 /* dd digit			      2      */
#define TYPE_HH 	  5	 /* hh,hh  h-hexidemical	      2      */
#define TYPE_HHHH	  6	 /* hhhh   h-hexidemical	      2      */
#define TYPE_O		  7	 /* optional 'd'                      1      */
#define TYPE_DDDD	  8	 /* dddd			      2      */
#define TYPE_PCILOC	  9	 /* bus:device.function#channel       3      */
#ifdef BBR
#define TYPE_BBR	 12	 /* bad block sequence (hhhhhhhh,hhhh)[,(hhhhhhhh,hhhh)] length varies, 6 per sequence */
#define TYPE_HHHHHHHH	 13	 /* hhhhhhhh   h-hexidemical	      4      */
#endif


/*  SPECIAL OPT.state values */

#define E		-1	/* identifies an invalid state		      */
#define O		-2	/* identifies that no option is required      */
#define R		-3	/* identifies that an valid option is required*/


/* Output Buffer Token Layout */

#define TOKL_LEN	       0	    /* offset of length in token      */
#define TOKL_ID 	       1	    /* offset of id in token	      */
#define TOKL_VALUE	       2	    /* offset of value in token       */
#define TOK_MIN_LENGTH	   TOKL_VALUE	    /* length field + id field	      */


/*******************************************************************************
*									       *
*   FUNCTION:  The Command_Parser ADD help routine, assists ADDs with	       *
*	       parsing the CONFIG.SYS command line options. Using the	       *
*	       rules defined by the ADD supplied OPTIONTABLE, the command      *
*	       parser:							       *
*									       *
*	       1) Parses the CONFIG.SYS command line (pCmdLine).	       *
*									       *
*	       2) Sets up the ADD supplied output buffer (pOutBuf) with        *
*		  the parsed command line options.			       *
*									       *
*	       3) Returns to the ADD with a completion code in the function    *
*		  name. 						       *
*									       *
*   SETUP:     To call the Command_Parser routine, include the "CMDPHDR.H"     *
*	       file in your calling module and	define the necessary parms by  *
*	       using one of the supplied tables as a based. The supplied       *
*	       tables reside in the following files:			       *
*									       *
*		  CMDPDSKT.C/CMDPDSKT.H --> diskette type ADD parm definitions *
*		  CMDPDISK.C/CMDPDISK.H --> disk type ADD parm definitions     *
*		  CMDPSCSI.C/CMDPSCSI.H --> SCSI type ADD parm definitions     *
*									       *
*									       *
*   PROTOTYPE:	 CC  FAR Command_Parser(PSZ pCmdLine, POPTIONTABLE pOptTable,  *
*					BYTE *pOutBuf, USHORT OutBuf_Len);     *
*									       *
*   ENTRY:     pCmdLine     = Far pointer to an array of characters, which     *
*			      contains the CONFIG.SYS command line.	       *
*									       *
*	       pOptTable    = Far pointer to the OPTIONTABLE structure, which  *
*			      contains the ADD defined rules for the	       *
*			      Command_Parser to follow.  It includes a list    *
*			      of the valid options and each for each option the*
*			      command line syntax and output buffer format.    *
*									       *
*		pOutBuf      = Pointer to an ADD supplied buffer (array of     *
*			      unsign characters), for the Command_Parser       *
*			      to return the parsed command options and	       *
*			      their associated values.			       *
*									       *
*	       OutBuf_Len   = Size, in bytes of the ADD supplied output        *
*			      buffer (pOutBuf).  This value is relative to 1.  *
*									       *
*									       *
*   RETURN:    CC	    = The function name contains the completion        *
*			      code information in the CC structure. Which      *
*			      consists of the following 2 fields:	       *
*									       *
*	      ret_code = Completion Code				       *
*									       *
*			 Value		       Description		       *
*			 -----------------     --------------------------------*
*			 NO_ERR 	     - Successful		       *
*									       *
*			 SYNTAX_ERR	     - Based on Option Table, type     *
*					       field, a syntax error was found *
*					       parsing the value assigned to   *
*					       option.			       *
*									       *
*			 BUF_TOO_SMALL_ERR   - OutBuf_Len value is too small.  *
*									       *
*			 UNDEFINED_TYPE_ERR  - Option Table, type field is     *
*					       undefined.		       *
*									       *
*			 UNDEFINED_STATE_ERR - Option Table state table field  *
*					       contains an invalid state.      *
*									       *
*			 NO_OPTIONS_FND_ERR  - No options as defined in the    *
*					       Option Table were found.        *
*									       *
*			 REQ_OPT_ERR	     - Based on Option Table last entry*
*					       a required option was not found *
*									       *
*			 INVALID_OPT_ERR     - A / was found, but the option   *
*					       was not in the Option Table     *
*									       *
*									       *
*		err_index = This field contains an index into the	       *
*			    command line (pCmdLine) of the character	       *
*			    being parsed when an error was detected.	       *
*			    The index returned in this field is valid	       *
*			    only when ret_code != NO_ERR.		       *
*									       *
*	       pOutBuf	    = The ADD supplied buffer pointed to by pOutBuf    *
*			      contains the successfully parsed command line    *
*			      options and their associated values, in the      *
*			      following format: 			       *
*									       *
*									       *
*			      -------------------------------------------------*
*	    OutBuf Format --> |option token| option token| ...| end of tokens |*
*			      -------------------------------------------------*
*				   |				  |	       *
*				   |				  |	       *
*				   v				  v TOK_END    *
*			    ----------------------	      ---------------  *
*   Option Token Format --> |length | id | value |	      |  2    | -1  |  *
*			    ----------------------	      ---------------  *
*							       length	 id    *
*									       *
*			      length	=  This field contains the total       *
*					   length of the token, in bytes.      *
*									       *
*					   length (1) + id (1) + value	       *
*									       *
*					   Note: The length of the value field *
*						 varies, based on the type     *
*						 assigned to the option in     *
*						 the OPTIONTABLE.	       *
*									       *
*			      id	=  This field contains the unique id   *
*					   assigned to the option in the       *
*					   OPTIONTABLE. opt_id = TOK_END,      *
*					   denotes the end of the token string.*
*									       *
*			      value	=  This field contains the value       *
*					   assigned to a option on the command *
*					   line.  The format and meaning       *
*					   of this field varies based on the   *
*					   type assigned to the option in      *
*					   the OPTIONTABLE.		       *
*									       *
*******************************************************************************/
CC NEAR Command_Parser(PSZ, NPOPTIONTABLE, NPBYTE, USHORT);

#pragma alloc_text( FCode, Command_Parser )

