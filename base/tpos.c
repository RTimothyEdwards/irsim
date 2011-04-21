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
#include "defs.h"
#include "net.h"
#include "globals.h"

#define	HASHSIZE	1021
#define	HN1		1103515245
#define	HN2		12345


public	int	txt_coords = 0;		/* # of trans. with coordinates */

private tptr	tpostbl[ HASHSIZE ];	/* hash table of trans position */

private	tptr	other_t = NULL;		/* transistors without coords */
private struct Trans  OtherT;


#define	UN( N )		( (Ulong) ( N ) )
#define	HashPos( X, Y )	( UN( UN( X ) * HN1 + UN( Y ) + HN2 ) % HASHSIZE )


public void EnterPos( tran, is_pos )
  tptr  tran;
  int   is_pos;
  {
    long  n;

    if( is_pos )
      {
	n = HashPos( tran->x.pos, tran->y.pos );

	tran->tlink = tpostbl[n];
	tpostbl[n] = tran;
	txt_coords ++;
      }
    else
      {
	if( other_t == NULL )
	    other_t = OtherT.x.ptr = OtherT.y.ptr = &OtherT;
	tran->y.ptr = other_t;
	tran->x.ptr = other_t->x.ptr;
	other_t->x.ptr->y.ptr = tran;
	other_t->y.ptr = tran;
	tran->tlink = tran;
      }
  }


public tptr FindTxtorPos( x, y )
  register long  x, y;
  {
    register tptr  t;
    long           n;

    n = HashPos( x, y );

    for( t = tpostbl[n]; t != NULL; t = t->tlink )
      {
	if( t->x.pos == x and t->y.pos == y )
	    return( t );
      }
    return( NULL );
  }


public void DeleteTxtorPos( tran )
  tptr  tran;
  {
    register tptr  *t;
    long           n;

    n = HashPos( tran->x.pos, tran->y.pos );

    for( t = &(tpostbl[n]); *t != NULL; t = &((*t)->tlink) )
      {
	if( *t == tran )
	  {
	    *t = tran->tlink;
	    tran->tlink = tran;
	    txt_coords --;
	    break;
	  }
      }
  }


public nptr FindNode_TxtorPos( s )
  char  *s;
  {
    long  x, y;
    tptr  t;

    if( sscanf( &s[3], "%ld,%ld", &x, &y ) != 2 )
	return( NULL );

    if( (t = FindTxtorPos( x, y )) == NULL )
	return( NULL );

    switch( s[2] )
      {
	case 'g': return( t->gate );
	case 'd': return( t->drain );
	case 's': return( t->source );
      }
    return( NULL );
  }


public void walk_trans( func, arg )
  void  (*func)();
  char  *arg;
  {
    register int   index;
    register tptr  t;

    for( index = 0; index < HASHSIZE; index++ )
      {
	for( t = tpostbl[ index ]; t != NULL; t = t->tlink )
	    (*func)( t, arg );
      }

    if( other_t != NULL )
      {
	for( t = other_t->x.ptr; t != other_t; t = t->x.ptr )
	    (*func)( t, arg );
      }
  }
