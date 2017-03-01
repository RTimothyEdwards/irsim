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
#include <stdlib.h>
#include <string.h>
#include <math.h>    /* **mdg** */
#include <time.h>
#include <sys/types.h>

#include "ana.h"
#include "ana_glob.h"

#define	DATE_LEN	25	/* length of ascii date returned by ctime */

#ifdef TCL_IRSIM

public int    psBanner = TRUE;
public int    psLegend = FALSE;
public int    psTimes = TRUE;
public int    psOutline = TRUE;

#else

private int    psBanner = TRUE;
private int    psLegend = FALSE;
private int    psTimes = TRUE;
private int    psOutline = TRUE;

#endif /* TCL_IRSIM */

private void DrawOutline(), PrintNames(), PrintTraces();
private	void PrintSignal(), PrintVector(), PrintTimes(), PrintLegend();
private void WritePreamble();

public void WritePSfile();

#ifndef TCL_IRSIM

public void SetPSParms( m )
  MenuItem *m;
  {
    int  *parm;
    char *s = m->str;

    switch( s[1] )
      {
	case 'b' :
	    psBanner ^= TRUE;
	    break;
	case 'l' :
	    psLegend ^= TRUE;
	    break;
	case 't' :
	    psTimes ^= TRUE;
	    break;
	case 'o' :
	    psOutline ^= TRUE;
	    break;
      }
    if (m->mark == MENU_UNMARK)
	m->mark = MENU_MARK;
    else
	m->mark = MENU_UNMARK;
  }

#endif /* !TCL_IRSIM */

typedef enum { black, white, gray, xpat } pattern;
private	char      fname[256] = "";
private	FILE      *psout;
private	pattern   currPat;


public void printPS( s )
  char *s;
  {
    if( traces.disp == 0 or tims.first >= tims.last )
      {
	PRINT( "\nThere's nothing to print" );
	XBell( display, 0 );
	return;
      }

#ifdef TCL_IRSIM

    if ((s == NULL) && (banner != NULL)) {
	(void) strncpy( fname, banner, bannerLen );
	fname[bannerLen] = '\0';
    }
    else if (s != NULL) {
	strcpy(fname, s);
    }

    if ((banner == NULL) && psBanner) {
	banner = strdup(fname);
	bannerLen = strlen(banner);
    }

    if (!strstr(fname, ".ps"))
	strcat(fname, ".ps" );

    WritePSfile(fname);

#else

    if( *fname == '\0' )
      {
	(void) strncpy( fname, banner, bannerLen );
	fname[bannerLen] = '\0';
	strcat(fname, ".ps" );
      }
    PRINTF( "\nEnter filename (%s)", fname );
    Query( " > ", WritePSfile );

#endif

  }


public void WritePSfile( psfname )
  char  *psfname;
  {
    char    *date;
    time_t  theTime, time();
    int     page = 1;

    if( psfname == NULL )
	return;

    if( *psfname == '\0' )
	psfname = fname;
    else if (fname != psfname)
	(void) strcpy( fname, psfname );

    if( (psout = fopen( psfname, "w" )) == NULL )
      {
	PRINTF( "\ncan't open '%s' for output", psfname );
	return;
      }
    PRINTF( "\nWriting %s...", psfname );

    if (window != 0) {
	XDefineCursor( display, window, cursors.timer );
	XFlush( display );
    }

    currPat = black;
    WritePreamble();
    theTime = time( 0 );
    date = ctime( &theTime );

    fprintf( psout, "%%%%Page: 1 %d\n", page++);
    fprintf( psout, "%%%%PageOrientation: Landscape\n");
    fprintf( psout, "MSAVE\n" );
    DrawOutline( date );
    if( psTimes )
	PrintTimes( tims.start, tims.end );
    PrintNames();
    PrintTraces( tims.start, min( tims.end, tims.last ) );
    fprintf( psout, "showpage MRESTORE\n" );

    if( psLegend )
      {
	fprintf( psout, "%%%%Page: 1 %d\n", page++);
	fprintf( psout, "MSAVE\n" );
	DrawOutline( date );
	PrintLegend();
	fprintf( psout, "showpage MRESTORE\n" );
      }

    fprintf( psout, "%%%%EOF\n");
    (void) fclose( psout );
    PRINT( "done" );

    if (window != 0) {
	XDefineCursor( display, window, cursors.deflt );
    }
  }


	/* Macros to Convert from screen to PostScript units */

