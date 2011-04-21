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

#ifndef _ANA_H
#define _ANA_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifndef _NET_H
#include "net.h"
#endif

#ifndef _DEFS_H
#include "defs.h"
#endif

#define max( a, b )		( ( (a) > (b) ) ? (a) : (b) )
#define min( a, b )		( ( (a) < (b) ) ? (a) : (b) )
#define	round( aa )		( (int) ( (aa) + 0.5 ) )
#define	Round( aa )		( (TimeType) ( (aa) + 0.5 ) )


typedef unsigned long Pixel;

			/* color information */
typedef struct
  {
    Pixel  black;
    Pixel  white;
    Pixel  traces;
    Pixel  hilite;
    Pixel  banner_bg;
    Pixel  banner_fg;
    Pixel  border;
    Pixel  disj;
    int    mono;
    int    color_hilite;
  } COL;

			/* pixmaps */
typedef struct
  {
    Pixmap  gray;		/* full plane pixmap */
    Pixmap  xpat;		/* full plane pixmap */
    Pixmap  left_arrows;	/* full plane pixmap */
    Pixmap  right_arrows;	/* full plane pixmap */
    Pixmap  tops[3];		/* full plane pixmap */
    Pixmap  bots[3];		/* full plane pixmap */
    Pixmap  chk;		/* full plane pixmap */
    Pixmap  icon;		/* bitmap */
    Pixmap  iconbox;
    Pixmap  sizebox;
    Pixmap  select;		/* select hilight pattern */
  } PIX;

			/* cursors */
typedef struct
  {
    Cursor  deflt;
    Cursor  left;
    Cursor  right;
    Cursor  timer;
    Cursor  move;
  } CURS;


typedef struct
  {
    GC  black;		/* fg = black, bg = white */
    GC  white;		/* fg = white, bg = black */
    GC  inv;		/* invert gc, for menus */
    GC  invert;		/* invert: for move, resize, highlight menu */
    GC  gray;		/* for gray pattern */
    GC  traceBg;	/* for traces window */
    GC  traceFg;
    GC  xpat;		/* fpr X pattern */
    GC  hilite;		/* for hilighting */
    GC  unhilite;
    GC  curs_on;	/* turn cursor on/off */
    GC  curs_off;
    GC  bannerFg;	/* banner fg and bg */
    GC  bannerBg;
    GC  select;		/* select pattern */
    GC  border;
  } GCS;


#ifndef NULL
#define	NULL		0
#endif

typedef	int	Coord;
typedef	Ulong	TimeType;		/* 8 bytes (see net.h)	*/


typedef struct
  {
    int  selected;
    int  iconified;
    int  tooSmall;
  } Wstate;


typedef struct				/* Bounding box */
  {
    Coord    top, left;			/* top left corner */
    Coord    bot, right;		/* bottom right corner */
  } BBox;


typedef struct
  {
    TimeType    first;
    TimeType    last;
    TimeType    start;
    TimeType    steps;
    TimeType    end;
    TimeType    cursor;
    TimeType    delta;
  } Times;


typedef struct TraceEnt *Trptr;

typedef struct
  {
    int      total;		/* total number of traces */
    int      disp;		/* number of traces displayed */
    int      maxName;		/* longest name */
    int      maxDigits;		/* longest string of digits */
    Trptr    first;		/* ptr. to last trace displayed */
    Trptr    last;		/* list of traces */
  } Traces;


typedef struct			/* Cache for history pointer */
  {
    hptr  wind;				/* window start */
    hptr  cursor;			/* cursor value */
  } Cache;

typedef struct TraceEnt
  {
    Trptr    next;		/* doubly linked list of traces */
    Trptr    prev;
    char     *name;		/* name stripped of path */
    int      len;		/* length of name string */
    Coord    top, bot;		/* position of the trace */
    short    bdigit;		/* # of bits per digit for displaying */
    unsigned char vector;	/* 1 if bit vector, 0 if node */
    union
      {
	nptr    nd;		/* what makes up this trace */
	bptr    vec;
      } n;
    Cache    cache[1];
  } TraceEnt;


#define IsVector( to )		( to->vector == TRUE and to->n.vec->nbits > 1 )


typedef void (*Func)();


typedef struct
  {
    char   *str;		/* item string */
    char   mark;		/* mark ('+', '-', or 0) */
    Func   func;		/* function to call */
    int    len;			/* length of str */
    Coord  top, bot;		/* y position */
  } MenuItem;


typedef struct
  {
    char      *str;		/* header string */
    MenuItem  *items;		/* items for this menu */
    int       len;		/* length of header */
    BBox      box;		/* bounding box of header */
    Window    w;		/* window to pop-up */
    Coord     width, height;	/* size of window */
  } Menu;


#define	MENU_MARK	'+'
#define	MENU_UNMARK	'-'
#define MENU_NOMARK	'\0'


	/* get next history pointer, skipping punted events */
#define	NEXTH( H, P )	for( (H) = (P)->next; (H)->punt; (H) = (H)->next )

#endif /* _ANA_H */
