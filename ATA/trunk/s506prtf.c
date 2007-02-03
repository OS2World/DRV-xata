/**************************************************************************
 *
 * SOURCE FILE NAME =  S506PRTF.C
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : C SPRINTF Routine
 *
 ****************************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_NO_SCB
#define INCL_INITRP_ONLY
#include "os2.h"

#include "dskinit.h"
#include "iorb.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "addcalls.h"

#include "s506cons.h"
#include "s506type.h"
#include "s506pro.h"

#pragma optimize(OPTIMIZE, on)

#define va_start(ap,v) ap = (PUCHAR) &v + sizeof(v)
#define va_arg(ap,t)   ((t FAR *)(ap += sizeof(t)))[-1]
#define va_end(ap)     ap = NULL

struct _FMTPARMS {
  USHORT      type;
#define 	TYPE_END	 0
#define 	TYPE_INVALID	 1
#define 	TYPE_STRING	 2
#undef TYPE_CHAR
#define 	TYPE_CHAR	 3
#define 	TYPE_DECIMAL	 4
#define 	TYPE_UNSIGNED	 5
#define 	TYPE_HEX	 6
#define 	TYPE_PCT	 7

  USHORT      flags;
#define 	FS_NEGLEN	 0x8000
#define 	FS_LEN		 0x4000
#define 	FS_PRC		 0x2000
#define 	FS_LONG 	 0x1000

  UINT	      len;
  USHORT      prc;
  UCHAR       pad;

  NPCH	      buf;
  USHORT      off;

  NPCH	      fmtstrt,
	      fmtptr;

};

typedef struct _FMTPARMS  FMTPARMS;
typedef FMTPARMS FAR  *LPFMTPARMS;
typedef FMTPARMS NEAR *NPFMTPARMS;

#define MIN(a,b)	((a) < (b) ? (a) : (b))
#define MAX(a,b)	((a) > (b) ? (a) : (b))




VOID FAR sprntf (NPSZ buf, NPSZ fmt, ...);
USHORT FAR prntf (NPSZ buf, NPSZ fmt, PUCHAR arg_ptr);

static process_format (NPFMTPARMS fs);
static put_field (NPFMTPARMS fs, NPSZ buf);
static cvtld (	  NPSZ buf, LONG n  );
static cvtlu (	  NPSZ buf, ULONG n );
static cvtlx(	  NPSZ s,   ULONG n );
static reverse (  NPSZ s);
static strnovl (  NPSZ d, NPSZ s,  USHORT n);
static chrdup(	  NPSZ d, UCHAR c,USHORT n);


/******************************/
/* Format to Buffer (USER EP) */
/******************************/

VOID FAR sprntf (NPSZ buf, NPSZ fmt, ...)
{
  PUCHAR arg_ptr;

  va_start (arg_ptr, fmt);
  prntf (buf, fmt, arg_ptr);
  va_end (arg_ptr);
}



/******************/
/* Format string  */
/******************/

UCHAR sbuf[20];
FMTPARMS fs;

USHORT FAR prntf (NPSZ buf, NPSZ fmt, PUCHAR arg_ptr)
{
  NPCH	sptr;

  LONG	lval;
  ULONG uval;

  fs.buf    = buf;
  fs.off    = 0;
  fs.fmtptr = fmt;

  while (process_format (&fs) != TYPE_END) {
    switch (fs.type) {
      case TYPE_STRING:
	sptr = va_arg (arg_ptr, NPCH);
	put_field (&fs, sptr);
	break;

      case TYPE_CHAR:
	*(short far *)(fs.buf+fs.off) = va_arg (arg_ptr, USHORT);
	fs.off++;
	break;

      case TYPE_PCT:
	*(short far *)(fs.buf+fs.off) = '%';
	fs.off++;
	break;

      case TYPE_DECIMAL:
	lval = (fs.flags & FS_LONG) ? va_arg (arg_ptr, LONG)
				    : va_arg (arg_ptr, INT );
	cvtld (sbuf, lval);

	put_field (&fs, sbuf);
	break;

      case TYPE_UNSIGNED:
      case TYPE_HEX:
	uval = (fs.flags & FS_LONG) ? va_arg (arg_ptr, ULONG)
				    : va_arg (arg_ptr, USHORT );
	if (fs.type == TYPE_HEX)
	  cvtlx (sbuf, uval);
	else
	  cvtlu (sbuf, uval);

	put_field (&fs, sbuf);
	break;

      case TYPE_INVALID:
	_strncpy (fs.buf+fs.off, fs.fmtstrt, fs.fmtptr-fs.fmtstrt);
	fs.off += fs.fmtptr - fs.fmtstrt;
	break;
    }
  }

  return (0);
}


/****************************/
/* Process Next Format Spec */
/****************************/

