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


private TimeType FindPreviousEdge( t, tm, edgeH )
  Trptr              t;
  register TimeType  tm;
  hptr               *edgeH;
  {
    register hptr      h;
    register int       val;
    register TimeType  edgeT;

    edgeT = tims.start;
    *edgeH = NULL;
    if( IsVector( t ) )
      {
	register int  i;

	for( i = t->n.vec->nbits - 1; i >= 0; i-- )
	  {
	    h = t->cache[i].wind;
	    val = h->val;
	    while( h->time <= tm )
	      {
		if( val != h->val )
		  {
		    val = h->val;
		    if( h->time > edgeT )
			edgeT = h->time;
		  }
		NEXTH( h, h );
	      }
	  }
      }
    else
      {
	h = t->cache[0].wind;
	val = h->val;
	while( h->time <= tm )
	  {
	    if( val != h->val )
	      {
		val = h->val;
		edgeT = h->time;
		*edgeH = h;
	      }
	    NEXTH( h, h );
	  }
      }
    return( edgeT );
  }


private TimeType FindNextEdge( t, tm, edgeH )
  Trptr              t;
  register TimeType  tm;
  hptr               *edgeH;
  {
    register hptr      h, p;
    register int       val;
    register TimeType  edgeT, endT;

    endT = edgeT = min( tims.end, tims.last );
    *edgeH = NULL;
    if( IsVector( t ) )
      {
	register int  i;

	for( i = t->n.vec->nbits - 1; i >= 0; i-- )
	  {
	    p = h = t->cache[i].wind;
	    while( h->time <= tm )
	      {
		p = h;
		NEXTH( h, h );
	      }
	    val = p->val;
	    while( h->time <= endT )
	      {
		if( val != h->val )
		  {
		    if( h->time < edgeT )
			edgeT = h->time;
		    break;
		  }
		NEXTH( h, h );
	      }
	  }
      }
    else
      {
	p = h = t->cache[0].wind;
	while( h->time <= tm )
	  {
	    p = h;
	    NEXTH( h, h );
	  }
	val = p->val;
	while( h->time <= endT )
	  {
	    if( val != h->val )
	      {
		edgeT = h->time;
		*edgeH = h;
		break;
	      }
	    NEXTH( h, h );
	  }
      }
    return( edgeT );
  }


private void WaitForRelease()
  {
    XEvent  e;
    
    GrabMouse( window, ButtonPressMask | ButtonReleaseMask, None );

    do
	XNextEvent( display, &e );
    while( e.type != ButtonRelease );

    XUngrabPointer( display, CurrentTime );
  }


private	Trptr       t1;
private Coord       x1;
private TimeType    time1;
private void        SetEdge2();


private void Terminate( cancel )
  int  cancel;
  {
    if( cancel )
	PRINT( "(canceled: click on a trace)" );
    SendEventTo( NULL );
    XDefineCursor( display, window, cursors.deflt );
    RestoreScroll();
  }


private void SetEdge1( ev )
  XButtonEvent  *ev;
  {
    TimeType  tm;
    hptr      h;

    if( ev == NULL )
      {
	Terminate( FALSE );
	return;
      }

    if( ev->type != ButtonPress )
	return;
    
    t1 = GetYTrace( ev->y );
    tm = XToTime( ev->x );
    if( t1 == NULL or tm < 0 )
      {
	Terminate( TRUE );
	return;
      }
    time1 = FindPreviousEdge( t1, tm, &h );
    x1 = TimeToX( time1 );
    PRINTF( "%.2f", d2ns( time1 ) );
    if( h != NULL )
	PRINTF( " [%.2f, %.2f]", d2ns( h->t.r.delay ), d2ns( h->t.r.rtime ) );

    FillAREA( window, x1 - 1, t1->top, 3, t1->bot - t1->top + 1, gcs.hilite );
    WaitForRelease();
    FillAREA( window, x1 - 1, t1->top, 3, t1->bot - t1->top + 1, gcs.unhilite);

    PRINT( " | t2 = " );
    XDefineCursor( display, window, cursors.right );
    SendEventTo( SetEdge2 );
  }


private void SetEdge2( ev )
  XKeyEvent  *ev;
  {
    TimeType  time2, diff;
    Trptr     t2;
    Coord     x2, y1, y2;
    hptr      h;

    if( ev == NULL )
      {
	Terminate( FALSE );
	return;
      }

    if( ev->type != ButtonPress )
	return;

    t2 = GetYTrace( ev->y );
    time2 = XToTime( ev->x );
    if( t2 == NULL or time2 < 0 )
      {
	Terminate( TRUE );
	return;
      }
    time2 = FindNextEdge( t2, time2, &h );
    x2 = TimeToX( time2 );

    if( time2 < time1 )
	diff = time1 - time2;
    else
	diff = time2 - time1;

    PRINTF( "%.2f", d2ns( time2 ) );
    if( h != NULL )
	PRINTF( " [%.2f, %.2f]", d2ns( h->t.r.delay ), d2ns( h->t.r.rtime ) );
    PRINTF( " | diff = %.2f", d2ns( diff ) );

    y1 = (t1->top + t1->bot) / 2;
    y2 = (t2->top + t2->bot) / 2;
    XDrawLine( display, window, gcs.hilite, x1, y1, x2, y2 );
    WaitForRelease();
    XDrawLine( display, window, gcs.unhilite, x1, y1, x2, y2 );

    Terminate( FALSE );
  }


public void DeltaT( s )
  char *s;			/* menu string => ignore it */
  {
    PRINT( "\nt1 = " );
    SendEventTo( SetEdge1 );
    XDefineCursor( display, window, cursors.left );
    DisableScroll();
  }
