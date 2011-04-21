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
#include "ana.h"
#include "ana_glob.h"
#include "X11/Xutil.h"


public void GetWindowGeometry( w, x, y, width, height, border )
  Window  w;
  Coord   *x, *y, *width, *height;
  int     *border;
  {
    Window  root, subw;
    int d;
    unsigned int ww, wh, wb, wd;

    XGetGeometry( display, w, &subw, x, y, &ww, &wh, &wb, &wd );
    *width = ww;
    *height = wh;
    *border = wb;
    root = RootWindowOfScreen( screen );
    d = -(*border);
    XTranslateCoordinates( display, w, root, d, d, x, y, &subw );
  }


public void GrabMouse( w, ev_mask, cursor )
  Window         w;
  unsigned long  ev_mask;
  Cursor         cursor;
  {
   while( XGrabPointer( display, w, False, ev_mask, GrabModeAsync,
     GrabModeAsync, None, cursor, CurrentTime ) != GrabSuccess );
  }


private int GetMotionEvent( w, ev, x, y )
  Window  w;
  XEvent  *ev;
  Coord   *x, *y;
  {
    int           n;
    unsigned int  mask;
    Coord         rx, ry;
    Window        root, subw;

    for( n = XPending( display ); n != 0; n-- )
      {
	XNextEvent( display, ev );
	if( ev->type == ButtonPress or ev->type == ButtonRelease )
	    return( FALSE );
      }
    XQueryPointer( display, w, &root, &subw, &rx, &ry, x, y, &mask );
    return( TRUE );
  }


private void MoveOutline( wind, x1, y1, x2, y2 )
  Window  wind;
  Coord   x1, y1, x2, y2;
  {
    static Coord  oldx1 = 0;
    static Coord  oldy1 = 0;
    static Coord  oldx2 = 0;
    static Coord  oldy2 = 0;
    Coord         x3, x2_3, y3, y2_3;
    XSegment      outline[16];
    XSegment      *s;
    int           nseg = 0;

    if( x1 == oldx1 and x2 == oldx2 and y1 == oldy1 and y2 == oldy2 )
	return;

    s = outline;
    nseg = 0;
    if( oldx1 != oldx2 and oldy1 != oldy2 )
      {
	x3 = (oldx2 - oldx1)/3;
	x2_3 = oldx1 + x3 + x3;
	x3 += oldx1;
	y3 = (oldy2 - oldy1)/3;
	y2_3 = oldy1 + y3 + y3;
	y3 += oldy1;
	s[0].x1 = oldx1;  s[0].y1 = oldy1;  s[0].x2 = oldx2;  s[0].y2 = oldy1;
	s[1].x1 = oldx2;  s[1].y1 = oldy1;  s[1].x2 = oldx2;  s[1].y2 = oldy2;
	s[2].x1 = oldx1;  s[2].y1 = oldy2;  s[2].x2 = oldx2;  s[2].y2 = oldy2;
	s[3].x1 = oldx1;  s[3].y1 = oldy1;  s[3].x2 = oldx1;  s[3].y2 = oldy2;

	s[4].x1 = oldx1;  s[4].y1 = y3;     s[4].x2 = oldx2;  s[4].y2 = y3;
	s[5].x1 = oldx1;  s[5].y1 = y2_3;   s[5].x2 = oldx2;  s[5].y2 = y2_3;
	s[6].x1 = x3;     s[6].y1 = oldy1;  s[6].x2 = x3;     s[6].y2 = oldy2;
	s[7].x1 = x2_3;   s[7].y1 = oldy1;  s[7].x2 = x2_3;   s[7].y2 = oldy2;
	s += 8;
	nseg += 8;
      }
    if( x1 != x2 and y1 != y2 )
      {
	x3 = (x2 - x1)/3;
	x2_3 = x1 + x3 + x3;
	x3 += x1;
	y3 = (y2 - y1)/3;
	y2_3 = y1 + y3 + y3;
	y3 += y1;
	s[0].x1 = x1;     s[0].y1 = y1;     s[0].x2 = x2;     s[0].y2 = y1;
	s[1].x1 = x2;     s[1].y1 = y1;     s[1].x2 = x2;     s[1].y2 = y2;
	s[2].x1 = x1;     s[2].y1 = y2;     s[2].x2 = x2;     s[2].y2 = y2;
	s[3].x1 = x1;     s[3].y1 = y1;     s[3].x2 = x1;     s[3].y2 = y2;

	s[4].x1 = x1;     s[4].y1 = y3;     s[4].x2 = x2;     s[4].y2 = y3;
	s[5].x1 = x1;     s[5].y1 = y2_3;   s[5].x2 = x2;     s[5].y2 = y2_3;
	s[6].x1 = x3;     s[6].y1 = y1;     s[6].x2 = x3;     s[6].y2 = y2;
	s[7].x1 = x2_3;   s[7].y1 = y1;     s[7].x2 = x2_3;   s[7].y2 = y2;
	nseg += 8;
      }
    if( nseg != 0 )
	XDrawSegments( display, wind, gcs.invert, outline, nseg );

    oldx1 = x1;
    oldx2 = x2;
    oldy1 = y1;
    oldy2 = y2;
  }


