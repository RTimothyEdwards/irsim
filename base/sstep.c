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
#include "net.h"
#include "defs.h"
#include "globals.h"

/* event-driven switch-level simulation step Chris Terman 7/84 */

/* the following file contains most of the declarations, conversion
 * tables, etc, that depend on the particular representation chosen
 * for node values.  This info is automatically created for interval
 * value sets (see Chapter 5 of my thesis) by gentbl.c.  Except
 * for the charged_state and thev_value arrays and their associated
 * code, the rest of the code is independent of the value set details...
 */
#include "stables.c"


private	int    sc_thev();


/* tables for converting node value to corresponding charged state */
private	char    charged_state[4] = 
  {
    CL, CHCL, CHCL, CH
  };

private	char    xcharged_state[4] = 
  {
    cL, cHcL, cHcL, cH
  };

/* table for converting node value to corresponding value state */
private	char    thev_value[4] = 
  {
    DL, DHDL, DHDL, DH
  };



/* calculate new value for node and its electrical neighbors */
public void switch_model( n )
  nptr    n;
  {
    register nptr  this, next;

    nevals++;

    if( n->nflags & VISITED )
	BuildConnList( n );

	/* for each node on list we just built, recompute its value using a
	 * recursive tree walk.  If logic state of new value differs from
	 * logic state of current value, node is added to the event list (or
	 * if it's already there, just the eval field is updated).  If logic
	 * states do not differ, node's value is updated on the spot and any
	 * pending events removed.
	 */
    for( this = n; this != NULL; this = this->nlink )
      {
	register int    newval, queued;
	register evptr  e;
	register long   delta = 0;
	long            tau;

	if( this->nflags & INPUT )
	    newval = this->npot;
	else
	  {
	    newval = logic_state[sc_thev( this, this->nflags & WATCHED ? 1 : 0 )];
	    switch( newval )
	      {
		case LOW :
		    delta = this->tphl;
		    break;
		case X :
		    delta = 0;
		    break;
		case HIGH :
		    delta = this->tplh;
		    break;
	      }
	    tau = delta;
	    if( delta == 0 )	/* no zero-delay events */
		delta = 1;
	  }

	if( not (this->nflags & INPUT) )
	  {
	    /* 
	     * Check to see if this new value invalidates other events. 
	     * Since this event has newer info about the state of the network,
	     * delete transitions scheduled to come after it.
	     */
	    while( (e = this->events) != NULL and e->ntime >= cur_delta + delta )
	      {
		/* 
		 * Do not try to kick the event scheduled at cur_delta if
		 * newval is equal to this->npot because that event sets
		 * this->npot, but its consequences has not been handled yet.
		 * Besides, this event will not be queued. 
		 *
		 * However, if there is event scheduled now but driving to a 
		 * different value, then kick it becuase we will enqueue this 
		 * one, and source/drain of transistors controlled by this
		 * node will be re-evaluated. At worst, some extra computation
		 * will be carried out due to VISITED flags set previously.
		 */
/*		if( e->ntime == cur_delta and newval == this->npot ) */
		if( e->ntime == (cur_delta + delta) and e->eval == newval )
		    break;
		PuntEvent( this, e );
	      }
		/*
		 * Now see if the new value is different from the last value
		 * scheduled for the node. If there are no pending events then
		 * just use the current value.
		 */

	    if( newval != ((e == NULL) ? this->npot : e->eval) )
	      {
		queued = 1;
		enqueue_event( this, newval, delta, tau );
	      }
	    else
		queued = 0;

	    if( (this->nflags & WATCHED) and (debug & (DEBUG_DC | DEBUG_EV)) )
	      {
		lprintf( stdout, " [event %s->%c @ %.2f] ", pnode( cur_node ),
		  vchars[ cur_node->npot ], d2ns( cur_delta ) );

		lprintf( stdout, queued ? "causes transition for" : "sets" );
		lprintf( stdout, " %s: %c -> %c (delay = %2.2fns)\n",
		  pnode( this ), vchars[ this->npot ], vchars[ newval ],
		  d2ns( delta ) );
	      }
	  }
      }
	/* undo connection list */
    for( this = n; this != NULL; this = next )
      {
	next = this->nlink;
	this->nlink = NULL;
      }
  }


/* compute new value for a node using recursive tree walk.  We start off by
 * assuming that node is charged to whatever logic state it had last.  The
 * contribution of each path leading away from the node is computed
 * recursively and merged with accumulated result.  After all paths have been
 * examined, return the answer!
 *
 * Notes: The node-visited flag is set during the computation of a node's
 *	  value. This keeps the tree walk exanding outward; loops are avoided.
 *	  Since the flag is cleared at the end of the computation, all paths
 *	  through the network will eventually be explored.  (If it had been
 *	  left set, only a single path through a particular cycle would be
 *	  explored).
 *
 *        In order to speed up the computation, the result of each recursive
 *	  call is cached.  There are 2 caches associated with each transistor:
 *	  the source cache remembers the value of the subnetwork that includes
 *	  the transistor and everything attached to its drain. The drain cache
 *	  is symmetrical.  Use of these caches reduces the complexity of the
 *	  new-value computation from O(n**2) to O(n) where n is the number of
 *	  nodes in the connection list built in the new_value routine.
 */
private int sc_thev( n, level )
  register nptr  n;
  int            level;
  {
    register tptr  t;
    register lptr  l;
    register int   result;

    if( n->nflags & INPUT )
      {
	result = thev_value[n->npot];
	goto done;
      }

	/* initial values and stuff... */
    n->nflags |= VISITED;
    result = (n->ngate == NULL) ? xcharged_state[n->npot] :
				  charged_state[n->npot];

    for( l = n->nterm; l != NULL; l = l->next )
      {
	t = l->xtor;

	  /* don't bother with off transistors */
	if( t->state == OFF )
	    continue;

	/* for each non-off transistor, do nothing if node on other side has
	 * its visited flag set (this is how cycles are detected).  Otherwise
	 * check cache and use value found there, if any.  As a last resort,
	 * actually compute value for network on other side of transistor,
	 * transmit the value through the transistor, and save that result
	 * for later use.
	 */
	if( n == t->source )
	  {
	    if( not (t->drain->nflags & VISITED) )
	      {
		if( t->dcache.i == EMPTY )
		    t->dcache.i = transmit[sc_thev( t->drain, level ? level + 1 : 0 )][t->state];
		result = smerge[result][t->dcache.i];
	      }
	  }
	else
	  {
	    if( not (t->source->nflags & VISITED) )
	      {
		if( t->scache.i == EMPTY )
		    t->scache.i = transmit[sc_thev( t->source, level ? level + 1 : 0 )][t->state];
		result = smerge[result][t->scache.i];
	      }
	  }
      }

    n->nflags &= ~VISITED;

  done :
    if( (debug & (DEBUG_DC | DEBUG_TW)) and level > 0 )
      {
	register int  i;

	lprintf( stdout, "  " );
	for( i = level; --i > 0;  )
	    lprintf( stdout, " " );
	lprintf( stdout, "sc_thev(%s) = %s\n", pnode(n), node_values[result]);
      }

    return( result );
  }
