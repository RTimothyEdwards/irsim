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


#define	VLine( WIND, X, BOT, TOP, GCX )					\
  XFillRectangle( display, WIND, GCX, X, TOP, 1, (BOT) - (TOP) + 1 )

#define	HLine( WIND, LEFT, RIGHT, Y, GCX )				\
  XFillRectangle( display, WIND, GCX, LEFT, Y, (RIGHT) - (LEFT) + 1, 1 )

#define FillAREA( WIND, X, Y, WIDTH, HEIGHT, GCX )			\
    XFillRectangle( display, WIND, GCX, X, Y, WIDTH, HEIGHT )

#define	InvAREA( WIND, X, Y, WIDTH, HEIGHT )				\
    XFillRectangle( display, WIND, gcs.inv, X, Y, WIDTH, HEIGHT )

#define	BWID( Bx )		( (Bx).right - (Bx).left + 1 )
#define	BHIG( Bx )		( (Bx).bot - (Bx).top + 1 )

#define FillBox( WIND, B, GCX )						\
    XFillRectangle( display, WIND, GCX, B.left, B.top, BWID(B), BHIG(B) )

#define InvBox( W, B )						\
 XFillRectangle( display, W, gcs.invert, (B).left, (B).top, BWID(B), BHIG(B) )

#define	OutlineBox( WIND, B, GCX )					\
 XDrawRectangle( display, WIND, GCX, (B).left, (B).top, BWID(B)-1, BHIG(B)-1 )

#define StrLeft( WIND, S, LEN, LEFT, BOT, GCX )				\
    XDrawImageString( display, WIND, GCX, LEFT, (BOT) - descent, S, LEN )

#define StrRight( WIND, S, LEN, RIGHT, BOT, GCX )			\
    XDrawImageString( display, WIND, GCX,				\
     (RIGHT) - (LEN) * CHARWIDTH +1, (BOT) - descent, S, LEN )

#define StrCenter( WIND, S, LEN, LEFT, RIGHT, BOT, GCX )		\
  XDrawImageString( display, WIND, GCX,					\
  ((RIGHT) + (LEFT) - CHARWIDTH * (LEN)) / 2, (BOT) - descent, S, LEN )