#define	PS_UNITS	72			/* PostScript unit (1/inch) */
#define	ps_YSIZE	(15 * PS_UNITS / 2)	/* y page size = 7.5 inch */
#define	ps_XSIZE	(10 * PS_UNITS)		/* x page size = 10 inch */
#define	XMARGIN		(PS_UNITS / 2)		/* margins 1/2 inch */
#define	YMARGIN		(PS_UNITS / 2)
#define	FSIZE		9			/* default font size */
#define	MINFSIZE	4			/* minimum font size */
#define	LEGFSIZE	7			/* legend font size */

#define	YTRACE		(traceBox.bot - traceBox.top + 1)
#define	TIMES_HEIGHT	20
#define	BANNER_HEIGHT	15
#define	psHEIGHT	( ps_YSIZE - TIMES_HEIGHT - BANNER_HEIGHT )

		/* scale conversion macros */
#define	PSX( x )	( (x) * (ps_XSIZE - 2) / traceBox.right )
#define	PSY( y )	( (YWINDOWSIZE - (y)) * psHEIGHT / YTRACE )
#define	PS( x, y )	PSX( x ), PSY( y )

#define	BANNER_BOT	( psHEIGHT + PSY( traceBox.bot ) )
#define BANNER_TOP	( BANNER_BOT + BANNER_HEIGHT )
#define BANNER_MID	( (BANNER_TOP + BANNER_BOT + 1) / 2 )

#define	TIMES_TOP	PSY( traceBox.bot )
#define	TIMES_BOT	(TIMES_TOP - TIMES_HEIGHT )


/**************************************************************************/
/*	Redefine the graphics operators to generate the PostScript code   */
/**************************************************************************/

#define	BLACK		1		/* anything will do */
#define	WHITE		0

#define	VLine( X, BOT, TOP, COLOR )					\
    fprintf( psout, "%d %d %d VL\n", PS( X, BOT), PSY( TOP ) );

#define	HLine( LEFT, RIGHT, Y, COLOR )					\
    fprintf( psout, "%d %d %d HL\n", PS( LEFT, Y ), PSX( RIGHT ) );

#define	Line( X1, Y1, X2, Y2 )						\
    fprintf( psout, "%d %d %d %d L\n", PS( X1, Y1 ), PS( X2, Y2 ) );

#define FillAREA( X, Y, WIDTH, HEIGHT, COLOR )				\
  {									\
    pattern  opatt = SetPattern( COLOR );				\
    fprintf( psout, "%d %d %d %d BOX fill\n", PS( X, Y ),		\
      PS( (X) + (WIDTH) - 1, (Y) + (HEIGHT) - 1 ) );			\
    (void) SetPattern( opatt );						\
  }									\

