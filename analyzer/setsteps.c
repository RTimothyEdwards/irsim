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

#include "ana.h"
#include "ana_glob.h"


private void GetWidth( str )
  char  *str;
  {
    TimeType  steps;
    double    tmp;

    if( str == NULL )
      {
	XBell( display, 0 );
	return;
      }
    tmp = atof( str );
    steps = ns2d( tmp );
    if( steps < 10 or (tims.start + steps) > MAX_TIME )
      {
	XBell( display, 0 );
	return;
      }

    tims.steps = steps;
    tims.end = tims.start + steps;
    RedrawTimes();
    UpdateScrollBar();
    DrawTraces( tims.start, tims.end );
  }


public void SetWidth( s )
   char  *s;				/* the menu string => ignore it */
  {
    Query( "\nEnter Time Steps > ", GetWidth );
  }

#ifdef TCL_IRSIM

public void SetTimeLeft(TimeType tx)
{
    tims.start = tx;
    if (tims.start >= tims.end)
	tims.start = tims.end - 1;
    if (tims.start < tims.first)
	tims.start = tims.first;
    tims.steps = tims.end - tims.start;
    RedrawTimes();
    DrawTraces(tims.start, tims.end);
}

public void SetTimeRight(TimeType tx)
{
    tims.end = tx;
    if (tims.end < tims.start)
	tims.end = tims.start + 1;
    tims.steps = tims.end - tims.start;
    RedrawTimes();
    DrawTraces(tims.start, tims.end);
}

#endif
