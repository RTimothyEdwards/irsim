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
 * Calculate the Position/bounding-boxes of all the objects on the screen.
 */
#include <string.h>  /* for strlen() */
#include "ana.h"
#include "ana_glob.h"


#define	TRACEGAP	4			/* separation between traces */
#define	MAX_INCR	( 3*CHARWIDTH+TRACEGAP )   /* max. height of a trace */
#define	MINWIDTH	( 14 * CHARWIDTH )	   /* minimum traces width */
#define	MINHEIGHT	(7 * CHARHEIGHT)

#ifdef TCL_IRSIM
#define	BOX_SIZE	0
#else
#define	BOX_SIZE	13			/* size of banner icons */
#endif


        /* this is the same as traceBox.bot - traceBox.top + 1 */
#define TracesHeight() ( YWINDOWSIZE - SCROLLBARHEIGHT - 2 * CHARHEIGHT - 4 - \
  max( CHARHEIGHT + 3, BOX_SIZE + 6 - 1 ) )


public	int     XWINDOWSIZE = 0;
public	int     YWINDOWSIZE = 0;

public	BBox    traceBox;

/* Note:  These should be got rid of in the Tcl/Tk version. . . */
public	BBox    bannerBox;
public	BBox    scrollBox;
public	BBox    barPos;
public	BBox    namesBox;
public	BBox    cursorBox;
public	BBox    timesBox;
public	BBox    textBox;
public	BBox    iconBox;
public	BBox    sizeBox;
public	BBox    selectBox;
public	BBox    menuBox;

private	int     tw_left = 0;
private	int     tw_right = 0;

#ifdef TCL_IRSIM

private void SetWindowCoords()
{
    traceBox.left = 0;
    traceBox.bot = YWINDOWSIZE - 1;
    traceBox.right = XWINDOWSIZE - 1;
    traceBox.top = 0;
}

#else /* (!TCL_IRSIM) */

/*
 * Set the Bounding Box/Location of all the objects in the analyzer window.
 */
private void SetWindowCoords()
  {
    bannerBox.left = 0;
    bannerBox.top = 0;
    bannerBox.bot = max( bannerBox.top + CHARHEIGHT + 3, (BOX_SIZE + 6 - 1) );
    bannerBox.right = XWINDOWSIZE - 1;

    iconBox.top = bannerBox.top + 2;
    iconBox.bot = iconBox.top + BOX_SIZE - 1;
    iconBox.left = 2;
    iconBox.right = iconBox.left + BOX_SIZE - 1;

    sizeBox.top = iconBox.top;
    sizeBox.bot = iconBox.top;
    sizeBox.right = XWINDOWSIZE - 4;
    sizeBox.left = sizeBox.right - (BOX_SIZE - 1);

      {
	register Menu  *mp;
	Coord          x, x1;

        menuBox.bot = bannerBox.bot - 2;
        menuBox.top = menuBox.bot - CHARHEIGHT;
	for( mp = menu; mp->str != NULL; mp++ );

	menuBox.right = sizeBox.left - 3;
	for( mp--, x = menuBox.right + 1 ; mp >= menu; mp-- )
	  {
	    mp->box.top = menuBox.top;
	    mp->box.bot = menuBox.bot;
	    mp->box.right = x - 1;
	    mp->box.left = x - CHARWIDTH * (mp->len + 1);
	    x = mp->box.left - 3;
	  }
	menuBox.left = x + 1;
	selectBox.right = x - 4;
      }
    
    selectBox.top = iconBox.top;
    selectBox.bot = iconBox.bot;
    selectBox.left = iconBox.right + bannerLen * CHARWIDTH + 4 + 3;

    timesBox.left = tw_left - 1;
    timesBox.top = bannerBox.bot + 1;
    timesBox.right = tw_right + 1;
    timesBox.bot = timesBox.top + CHARHEIGHT + 2 - 1;

    textBox.left = 0;
    textBox.top = YWINDOWSIZE - CHARHEIGHT - 3;
    textBox.right = XWINDOWSIZE - 1;
    textBox.bot = YWINDOWSIZE - 1;

    namesBox.left = 0;
    namesBox.top = timesBox.bot + 1;
    namesBox.right = tw_left - 1;
    namesBox.bot = textBox.top - SCROLLBARHEIGHT;

    cursorBox.top = namesBox.top;
    cursorBox.right = XWINDOWSIZE - 1;
    cursorBox.left = tw_right + 1;
    cursorBox.bot = namesBox.bot;

    traceBox.left = tw_left;
    traceBox.bot = namesBox.bot + 1;
    traceBox.right = tw_right;
    traceBox.top = namesBox.top;

    scrollBox.left = traceBox.left - ARROW_WIDTH + 1;
    scrollBox.bot = textBox.top;
    scrollBox.right = traceBox.right + ARROW_WIDTH;
    scrollBox.top = scrollBox.bot - SCROLLBARHEIGHT + 1;

    barPos.bot = scrollBox.bot - (SCROLLBARHEIGHT / 2) + 2;
    barPos.top = barPos.bot - 7;
  }

#endif /* (!TCL_IRSIM) */
					/* char + 2 spaces + 2 lines */
#define	MINBUSHEIGHT		( CHARHEIGHT + 4 + TRACEGAP )
					/* a bit smaller that a char */
#define	MINSIGHEIGHT		( CHARHEIGHT + TRACEGAP - 3 )

/*
 * Calculate the number of traces that will fit (vertically).
 */
