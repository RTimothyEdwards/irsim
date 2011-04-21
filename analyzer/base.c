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
#include "graphics.h"

/*------------------------------------------------------*/
/* Change the base of the indicated vector trace	*/
/*------------------------------------------------------*/

public void ChangeTraceBase(trace, base)
  Trptr  trace;
  char  *base;
{
    int   change;
    short bdigits;

    if (!trace) {
	PRINT("\nSelect a trace first!");
	XBell(display, 0);
	return;
    }

    switch (tolower(*base)) {
	case 'b' :
	    bdigits = 1;
	    break;
	case 'q' :
	    bdigits = 2;
	    break;
	case 'o' :
	    bdigits = 3;
	    break;
	case 'h' :
	    bdigits = 4;
	    break;
	case 'd' :		/* special cases: unsigned & signed decimal */
	    bdigits = 5;
	    break;
	case 's' :
	    bdigits = 6;
	    break;
	default:
	    PRINT("\nUnknown base type!");
	    return;
    }

    if (IsVector(trace) && trace->bdigit != bdigits) {
	trace->bdigit = bdigits;
	change = WindowChanges();
	if (change & RESIZED)
	    return;		/* will get ExposeWindow event later */

	if (change & WIDTH_CHANGE) {	/* reshape the trace window */
	    DrawScrollBar(FALSE);
	    RedrawTimes();
	    DrawCursVal(cursorBox);
	    DrawTraces(tims.start, tims.end);
	}
	else {
	    BBox  rb;

	    rb.top = trace->top;
	    rb.bot = trace->bot;
	    rb.left = cursorBox.left;
	    rb.right = cursorBox.right;
	    DrawCursVal(rb);
	    rb.left = traceBox.left;
	    rb.right = traceBox.right;
	    RedrawTraces(&rb);		/* just redraw this trace */
	}
    }
}

/*-----------------------------------------------------------------------*/
/* This is the original function usage---acts on the selected trace only */
/*-----------------------------------------------------------------------*/

public void ChangeBase(base)
    char *base;
{
    ChangeTraceBase(selectedTrace, base);
}
