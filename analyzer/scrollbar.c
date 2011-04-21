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
 * Scrollbar manager.
 */
#include "ana.h"
#include "ana_glob.h"
#include "graphics.h"

public
#define	SCROLLBARHEIGHT		16
public
#define	ARROW_WIDTH		32


private	void    StretchLeft(), StretchRight(), MoveBar();
private	void    MoveLeft(), MoveRight();


public void DrawScrollBar( isExpose )
  int  isExpose;
  {
    static Coord  lastLeft = 0;
    static Coord  lastRight = 0;

    if( isExpose or (scrollBox.left > lastLeft) )
	FillAREA( window, 0, scrollBox.top, scrollBox.left, SCROLLBARHEIGHT,
	 gcs.white );
    if( isExpose or (scrollBox.right < lastRight) )
	FillAREA( window, scrollBox.right, scrollBox.top,
	 XWINDOWSIZE - scrollBox.right, SCROLLBARHEIGHT, gcs.white );
    HLine( window, textBox.left, textBox.right, textBox.top, gcs.black );
    lastLeft = scrollBox.left;
    lastRight = scrollBox.right;
    XCopyArea( display, pix.left_arrows, window, gcs.black, 0, 0, ARROW_WIDTH,
     SCROLLBARHEIGHT, scrollBox.left, scrollBox.top );
    XCopyArea( display, pix.right_arrows, window, gcs.black, 0, 0, ARROW_WIDTH,
     SCROLLBARHEIGHT, scrollBox.right - ARROW_WIDTH, scrollBox.top );
    UpdateScrollBar();
  }


/*
 * Update and redraw the scrollbar.
 */
public void UpdateScrollBar()
  {
    float  tmp;

    if( tims.last == tims.first  )
      {
	barPos.left = traceBox.left;
	barPos.right = traceBox.right;
      }
    else
      {
        TimeType  maxTime;

	maxTime = max( tims.last, tims.end );
	tmp = (float)(traceBox.right - traceBox.left) / (maxTime-tims.first);
	barPos.left = traceBox.left + round( tmp * (tims.start-tims.first) );
	barPos.right = barPos.left + round( tmp * tims.steps );
	if( barPos.right - barPos.left < 3 )
	  {
	    if( barPos.left + 3 > traceBox.right )
		barPos.left = barPos.right - 3;
	    else
		barPos.right = barPos.left + 3;
	  }
      }
    FillAREA( window, traceBox.left + 1, scrollBox.top + 1,
      traceBox.right - traceBox.left - 1, SCROLLBARHEIGHT - 2, gcs.gray );
    VLine( window, traceBox.left, scrollBox.bot, scrollBox.top, gcs.black );
    VLine( window, traceBox.right, scrollBox.bot, scrollBox.top, gcs.black );

    FillBox( window, barPos, gcs.white );
    OutlineBox( window, barPos, gcs.black );
  }



/*
 * Handle button clicks within the scrollbar.
 */
public void DoScrollBar( ev )
  XButtonEvent  *ev;
  {
    TimeType  oldStart, oldSteps;

    if( (ev->x < scrollBox.left) or (ev->x > scrollBox.right) )
	return;

    if( ev->x < scrollBox.left + (ARROW_WIDTH / 2 - 1) )
	MoveLeft( 2 );
    else if( ev->x < traceBox.left )
	MoveLeft( 1 );
    else if( ev->x <= traceBox.right )
      {
	oldStart = tims.start;
	oldSteps = tims.steps;

	switch( ev->button & (Button1 | Button2 | Button3) )
	  {
	    case Button1 :
		StretchLeft( ev->x );
		break;
	    case Button2 :
		MoveBar( ev->x );
		break;
	    case Button3 :
		StretchRight( ev->x );
		break;
	  }
	UpdateScrollBar();
	if( (oldStart != tims.start) or (oldSteps != tims.steps) )
	  {
	    DrawTraces( tims.start, tims.end );
	    RedrawTimes();
	  }
      }
    else if( ev->x < scrollBox.right - (ARROW_WIDTH / 2 - 1) )
      {
	MoveRight( 1 );
      }
    else		    /* ( x < scrol.right ) */
	MoveRight( 2 );
  }


/*
 * Stretch the scroll bar to the left, up to the current right side.
 */
private void StretchLeft( x )
  Coord  x;
  {
    Coord     xmax;
    XEvent    ev;
    float     tmp;
    TimeType  maxTime, ti;
    BBox      b;

    maxTime = max( tims.last, tims.end );
    if( maxTime <= tims.first )
	return;

    tmp = (float)(maxTime - tims.first) / (traceBox.right - traceBox.left);
    xmax = barPos.right - max( 3, round( 10.0 / tmp ) );

    b.top = barPos.top + 1;
    b.bot = barPos.bot - 1;
    b.left = barPos.left;
    b.right = barPos.right - 1;
    FillBox( window, b, gcs.gray );
    b.left = min( x, xmax );
    FillBox( window, b, gcs.white );

    GrabMouse( window, ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
     None );

    UpdateTimes( tims.start, tims.end );	/* initialize it */

    do
      {
	XNextEvent( display, &ev ); 
	if( ev.type == MotionNotify )
	    x = ev.xmotion.x;
	else if( ev.type == ButtonRelease )
	    x = ev.xbutton.x;
	else
	    continue;

	FillBox( window, b, gcs.gray );
	if( x < traceBox.left )
	    b.left = traceBox.left;
	else if( x > xmax )
	    b.left = xmax;
	else
	    b.left = x;
	FillBox( window, b, gcs.white );
	ti = tims.first + round( (b.left - traceBox.left) * tmp );
	UpdateTimes( ti, tims.end );
      }
    while( ev.type != ButtonRelease );

    XUngrabPointer( display, CurrentTime );
    XFlush( display );

    barPos.left = b.left;
    tims.start = tims.first + round( (b.left - traceBox.left) * tmp );
    tims.steps = tims.end - tims.start;
  }