public void ResizeMe()
  {
    Window      root;
    Coord       left, top, right, bot, width, height;
    Coord       lim_left, lim_right, lim_top, lim_bot;
    Coord       x, y, w, h, min_w, min_h, ox, oy;
    int         border, d;
    XEvent      ev;
    XSizeHints  shint;

    GetWindowGeometry( window, &left, &top, &width, &height, &border );
    border *= 2;
    right = left + width + border - 1;
    bot = top + height + border - 1;

    GetMinWsize( &min_w, &min_h );
    min_w += border;
    min_h += border;

    w = min_w / 2;
    h = min_h / 2;
    lim_left = left + w;
    lim_right = right - w;
    lim_top = top + h;
    lim_bot = bot - h;

    root = RootWindowOfScreen( screen );
    XGrabServer( display );
    GrabMouse( root, ButtonPressMask | ButtonReleaseMask, cursors.move );

    ev.type = MotionNotify;
    w = width + border;
    h = height + border;
    ox = left; oy = bot;
    while( GetMotionEvent( root, &ev, &x, &y ) )
      {
	if( x < lim_left )
	    ox = right;
	else if( x > lim_right )
	    ox = left;
	if( y < lim_top )
	    oy = bot;
	else if( y > lim_bot )
	    oy = top;

	w = x - ox;
	if( w < 0 )
	  {
	    w = 1 - w;
	    if( w < min_w )
		w = min_w;
	    x = ox - w + 1;
	  }
	else
	  {
	    w += 1;
	    if( w < min_w )
		w = min_w;
	    x = ox + w - 1;
	  }

	h = y - oy;
	if( h < 0 )
	  {
	    h = 1 - h;
	    if( h < min_h )
		h = min_h;
	    y = oy - h + 1;
	  }
	else
	  {
	    h += 1;
	    if( h < min_h )
		h = min_h;
	    y = oy + h - 1;
	  }
	MoveOutline( root, ox, oy, x, y );
      }
    MoveOutline( root, 0, 0, 0, 0 );		/* erase outline */
    XUngrabPointer( display, CurrentTime );
    XUngrabServer( display );

    w -= border;
    h -= border;
    if( ev.type == ButtonPress or (w == width and h == height) )
	return;

    if( ox < x ) x = ox;
    if( oy < y ) y = oy;

    shint.x = x;
    shint.y = y;
    shint.width = w;
    shint.height = h;
    shint.max_width = shint.max_height = 16000;
    shint.width_inc = shint.height_inc = 1;
    GetMinWsize( &shint.min_width, &shint.min_height );
    shint.flags = ( PMinSize | PMaxSize | PResizeInc | USPosition | USSize );
    XSetNormalHints( display, window, &shint );
    XMoveResizeWindow( display, window, x, y, w, h );  
  }


public void MoveMe( x, y )
  Coord  x, y;
  {
    Coord       dx, dy;
    Window      root;
    Coord       top, left, w, h;
    int         border, d;
    XEvent      ev;
    XSizeHints  shint;

    GetWindowGeometry( window, &left, &top, &w, &h, &border );
    dx = x + border;
    dy = y + border;
    border *= 2;
    w += border - 1;
    h += border - 1;

    root = RootWindowOfScreen( screen );
    XGrabServer( display );
    GrabMouse( root, ButtonReleaseMask | ButtonPressMask, cursors.move );

    ev.type = MotionNotify;
    x = left; y = top;
    while( GetMotionEvent( root, &ev, &x, &y ) )
      {
	x -= dx;
	y -= dy;
	MoveOutline( root, x, y, x + w, y + h );
      }
    MoveOutline( root, 0, 0, 0, 0 );

    XUngrabPointer( display, CurrentTime );
    XUngrabServer( display );

    if( ev.type == ButtonPress or (x == left and y == top) )
	return;

    shint.x = x;
    shint.y = y;
    shint.width = w - border + 1;
    shint.height = h - border + 1;
    shint.max_width = shint.max_height = 16000;
    shint.width_inc = shint.height_inc = 1;
    GetMinWsize( &shint.min_width, &shint.min_height );
    shint.flags = ( PMinSize | PMaxSize | PResizeInc | USPosition | USSize );
    XSetNormalHints( display, window, &shint );
    XMoveWindow( display, window, x, y );
  }


public void IconifyMe()
  {
    Coord  x, y, ix, iy, w, h, b;

    GetWindowGeometry( iconW, &ix, &iy, &w, &h, &b );
    GetWindowGeometry( window, &x, &y, &w, &h, &b );
    XDestroyWindow( display, window );
    XDestroyWindow( display, iconW );
    XFlush( display );
    iconW = CreateIconWindow( ix, iy );
    InitWindow( FALSE, IconicState, x, y, w, h, ix, iy );
    XMapWindow( display, window );
  }