#define RECT( X, Y, X2, Y2, OP )					\
    fprintf( psout, "%d %d %d %d BOX %s\n", X, Y, X2, Y2, #OP );

#define StrLeft( S, LEN, LEFT, BOT, FG, BG )				\
  {									\
    psString( S, LEN );							\
    fprintf( psout, "%d %d SL\n", PS( LEFT, BOT ) );			\
  }									\


#define StrRight( S, LEN, RIGHT, MID, FG, BG )				\
  {									\
    psString( S, LEN );							\
    fprintf( psout, "%d %d SR\n", PS( RIGHT, MID ) );			\
  }									\


#define StrCenter( S, LEN, LEFT, RIGHT, MID, FG, BG )			\
  {									\
    psString( S, LEN );							\
    fprintf( psout, "%d %d %d SC\n", PSX( LEFT ), PS( RIGHT, MID ) );	\
  }									\


/**************************************************************************/

/*
 * Write out an string, prepending parens with '\'
 */
private void psString( s, len )
  char  *s;
  int   len;
  {
    putc( '(', psout );
    while( *s != '\0' and len != 0 )
      {
	if( (*s == '(' ) || (*s == ')') )
	    putc( '\\', psout );
        putc( *s, psout );
	s++;
	len--;
      }
    putc( ')', psout );
  }


private pattern SetPattern( patt )
  pattern  patt;
  {
    float    grayscale;
    pattern  ret;

    if( patt == white )
	grayscale = 1.0;
    else if( patt == gray )
	grayscale = 0.82;
    else if( patt == xpat )
	grayscale = 0.68;
    else /* black */
	grayscale = 0.0;
    fprintf( psout, "%g setgray\n", grayscale );
    ret = currPat;
    currPat = patt;
    return( ret );
  }


private void DrawOutline( date )
  char  *date;
  {
    pattern  opatt;
    char     info[ 256 ];

    if( psBanner )
      {
	opatt = SetPattern( gray );
	RECT( 0, BANNER_BOT, ps_XSIZE, BANNER_TOP, fill );
	(void) SetPattern( opatt );
	RECT( 0, BANNER_BOT, ps_XSIZE, BANNER_TOP, stroke );
	psString( banner, bannerLen );
	fprintf( psout, "%d %d FSIZE 2 div sub SL\n", 6, BANNER_MID );
	if( strncmp( banner, fname, bannerLen ) != 0 )
	  {
	    (void) sprintf( info, "(%s)  %s", fname, date );
	    psString( info, bannerLen + DATE_LEN );
	  }
	else
	    psString( date, DATE_LEN );
	fprintf( psout, " %d %d SR\n", PSX( traceBox.right - 4 ), BANNER_MID);
      }
    if( psOutline )
	RECT( 0, TIMES_BOT, ps_XSIZE, BANNER_TOP, stroke )
  }



	/* FactorTbl: time grid at multiples of 1/FactorTbl => .1 .5 .25 1 */
private	int    FactorTbl[] = { 10, 2, 4, 1};

private void PrintTimes( t1, t2 )
  TimeType t1, t2;
  {
    TimeType  step, half_step, i;
    int       x;
    char      s[ 25 ];

    for( step = tims.steps, i = 1; step >= 10; step /= 10, i *= 10 );
    for( x = 0; ; x++ )
      {
	int nsteps = tims.steps /( i / FactorTbl[x] );
	if( nsteps > 5 and nsteps < 15 )
	    break;
      }
    step = i / FactorTbl[x];
    half_step = ( step > 2 ) ? (step / 2) - 1 : 2;

    fprintf( psout, "0 setlinewidth [1 3] 0 setdash /svfnt currentfont def\n");
    fprintf( psout, "theFont 0.7 FSIZE mul scalefont setfont\n" );
    psString( "time (ns)", 30 );
    fprintf( psout, " %d %d %d SC\n", 0, PSX(traceBox.left) - 1, TIMES_TOP );

    i = ((t1 + step - 1) / step) * step;
    if( i != t1 )
      {
	x = TimeToX( t1 );
	VLine( x, traceBox.top, traceBox.bot + 3, BLACK );
	if( i - t1 >= half_step )
	  {
	    (void) sprintf( s, "%.2f", d2ns( t1 ) );
	    x = 2 * PSX( x );
	    fprintf( psout, "(%s) 0 %d %d SC\n", s, x, (TIMES_BOT+TIMES_TOP)/2 );
	  }
      }

    while( i <= t2 )
      {
	x = TimeToX( i );
	VLine( x, traceBox.top, traceBox.bot + 3, BLACK );
	(void) sprintf( s, "%.2f", d2ns( i ) );
	x = 2 * PSX( x );
	fprintf( psout, "(%s) 0 %d %d SC\n", s, x, (TIMES_BOT+TIMES_TOP)/2 );
	i += step;
      }

    if( i > t2 and (t2 - i + step) >= half_step )
      {
	x = TimeToX( t2 );
	VLine( x, traceBox.top, traceBox.bot + 3, BLACK );
	(void) sprintf( s, "%.2f", d2ns( t2 ) );
	x = PSX( x );
	fprintf( psout, "(%s) %d %d SR\n", s, x, (TIMES_BOT+TIMES_TOP)/2 );
      }

    HLine( traceBox.left - 2, traceBox.right, traceBox.bot, BLACK );
    fprintf( psout, "0.6 setlinewidth [] 0 setdash svfnt setfont\n" );
  }


private void PrintNames()
  {
    Coord         x, y;
    TraceEnt      *t;
    int           i;

#ifdef TCL_IRSIM
    x = traceBox.right - 2;
#else
	x = namesBox.right - 2;
#endif
    for( t = traces.first, i = traces.disp; i != 0; i--, t = t->next )
      {
	y = (t->bot + t->top) / 2;
	StrRight( t->name, t->len, x, y, BLACK, WHITE );
      }
  }


private void PrintTraces( t1, t2 )
  TimeType  t1,t2;
  {
    Trptr  t;
    int    nt;

    for( t = traces.first, nt = traces.disp; nt != 0; nt--, t = t->next )
      {
	if( IsVector( t ) )
	    PrintVector( t, t1, t2 );
	else
	    PrintSignal( t, t1, t2 );
      }
  }


private void PrintSignal( t, t1, t2 )
  Trptr              t;
  register TimeType  t1, t2;
  {
    register hptr  h;
    register int   val, change;
    int            x1, x2;

    if( t1 >= tims.last )
	return;

    h = t->cache[0].wind;

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
		HLine( x1, x2, t->bot, WHITE );
		break;
	    case HIGH :
		HLine( x1, x2, t->top, WHITE );
		break;
	    case X :
		FillAREA( x1, t->top, x2 - x1 + 1, t->bot - t->top + 1, xpat );
		if( x1 > traceBox.left + 1 )
		    VLine( x1, t->bot, t->top, WHITE );
		break;
	  }
	if( change )
	    VLine( x2, t->bot, t->top, WHITE );
	x1 = x2;
      }
  }


