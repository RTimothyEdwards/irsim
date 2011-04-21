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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>   /* for strlen() */
#include <sys/types.h>
#include <pwd.h>

#include "defs.h"

public	char    *cad_lib;
public	char    *cad_bin;

extern	char           *getenv();
extern	struct passwd  *getpwnam();
extern	char           *Valloc();


public void InitCAD()
{
    char           *s;
    struct passwd  *pwd;
    int            len;

    /* Try CAD_ROOT environment variable */

    if (s = getenv("CAD_ROOT"))
    {
	if (!access(s, F_OK) == 0)
	    s = CAD_DIR;
    }

    /* Default is taken from CAD_DIR definition at compile time */

    else
       s = CAD_DIR;

    len = strlen( s );
    cad_lib = Valloc( len + 1, 1 );
    (void) sprintf( cad_lib, "%s", s );

    len = strlen( BIN_DIR );
    cad_bin = Valloc( len + 1, 1 );
    (void) sprintf( cad_bin, "%s", BIN_DIR );
}
