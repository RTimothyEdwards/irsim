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
 * front-end to the more general 'access' facility.
 */

#include <stdio.h>
#include <unistd.h>	/* defines access() */
#include <fcntl.h>
#include <sys/types.h>

#include "defs.h"

public	typedef	struct
  {
    char  exist;
    char  read;
    char  write;
  } Fstat;


public Fstat *FileStatus( name )
  char   *name;
  {
    static Fstat   ret;
    char           dir[ 256 ];
    register char  *s, *q, *p;
    
    if( access( name, R_OK ) == 0 )
	ret.read = 1;		    	/* file exists and is readable */
    else
	ret.read = 0;

    if( access( name, W_OK ) == 0 )	/* file exists and is writeable */
      {
	ret.exist = 1;
	ret.write = 1;
	return( &ret );
      }

    if( access( name, F_OK ) == 0 )	/* file exists but isn't writeable */
      {
	ret.exist = 1;
	ret.write = 0;
	return( &ret );
      }
					/* file doesn't exist, check dir. */
    p = name;
    for( s = p; *s != '\0'; s++ );
    while( s > p and *s != '/' ) s--;
    if( *s == '/' ) s++;
    q = dir;
    while( p < s )
	*q++ = *p++;
    *q++ = '.';
    *q = '\0';
    if( access( dir, W_OK ) == 0 )
	ret.write = 1;				/*  dir. is writeable */
    else
	ret.write = 0;
    ret.exist = 0;
    return( &ret );
  }
