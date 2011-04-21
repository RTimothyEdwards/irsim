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
#include <stdarg.h>

#include "globals.h"

#include "ana.h"
#include "ana_glob.h"
#include "graphics.h"


public Trptr    selectedTrace = NULL;


public	void    DeleteTrace(), SelectTrace(), MoveTraces();


/*
 * Return a pointer to the trace that contains y in its vertical position 
 */
public Trptr GetYTrace( y )
  register Coord  y;
  {
    register Trptr  t;
    register int    i;

    if( y >= namesBox.bot or y <= namesBox.top )
	return( NULL );

    for( i = traces.disp, t = traces.first; i != 0; i--, t = t->next )
      {
	if( y <= t->bot )
	  {
	    return( t );
	  }
      }
    return( NULL );
  }


/*
 * Underline the name of a trace, usually the selected trace.
 */
public void UnderlineTrace( t, color )
  Trptr  t;
  GC     color;
  {
    if( t == NULL )
      return;
    HLine( window, namesBox.right - 1 - t->len * CHARWIDTH, namesBox.right - 2,
      (t->top + t->bot + CHARHEIGHT)/2 + 1, color );
  }


/*
 * Allow the user to select a trace and move it to another position.  If the
 * the new position is outside the traces height then delete that trace.
 * If the trace is not moved, then it becomes the selected trace.
 */
public void MoveTrace( y )
  Coord  y;
  {
    BBox         b;
    XEvent       ev;
    Trptr        old, new, select;
    static char  delStr[] = "delete";

    if( traces.disp == 0 )
	return;

    if( (select = GetYTrace( y )) == NULL )
      {
	XBell( display, 0 );
	return;
      }
    old = select;

    b.top = select->top - 1;
    b.right = namesBox.right - 1;
    b.bot = select->bot + 1;
    b.left = namesBox.left + 1;
    
    y = (select->bot + select->top + CHARHEIGHT) / 2;
    FillAREA( window, b.left + 1, select->top,
      b.right - b.left - 1, select->bot - select->top + 1, gcs.black );
    StrRight( window, select->name, select->len, namesBox.right - 2, y,
     gcs.white );
    OutlineBox( window, b, gcs.black );

    GrabMouse( window, ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
     None );

    do
      {
	XNextEvent( display, &ev );
	if( ev.type == MotionNotify )
	    new = GetYTrace( ev.xmotion.y );
	else if( ev.type == ButtonRelease )
	    new = GetYTrace( ev.xbutton.y );
	else
	    continue;

	if( old != new )
	  {
	    if( old )
		OutlineBox( window, b, gcs.white );
	    else
		FillAREA( window, 1, timesBox.bot - CHARHEIGHT,
		 6 * CHARWIDTH, CHARHEIGHT+2, gcs.white );

	    if( new )
	      {
		b.top = new->top - 1;
		b.bot = new->bot + 1;
		OutlineBox( window, b, gcs.black );
	      }
	    else
		StrLeft( window, delStr, 6, 1, timesBox.bot, gcs.white );

	    old = new;
	  }
      }
     while( ev.type != ButtonRelease );

    XUngrabPointer( display, CurrentTime );
    XFlush( display );

    if( old )
	OutlineBox( window, b, gcs.white );
    else
	FillAREA( window, 1, timesBox.bot - CHARHEIGHT, 6 * CHARWIDTH,
	 CHARHEIGHT, gcs.white );

    FillAREA( window, b.left + 1, select->top, b.right - b.left - 1,
     select->bot - select->top + 1, gcs.white );
    StrRight( window, select->name, select->len, namesBox.right - 2, y,
     gcs.black );

    if( select == old )
	SelectTrace( select );
    else if( old )
      {
	MoveTraces( select, old );
	UnderlineTrace( selectedTrace, gcs.black );  /* select box erased it */
      }
    else 
	DeleteTrace( select );
  }


/*
 * Delete a trace from the analyzer.  Remove it from the list, dispose of the
 * memory allocated to it, and redraw the required parts of the window.
 */
public void DeleteTrace( t )
  Trptr  t;
  {
    int  change;

    traces.total--;
    if( t == traces.first )
      {
	traces.first = t->next;
	if( t->next )
	    t->next->prev = NULL;
	else
	    traces.last = NULL;
      }
    else
      {
	t->prev->next = t->next;
	if( t->next )
	    t->next->prev = t->prev;
	else
	    traces.last = t->prev;
      }
    
    if( selectedTrace == t )
	selectedTrace = NULL;
    Vfree( t );
    traces.disp--;
    change = WindowChanges();

    if( change & RESIZED )
	return;

    if( not (change & NTRACE_CHANGE) )		/* no trace became visible */
	SetSignalPos();

    if( change & WIDTH_CHANGE )
      {
	DrawScrollBar( FALSE );
	RedrawTimes();
      }

    RedrawNames( namesBox );
    DrawCursVal( cursorBox );
    DrawTraces( tims.start, tims.end );
  }


