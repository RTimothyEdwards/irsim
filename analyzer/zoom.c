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

#include "ana.h"
#include "ana_glob.h"


public void Zoom( what )
  char  *what;
  {
    TimeType  start;
    int       steps;

    switch( *what )
      {
	case 'i' :				/* zoom in */
	    steps = tims.steps / 2;
	    if( steps < 10 )
		steps = 10;
	    start = tims.start + steps / 2;
	    if( start > tims.last )
	      {
		start = tims.last - steps / 2;
		if( start < tims.first )
		    start = tims.first;
	      }
	    break;

	case 'o' :				/* zoom out */
	    steps = tims.steps * 2;
	    if (tims.start < tims.steps / 2)
		start = 0;
	    else
		start = tims.start - tims.steps / 2;
	    if( start < tims.first )
		start = tims.first;
	    if( steps > MAX_TIME or (start + steps) > MAX_TIME )
		return;
	    break;
      }

    if( tims.steps != steps )
      {
	tims.start = start;
	tims.steps = steps;
	tims.end = start + steps;
	RedrawTimes();
	UpdateScrollBar();
 	DrawTraces( start, tims.end );
     }
  }
