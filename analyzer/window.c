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
 * Window Manager for logic analyzer
 *
 */
#include <stdio.h>
#include <string.h>   /* for strlen() */
#include "ana.h"
#include <X11/Xutil.h>
#include "graphics.h"
#include "ana_glob.h"
#include "ana_tk.h"

public	Display     *display = NULL;
public	Screen      *screen;
public	Window      window = 0;
public	Window      iconW = 0;

public	int         CHARHEIGHT = 0;
public	int         CHARWIDTH = 0;
public	int	    descent;

public	Times       tims;
public	Traces      traces;
public	Wstate      windowState = { FALSE, FALSE, FALSE };

private	char        *wname = "analyzer";
public	char        *banner;
public	int         bannerLen;


private	void    DrawSignal(), DrawVector(), DrawCursor();
private	void    MoveCursorToPos(), EraseCursor();

/*
 * Convert a time to its corresponding x position.
 */

public Coord TimeToX(t)
    TimeType t;
{
    Coord xval;

    if (t >= TIME_BOUND) return 0;  /* Out-of-bounds time value */
    if (tims.steps == 0) return 0;  /* Avoid divide-by-0 */

    xval = ((t) - tims.start) * (traceBox.right - traceBox.left - 2)
		/ tims.steps + traceBox.left + 1;

    return xval;
}

/*
 * Convert an x position to the closest time-step.
 * Return (MAX_TIME) if the point lies outside the traces window.
 */

public TimeType XToTime(x)
    Coord  x;
{
    float  tmp;
    int denom;

    if ((x <= traceBox.left) || (x >= traceBox.right))
	return TIME_BOUND;

    denom = traceBox.right - traceBox.left - 2;
    if (denom == 0) return TIME_BOUND;	/* avoid divide-by-zero */

    tmp = (float)tims.steps / (float)denom;
    return (tims.start + Round((x - traceBox.left - 1) * tmp));
}


/*
 * return TRUE if the 2 bounding boxes intersect, otherwise FALSE
 */
#define Intersect( b1, b2 )						\
    ( ( (b1.top > b2.bot) or (b2.top > b1.bot) or			\
      (b1.left > b2.right) or (b2.left > b1.right ) ) ? FALSE : TRUE )

/*
 * Graphics initialization---load a font to use for drawing text
 * and set up all of the Graphics Contexts.
 */

public int SetFont()
{
    XFontStruct  *font;
    char  *fontname;

    if( CHARHEIGHT == 0 )
      {
	fontname = GetXDefault( DEFL_FONT );
	if( (font = XLoadQueryFont( display, fontname )) == NULL )
	  {
	    fprintf( stderr, "Could not load font `%s'", fontname );
	    if( not IsDefault( DEFL_FONT, fontname ) )
	      {
		fontname = ProgDefault( DEFL_FONT );
		if( (font = XLoadQueryFont( display, fontname )) == NULL )
		  {
		    fprintf( stderr, " or `%s'\n", fontname );
		    return( FALSE );
		  }
		else
		    fprintf( stderr, " using `%s' instead\n", fontname );
	      }
	    else
	      {
		fprintf( stderr, "\n" );
		return( FALSE );
	      }
	  }
	CHARHEIGHT = font->max_bounds.ascent + font->max_bounds.descent;
	CHARWIDTH = font->max_bounds.width;
	descent = font->max_bounds.descent;

	InitGraphics(font->fid);
      }
    return TRUE;
}

#ifndef TCL_IRSIM

/*
 * Initialize the windows and various other metrics.
 */

public int InitDisplay( fname, display_unit )
  char  *fname;
  char  *display_unit;
  {
#ifdef HAVE_PTHREADS
    XInitThreads();
#endif

    if( display == NULL )
      {
	if( (display = XOpenDisplay( display_unit )) == NULL )
	  {
	    fprintf( stderr, "could not open display\n" );
	    return( FALSE );
	  }
	screen = ScreenOfDisplay( display, DefaultScreen( display ) );
      }

    if (SetFont() == FALSE) return FALSE;
      
    banner = (fname != NULL and *fname != '\0') ? fname : wname;
    bannerLen = strlen( banner );

    if( iconW == 0 )
	iconW = CreateIconWindow( 10, 10 );

    if( window == 0 )
      {
	InitWindow( TRUE, NormalState, 0, 0, 0, 0, 10, 10 );
	InitMenus();
      }


    if( not InitHandler( ConnectionNumber( display ) ) )
	return( FALSE );

    return( TRUE );
  }


