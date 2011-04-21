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

#include "net_macros.h"


typedef struct
  {
    float  dynres[ 2 ];
    float  rstatic;
  } TranResist;


public	int     stack_txtors = FALSE;
private	double  min_cap_ratio = 0;		/* for safe charge sharing */
private	int     nmerged[ NTTYPES ];		/* number of txtors merged */


private void calc_ratio()
  {
    double  minv;

    minv = 1.0 - HIGHTHRESH;
    if( LOWTHRESH < minv )
	minv = LOWTHRESH;
    min_cap_ratio = (1.0 - minv) / minv;
  }


public void pStackedTxtors()
  {
    int   i, any;

    if( not stack_txtors )
	return;

    lprintf( stdout, "stacked transistors:" );
    for( any = FALSE, i = 0; i < NTTYPES; i++ )
	if( nmerged[i] != 0 )
	  {
	    lprintf( stdout, " %s=%d", ttype[i], nmerged[i] );
	    any = TRUE;
	  }
    lprintf( stdout, "%s", (any) ? "\n" : " none\n" );
  }


private int CanMerge( n )
  nptr  n;
  {
    register lptr l;
    register int  type;

    if( (l = n->nterm) == NULL or l->next == NULL )
	return( FALSE );
    type = l->xtor->ttype & ~(GATELIST | ORED);
    l = l->next;
    if( l->next == NULL and type == (l->xtor->ttype & ~(GATELIST | ORED)) )
	return( TRUE );
    return( FALSE );
  }


#define	NodeQualifies( ND )		\
    ( ((ND)->ngate == NULL) and CanMerge( ND ) )



#define	CSproblem( ND, C_SHARE, C_ADJ )			\
    ( ((ND)->nflags & POWER_RAIL) == 0 and		\
      ((ND)->ncap - C_ADJ) <= min_cap_ratio * (C_SHARE) )



#define	other_tran( T, N )					\
  {								\
    register lptr  l;						\
								\
    if( (l = (N)->nterm)->xtor == (T) )				\
	l = l->next;						\
    T = l->xtor;						\
  }								\


public double StackCap( t )
  register tptr  t;
  {
    register nptr  n;
    double         cap;

    cap = 0.0;
    n = (t->source->nflags & MERGED) ? t->drain : t->source;
    t = (tptr) t->gate;
    do
      {
	n = other_node( t, n );
	cap += n->ncap;
	t = t->scache.t;
      }
    while( t->scache.t != NULL );
    return( cap );
  }


#define	GetAnchor( t, n, SHARECAP )				\
  {								\
    n = other_node( t, n );					\
    while( NodeQualifies( n ) )					\
      {								\
	n->nflags &= ~VISITED;					\
	SHARECAP += n->ncap;					\
	other_tran( t, n );					\
	n = other_node( t, n );					\
      }								\
    n->nflags &= ~VISITED;					\
  }								\


