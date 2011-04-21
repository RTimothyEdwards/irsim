/* 
 *     ********************************************************************* 
 *     * Copyright (C) 1988, 1990 Stanford University.                     * 
 *     * Permission to use, copy, modify, and distribute this              * 
 *     * software and its documentation for any purpose and without        * 
 *     * fee is hereby granted, provided that the above copyright          * 
 *     * notice appear in all copies.  Stanford University                 * 
 *     * makes no representations about the suitability of this            * 
 *     * software for any purpose.  It is provided "as is" without         * 
 *     * express or implied warranty.  Export of this software outside     * 
 *     * of the United States of America may require an export license.    * 
 *     ********************************************************************* 
 */

/*
 * Prints various messages but deals with varargs correctly.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "defs.h"

extern	FILE  *logfile;


public void logprint( s )
  register char  *s;
  {
    static int  docomment = 1;

    while( *s != '\0' )
      {
	if( docomment )
	  {
	    (void) putc( '|', logfile );
	    (void) putc( ' ', logfile );
	    docomment = 0;
	  }
	(void) putc( *s, logfile );
	if( *s++ == '\n' )
	    docomment = 1;
      }
  }

#ifndef TCL_IRSIM

/* VARARGS */

public void lprintf(FILE *max, ...)
  {
    va_list  args;
    char     *fmt;
    char     buff[300];

    va_start(args, max);

    fmt = va_arg(args, char *);
    (void) vsprintf(buff, fmt, args);
    va_end(args);
    (void) fputs(buff, max);

    if (logfile != NULL)
	logprint(buff);
  }

/* VARARGS */

public void rsimerror(char *filename, ...)
  {
    va_list  args;
    int      lineno;
    char     *fmt;
    char     buf1[ 100 ], buf2[ 200 ];

    va_start(args, filename);
    lineno = va_arg(args, int);
    fmt = va_arg(args, char *);
    (void) sprintf(buf1, "(%s,%d): ", filename, lineno);
    (void) vsprintf(buf2, fmt, args);
    va_end(args);

    (void) fputs(buf1, stderr);
    (void) fputs(buf2, stderr);
    if(logfile != NULL)
      {
	logprint(buf1);
	logprint(buf2);
      }
  }

#endif  /* TCL_IRSIM */