public int InitWindow( firstTime, state, x, y, w, h, ix, iy )
  int    firstTime;
  int    state;
  Coord  x, y, w, h;
  Coord  ix, iy;
  {
    int                   spec, u_spec;
    static int            b;
    char                  *geo;
    XSizeHints            shint;
    XWMHints              wmh;
    XSetWindowAttributes  att;
    XClassHint            class;

    if( firstTime )
      {
	unsigned int ww, wh;

	b = atoi( GetXDefault( DEFL_BDRWIDTH ) );
	if( b <= 0 )
	    b = atoi( ProgDefault( DEFL_BDRWIDTH ) );

	geo = ProgDefault( DEFL_GEOM );
	spec = XParseGeometry( geo, &x, &y, &ww, &wh );
	geo = GetXDefault( DEFL_GEOM );
	u_spec = IsDefault( DEFL_GEOM, geo ) ? FALSE : TRUE;
	if( u_spec )
	    spec = XParseGeometry( geo, &x, &y, &ww, &wh );

	w = ww;
	h = wh;

	if( (spec & (XValue | XNegative)) == (XValue | XNegative) )
	    x += WidthOfScreen( screen ) - w - 2 * b;

	if( (spec & (YValue | YNegative)) == (YValue | YNegative) )
	    y += HeightOfScreen( screen ) - h - 2 * b;

	XWINDOWSIZE = w;
	YWINDOWSIZE = h;
      }
    else
	u_spec = TRUE;

    att.background_pixel = colors.white;
    att.border_pixmap = pix.gray;
    att.backing_planes = colors.black | colors.white | colors.hilite |
     colors.traces | colors.banner_bg | colors.banner_fg;
    att.cursor = cursors.deflt;

    window = XCreateWindow( display, RootWindowOfScreen( screen ),
     x, y, w, h, b, DefaultDepthOfScreen( screen ), InputOutput,
     (Visual *) CopyFromParent,
     (CWBackPixel | CWBorderPixmap | CWBackingPlanes | CWCursor), &att );

    XStoreName( display, window, wname );
    XSetIconName( display, window, wname );
    class.res_name = "irsim";
    class.res_class = wname;
    XSetClassHint( display, window, &class );

    wmh.input = True;
    wmh.initial_state = state;
    wmh.icon_pixmap = pix.icon;
    wmh.icon_window = iconW;
    wmh.icon_x = ix;
    wmh.icon_y = iy;
    wmh.flags = InputHint | StateHint | IconPixmapHint | IconWindowHint |
     IconPositionHint;
    XSetWMHints( display, window, &wmh );

    shint.x = x;
    shint.y = y;
    shint.width = w;
    shint.height = h;
    shint.max_width = shint.max_height = 16000;    /* any big number */
    shint.width_inc = shint.height_inc = 1;
    GetMinWsize( &shint.min_width, &shint.min_height );
    shint.flags = ( u_spec ) ?
      ( PMinSize | PMaxSize | PResizeInc | USPosition | USSize ) :
      ( PMinSize | PMaxSize | PResizeInc | PPosition | PSize );
    XSetNormalHints( display, window, &shint );

    XSelectInput( display, window, ExposureMask | StructureNotifyMask |
     KeyPressMask | ButtonPressMask | EnterWindowMask | LeaveWindowMask );
    XFlush( display );

#ifdef HAVE_PTHREADS
    xloop_create();
#endif

  }

#endif /* TCL_IRSIM */

public
#define	DEF_STEPS	4	/* default simulation steps per screen */

/*
 * Initialize the display times so that when first called the last time is
 * shown on the screen.  Default width is DEF_STEPS (simulation) steps.
 */

public void InitTimes(firstT, stepsize, lastT, reInit)
  TimeType  firstT, stepsize, lastT;
  int reInit;
{
    tims.first = firstT;
    tims.last = lastT;
    tims.steps = 4 * stepsize;

    if (reInit == 0)
    {
	/* Once-only initialization */
	tims.start = tims.first;
	tims.end = tims.start + tims.steps;
    }
    else if (autoScroll || tims.start <= tims.first)
    {
	if (lastT < tims.steps)
	{
	    tims.start = tims.first;
	    tims.end = tims.start + tims.steps;
	}
	else
	{
	    tims.end = lastT + 2 * stepsize;
	    tims.start = tims.end - tims.steps;
	    if (tims.start < tims.first)
	    {
		stepsize = tims.first - tims.start;
		tims.start += stepsize;
		tims.end += stepsize;
	    }
	}
    }
    tims.cursor = TIME_BOUND;
    tims.delta = TIME_BOUND;
}


/*
 * Redraw any region that overlaps the redraw-box.
 */