/*
 * Print out information regarding the selected trace.
 */
public void SelectTrace( t )
  Trptr  t;
  {
    if( t->vector )
      {
	if( t->n.vec->nbits > 1 )
	  {
	    PRINT( "\nvector: " );
	    PRINT( t->n.vec->name );
	    PRINTF( " bits=%d  base=%d", t->n.vec->nbits, (1 << t->bdigit) );
	  }
	else
	  {
	    PRINT( "\nalias: " );
	    PRINT( t->n.vec->nodes[0]->nname );
	  }
      }
    else
      {
	PRINT( "\nnode: " );
	PRINT( t->n.nd->nname );
      }

    if( selectedTrace )
	UnderlineTrace( selectedTrace, gcs.white );

    UnderlineTrace( t, gcs.black );
    selectedTrace = t;
  }


/*
 * Move trace 'from' to the position ocupied by trace 'to'.
 */
public void MoveTraces( from, to )
  Trptr  from, to;
  {
    Trptr  tmp;
    BBox   rb;

    if( to->next == from )		/* this simplifies the tests below */
      {
	tmp = to; to = from; from = tmp;
      }

    rb.top = min( to->top, from->top );
    rb.bot = max( to->bot, from->bot ) + 2;
    if( from->next == to )		/* adjacent traces, swap them */
      {
	from->next = to->next;
	to->next = from;
	to->prev = from->prev;
	from->prev = to;

	if( from->next )
	    from->next->prev = from;
	else
	    traces.last = from;

	if( to->prev )
	    to->prev->next = to;
	else
	    traces.first = to;
      }
    else
      {
	if( from->prev )
	    from->prev->next = from->next;
	else
	    traces.first = from->next;

	if( from->next )
	    from->next->prev = from->prev;
	else
	    traces.last = from->prev;

	if( from->top > to->top )	/* moving upwards */
	  {				/* insert it before 'to' */
	    from->next = to;
	    from->prev = to->prev;
	    if( to->prev )
		to->prev->next = from;
	    else
		traces.first = from;
	    to->prev = from;
	  }
	else				/* moving downwards */
	  {				/* insert it after 'to' */
	    from->next = to->next;
	    from->prev = to;
	    to->next = from;
	    if( from->next )
		from->next->prev = from;
	    else
		traces.last = from;
	  }
      }

    SetSignalPos();
    rb.left = 0;
    rb.right = XWINDOWSIZE;
    RedrawNames( rb );
    DrawCursVal( rb );
    rb.left = traceBox.left;
    rb.right = traceBox.right;
    RedrawTraces( &rb );
  }


/*
 * Allow the user to select one of the cursor values (on the right side) and
 * expand it on the text window.  This is useful for buses displayed on a
 * base other that binary, as well as to find out the input status.
 */
public void SelectCursTrace( y )
  Coord  y;
  {
    BBox    b;
    XEvent  ev;
    Trptr   new, select;

    if( traces.disp == 0 or tims.cursor < tims.first or
      tims.cursor > tims.last )
	return;

    b.left = cursorBox.left + 1;
    b.right = cursorBox.right - 1;

    if( (select = GetYTrace( y )) )
      {
	b.top = select->top - 1;
	b.bot = select->bot + 1;
	OutlineBox( window, b, gcs.black );
      }

    GrabMouse( window, ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
     None );

    do
      {
	XNextEvent( display, &ev );
	if( ev.type == MotionNotify )
	    new = GetYTrace( ev.xmotion.y );
	else if( ev.type == ButtonRelease )
	    new = GetYTrace( ev.xbutton.y );
	else
	    continue;

	if( select != new )
	  {
	    if( select )
		OutlineBox( window, b, gcs.white );

	    if( new )
	      {
		b.top = new->top - 1;
		b.bot = new->bot + 1;
		OutlineBox( window, b, gcs.black );
	      }
	    select = new;
	  }
      }
    while( ev.type != ButtonRelease );

    XUngrabPointer( display, CurrentTime );
    XFlush( display );

    if( select )
      {
	OutlineBox( window, b, gcs.white );
	ExpandCursVal( select );
      }
  }
