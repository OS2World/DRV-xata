/**************************************************************************
 *
 * SOURCE FILE NAME = TRACE.H
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for ST506/IDE DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : trace definitions
 ****************************************************************************/

#ifndef TRACE_H_INCLUDED

USHORT NEAR DiffTime (VOID);
VOID   NEAR TraceTime (VOID);
VOID   NEAR TraceCh (CHAR c);
VOID   FAR  TraceStr (NPSZ fmt, ...);
VOID   NEAR TraceTTime (VOID);
USHORT NEAR TotalTime (VOID);
VOID   NEAR TWrite (USHORT DbgLevel);
VOID   NEAR TErr (NPU npU, UCHAR x);

#define TRACE_H_INCLUDED 1
#endif

#define TSTR if(TRACES)TraceStr

#if TRACES

#undef TINIT
#undef T
#undef TS
#undef TTIME
#undef TWRITE
#undef TEND

#define TINIT npTrace=TraceBuffer;DiffTime();
#define T(x) TraceCh(x);
#define TS(x,y) TraceStr(x,y);
#define TTIME TraceTime();
#define TWRITE(x) TWrite(x);
#define TEND

#else // !TRACES

#undef TINIT
#undef T
#undef TS
#undef TTIME
#undef TWRITE
#undef TEND

#define TINIT
#define T(x)
#define TS(x,y)
#define TTIME
#define TWRITE(x)
#define TEND

#endif

#if TINYTRACER
#define TERR(u,x) TErr (u, x);
#else
#define TERR(u,x)
#endif

