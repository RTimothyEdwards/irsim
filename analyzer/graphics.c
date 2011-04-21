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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>  /* for strcmp() */

#include <X11/cursorfont.h>

#include "ana.h"
#include "graphics.h"
#include "ana_glob.h"

#include "Bitmaps/gray"
#include "Bitmaps/xpat"
#include "Bitmaps/left_arrows"
#include "Bitmaps/right_arrows"
#include "Bitmaps/left_curs"
#include "Bitmaps/left_mask"
#include "Bitmaps/right_curs"
#include "Bitmaps/right_mask"
#include "Bitmaps/chk"
#include "Bitmaps/iconbox"
#include "Bitmaps/sizebox"
#include "Bitmaps/select"


public	PIX       pix;
public	GCS       gcs;
public	COL       colors;
public	CURS      cursors;


private	char   tops_bits[3][2] = { 0x5, 0x7, 0x1, 0x3, 0x4, 0x6 };
private	char   bots_bits[3][2] = { 0x7, 0x5, 0x3, 0x1, 0x6, 0x4 };


#define	SAME_COLOR( A, B )	\
    ( (A).red == (B).red and (A).green == (B).green and (A).blue == (B).blue )

private int GetColor( key, colors, ndefined )
  int     key;
  XColor  colors[];
  int     ndefined;
  {
    char      *s;
    XColor    *color;
    int       i;
    Colormap  cmap = DefaultColormapOfScreen( screen );

    color = &colors[ndefined];
    s = GetXDefault( key );
    if( XParseColor( display, cmap, s, color ) == 0 )
      {
	if( IsDefault( key, s ) )
	    return( FALSE );

	fprintf( stderr, "server doesn't know color '%s'", s );
	s = ProgDefault( key );
	if( XParseColor( display, cmap, s, color ) == 0 )
	  {
	    fprintf( stderr, " or '%s'\n", s );	    /* weird, should not be */
	    return( FALSE );
	  }
	fprintf( stderr, "using '%s' instead\n", s );
      }
    for( i = 0; i < ndefined; i++ )
      {
	if( SAME_COLOR( *color, colors[i] ) )
          {
            color->pixel = colors[i].pixel;
            return( TRUE );
          }
      }
    return( (XAllocColor( display, cmap, color ) == 0) ? FALSE : TRUE );
  }

#define	BACKGROUND_COLOR	0
#define	FOREGROUND_COLOR	1
#define	TRACE_COLOR		2
#define	HIGHLIGHT_COLOR		3
#define	BANNER_BG_COLOR		4
#define	BANNER_FG_COLOR		5
#define	BORDER_COLOR		6
#define	MAX_NCOLORS		7

private void SetColors()
  {
    XColor    coldef[ MAX_NCOLORS ];

    if( PlanesOfScreen( screen ) < 2 )
	goto mono;

    if( not GetColor( DEFL_BG, coldef, BACKGROUND_COLOR ) )
	goto mono;
    colors.black = coldef[ BACKGROUND_COLOR ].pixel;

    if( not GetColor( DEFL_FG, coldef, FOREGROUND_COLOR ) )
	goto mono;
    colors.white = coldef[ FOREGROUND_COLOR ].pixel;

    if( GetColor( DEFL_TRCOLOR, coldef, TRACE_COLOR ) )
	colors.traces = coldef[ TRACE_COLOR ].pixel;
    else
	colors.traces = colors.white;

    if( GetColor( DEFL_HIGL, coldef, HIGHLIGHT_COLOR ) )
      {
	colors.color_hilite = TRUE;
	colors.hilite = coldef[ HIGHLIGHT_COLOR ].pixel;
	if( ((colors.black | colors.hilite) == colors.hilite and
	 (colors.traces & ~colors.hilite) == colors.traces ) )
	    colors.disj = 1;
	else if( (colors.black & ~colors.hilite) == colors.hilite and
	 (colors.traces | colors.hilite) == colors.traces )
	    colors.disj = 2;
	else
	    colors.disj = 0;
      }
    else
      {
	colors.disj = 0;
	colors.hilite = colors.black;
	colors.color_hilite = FALSE;
	colors.mono = TRUE;
      }

    if( GetColor( DEFL_BANN_BG, coldef, BANNER_BG_COLOR ) )
	colors.banner_bg = coldef[ BANNER_BG_COLOR ].pixel;
    else
	colors.banner_bg = colors.white;

    if( GetColor( DEFL_BANN_FG, coldef, BANNER_FG_COLOR ) )
	colors.banner_fg = coldef[ BANNER_FG_COLOR ].pixel;
    else
	colors.banner_fg = colors.black;

    if( GetColor( DEFL_BDRCOLOR, coldef, BORDER_COLOR ) )
	colors.border = coldef[ BORDER_COLOR ].pixel;
    else
	colors.border = colors.black;

    return;

  mono:
    colors.mono = TRUE;
    colors.color_hilite = FALSE;
    colors.disj = 0;
    colors.black = BlackPixelOfScreen( screen );
    colors.white = WhitePixelOfScreen( screen );
    if( strcmp( GetXDefault( DEFL_RV ), "on" ) == 0 )
	SWAP( Pixel, colors.black, colors.white );
    colors.traces = colors.banner_bg = colors.white;
    colors.hilite = colors.banner_fg = colors.border = colors.black;
  }