public void make_stacks( nlist )
  nptr  nlist;
  {
    register nptr  nd;
    register tptr  t1, t2, stack;
    register nptr  n1, n2;
    register int   tcount;
    double         Cshare, C1, C2;
    
    if( not stack_txtors )
	return;

    if( min_cap_ratio == 0 )
	calc_ratio();
    for( nd = nlist; nd != NULL; nd->nflags &= ~VISITED, nd = nd->n.next )
      {
	if( (nd->nflags & VISITED) == 0 or NodeQualifies( nd ) == FALSE )
	    continue;

	n1 = n2 = nd;
	t1 = nd->nterm->xtor;
	t2 = nd->nterm->next->xtor;

	Cshare = nd->ncap;
	C1 = C2 = 0.0;

	GetAnchor( t1, n1, Cshare );
	GetAnchor( t2, n2, Cshare );

	if( t1->ttype & GATELIST )
	  {
	    C1 = StackCap( t1 ) / 2.0;
	    Cshare += C1;
	  }

	if( t2->ttype & GATELIST )
	  {
	    C2 = StackCap( t2 ) / 2.0;
	    Cshare += C2;
	  }

	if( (n1 == n2) or (n1->nflags & n2->nflags & POWER_RAIL)
	  or CSproblem( n1, Cshare, C1 ) or CSproblem( n2, Cshare, C2 ) )
	    continue;

	if( n1->nflags & POWER_RAIL )
	  {
	    SWAP_NODES( n1, n2 );
	    stack = t1; t1 = t2; t2 = stack;	/* swap t1 & t2 */
	  }

	NEW_TRANS( stack );
	stack->r = (Resists *) Falloc( sizeof( TranResist ), 1 );
	stack->gate = (t1->ttype & GATELIST) ? t1->gate : (nptr) t1;
	stack->r->rstatic = stack->r->dynhigh = stack->r->dynlow = 0.0;
	stack->ttype = (t1->ttype & ~ORED) | GATELIST;
	stack->state = (t1->ttype & ALWAYSON) ? WEAK : UNKNOWN;
	stack->tflags = 0;
	stack->source = n1;
	stack->drain = n2;

	Cshare /= 2.0;
	n1->ncap += Cshare;
	n2->ncap += Cshare;
	REPLACE( n1->nterm, t1, stack );
	REPLACE( n2->nterm, t2, stack );

	for( tcount = 0;;)
	  {
	    n1 = other_node( t1, n1 );

	    stack->r->rstatic += t1->r->rstatic;
	    stack->r->dynhigh += t1->r->dynhigh;
	    stack->r->dynlow += t1->r->dynlow;

	    if( t1->ttype & GATELIST )
	      {
		t2 = (tptr) t1->gate;
		do
		  {
		    REPLACE( t2->gate->ngate, t2, stack );
		    t2->dcache.t = stack;
		    if( t2->scache.t == NULL )
		        break;
		    t2 = t2->scache.t;
		  }
		while( TRUE );

		C1 = StackCap( t1 ) / 2.0;
		t1->source->ncap -= C1;
		t1->drain->ncap -= C1;

		Ffree( t1->r, sizeof( TranResist ) );
		FREE_TRANS( t1 );
	      }
	    else
	      {
		tcount++;
		t1->ttype |= STACKED;
		t1->dcache.t = stack;
		REPLACE( t1->gate->ngate, t1, stack );
		t2 = t1;
	      }

	    if( n1 == n2 )
		break;

	    other_tran( t1, n1 );
	    t2->scache.t = (t1->ttype & GATELIST) ? (tptr) t1->gate : t1;

	    n1->nterm->next->next = freeLinks;
	    freeLinks = n1->nterm;
	    n1->nterm = NULL;
	    n1->t.tran = stack;
	    n1->nflags |= MERGED;
	    if( n1->curr != &(n1->head) )
		FreeHistList( n1 );
	  }
	t2->scache.t = NULL;
	nmerged[ BASETYPE( t2->ttype ) ] += tcount;
      }
  }


public void DestroyStack( stack )
  register tptr stack;
  {
    register tptr  t;
    register nptr  n;
    register int   tcount;
    double         cap;

    cap = 0.0;
    tcount = 0;
    t = (tptr) stack->gate;
    n = stack->source;
    REPLACE( n->nterm, stack, t );
    n = NULL;
    do
      {
	if( n == NULL )
	    n = stack->source;
	else
	  {
	    n->nflags &= ~MERGED;
	    n->t.cause = NULL;
	    cap += n->ncap;
	    CONNECT( n->nterm, t );
	  }
	REPLACE( t->gate->ngate, stack, t );
	t->ttype &= ~STACKED;
	tcount++;

	n = other_node( t, n );
	if( t->scache.t == NULL )
	    break;

	n->nflags &= ~MERGED;
	n->t.cause = NULL;
	CONNECT( n->nterm, t );

	t = t->scache.t;
      }
    while( TRUE );

    REPLACE( n->nterm, stack, t );

    cap /= 2.0;
    stack->source->ncap -= cap;		/* re-adjust capacitance */
    stack->drain->ncap -= cap;

    nmerged[ BASETYPE( stack->ttype ) ] -= tcount;

    Ffree( stack->r, sizeof( TranResist ) );
    FREE_TRANS( stack );
  }