public void RedrawWindow( box )
  BBox  box;
  {

#ifndef TCL_IRSIM

    if( Intersect( bannerBox, box ) )
	RedrawBanner();
    if( Intersect( timesBox, box ) )
	RedrawTimes();
    if( Intersect( namesBox, box ) )
	RedrawNames( box );
    if( Intersect( scrollBox, box ) )
	DrawScrollBar( TRUE );
    if( Intersect( cursorBox, box ) )
	DrawCursVal( box );
    if( Intersect( textBox, box ) )
	RedrawText();

#endif

    if( Intersect( traceBox, box ) )
	RedrawTraces( &box );
  }


#ifndef TCL_IRSIM

/*
 * Displays the window banner.
 */
public void RedrawBanner()
  {
    Menu  *mp;

    FillBox( window, bannerBox, gcs.bannerBg );
    XCopyArea( display, pix.iconbox, window, gcs.bannerFg, 0, 0, 13, 13, 
     iconBox.left, iconBox.top );

    StrLeft( window, banner, bannerLen, iconBox.right + 4,
    (bannerBox.bot - 2 + CHARHEIGHT) / 2, gcs.bannerFg );

    if( windowState.selected )
      {
	if( selectBox.right > selectBox.left )
	    FillBox( window, selectBox, gcs.select );
	FillAREA( window, bannerBox.left, bannerBox.bot - 1, bannerBox.right -
	 bannerBox.left + 1, 2, gcs.border );
	XSetWindowBorder( display, window, colors.border );
      }
    else
      {
	FillAREA( window, bannerBox.left, bannerBox.bot - 1, bannerBox.right -
	 bannerBox.left + 1, 2, gcs.gray );
	XSetWindowBorderPixmap( display, window, pix.gray );
      }

    for( mp = menu; mp->str != NULL; mp++ )
      {
	StrCenter( window, mp->str, mp->len, mp->box.left, mp->box.right,
	 mp->box.bot, gcs.bannerFg );
      }

    XCopyArea( display, pix.sizebox, window, gcs.bannerFg, 0, 0, 13, 13, 
     sizeBox.left, sizeBox.top );
  }


public void WindowCrossed( selected )
  int  selected;
  {
    GC  color;

    if( selected == windowState.selected )
	return;

    windowState.selected = selected;
    
    if( selected )
	XSetWindowBorder( display, window, colors.border );
    else
	XSetWindowBorderPixmap( display, window, pix.gray );

    if( windowState.tooSmall )
	return;

    if( selectBox.right > selectBox.left )
      {
	if( selected )
	    FillBox( window, selectBox, gcs.select );
	else
	    FillBox( window, selectBox, gcs.bannerBg );
      }
    FillAREA( window, bannerBox.left, bannerBox.bot - 1, XWINDOWSIZE - 1, 2,
     selected ? gcs.border : gcs.gray );
  }

#endif /* (!TCL_IRSIM) */

public void RedrawSmallW()
  {
    static  char  *msg = "I'm too small";
    Coord   y;
    int     len;

    XClearWindow( display, window );
    len = strlen( msg );
    y = (YWINDOWSIZE - CHARHEIGHT) / 2;
    StrCenter( window, msg, len, 0, XWINDOWSIZE, y, gcs.black );
  }

#ifndef TCL_IRSIM

public void RedrawTimes()
  {
    char   s[ 20 ];
    int    len;
    Coord  x;

    FillAREA( window, 0, timesBox.top, XWINDOWSIZE,
     timesBox.bot - timesBox.top + 1, gcs.white );

    (void) sprintf( s, " %.2f ", d2ns( tims.start ) );
    len = strlen( s );
    StrLeft( window, s, len, timesBox.left, timesBox.bot, gcs.white );
    (void) sprintf( s, " %.2f ", d2ns( tims.end ) );
    len = strlen( s );
    StrRight( window, s, len, timesBox.right - 1, timesBox.bot, gcs.white );
    if( tims.cursor < TIME_BOUND)
	(void) sprintf( s, "%.2f", d2ns( tims.cursor ) );
    else
	(void) strcpy( s, "-" );

    len = strlen( s );
    StrCenter( window, s, len, timesBox.left, timesBox.right, timesBox.bot,
      gcs.black );
  }


public void UpdateTimes( start, end )
  TimeType  start, end;
  {
    static TimeType  ostart, oend;
    static int       slen, elen;
    int              len;
    char             s[20];

    if( start != ostart )
      {
	(void) sprintf( s, " %.2f ", d2ns( start ) );
	len = strlen( s );
	if( len < slen )
	    FillAREA( window, timesBox.left + (len * CHARWIDTH), 
	     timesBox.bot - CHARHEIGHT - 1, (slen - len) * CHARWIDTH,
	     timesBox.bot - timesBox.top + 1, gcs.white );
	StrLeft( window, s, len, timesBox.left, timesBox.bot, gcs.white );
	ostart = start;
	slen = len;
      }

    if( end != oend )
      {
	(void) sprintf( s, " %.2f ", d2ns( end ) );
	len = strlen( s );
	if( len < elen )
	    FillAREA( window, timesBox.right - (elen * CHARWIDTH),
	     timesBox.bot - CHARHEIGHT - 1, (elen - len) * CHARWIDTH,
	     timesBox.bot - timesBox.top + 1, gcs.white );
	StrRight( window, s, len, timesBox.right, timesBox.bot, gcs.white );
	oend = len;
	elen = len;
      }
  }