#define MakeBitmap( D, W, H )	\
 XCreateBitmapFromData( display, DefaultRootWindow( display ), D, W, H );


#define MakePixmap( D, W, H, FG, BG )	\
 XCreatePixmapFromBitmapData( display, DefaultRootWindow( display ), \
  D, W, H, FG, BG, DefaultDepthOfScreen( screen ) )

private void InitBitmaps()
  {
    int     i;

    pix.gray = MakePixmap( gray_bits, gray_width, gray_height,
     colors.white, colors.black );

    pix.xpat = MakePixmap( xpat_bits, xpat_width, xpat_height, colors.traces,
     colors.black );

    pix.select = MakePixmap( select_bits, select_width, select_height,
     colors.banner_fg, colors.banner_bg );

    for( i = 0; i < 3; i++ )
      {
	pix.tops[i] = MakePixmap( tops_bits[i], 3, 2,
	 colors.traces, colors.black);
	pix.bots[i] = MakePixmap( bots_bits[i], 3, 2,
	 colors.traces, colors.black);
      }

#ifndef TCL_IRSIM
    /* Tcl doesn't use these bitmaps, as Tk handles the window decorations */

    pix.left_arrows = MakePixmap( left_arrows_bits, left_arrows_width,
     left_arrows_height, colors.black, colors.white );

    pix.right_arrows = MakePixmap( right_arrows_bits, right_arrows_width,
     right_arrows_height, colors.black, colors.white );

    pix.chk = MakePixmap( chk_bits, chk_width, chk_height, colors.black,
     colors.white );

    pix.iconbox = MakePixmap( iconbox_bits, iconbox_width, iconbox_height,
     colors.banner_fg, colors.banner_bg );

    pix.sizebox = MakePixmap( sizebox_bits, sizebox_width, sizebox_height,
     colors.banner_fg, colors.banner_bg );

    pix.icon = InitIconBitmap();

#endif /* (!TCL_IRSIM) */

  }


private Cursor MakeCursor( fg, bg, curs, mask, w, h, x, y )
  XColor  *fg, *bg;
  char    *curs, *mask;
  int     w, h;
  {
    Pixmap  pcurs, pmask;
    Cursor  cu;

    pcurs = MakeBitmap( curs, w, h );
    pmask = MakeBitmap( mask, w, h );
    cu = XCreatePixmapCursor( display, pcurs, pmask, fg, bg, x, y );
    XFreePixmap( display, pcurs );
    XFreePixmap( display, pmask );
    return( cu );
  }


private void InitCursors()
  {
    XColor  cols[2];

    cols[0].pixel = colors.white;
    cols[1].pixel = colors.black;
    XQueryColors ( display, DefaultColormapOfScreen( screen ), cols, 2 );
#define	WHITE	(&cols[0])
#define	BLACK	(&cols[1])

    cursors.deflt = XCreateFontCursor( display, XC_left_ptr );
    XRecolorCursor( display, cursors.deflt, WHITE, BLACK );

    cursors.left = MakeCursor( WHITE, BLACK, left_curs_bits, left_mask_bits,
     left_curs_width, left_curs_height, left_curs_x_hot, left_curs_y_hot );

    cursors.right = MakeCursor( WHITE, BLACK, right_curs_bits,
     right_mask_bits, right_curs_width, right_curs_height, right_curs_x_hot,
     right_curs_y_hot );

    cursors.timer = XCreateFontCursor( display, XC_watch );
    XRecolorCursor( display, cursors.timer, BLACK, WHITE );

    cursors.move = XCreateFontCursor( display, XC_fleur );
    XRecolorCursor( display, cursors.timer, BLACK, WHITE );
#undef	WHITE
#undef	BLACK
  }