/*
 * Stretch the scroll bar to the right, up to the current left side.
 */
private void StretchRight( x )
  Coord  x;
  {
    Coord     xmin;
    XEvent    ev;
    float     tmp;
    TimeType  maxTime, ti;
    BBox      b;

    maxTime = max( tims.last, tims.end );
    if( maxTime <= tims.first )
	return;

    tmp = (float)(maxTime - tims.first) / (traceBox.right - traceBox.left);
    xmin = barPos.left + max( 3, round( 10.0/tmp ) );

    b.top = barPos.top + 1;
    b.bot = barPos.bot - 1;
    b.left = barPos.left + 1;
    b.right = barPos.right;
    FillBox( window, b, gcs.gray );
    b.right = max( x, xmin );
    FillBox( window, b, gcs.white );

    GrabMouse( window, ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
     None );

    UpdateTimes( tims.start, tims.end );	/* initialize it */

    do
      {
	XNextEvent( display, &ev );
	if( ev.type == MotionNotify )
	    x = ev.xmotion.x;
	else if( ev.type == ButtonRelease )
	    x = ev.xbutton.x;
	else
	    continue;

	FillBox( window, b, gcs.gray );
	if( x < xmin )
	    b.right = xmin;
	else if( x > traceBox.right )
	    b.right = traceBox.right;
	else
	    b.right = x;
	FillBox( window, b, gcs.white );
	ti = tims.first + round( (b.right - traceBox.left) * tmp );
	UpdateTimes( tims.start, ti );
      }
    while( ev.type != ButtonRelease );

    XUngrabPointer( display, CurrentTime );
    XFlush( display );

    barPos.right = b.right;
    tims.end = tims.first + round( (b.right - traceBox.left) * tmp );
    tims.steps = tims.end - tims.start;
  }


/*
 * Move the scrollbar left or right, always within the scrollbar bounds.
 */
private void MoveBar( x )
  Coord     x;
  {
    XEvent    ev;
    Coord     width, handle;
    Coord     top, height;
    float     tmp;
    TimeType  maxTime, t1, t2;

    maxTime = max( tims.last, tims.end );
    if( maxTime <= tims.first )
	return;

    tmp = (float)(maxTime - tims.first) / (traceBox.right - traceBox.left);

    top = barPos.top + 1;
    height = barPos.bot - barPos.top - 1;	/* bot - top - 1 - 1 + 1 */
    width = barPos.right - barPos.left + 1;

    if( x >= barPos.left and x <= barPos.right )
	handle = x - barPos.left;
    else if( (x > barPos.right) and (x + width - 1 > traceBox.right) )
	handle = x + width - traceBox.right - 1;
    else
	handle = 0;

    FillAREA( window, barPos.left, top, width, height, gcs.gray );
    FillAREA( window, x - handle, top, width, height, gcs.white );

    GrabMouse( window, ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
     None );

    UpdateTimes( tims.start, tims.end );	/* initialize it */

    do
      {
	XNextEvent( display, &ev );
	if( ev.type != MotionNotify and ev.type != ButtonRelease )
	    continue;

	FillAREA( window, x - handle, top, width, height, gcs.gray );
	x = (ev.type == MotionNotify) ? ev.xmotion.x : ev.xbutton.x;
	if( x - handle + width - 1 > traceBox.right )
	  {
	    if( x > traceBox.right )
		x = traceBox.right;
	    handle = x - (traceBox.right - width + 1);
	  }
	else if( x - handle < traceBox.left )
	  {
	    if( x < traceBox.left )
		x = traceBox.left;
	    handle = x - traceBox.left;
	  }
	FillAREA( window, x - handle, top, width, height, gcs.white );
	t1 = round( (x - handle - traceBox.left) * tmp ) + tims.first;
	t2 = t1 + tims.steps;
	UpdateTimes( t1, t2 );
      }
    while( ev.type != ButtonRelease );

    XUngrabPointer( display, CurrentTime );
    XFlush( display );

    barPos.left = x - handle;
    barPos.right = x - handle + width - 1;

    tims.start = round( (barPos.left - traceBox.left) * tmp ) + tims.first;
    tims.end = tims.start + tims.steps;
  }


/*
 * Scroll left by 'hpages' half-pages.
 */
private void MoveLeft( hpages )
  int  hpages;
  {
    TimeType  start;

    start = tims.start - (hpages * tims.steps) / 2;

    if( start < tims.first )
	start = tims.first;

    if( start == tims.start )
	return;

    tims.start = start;
    tims.end = start + tims.steps;

    RedrawTimes();
    UpdateScrollBar();
    DrawTraces( start, tims.end );
  }


/*
 * Scroll right by 'hpages' half-pages.
 */
private void MoveRight( hpages )
  int  hpages;
  {
    TimeType  start;

    start = tims.start + (hpages * tims.steps) / 2;

    if( (start >= tims.last) or (start == tims.start) or
      (start + tims.steps >= MAX_TIME ) )
	return;

    tims.start = start;
    tims.end = start + tims.steps;
    RedrawTimes();
    UpdateScrollBar();
    DrawTraces( start, tims.end );
  }