#endif /* (!TCL_IRSIM) */

/*
 * Redraw signal names.
 */
public void RedrawNames( rb )
  BBox  rb;
  {
    Coord  x, y;
    Trptr  t;
    int    i;

    rb.left = max( rb.left, namesBox.left );
    rb.right = min( rb.right, namesBox.right );
    rb.top = max( namesBox.top, rb.top );
    rb.bot = min( namesBox.bot, rb.bot );
    FillBox( window, rb, gcs.white );

    for( i = traces.disp, t = traces.first; i != 0; i--, t = t->next )
	if( rb.top <= t->bot )
	    break;

    x = namesBox.right - 2;
    while( i != 0 and rb.bot >= t->top )
      {
	y = (t->bot + t->top + CHARHEIGHT) / 2;
	StrRight( window, t->name, t->len, x, y, gcs.black );
	if( t == selectedTrace )
	    UnderlineTrace( t, gcs.black );
	i--;
	t = t->next;
      }
  }

#define	CursorVisible(T1, T2) \
		(tims.cursor != TIME_BOUND && \
		tims.cursor >= (T1) and tims.cursor <= (T2))

/*
 * This will redraw the missing parts of the traces.  Used to selectivelly
 * repaint traces or during Exposure events.
 */
public void RedrawTraces( box )
  BBox  *box;
  {
    TimeType        t1, t2, tc;
    BBox            bg;
    register Trptr  t;
    register int    i;

    t1 = XToTime(box->left) - 1;
    if ((t1 == (TIME_BOUND - 1)) || (t1 < tims.start))
      {
	t1 = tims.start;
	bg.left = traceBox.left;
      }
    else
	bg.left = box->left;

    t2 = XToTime(box->right);
    if((t2 == TIME_BOUND) || (t2 < 0))
      {
	t2 = tims.end;
	bg.right = traceBox.right;
      }
    else
      {
	bg.right = box->right;
	if( t2 < tims.end )
	    t2++;
      }

    tc = t2;
    if( t2 > tims.last )
	t2 = tims.last;

    bg.top = max( box->top, traceBox.top );
    bg.bot = min( box->bot, traceBox.bot );
    if( CursorVisible( t1, tc ) )
	EraseCursor();

    FillBox( window, bg, gcs.black );

    for( i = traces.disp, t = traces.first; i != 0; i--, t = t->next )
	if( box->top <= t->bot )
	    break;

    while( i != 0 and box->bot >= t->top )
      {
	if( IsVector( t ) )
	    DrawVector( t, t1, t2, FALSE );
	else
	    DrawSignal( t, t1, t2 );
	i--;
	t = t->next;
      }
    if( CursorVisible( t1, tc ) )
	DrawCursor();
  }


/*
 * Update the cache (begining of window and cursor) for traces that just
 * became visible ( or were just added ).
 */
public void UpdateTraceCache( first_trace )
  int  first_trace;
  {
    register Trptr     t;
    register hptr      h,p;
    register int       n, i;
    register TimeType  startT, cursT;

    startT = tims.start;
    if (tims.cursor >= TIME_BOUND)
	cursT = tims.first;
    else
        cursT = max( tims.cursor, tims.first );

    for( t = traces.first, n = 0; n < traces.disp; n++, t = t->next )
      {
	if( n < first_trace )
	    continue;

	if( t->vector )
	  {
	    for( i = t->n.vec->nbits - 1; i >= 0; i-- )
	      {
		hptr  nexth;

		p = t->cache[i].wind;
		h = t->cache[i].cursor;
		NEXTH( nexth, h );
		if( h->time > cursT or nexth->time <= cursT )
		  {
		    if( p->time <= cursT )	/* whatever is closer */
			t->cache[i].cursor = p;
		    else
			t->cache[i].cursor = (hptr)&(t->n.vec->nodes[i]->head);
		  }
		if( startT <= p->time )			/* go back */
		    p = (hptr) &(t->n.vec->nodes[i]->head);

		NEXTH( h, p );
		while( h->time < startT )
		  {
		    p = h;
		    NEXTH( h, h );
		  }
		t->cache[i].wind = p;

		p = t->cache[i].cursor;
		NEXTH( h, p );
		while( h->time <= cursT )
		  {
		    p = h;
		    NEXTH( h, h );
		  }
		t->cache[i].cursor = p;
	      }
	  }
	else
	  {
	    hptr  nexth;

	    p = t->cache[0].wind;
	    h = t->cache[0].cursor;
	    NEXTH( nexth, h );
	    if( h->time > cursT or nexth->time <= cursT )
	      {
		if( p->time <= cursT )
		    t->cache[0].cursor = p;
		else
		    t->cache[0].cursor = (hptr) &(t->n.nd->head);
	      }

	    if( startT <= p->time )
		p = (hptr) &(t->n.nd->head);

	    NEXTH( h, p );
	    while( h->time < startT )
	      {
		p = h;
		NEXTH( h, h );
	      }
	    t->cache[0].wind = p;

	    p = t->cache[0].cursor;
	    NEXTH( h, p );
	    while( h->time <= cursT )
	      {
		p = h;
		NEXTH( h, h );
	      }
	    t->cache[0].cursor = p;
	  }
      }
  }