private void PrintVector( t, t1, t2 )
  register Trptr     t;
  register TimeType  t1, t2;
  {
    hptr      *start, *changes;
    TimeType  firstChange;
    int       x1, x2, xx, mid, nbits, strlen;
    
    if( t1 >= tims.last )
	return;

    nbits = t->n.vec->nbits;
    start = tmpHBuff;
    changes = &(tmpHBuff[ nbits ]);
    if (t->bdigit == 5)
       strlen = (nbits + 2) / 3;
    else if (t->bdigit == 6)
       strlen = 1 + ((nbits + 1) / 3);
    else
       strlen = (nbits + t->bdigit - 1) / t->bdigit;

      {
	register hptr      h, *s;
	register int       n, val;
	register hptr      *ch = changes;
	register TimeType  tm = tims.end;

	s = start;			/* initialize start array */
	firstChange = tims.start;
	for( n = nbits - 1; n >= 0; n-- )
	  {
	    h = s[n] = t->cache[n].wind;
	    val = h->val;
	    while( h->time < tm and h->val == val )
		NEXTH( h, h );
	    ch[n] = h;
	  }
      }

    mid = (t->top + t->bot) / 2;
    x2 = TimeToX( t2 );
    x1 = TimeToX( firstChange );

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
	    x2 = TimeToX( t1 );
	    if( x2 - x1 > 3 )
	      {
		HLine( x1 + 2, x2 - 2, t->top, WHITE );
		HLine( x1 + 2, x2 - 2, t->bot, WHITE );
		xx = 2;
	      }
	    else
		xx = (x2 - x1 - 2);

	    VLine( x2, t->bot - 2, t->top + 2, WHITE );
	    if( x2 > traceBox.left + 1 )
	      {
		Line( x2 - xx, t->top, x2, t->top + 2 );
		Line( x2 - xx, t->bot, x2, t->bot - 2 );
	      }
	    if( x2 < traceBox.right - 1 )
	      {
		Line( x2, t->top + 2, x2 + 2, t->top );
		Line( x2, t->bot - 2, x2 + 2, t->bot );
	      }
	  }
	else				/* change after t2 */
	  {
	    register TimeType  tm;

	    tm = min( t1, min( tims.end, tims.last ) );
	    x2 = TimeToX( tm );
	    HLine( x1 + 2, x2, t->top, WHITE );
	    HLine( x1 + 2, x2, t->bot, WHITE );
	  }

	 {
	    char  *str;
	    str = HistToStr( start, nbits, t->bdigit, 1 );
	    StrCenter( str, strlen, x1, x2, mid, WHITE, BLACK );
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


private void PrintLegend()
  {
    int       i, nbits;
    TraceEnt  *t;

#ifdef TCL_IRSIM
    fprintf( psout, "/GX %d  def\n", PSX( traceBox.left + 25 ) );
#else
    fprintf( psout, "/GX %d  def\n", PSX( namesBox.right + 25 ) );
#endif
    fprintf( psout, "/GY %d  def\n", BANNER_BOT - 2 * (FSIZE + 3)  );
    fprintf( psout, "(Legend:) 4 %d SL\n", BANNER_BOT - FSIZE - 2 );
    fprintf( psout, "/FSIZE %d def FSIZE SF\n", LEGFSIZE );

    for( i = traces.disp, t = traces.first; i != 0; i--, t = t->next )
      {
	if( t->vector )
	  {
	    for( nbits = t->n.vec->nbits - 1; nbits >= 0; nbits-- )
		psString( t->n.vec->nodes[nbits]->nname, 1000 );
	    nbits = t->n.vec->nbits;
	  }
	else
	  {
	    psString( t->n.nd->nname, 1000 );
	    nbits = 1;
	  }
	psString( t->name, 1000 );
	fprintf( psout, "%d LE\n", nbits );
      }
  }



void WritePreamble()
  {
    int pages;
    static char defs[] = "%!\n\
/MSAVE { /mStat save def } def\n\
/MRESTORE { mStat restore } def\n\
/SET { exch def } def\n\
/SF { /wi SET theFont [wi 0 0 FSIZE 0 0] makefont setfont } def\n\
/L { newpath moveto lineto stroke } def\n\
/VL { 2 index exch L } def\n\
/HL { 1 index L } def\n\
/BOX { /@yb SET /@xb SET /@yt SET /@xt SET newpath\n\
 @xb @yb moveto @xb @yt lineto @xt @yt lineto @xt @yb lineto closepath } def\n\
/SL { moveto show } def\n\
/SR { exch dup /@xr SET 2 index stringwidth pop sub dup\n\
 0 le { pop 0 @xr 3 -1 roll SC } { exch FSIZE 2 div sub moveto show }\n\
 ifelse } def\n\
/SCP { /@s SET @s stringwidth pop /@sw SET @sw 2 div sub\n\
 dup @sw add maxX gt { pop maxX @sw sub } if exch moveto @s show } def\n\
/SC { FSIZE 2 div sub /@y SET /@x2 SET /@x1 SET /@s SET\n\
 /@l @x2 @x1 sub def /@x @x1 @x2 add 2 div def\n\
 /@w @s stringwidth pop def @l @w gt\n\
 { @y @x @s SCP }\n\
 { @l FSIZE mul @w 1 add div dup MINFSIZE lt\n\
 { pop } { /svf currentfont def SF @y @x @s SCP svf setfont } ifelse }ifelse\n\
} def\n\
/LE { exch GX GY FSIZE 2 div add SR /@x GX def\n\
 { dup stringwidth pop /@w SET /@x @x FSIZE add def @x @w add\n\
 720 ge { /@x GX FSIZE add def /GY GY FSIZE 2 add sub def } if\n\
 @x GY SL /@x @x @w add def } repeat /GY GY FSIZE 4 add sub def } def\n\
";
	/* determine number of pages */
    pages = psLegend ? 2 : 1;

	/* print data and procedure definitions */
    fprintf( psout, "%%!PS-Adobe-2.0\n");
    fprintf( psout, "%%%%Pages: %d\n", pages);
    fprintf( psout, "%%%%EndComments\n");
    fprintf( psout, "%s", defs );
    fprintf( psout, "/FSIZE %d def /MINFSIZE %d def ", FSIZE, MINFSIZE );
    fprintf( psout, "/maxX %d def\n", PSX( traceBox.right ) );
	/* Switch to landscape mode */
    fprintf( psout, "%d 0 translate\n", 17 * PS_UNITS / 2 );
    fprintf( psout, "90 rotate\n" );
    fprintf( psout, "%d %d", XMARGIN, YMARGIN + TIMES_HEIGHT -
      PSY( traceBox.bot) );
    fprintf( psout, " translate\n" );
    fprintf( psout, "1 setlinecap 0.6 setlinewidth \n" );
	/* Set up text font */
    fprintf( psout,"/theFont /Helvetica findfont def FSIZE SF\n");
  }
