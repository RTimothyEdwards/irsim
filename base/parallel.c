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


private int    nored[ NTTYPES ];

#define	hash_terms(T)	((pointertype)((T)->source) ^ (pointertype)((T)->drain))

#define	COMBINE( R1, R2 )	( ((R1) * (R2)) / ((R1) + (R2)) )


typedef struct
  {
    float    dynres[ 2 ];
    float    rstatic;
  } TranResist;


/*
 * Run through the list of nodes, collapsing all transistors with the same
 * gate/source/drain into a compound transistor.
 * As a side effect, if stack_txtors is set clear the VISITED bit on all 
 * nodes on the list.
 */
public void make_parallel( nlist )
  register nptr  nlist;
  {
    register pointertype hval;
    register lptr   l1, l2, prev;
    register tptr   t1, t2;
    register int    type;
    register long   cl;

    cl = (stack_txtors) ? 0 : VISITED;
    for( cl = ~cl; nlist != NULL; nlist->nflags &= cl, nlist = nlist->n.next )
      {
	for( l1 = nlist->nterm; l1 != NULL; l1 = l1->next )
	  {
	    t1 = l1->xtor;
	    type = t1->ttype;
	    if( type & (GATELIST | ORED) )
		continue;	/* ORED implies processed, so skip as well */
#ifdef	USER_SUBCKT 
	    if ( type == SUBCKT ) /* dont merge subckts */
		continue;         /* we will loose the events if we 
				  merge them */
#endif

	    hval = hash_terms( t1 );
	    prev = l1;
	    for( l2 = l1->next; l2 != NULL; prev = l2, l2 = l2->next )
	      {
		t2 = l2->xtor;
		if( t1->gate != t2->gate or hash_terms( t2 ) != hval or 
		  type != (t2->ttype & ~ORED) )
		    continue;

		if( not (t1->ttype & ORED) )
		  {
		    NEW_TRANS( t2 );
		    t2->r = (Resists *) Falloc( sizeof( TranResist ), 1 );
		    t2->r->dynlow = t1->r->dynlow;
		    t2->r->dynhigh = t1->r->dynhigh;
		    t2->r->rstatic = t1->r->rstatic;
		    t2->gate = t1->gate;
		    t2->source = t1->source;
		    t2->drain = t1->drain;
		    t2->ttype = (t1->ttype & ~ORLIST) | ORED;
		    t2->state = t1->state;
		    t2->tflags = t1->tflags;
		    t2->tlink = t1;
		    t1->scache.t = NULL;
		    t1->dcache.t = t2;
		    REPLACE( t1->gate->ngate, t1, t2 );
		    REPLACE( t1->source->nterm, t1, t2 );
		    REPLACE( t1->drain->nterm, t1, t2 );
		    t1->ttype |= ORLIST;
		    t1 = t2;
		    t2 = l2->xtor;
		    nored[ BASETYPE( t1->ttype ) ]++;
		  }

		  {
		    register Resists  *r1 = t1->r, *r2 = t2->r;

		    r1->rstatic = COMBINE( r1->rstatic, r2->rstatic );
		    r1->dynlow = COMBINE( r1->dynlow, r2->dynlow );
		    r1->dynhigh = COMBINE( r1->dynhigh, r2->dynhigh );
		  }

		DISCONNECT( t2->gate->ngate, t2 );	/* disconnect gate */
		if( t2->source == nlist )		/* disconnect term1 */
		    DISCONNECT( t2->drain->nterm, t2 )
		else
		    DISCONNECT( t2->source->nterm, t2 );

		prev->next = l2->next;			/* disconnect term2 */
		FREE_LINK( l2 );
		l2 = prev;

		if( t2->ttype & ORED )
		  {
		    register tptr  t;

		    for( t = t2->tlink; t->scache.t != NULL; t = t->scache.t )
			t->dcache.t = t1;
		    t->scache.t = t1->tlink;
		    t1->tlink = t2->tlink;

		    Ffree( t2->r, sizeof( TranResist ) );
		    FREE_TRANS( t2 );
		  }
		else
		  {
		    t2->ttype |= ORLIST;	/* mark as part of or */
		    t2->dcache.t = t1;		/* this is the real txtor */
		    t2->scache.t = t1->tlink;	/* link unto t1 list */
		    t1->tlink = t2;
		    nored[ BASETYPE( t1->ttype ) ]++;
		  }
	      }
	  }
      }
  }


public void UnParallelTrans( t )
  register tptr  t;
  {
    tptr    tor;
    double  dr;

    if( not (t->ttype & ORLIST) )
	return;				/* should never be */

    tor = t->dcache.t;
    if( tor->tlink == t )
	tor->tlink = t->scache.t;
    else
      {
	register tptr  tp;

	for( tp = tor->tlink; tp != NULL; tp = tp->scache.t )
	  {
	    if( tp->scache.t == t )
	      {
		tp->scache.t = t->scache.t;
		break;
	      }
	  }
      }

    if( tor->tlink == NULL )
      {
	REPLACE( tor->gate->ngate, tor, t );
	REPLACE( tor->source->nterm, tor, t );
	REPLACE( tor->drain->nterm, tor, t );
	Ffree( tor->r, sizeof( TranResist ) );
	FREE_TRANS( tor );
      }
    else
      {
	register Resists  *ror = tor->r, *r = t->r;

	dr = r->rstatic - ror->rstatic;
	ror->rstatic = (ror->rstatic * r->rstatic) / dr;
	dr = r->dynlow - ror->dynlow;
	ror->dynlow = (ror->dynlow * r->dynlow) / dr;
	dr = r->dynhigh - ror->dynhigh;
	ror->dynhigh = (ror->dynhigh * r->dynhigh) / dr;

	if( t->ttype & ALWAYSON )
	  {
	    CONNECT( on_trans, t );
	  }
	else
	  {
	    CONNECT( t->gate->ngate, t );
	  }
	if( not (t->source->nflags & POWER_RAIL) )
	  {
	    CONNECT( t->source->nterm, t );
	  }
	if( not (t->drain->nflags & POWER_RAIL) )
	  {
	    CONNECT( t->drain->nterm, t );
	  }
      }
    t->ttype &= ~ORLIST;
    nored[ BASETYPE( t->ttype ) ] -= 1;
  }


public void pParallelTxtors()
  {
    int  i, any;

    lprintf( stdout, "parallel txtors:" );
    for( i = any = 0; i < NTTYPES; i++ )
      {
	if( nored[i] != 0 )
	  {
	    lprintf( stdout, " %s=%d", ttype[i], nored[i] );
	    any = TRUE;
	  }
      }
    lprintf( stdout, "%s\n", (any) ? "" : "none" );
  }