private	TimeType  lastStart;		/* last redisplay starting time */


public void FlushTraceCache()
  {
    lastStart = MAX_TIME;
  }


/*
 * Draw the traces horizontally from time1 to time2.
 */
public void DrawTraces( t1, t2 )
  TimeType  t1, t2;
  {
    TimeType         endT;
    register Trptr   t;
    int              nt;

    if( t1 == tims.start )
	FillBox( window, traceBox, gcs.black );
    else if( colors.disj == 0 and CursorVisible( tims.start, t2 ) )
	EraseCursor();

    if( tims.start != lastStart )		/* Update history cache */
      {
	int                begin;
	register TimeType  startT;
	register int       n, i;
	register hptr      h, p;

	startT = tims.start;
	begin = ( startT < lastStart );
	for( t = traces.first, n = traces.disp; n != 0; n--, t = t->next )
	  {
	    if( t->vector )
	      {
		for( i = t->n.vec->nbits - 1; i >= 0; i-- )
		  {
		    p = begin ? (hptr) &(t->n.vec->nodes[i]->head) : t->cache[i].wind;
		    NEXTH( h, p );
		    while( h->time < startT )
		      {
			p = h;
			NEXTH( h, h );
		      }
		    t->cache[i].wind = p;
		  }
	      }
	    else
	      {
		p = begin ? (hptr) &(t->n.nd->head) : t->cache[0].wind;
		NEXTH( h, p );
		while( h->time < startT )
		  {
		    p = h;
		    NEXTH( h, h );
		  }
		t->cache[0].wind = p;
	      }
	  }
	lastStart = tims.start;
      }

    endT = min( t2, tims.last );

    for( t = traces.first, nt = traces.disp; nt != 0; nt--, t = t->next )
      {
	if( IsVector( t ) )
	    DrawVector( t, t1, endT, (t1 != tims.start) ? TRUE : FALSE );
	else
	    DrawSignal( t, t1, endT );
      }
    if( CursorVisible( tims.start, t2 ) )
	DrawCursor();
  }


/*
 * Draw a 1 bit trace.
 */
private void DrawSignal( t, t1, t2 )
  Trptr              t;
  register TimeType  t1, t2;
  {
    register hptr  h;
    register int   val, change;
    int            x1, x2;

    if( t1 >= tims.last )
	return;

    h = t->cache[0].wind;
    if( t1 != tims.start )
      {
	register hptr  n;

	NEXTH( n, h );
	while( n->time < t1 )
	  {
	    h = n;
	    NEXTH( n, n );
	  }
      }

    x1 = TimeToX( t1 );
    while( t1 < t2 )
      {
	val = h->val;
	while( h->time < t2 and h->val == val )
	    NEXTH( h, h );
	if( h->time > t2 )
	  {
	    change = FALSE;
	    t1 = t2;
	  }
	else
	  {
	    change = ( h->val != val );
	    t1 = h->time;
	  }
	x2 = TimeToX( t1 );
	switch( val )
	  {
	    case LOW :
		HLine( window, x1, x2, t->bot, gcs.traceFg );
		break;
	    case HIGH :
		HLine( window, x1, x2, t->top, gcs.traceFg );
		break;
	    case X :
		if( x1 > traceBox.left + 1 )
		    x1++;
		FillAREA( window, x1, t->top, x2 - x1 + 1, t->bot - t->top + 1,
		  gcs.xpat );
		break;
	  }
	if( change )
	    VLine( window, x2, t->bot, t->top, gcs.traceFg );
	x1 = x2;
      }
  }


public	hptr    tmpHBuff[ 400 ];

