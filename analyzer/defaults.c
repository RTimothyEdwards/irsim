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

#ifdef LINUX
#include <string.h>
#endif
#include "ana.h"
#include "defs.h"
#include "ana_glob.h"

public
#define	DEFL_GEOM	0
public
#define	DEFL_BG		1
public
#define	DEFL_FG		2
public
#define	DEFL_RV		3
public
#define	DEFL_HIGL	4
public
#define	DEFL_TRCOLOR	5
public
#define	DEFL_BANN_BG	6
public
#define	DEFL_BANN_FG	7
public
#define	DEFL_BDRCOLOR	8
public
#define	DEFL_FONT	9
public
#define	DEFL_BDRWIDTH	10


typedef struct
  {
    char  *name;
    char  *defl;
  } Assoc;


private	Assoc assoc[] =
  {
    { "geometry",	"=1000x300+0+0" },
    { "background",	"black" },
    { "foreground",	"white" },
    { "reverseVideo",	"off" },
    { "highlight",	"red" },
    { "traceColor",	"white" },
    { "bannerBg",	"white" },
    { "bannerFg",	"black" },
    { "borderColor",	"black" },
    { "font",		"6x13" },
    { "borderWidth",	"2" },
  };


private	char  *irsim = "irsim";

public char *GetXDefault( key )
  int  key;
  {
    char         *val;

    val = XGetDefault( display, irsim, assoc[ key ].name );
    return( (val != NULL) ? val : assoc[ key ].defl );
  }


public int IsDefault( key, val )
  int   key;
  char  *val;
  {
    if( assoc[ key ].defl == val )
	return( TRUE );
    return( (strcmp( assoc[ key ].defl, val ) == 0) ? TRUE : FALSE );
  }


public char *ProgDefault( key )
  {
    return( assoc[ key ].defl );
  }
