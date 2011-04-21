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
#include <stdlib.h>	/* for atoi()	*/
#include <string.h>	/* for strlen()	*/

#include "ana.h"
#include "ana_glob.h"

public	int  max_name_len = 15;


public void GetNameLen( str )
  char  *str;
  {
    Trptr  t;
    int    len, change, wasTooSmall;

    if( str == NULL )
      {
	XBell( display, 0 );
	return;
      }
    len = atoi( str );
    if( len < 8 or len > 256 )
      {
	XBell( display, 0 );
	return;
      }

    if( len == max_name_len )
	return;

    max_name_len = len;

    DisableInput();

    for( t = traces.first; t != NULL; t = t->next )
      {
	if( t->vector )
	    t->name = SetName( t->n.vec->name );
	else
	    t->name = SetName( t->n.nd->nname );
	t->len = strlen( t->name );
      }

    if( windowState.iconified )
      {
	EnableInput();
	return;
      }

    wasTooSmall = windowState.tooSmall;

    change = WindowChanges();
    if( windowState.tooSmall )
      {
	RedrawSmallW();
	EnableInput();
	return;
      }

    if( change & WIDTH_CHANGE )
      {
	if( wasTooSmall )
	  {
	    RedrawBanner();
	    RedrawText();
	    DrawCursVal( cursorBox );
	  }
	RedrawNames( namesBox );
	DrawScrollBar( wasTooSmall );
	RedrawTimes();
	DrawTraces( tims.start, tims.end );
      }

    EnableInput();
  }


public void SetNameLen( s )
  char  *s;				/* the menu string => ignore it */
  {
    PRINTF( "\nEnter max. length (%d) > ", max_name_len );
    Query( " > ", GetNameLen );
  }
