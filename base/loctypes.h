/* 
 *---------------------------------------------------------------------
 * loctypes.h --- determine what to use for 4 and 8 byte integers.
 * Specifically, time units in IRSIM need to be 8 byte unsigned.
 *---------------------------------------------------------------------
 */

#ifndef _LOCTYPES_H
#define _LOCTYPES_H

/* Ulong is intended to be an 8-byte integer.  Systems which have type	*/
/* "long" defined as 4 bytes (e.g., Linux) should define Ulong as type	*/
/* "unsigned long long".						*/

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
  #ifdef HAVE_SYS_PARAM_H
  #include <sys/param.h>
  #endif
#endif
  
#if defined(LONG_MAX) && defined(INT_MAX)
  #if LONG_MAX == INT_MAX
  typedef long long dlong;
  typedef unsigned long long Ulong;
  #define strtoUlong strtoull
  #define PRINTF_LLONG "%ll"
  #else
  typedef long dlong;
  typedef unsigned long Ulong;
  #define strtoUlong strtoul
  #define PRINTF_LLONG "%l"
  #endif
#else
typedef long long dlong;
typedef unsigned long long Ulong;
#define strtoUlong strtoull
#define PRINTF_LLONG "%ll"
#endif

typedef unsigned int   Uint;
typedef unsigned char  Uchar;
typedef char  *uptr;
typedef int  (*ifun)();
typedef void (*vfun)();
typedef	uptr (*ufun)();

#if SIZEOF_VOID_P == SIZEOF_UNSIGNED_INT
typedef Uint pointertype;
#elif SIZEOF_VOID_P == SIZEOF_UNSIGNED_LONG
typedef Ulong pointertype;
#else
ERROR: Cannot compile without knowing the size of a pointer.  See base/net.h
#endif


#endif /* _LOCTYPES_H */