static process_format (NPFMTPARMS fs)
{
  NPCH	fp;
  UCHAR c;

  fs->len = fs->prc = fs->flags = 0;
  fs->pad = ' ';

  fp = fs->fmtstrt = fs->fmtptr;
  c = *fp;

  /**********************/
  /* Output Literal Data */
  /**********************/

  while (c && c != '%')
    {
      *(fs->buf+fs->off) = c;
      fs->off++;
      c = *++fp;
      if (fs->off >= 100) c = 0;
    }

  *(fs->buf+fs->off) = 0;

  /**********************************/
  /* Check for End of Format String */
  /**********************************/

  if (!c)
    {
      fs->type = TYPE_END;
      goto fmt_done;
    }

  fs->fmtstrt = fp;

  c = *++fp;

  /******************************/
  /* Check for %% Format String */
  /******************************/

  if (c == '%')
    {
      fs->type = TYPE_PCT;
      fp++;
      goto fmt_done;
    }

  /***********************/
  /* Check for Leading - */
  /***********************/

  if (c == '-')
    {
      fs->flags |= FS_NEGLEN;
      c = *++fp;
    }

  /**************************/
  /* Check for Zero Padding */
  /**************************/

  if (c == '0')
    {
      fs->pad = '0';
      c = *++fp;
    }

  /******************************/
  /* Check for Min Field Length */
  /******************************/

  while (c >= '0' && c <= '9')
    {
      fs->flags |= FS_LEN;
      fs->len	 = fs->len * 10 + c - '0';
      c = *++fp;
    }

  /******************************/
  /* Check for Max Field Length */
  /******************************/

  if (c == '.')
    {
      c = *++fp;

      while (c >= '0' && c <= '9')
	{
	  fs->flags |= FS_PRC;
	  fs->prc    = fs->prc * 10 + c - '0';
	  c = *++fp;
	}
    }

  /****************************/
  /* Check for Long Parameter */
  /****************************/

  if (c == 'l' || c == 'L')
    {
      fs->flags |= FS_LONG;
      c = *++fp;
    }

  /***************************/
  /* Check Valid Format Type */
  /***************************/

  switch (c)
    {
      case 's' :
      case 'S' :
	fs->type = TYPE_STRING;
	break;
      case 'd' :
      case 'D' :
	fs->type = TYPE_DECIMAL;
	break;
      case 'u' :
      case 'U' :
	fs->type = TYPE_UNSIGNED;
	break;
      case 'x' :
      case 'X' :
	fs->type = TYPE_HEX;
	break;
      case 'c' :
      case 'C' :
	fs->type = TYPE_CHAR;
	break;
      default:
	fs->type = TYPE_INVALID;
    }

  fp++;

  fmt_done: fs->fmtptr = fp;

  return (fs->type);

}


/********************************************/
/* Transfer Formatted Data to Output Buffer */
/********************************************/

static put_field (NPFMTPARMS fs, NPSZ buf)
{
  USHORT dlen,
	 flen,
	 slen;

  INT	boff;


  /*************************/
  /* Calculate Data Length */
  /*************************/

  slen = strlen (buf);
  dlen = (fs->flags & FS_PRC) ? MIN (fs->prc, slen) : slen;

  /**************************/
  /* Calculate Field Length */
  /**************************/

  flen = (fs->flags & FS_LEN) ? fs->len : dlen;

  /**********************************/
  /* Calculate Data Offset in Field */
  /**********************************/

  boff = ((fs->flags & FS_LEN) && !(fs->flags & FS_NEGLEN)) ? flen - dlen : 0;
  if (boff < 0) boff = 0;

  /****************************/
  /* Fill Field with Pad Char */
  /****************************/

  chrdup (fs->buf+fs->off, fs->pad, MAX(flen, dlen));

  /**********************/
  /* Copy Data to Field */
  /**********************/

  strnovl (fs->buf+fs->off+boff, buf, dlen);
  fs->off += MAX (flen, dlen) ;

  return (0);
}

/*******************************/
/* Convert LONG INT to CHAR    */
/*******************************/

static cvtld (NPSZ buf, LONG n)
{
  SHORT i=0,
	nz = 0;
  LONG	j,
	d;

  if (n < 0)
    {
      buf[i++] = '-';
      n = -n;
    }

  cvtlu (buf+i, n);
}

/*******************************/
/* Convert ULONG INT to CHAR   */
/*******************************/

static cvtlu (NPSZ buf, ULONG n)
{
  SHORT i=0,
	nz = 0;
  ULONG j,
	d;

  if (n == 0)
    buf[i++] = '0';
  else
  {

    for (j=1000000000L; j > 0; j /= 10)
      {
	d =  n / j;
	n =  n % j;

	if (nz = (nz || d))
	    buf[i++] = d + '0';
       }
  }
  buf[i] = 0;
  return (0);
}


/*******************************/
/* convert ULONG to hex string */
/*******************************/

static cvtlx (NPSZ s, ULONG n)
{
  SHORT i, sign;
  static CHAR table[] = "0123456789ABCDEF";

  i = 0;
  do {
    s[i++] = table[ n & 0x000F ];
  } while ((n >>= 4) > 0);

  s[i] = '\0';
  reverse(s);

 return (0);
}

/*******************************/
/* reverse string s in place   */
/*******************************/

static reverse(NPSZ s)
{
  SHORT i, j;
  CHAR	c;

  for (i = 0, j = strlen (s) - 1; i < j; i++, j--){
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  } /* endfor */

  return (0);
}

/*****************/
/* String N Copy */
/*****************/

VOID FAR _strncpy (NPSZ d, NPSZ s, USHORT n)
{
  while (*s && n--) *d++ = *s++;
  *d = 0;
}

/********************/
/* String N Overlay */
/********************/

static strnovl (NPSZ d, NPSZ s, USHORT n)
{
  while (*s && n--) *d++ = *s++;
  return (0);
}


/******************/
/* Char Duplicate */
/******************/

static chrdup (NPSZ d, UCHAR c, USHORT n)
{
  USHORT   i;

  while (n--) *d++ = c;
  *d = 0;

  return (0);
}