/*
 * Draw bus trace.
 */
private void DrawVector( t, t1, t2, clr_bg )
  register Trptr     t;
  register TimeType  t1, t2;
  int                clr_bg;
  {
    hptr      *start, *changes;
    TimeType  firstChange;
    int       x1, x2, xx, mid, nbits, slen, strwidth;
    
    if( t1 >= tims.last )
	return;

    nbits = t->n.vec->nbits;
    start = tmpHBuff;
    changes = &(tmpHBuff[ nbits ]);
    if (t->bdigit == 5)
       slen = (nbits + 2) / 3;
    else if (t->bdigit == 6)
       slen = 1 + ((nbits + 2) / 3);
    else
       slen = (nbits + t->bdigit - 1) / t->bdigit;
    strwidth = CHARWIDTH * slen + 1;

      {
	register hptr  h, *s;
	register int   n, val;

	s = start;			/* initialize start array */
	if( t1 != tims.start )
	  {
	    register hptr  p;

	    firstChange = tims.start;
	    for( n = nbits - 1; n >= 0; n-- )
	      {
		p = t->cache[n].wind;
		val = p->val;
		NEXTH( h, p );
		while( h->time < t1 )
		  {
		    if( h->val != val )
		      {
			if( h->time > firstChange )
			    firstChange = h->time;
			val = h->val;
		      }
		    p = h;
		    NEXTH( h, h );
		  }
		s[n] = p;
	      }
	  }
	else
	  {
	    firstChange = tims.start;
	    for( n = nbits - 1; n >= 0; n-- )
		s[n] = t->cache[n].wind;
	  }

	  {	/* Initialize changes array */
	    register hptr      *ch = changes;
	    register TimeType  tm = tims.end;

	    for( n = nbits - 1; n >= 0; n-- )
	      {
		h = s[n];
		val = h->val;
		while( h->time < tm and h->val == val )
		    NEXTH( h, h );
		ch[n] = h;
	      }
	  }
      }

    mid = (t->top + t->bot + CHARHEIGHT) / 2;
    xx = TimeToX( t1 );
    x2 = TimeToX( t2 );
    x1 = TimeToX( firstChange );
    HLine( window, xx, x2, t->top, gcs.traceFg );
    HLine( window, xx, x2, t->bot, gcs.traceFg );
    if( clr_bg and t1 != tims.start and (xx - x1) > strwidth )
	FillAREA( window, x1+1, mid - CHARHEIGHT+1, xx - x1+1, CHARHEIGHT,
	 gcs.traceBg );

    while( t1 < t2 )
      {
	  {				/* find nearest change in time */
	    register hptr  *ch;
	    register int   n;

	    t1 = tims.end + 1;
	    for( ch = changes, n = nbits - 1; n >= 0; n-- )
	      {
		if( ch[n]->time < t1 )
		    t1 = ch[n]->time;
	      }
	  }

	if( t1 <= t2 )			/* change before t2 => draw it */
	  {
	    register int  n;

	    x2 = TimeToX( t1 );
	    n = (x2 == traceBox.left+1) ? 2 : (x2 == traceBox.right-1) ? 1 : 0;
	    VLine( window, x2, t->bot, t->top, gcs.traceFg );
	    XCopyArea( display, pix.tops[n], window, gcs.traceBg, 0, 0, 3, 2,
	     x2 - 1, t->top );
	    XCopyArea( display, pix.bots[n], window, gcs.traceBg, 0, 0, 3, 2,
	     x2 - 1, t->bot - 1 );
	  }
	else				/* change after t2 */
	  {
	    register TimeType  tm;

	    tm = min( t1, min( tims.end, tims.last ) );
	    x2 = TimeToX( tm );
	  }

	if( x2 - x1 > strwidth )
	  {
	    char  *str;
	    str = HistToStr( start, nbits, t->bdigit, 1 );
	    if (t->bdigit == 5 || t->bdigit == 6) slen = strlen(str);
	    StrCenter( window, str, slen, x1, x2, mid, gcs.traceFg );
	  }

	  {
	    register hptr      h;
	    register hptr      *ch, *s;
	    register int       n, val;
	    register TimeType  tm = tims.end;

	    for( s = start, ch = changes, n = nbits - 1; n >= 0; n-- )
	      {
		if( ch[n]->time == t1 )
		  {
		    h = s[n] = ch[n];
		    val = h->val;
		    while( h->time < tm and h->val == val )
			NEXTH( h, h );
		    ch[n] = h;
		  }
	      }
	  }
	x1 = x2;
      }
  }



private void UpdateTraces( start, end )
  TimeType  start, end;
  {
    if( not (windowState.iconified or windowState.tooSmall) )
      {
	UpdateScrollBar();
	RedrawTimes();
	DrawTraces( start, end );
      }
  }


