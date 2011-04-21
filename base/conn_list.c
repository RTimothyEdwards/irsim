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
#include "net_macros.h"
#include "ASSERT.h"
#include "globals.h"

public	tptr  parallel_xtors[ MAX_PARALLEL ];

/*
 * Build a linked-list of nodes (using nlink entry in Node structure)
 * which are electrically connected to node 'n'.  No special order
 * is required so tree walk is performed non-recursively by doing a
 * breath-first traversal.  The value caches for each transistor we
 * come across are reset here.  Loops are broken at an arbitrary point
 * and parallel transistors are identified.
 */
public void BuildConnList( n )
  register nptr  n;
  {
    register nptr  next, this, other;
    register tptr  t;
    register lptr  l;
    int            n_par = 0;

    n->nflags &= ~VISITED;
    withdriven = FALSE;

    next = this = n->nlink = n;
    do
      {
	for( l = this->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( t->state == OFF )
		continue;
	    if( t->tflags & CROSSED )	/* Each transistor is crossed twice */
	      {
		t->tflags &= ~CROSSED;
		continue;
	      }
	    t->scache.r = t->dcache.r = NULL;

	    other = other_node( t, this );

	    if( other->nflags & INPUT )
	      {
		withdriven = TRUE;
		continue;
	      }

	    t->tflags |= CROSSED;		/* Crossing trans 1st time */

	    if( other->nlink == NULL )		/* New node in this stage */
	      {
		other->nflags &= ~VISITED;
		other->nlink = n;
		next->nlink = other;
		next = other;
		other->n.tran = t;		/* we reach other through t */
	      }
	    else if( model_num != LIN_MODEL )
		continue;
	    else if( hash_terms( other->n.tran ) == hash_terms( t ) )
	      {					    /* parallel transistors */
		register tptr  tran = other->n.tran;

		if( tran->tflags & PARALLEL )
		    t->dcache.t = par_list( tran );
		else
		  {
		    if( n_par >= MAX_PARALLEL )
		      {
			WarnTooManyParallel( this->nname, other->nname );
			t->tflags |= PBROKEN;		/* simply ignore it */
			continue;
		      }
		    tran->n_par = n_par++;
		    tran->tflags |= PARALLEL;
		  }
		par_list( tran ) = t;
		t->tflags |= PBROKEN;
	      }
	    else
	      {					/* we have a loop, break it */
		t->tflags |= BROKEN;
	      }
	  }
      }
    while( (this = this->nlink) != n );

    next->nlink = NULL;			/* terminate connection list */
  }


public void WarnTooManyParallel( s1, s2 )
  char  *s1, *s2;
  {
    static  int  did_it = FALSE;

    if( did_it )
	return;
    lprintf( stderr,
      "There are too many transistors in parallel (> %d)\n", MAX_PARALLEL );
    lprintf( stderr,
      "Simulation results may be inaccurate, to fix this you may have to\n" );
    lprintf( stderr,
      "increase this limit in '%s'.\n", __FILE__ );
    lprintf( stderr,
      "Note: This condition often occurs when Vdd or Gnd are not connected\n" );
    lprintf( stderr,
      "      to all cells.  Check the vicinity of the following 2 nodes:\n" );
    lprintf( stderr, "      %s\n      %s\n", s1, s2 );
    did_it = TRUE;
  }