private int VisibleTraces()
  {
    register Trptr  t;
    register int    Ysize, n, i, busHeight, logicHeight;

    Ysize = TracesHeight();

    Ysize -= TRACEGAP;
    busHeight = MINBUSHEIGHT;
    logicHeight = MINSIGHEIGHT;

    for( i = 0, n = traces.total, t = traces.first; i < n; i++, t = t->next )
      {
	Ysize -= ( IsVector( t ) ) ? busHeight : logicHeight;
	if( Ysize < 0 )
	    return( i );
      }
    return( n );
  }


/*
 * Calculate vertical position of traces.
 */
public void SetSignalPos()
  {
    register Trptr    t;
    register int      i, pos;
    int               busHeight, logicHeight, incr, Ysize;

    if( traces.disp == 0 )
	return;

    Ysize = TracesHeight();
    pos = TRACEGAP;
    busHeight = MINBUSHEIGHT;
    logicHeight = MINSIGHEIGHT;

    for( i = traces.disp, t = traces.first; i != 0 ; i--, t = t->next )
	pos += ( IsVector( t ) ) ? busHeight : logicHeight;

    incr = (Ysize - pos) / traces.disp;
    if( incr > MAX_INCR )
	incr = MAX_INCR;

    busHeight += (incr - TRACEGAP);
    logicHeight += (incr - TRACEGAP);

    pos = traceBox.top + TRACEGAP;
    
    for( t = traces.first, i = traces.disp; i != 0; i--, t = t->next )
      {
        t->top = pos;
	pos += ( IsVector( t ) ) ? busHeight : logicHeight;
        t->bot = pos;
	pos += TRACEGAP;
      }
  }


/*
 * Return the maximum number of digits required to display a trace value.
 */
private int MaxTraceDigits( n )
  register int  n;
  {
    register int     ndigits, maxDigits;
    register Trptr   t;
    
    maxDigits = 1;
    for( t = traces.first; n != 0; n--, t = t->next )
      {
	if( IsVector( t ) )
	  {
	    if (t->bdigit == 5)
	       ndigits = (t->n.vec->nbits + 2) / 3;
	    else if (t->bdigit == 6)
	       ndigits = 1 + ((t->n.vec->nbits + 2) / 3);
	    else
	       ndigits = (t->n.vec->nbits + t->bdigit - 1) / t->bdigit;
	    if( ndigits > maxDigits )
		maxDigits = ndigits;
	  }
      }
    return( maxDigits );
  }


/*
 * Return the length of the longest trace name.
 */
private int MaxTraceName( n )
  register int  n;
  {
    register int      len, maxLen;
    register Trptr    t;
    
    maxLen = 0;
    for( t = traces.first; n != 0; n--, t = t->next )
      {
	len = strlen( t->name );
	if( len > maxLen )
	    maxLen = len;
      }
    return( maxLen );
  }


public void GetMinWsize( w, h )
  int  *w, *h;
  {
    int  maxDigits, maxName;

    maxDigits = MaxTraceDigits( traces.total );
    maxDigits = max( maxDigits, 16 );
    maxName = MaxTraceName( traces.total );
    maxName = max( maxName, 15 );

    *w = max( (CHARWIDTH * maxName) + 2, ARROW_WIDTH + 4 ) + 2 +
	 max( (CHARWIDTH * maxDigits) + 2, ARROW_WIDTH ) + 2 + MINWIDTH;
    *h = MINHEIGHT;
  }


public
#define	NTRACE_CHANGE		0x01	/* # of visible traces changed */
public
#define	WIDTH_CHANGE		0x02	/* change in width of trace window */
public
#define	HEIGHT_CHANGE		0x04	/* change in height of trace window */
public
#define	RESIZED			0x10	/* window is too small,should resize */

public int WindowChanges()
  {
    int         left, right, tooSmall;
    int         ndisp, maxDigits, maxName, ret;
    static int  lastY = -1;

    ndisp = VisibleTraces();
    maxDigits = MaxTraceDigits( ndisp );
    maxName = MaxTraceName( ndisp );

    left = max( (CHARWIDTH * maxName) + 2, ARROW_WIDTH + 4 ) + 2;
    right = XWINDOWSIZE - max( (CHARWIDTH * maxDigits) + 2, ARROW_WIDTH ) - 2;

    tooSmall = FALSE;
    if( right - left < MINWIDTH )
	tooSmall = TRUE;

    if( YWINDOWSIZE < MINHEIGHT )
	tooSmall = TRUE;

    if( tooSmall )
      {
	windowState.tooSmall = TRUE;
	lastY = 0;
	return( RESIZED );
      }

    windowState.tooSmall = FALSE;	/* could have been set before */

    ret = 0;
    if( ndisp != traces.disp )
      {
	int  last_disp = traces.disp;

	traces.disp = ndisp;
	ret |= NTRACE_CHANGE;
	if( ndisp > last_disp )
	    UpdateTraceCache( last_disp );
      }

    if( (left != tw_left or right != tw_right) )
      {
	tw_left = left;
	tw_right = right;
	ret |= WIDTH_CHANGE;
      }

    if( lastY != YWINDOWSIZE )
      {
	lastY = YWINDOWSIZE;
	ret |= HEIGHT_CHANGE;
      }

    if( ret & (HEIGHT_CHANGE | WIDTH_CHANGE) )
	SetWindowCoords();

    if( ret & (HEIGHT_CHANGE | NTRACE_CHANGE) )
	SetSignalPos();

    return( ret );
  }