private void ScrollTraces( endT )
  TimeType  endT;
  {
    tims.start = endT - tims.steps / 2;
    tims.end = tims.start + tims.steps;
    UpdateTraces( tims.start, endT );
  }


/*
 * Update the trace window so that endT is shown.  If the update fits in the
 * window, simply draw the missing parts.  Otherwise scroll the traces,
 * centered around endT.
 */
public void UpdateWindow( endT )
  TimeType  endT;
  {
    TimeType  lastT;

    DisableInput();

    if( freezeWindow )
      {
	updatePending = TRUE;
	updPendTime = endT;
      }
    else
      {
	lastT = tims.last;
	tims.last = endT;

	if( endT <= tims.end )
	  {
	    if( lastT >= tims.start )
		UpdateTraces( lastT, endT );
	    else if( autoScroll )
		ScrollTraces( endT );
	    else if( endT > tims.start )
		UpdateTraces( tims.start, endT );
	  }
	else					/* endT > tims.end */
	  {
	    if( autoScroll )
		ScrollTraces( endT );
	    else if( lastT < tims.end )
		UpdateTraces( lastT, tims.end );
	  }
      }
    EnableInput();
  }


/*
 * Erase/Redraw the cursor.
 */
private void EraseCursor()
  {
    Coord  x, d;
    int w;
    
    x = TimeToX( tims.cursor );
    FillAREA( window, x, traceBox.top, 1, traceBox.bot - traceBox.top,
     gcs.curs_off );

    if (tims.cursor < TIME_BOUND && tims.delta < TIME_BOUND) {
       d = TimeToX(tims.delta);
       w = (int)(d - x);
       if (w < 0) {
	  x = d;
	  w = -w;
       }
       FillAREA(window, x, traceBox.top, w, traceBox.bot - traceBox.top,
           		gcs.unhilite);
    }
  }


private void  DrawCursor()
  {
    Coord  x, d;
    int w;
    
    x = TimeToX(tims.cursor);
    FillAREA( window, x, traceBox.top, 1, traceBox.bot - traceBox.top,
     gcs.curs_on );

    if (tims.cursor < TIME_BOUND && tims.delta < TIME_BOUND) {
       d = TimeToX(tims.delta);
       w = (int)(d - x);
       if (w < 0) {
	  x = d;
	  w = -w;
       }
       FillAREA(window, x, traceBox.top, w, traceBox.bot - traceBox.top,
           gcs.hilite);
    }
  }


public void SetCursor( t, time )
  Trptr t;
  TimeType time;
{
    register hptr  h, p;
    register int   n;
    register char  *val, *inp;
    int            nbits;
    char      tbuff[15];

    if (t == NULL || time < 0 || time > tims.last || time == TIME_BOUND)
      {
	XBell( display, 0 );
	return;
      }
    (void) sprintf( tbuff, "%.2f", d2ns( time ) );
    PRINTF( "\n%s @ %s: value=", t->name, tbuff );

    val = (char *) tmpHBuff;
    if( IsVector( t ) )
	nbits = t->n.vec->nbits;
    else
	nbits = 1;
	    
    inp = &(val[nbits]);
    *inp++ = '\0';
    inp[nbits] = '\0';
    for( n = nbits - 1; n >= 0; n-- )
    {
	p = t->cache[n].wind;
	NEXTH( h, p );
	while( h->time <= time )
	{
	    p = h;
	    NEXTH( h, h );
	}
	val[n] = "0X 1"[ p->val ];
	inp[n] = "-i"[p->inp];
    }
    PRINTF( "%s, input=%s", val, inp );
}

void DoCursor( ev )
  XButtonEvent  *ev;
  {
    Trptr     t;
    TimeType  time;

    if( not (ev->state & ShiftMask) )
      {
	MoveCursorToPos( ev->x );
	return;
      }

    t = GetYTrace( ev->y );
    time = XToTime( ev->x );
    SetCursor(t, time);
  }

#ifdef TCL_IRSIM

public void MoveDeltaToTime(TimeType time)
{
    if (time == tims.delta)
	return;

    if (CursorVisible(tims.start, tims.end))
	EraseCursor();

    tims.delta = time;

    DrawCursor();
}

#endif /* TCL_IRSIM */