public void InitGraphics(font)
  Font  font;
  {
    XGCValues      gcv;
    unsigned long  mask;
    Window         root;

    SetColors();

    InitBitmaps();
    InitCursors();
    
#ifdef TCL_IRSIM
    root = window;
#else
    root = RootWindowOfScreen(screen);
#endif

    mask = 0;
    gcv.foreground = colors.white;			mask |= GCForeground;
    gcv.background = colors.black;			mask |= GCBackground;
    gcv.line_width = 1;					mask |= GCLineWidth;
    gcv.font = font;					mask |= GCFont;
    gcs.white = XCreateGC( display, root, mask, &gcv );

    mask = 0;
    gcv.foreground = colors.black;			mask |= GCForeground;
    gcv.background = colors.white;			mask |= GCBackground;
    gcv.line_width = 1;					mask |= GCLineWidth;
    gcv.font = font;					mask |= GCFont;
    gcs.black = XCreateGC( display, root, mask, &gcv );

    mask = 0;
    gcv.foreground = (colors.black | colors.white);	mask |= GCForeground;
    gcv.function = GXinvert;				mask |= GCFunction;
    gcv.plane_mask = (colors.black ^ colors.white);	mask |= GCPlaneMask;
    gcv.line_width = 1;					mask |= GCLineWidth;
    gcs.inv = XCreateGC( display, root, mask, &gcv );

    mask = 0;
    gcv.foreground = colors.white;			mask |= GCForeground;
    gcv.background = colors.black;			mask |= GCBackground;
    gcv.function = GXcopy;				mask |= GCFunction;
    gcv.tile = pix.gray;				mask |= GCTile;
    gcv.fill_style = FillTiled;				mask |= GCFillStyle;
    gcs.gray = XCreateGC( display, root, mask, &gcv );

    if( colors.disj )
      {
	mask = 0;
	gcv.foreground = colors.black;			mask |= GCForeground;
	gcv.background = colors.traces;			mask |= GCBackground;
	gcv.line_width = 1;				mask |= GCLineWidth;
	gcv.font = font;				mask |= GCFont;
	gcv.plane_mask = (colors.traces|colors.black);	mask |= GCPlaneMask;
	gcs.traceBg = XCreateGC( display, root, mask, &gcv );

	mask = 0;
	gcv.foreground = colors.traces;			mask |= GCForeground;
	gcv.background = colors.black;			mask |= GCBackground;
	gcv.line_width = 1;				mask |= GCLineWidth;
	gcv.font = font;				mask |= GCFont;
	gcv.plane_mask = (colors.traces|colors.black);	mask |= GCPlaneMask;
	gcs.traceFg = XCreateGC( display, root, mask, &gcv );

	mask = 0;
	gcv.foreground = colors.traces;			mask |= GCForeground;
	gcv.background = colors.black;			mask |= GCBackground;
	gcv.tile = pix.xpat;				mask |= GCTile;
	gcv.fill_style = FillTiled;			mask |= GCFillStyle;
	gcv.plane_mask = (colors.traces|colors.black);	mask |= GCPlaneMask;
	gcs.xpat = XCreateGC( display, root, mask, &gcv );

	mask = 0;
	gcv.foreground = colors.hilite;			mask |= GCForeground;
	gcv.plane_mask = (~colors.traces & ~colors.black & colors.hilite);
							mask |= GCPlaneMask;
	gcv.function = (colors.disj == 1) ? GXset : GXclear;
							mask |= GCFunction;
	gcs.hilite = XCreateGC( display, root, mask, &gcv );

	mask = 0;
	gcv.foreground = colors.hilite;			mask |= GCForeground;
	gcv.plane_mask = (~colors.traces & ~colors.black & colors.hilite);
							mask |= GCPlaneMask;
	gcv.function = (colors.disj == 2) ? GXset : GXclear;
							mask |= GCFunction;
	gcs.unhilite = XCreateGC( display, root, mask, &gcv );
	gcs.curs_on = gcs.hilite;
	gcs.curs_off = gcs.unhilite;
      }
    else	/* colors not disjoint */
      {
	if( colors.traces == colors.white )
	    gcs.traceFg = gcs.white;
	else
	  {
	    mask = 0;
	    gcv.foreground = colors.traces;		mask |= GCForeground;
	    gcv.background = colors.black;		mask |= GCBackground;
	    gcv.line_width = 1;				mask |= GCLineWidth;
	    gcv.font = font;				mask |= GCFont;
	    gcs.traceFg = XCreateGC( display, root, mask, &gcv );
	  }
	gcs.traceBg = gcs.black;

	mask = 0;
	gcv.foreground = colors.traces;			mask |= GCForeground;
	gcv.background = colors.black;			mask |= GCBackground;
	gcv.tile = pix.xpat;				mask |= GCTile;
	gcv.fill_style = FillTiled;			mask |= GCFillStyle;
	gcs.xpat = XCreateGC( display, root, mask, &gcv );

	if( colors.color_hilite )
	  {
	    mask = 0;
	    gcv.foreground = colors.hilite | colors.black;
							mask |= GCForeground;
	    gcv.plane_mask = colors.hilite ^ colors.black;
							mask |= GCPlaneMask;
	    gcv.function = GXinvert;			mask |= GCFunction;
	    gcv.line_width = 1;				mask |= GCLineWidth;
	    gcs.hilite = XCreateGC( display, root, mask, &gcv );
	    gcs.curs_off = gcs.curs_on = gcs.unhilite = gcs.hilite;  /* xor */
	  }
	else
	  {
	    mask = 0;
	    gcv.function = GXinvert;			mask |= GCFunction;
	    gcv.fill_style = FillTiled;			mask |= GCFillStyle;
	    gcv.tile = pix.gray;			mask |= GCTile;
	    gcs.curs_on = XCreateGC( display, root, mask, &gcv );
	    gcs.curs_off = gcs.curs_on;
	    gcs.hilite = gcs.unhilite = gcs.inv;	/* use xor */
	  }
      }

    if( colors.banner_bg == colors.white and
      colors.banner_fg == colors.black )
      {
	gcs.bannerBg = gcs.white;
	gcs.bannerFg = gcs.black;
      }
    else
      {
	mask = 0;
	gcv.foreground = colors.banner_fg;		mask |= GCForeground;
	gcv.background = colors.banner_bg;		mask |= GCBackground;
	gcv.line_width = 1;				mask |= GCLineWidth;
	gcv.font = font;				mask |= GCFont;
	gcs.bannerFg = XCreateGC( display, root, mask, &gcv );

	mask = 0;
	gcv.foreground = colors.banner_bg;		mask |= GCForeground;
	gcv.background = colors.banner_fg;		mask |= GCBackground;
	gcv.line_width = 1;				mask |= GCLineWidth;
	gcv.font = font;				mask |= GCFont;
	gcs.bannerBg = XCreateGC( display, root, mask, &gcv );
      }

    mask = 0;
    gcv.foreground = colors.banner_fg;			mask |= GCForeground;
    gcv.background = colors.banner_bg;			mask |= GCBackground;
    gcv.tile = pix.select;				mask |= GCTile;
    gcv.fill_style = FillTiled;				mask |= GCFillStyle;
    gcs.select = XCreateGC( display, root, mask, &gcv );

    if( colors.border == colors.black )
	gcs.border = gcs.black;
    else if( colors.border == colors.white )
	gcs.border = gcs.white;
    else
      {
	mask = 0;
	gcv.foreground = colors.border;			mask |= GCForeground;
	gcs.border = XCreateGC( display, root, mask, &gcv );
      }

    mask = 0;
    gcv.foreground = (colors.banner_fg | colors.banner_bg);
							mask |= GCForeground;
    gcv.function = GXinvert;				mask |= GCFunction;
    gcv.plane_mask = (colors.banner_fg ^ colors.banner_bg);
							mask |= GCPlaneMask;
    gcv.line_width = 0;					mask |= GCLineWidth;
    gcv.subwindow_mode = IncludeInferiors;	      mask |= GCSubwindowMode;
    gcs.invert = XCreateGC( display, root, mask, &gcv );
  }