public void MoveCursorToTime(TimeType time)
  {
    register int       i;
    register Trptr     t;
    char               s[ 20 ];
    static int         olen = 1;

    if (time == tims.cursor)
	return;

    if (CursorVisible( tims.start, tims.end))
	EraseCursor();

    tims.cursor = time;

    if (!CursorVisible(tims.start, tims.end))
	return;

    DrawCursor();
    (void) sprintf( s, "%.2f", d2ns( time ) );
    i = strlen( s );
    if( i < olen )
      {
	FillAREA( window, (timesBox.left+timesBox.right-(olen * CHARWIDTH))/2,
	timesBox.bot - CHARHEIGHT - 1, olen * CHARWIDTH,
	timesBox.bot - timesBox.top + 1, gcs.white );
      } 
    olen = i;
    StrCenter( window, s, i, timesBox.left, timesBox.right, timesBox.bot,
       gcs.black );
    for( i = traces.disp, t = traces.first; i != 0; i--, t = t->next )
      {
	register hptr   h, p;

	if( IsVector( t ) )
	  {
	    register int  n;

	    for( n = t->n.vec->nbits - 1; n >= 0; n-- )
	      {
		p = t->cache[n].wind;
		NEXTH( h, p );
		while( h->time <= time )
		  {
		    p = h;
		    NEXTH( h, h );
		  }
		t->cache[n].cursor = p;
	      }
	  }
	else
	  {
	    p = t->cache[0].wind;
	    NEXTH( h, p );
	    while( h->time <= time )
	      {
		p = h;
		NEXTH( h, h );
	      }
	    t->cache[0].cursor = p;
	  }
      }
    DrawCursVal( cursorBox );
  }

private void MoveCursorToPos( x )
  Coord  x;
{
    register TimeType  time;

    time = XToTime( x );
    MoveCursorToTime(time);
}

private	char  *StrMap[] = { "0", "X", "", "1" };


/*
 * Display signal values under cursor.
 */
public void DrawCursVal( rb )
  BBox  rb;
  {
    Coord        y;
    Trptr        t;
    int          i, len;
    char         *val;

    rb.left = max( rb.left, cursorBox.left );
    rb.right = min( rb.right, cursorBox.right );
    rb.top = max( cursorBox.top, rb.top );
    rb.bot = min( cursorBox.bot, rb.bot );
    FillBox( window, rb, gcs.white );

    if( tims.cursor < tims.first or tims.cursor > tims.last )
	return;

    for( i = traces.disp, t = traces.first; i != 0; i--, t = t->next )
	if( rb.top <= t->bot )
	    break;

    while( i != 0 and rb.bot >= t->top )
      {
	y = ( t->bot + t->top + CHARHEIGHT ) / 2;
	val = IsVector( t ) ?
	    HistToStr( &(t->cache[0].cursor), t->n.vec->nbits, t->bdigit, 2 ) :
	    StrMap[ t->cache[0].cursor->val ];

	len = strlen( val );
	StrCenter( window, val, len, cursorBox.left, cursorBox.right, y,
	 gcs.black );
	i--;
	t = t->next;
      }
  }


public void ExpandCursVal( t )
  Trptr  t;
  {
    char          *val;
    int           nbits;

    nbits = IsVector( t ) ? t->n.vec->nbits : 1;
    val = HistToStr( &(t->cache[0].cursor), nbits, 1, 2 );

    PRINTF( "\n %s : value=%s", t->name, val );
      {
	register int    n;
	register Cache  *c;
	register char   *s;

	for( n = 0, s = val, c = t->cache; n < nbits; n++ )
	    *s++ = ( c[n].cursor->inp) ? 'i' : '-';
      }
    PRINTF( "  input=%s", val );
  }

#ifdef TCL_IRSIM

public void TraceValue(Trptr t, int isBinary)
{
    char *val;
    int  nbits, n;

    n = (isBinary == 1) ? 1 : t->bdigit;

    if ((tims.cursor < tims.start) || (tims.cursor > tims.end))
	return;

    nbits = IsVector(t) ? t->n.vec->nbits : 1;
    val = HistToStr(&(t->cache[0].cursor), nbits, n, 2);

    /* Convert to the base of the trace */

    Tcl_SetResult(irsiminterp, val, 0);
}

public void TraceInput(Trptr t)
{
    char *s, *val;
    int  n, nbits;
    Cache *c;

    if ((tims.cursor < tims.start) || (tims.cursor > tims.end))
	return;

    nbits = IsVector(t) ? t->n.vec->nbits : 1;
    val = HistToStr(&(t->cache[0].cursor), nbits, 1, 2);

    for (n = 0, s = val, c = t->cache; n < nbits; n++)
	*s++ = (c[n].cursor->inp) ? 'i' : '-';
    Tcl_SetResult(irsiminterp, val, 0);
}

public void TraceBits(Trptr t)
{
    int  nbits;

    nbits = IsVector(t) ? t->n.vec->nbits : 1;
    Tcl_SetObjResult(irsiminterp, Tcl_NewIntObj(nbits));
}

#endif	/* TCL_IRSIM */


