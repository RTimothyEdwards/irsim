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

#ifdef TCL_IRSIM
#include <tk.h>
#endif

#include "defs.h"
#include "rsim.h"
#include "globals.h"

/*
 * simulator can use one of several models, selectable by the user.  Which
 * model to use is kept as index into various dispatch tables, all defined
 * below.
 */
public
#define	LIN_MODEL	0
public
#define	SWT_MODEL	1
public
#define	NMODEL		2		/* number of models supported */


public void (*model_table[NMODEL])() =	/* model dispatch table */
  {
    linear_model, switch_model
  };


private void SetInputs( /* listp, val */ );
private void MarkNOinputs();

public	int	model_num = LIN_MODEL;		/* index of model in table */
public	void	(*model)() = linear_model;	/* evaluation model */
public	int	sm_stat = NORM_SIM;		/* simulation status */
public	int	treport = 0;			/* report format */

private	int	firstcall = 1;	    /* reset when calling init_vdd_gnd */

#ifdef TCL_IRSIM
extern Tcl_Interp *irsiminterp;
#endif

/*
 * find transistors with gates of VDD or GND and calculate values for source
 * and drain nodes just in case event driven calculations don't get to them.
 */
private void init_vdd_gnd()
  {
    enqueue_input( VDD_node, HIGH );
    enqueue_input( GND_node, LOW );

    firstcall = 0;		/* initialization now taken care of */
  }

/*
 * Reset the firstcall flag.  Usually after reading history dump or net state
 */
public void NoInit()
  {
    firstcall = 0;
  }


/*
 * Set the firstcall flags.  Used when moving back to time 0.
 */
public void ReInit()
  {
    firstcall = 1;
  }

/*
 * Print decay event.
 */
private void pr_decay( e )
  evptr  e;
  {
    nptr  n = e->enode;

    lprintf( stdout, " @ %.2fns %s: decay %c -> X\n",
      d2ns( e->ntime ), pnode( n ), vchars[ n->npot ] );
  }


/*
 * Print watched node event.
 */
private void pr_watched( e, n )
  evptr  e;
  nptr   n;
  {
    int   tmp;

    if( n->nflags & INPUT )
      {
	lprintf( stdout, " @ %.2fns input %s: -> %c\n",
	  d2ns( e->ntime ), pnode( n ), vchars[ e->eval ] );
	return;
      }

    tmp = (debug & DEBUG_EV) ? (REPORT_TAU | REPORT_DELAY) : treport;

    lprintf( stdout, " @ %.2fns %s: %c -> %c",
      d2ns( e->ntime ), pnode( n ), vchars[ n->npot ], vchars[ e->eval ] );

    switch( tmp & (REPORT_TAU | REPORT_DELAY) )
      {
	case 0 :
	    lprintf( stdout, "\n" );					break;
	case REPORT_TAU :
	    lprintf( stdout, " (tau=%.2fns)\n", d2ns( e->rtime ) );	break;
	case REPORT_DELAY :
	    lprintf( stdout, " (delay=%.2fns)\n", d2ns( e->delay ) );	break;
	default :
	    lprintf( stdout, " (tau=%.2fns, delay=%.2fns)\n",
	      d2ns( e->rtime ), d2ns( e->delay ) );
      }
  }

#ifdef POWER_EST
/*
 * Print capwatched node event.
 */
private void pr_capwatched( e, n )
  evptr  e;
  nptr   n;
  {
    if ( caplogfile == NULL ) 
	return;

    if( n->nflags & INPUT )
      {
	fprintf( caplogfile, "%.2f *INPUT* %s -> %c\t",
	  d2ns( e->ntime ), pnode( n ), vchars[ e->eval ] );
	fprintf( caplogfile, "%.2f %.2f %4.3f\n",
	  d2ns( e->rtime ), d2ns( e->delay ), n->ncap );
	return;
      }

    fprintf( caplogfile,"%.2f\t%s\t%c -> %c\t",
	  d2ns( e->ntime ), pnode( n ), vchars[ n->npot ], vchars[ e->eval ] );
    fprintf( caplogfile, "%.2f %.2f %4.3f\n",
	    d2ns( e->rtime ), d2ns( e->delay ), n->ncap );
    n->toggles++;
    toggled_cap += n->ncap;    /* When finished, P = toggle_cap * v*v / (2t) */
  }


/*
 * Tally cap * trans for giving stepwise power display
 */
private void acc_step_power( n )
  nptr   n;
  {
     if ( not (n->nflags & INPUT) )
	 step_cap_x_trans += n->ncap;
  }
#endif /* POWER_EST */

/*
 * Run through the event list, marking all nodes that need to be evaluated.
 */
private void MarkNodes( evlist )
  evptr  evlist;
  {
    register nptr   n;
    register tptr   t;
    register lptr   l;
    register evptr  e = evlist;
    long            all_flags = 0;
				    
    do
      {
	if (e->type == TIMED_EV)
	{
	    char *proc = (char *)e->enode;
#ifdef TCL_IRSIM
	    Tcl_Eval(irsiminterp, proc);
#else
	    /* No Tcl---evaluate procedure as a single expression */
            parse_line(proc, LSIZE);
	    if (targc != 0) exec_cmd();
#endif
	    /* Apply any node changes */
	    MarkNOinputs();			/* nodes no longer inputs */
	    SetInputs( &hinputs, HIGH );	/* HIGH inputs */
	    SetInputs( &linputs, LOW );		/* LOW inputs */
	    SetInputs( &uinputs, X );		/* X inputs */

	    /* Reschedule recurring events */

	    if (e->delay != 0)
	    {
		evptr new;

		new = EnqueueOther(TIMED_EV, cur_delta + (Ulong)e->delay);
		new->enode = e->enode;
		new->delay = e->delay;
		new->rtime = e->rtime;
	    }
	    else
		free((char *)e->enode);
	    e->enode = NULL;

	    /* Ignore the rest of this routine and continue	*/
	    /* with the next event				*/

	    e = e->flink;
	    if (e == (evptr)NULL) break;
	    continue;
	}

	n = e->enode;
	all_flags |= n->nflags;

	if( e->type == DECAY_EV and ( (treport & REPORT_DECAY) or
	  (n->nflags & (WATCHED | STOPONCHANGE)) ) )
	    pr_decay( e );
	else if( n->nflags & (WATCHED | STOPONCHANGE) )
	    pr_watched( e, n );
#ifdef POWER_EST
	else if( n->nflags & (POWWATCHED | STOPONCHANGE) )
	    pr_capwatched( e, n );
	if( pstep && (n->nflags & (POWWATCHED | STOPONCHANGE) ) )
	    acc_step_power( n );
#endif /* POWER_EST */

	n->npot = e->eval;
	
	    /* Add the new value to the history list (if they differ) */
	if( not (n->nflags & INPUT) and (n->curr->val != n->npot) )
	    AddHist( n, n->npot, 0, e->ntime, (long) e->delay, (long) e->rtime );

	if ( n->awpending != NULL && (POT2MASK(n->npot) & n->awmask) )
	    evalAssertWhen(n);

#ifdef STATS
	{ extern int ev_hgm; if( ev_hgm ) IncHistEvCnt( -1 ); }
#endif /* STATS */

	    /* for each transistor controlled by event node, mark 
	     * source and drain nodes as needing recomputation.
	     *
	     * Added MOSSIMs speed up by first checking if the 
	     * node needs to be rechecked  mh
	     *
	     * Fixed it so nodes with pending events also get
	     * re_evaluated. Kevin Karplus
	     */
	for( l = n->ngate; l != NULL; l = l->next )
	  {
#ifdef notdef
	    char  oldstate;

	    t = l->xtor;
		/*
		 * Some of these optimizations only work right when the
		 * stage at the src/drn contains no loops.  For example,
		 * when a transistor turns off and the src=1 and drn=0,
		 * the transistor may break a previous loop causing the
		 * src or drn to change value.
		 * 
		 * Also, while the state of the src/drn of the transistor
		 * in question may not change state, the current through
		 * their stage is altered; this current change may change
		 * the state of other nodes in the stage, so they better
		 * be avaluated.  These optimizations work best in MOSSIM
		 * since that model does not account for the current through
		 * the stage.	A.S.
		 */
	    oldstate = t->state;

	    t->state = compute_trans_state( t );
	    if( (t->drain->npot == X) or (t->source->npot == X) or
	      ((t->drain->npot != t->source->npot) and
	      (t->state == ON)) or
	      ((t->drain->npot == t->source->npot) and
	      (t->state == OFF)) or
	      ((t->state == UNKNOWN) and
	      not (oldstate == OFF and
	      (t->drain->npot == t->source->npot))) or
	      (t->drain->events != NULL) or
	      (t->source->events != NULL) )
	      {
		if( not (t->source->nflags & INPUT) )
		    t->source->nflags |= VISITED;
		if( not (t->drain->nflags & INPUT) )
		    t->drain->nflags |= VISITED;
	      }
#else
	    t = l->xtor;
#ifdef USER_SUBCKT 
	    if ( t->ttype != SUBCKT ) {
	    	t->state = compute_trans_state( t );
	    	if( not (t->source->nflags & INPUT) )
			t->source->nflags |= VISITED;
	    	if( not (t->drain->nflags & INPUT) )
			t->drain->nflags |= VISITED;
	    } 
	    else t->drain->nflags |= VISITED;
#else
	    t->state = compute_trans_state( t );
	    if( not (t->source->nflags & INPUT) )
		t->source->nflags |= VISITED;
	    if( not (t->drain->nflags & INPUT) )
		t->drain->nflags |= VISITED;
#endif
#endif
	  }
	free_from_node( e, n );    /* remove to avoid punting this event */
	e = e->flink;
      }
    while( e != NULL );

	/* run thorugh event list again, marking src/drn of input nodes */
    if( all_flags & INPUT )
      {
	for( e = evlist; e != NULL; e = e->flink )
	  {
	    if (e->type == TIMED_EV) continue;

	    n = e->enode;

	    if( (n->nflags & (INPUT | POWER_RAIL)) != INPUT )
		continue;
	    
	    for( l = n->nterm; l != NULL; l = l->next )
	      {
		    t = l->xtor;
#ifdef USER_SUBCKT 
		    if (t->ttype != SUBCKT) {
#endif
		       if( t->state != OFF )
		         {
		           register nptr other = other_node( t, n );
		           if( not( other->nflags & (INPUT | VISITED) ) )
			           other->nflags |= VISITED;
			 }
#ifdef USER_SUBCKT 
		    } else {
			    t->drain->nflags |= VISITED;
		    }
#endif
	      }
	   }
      }
  }


private long EvalNodes( evlist )
  evptr  evlist;
  {
    register tptr   t;
    register lptr   l;
    register nptr   n;
    register evptr  event = evlist;
    long            brk_flag = 0;

    do
      {
	nevent++;		/* advance counter to that of this event */
	if (event->type == TIMED_EV)	/* special case---enode is not a node */
	{
	    npending--;
	    spending--;
	    event = event->flink;
	    if (event == NULL) break;
	    continue;
	}
	n = cur_node = event->enode;
	n->c.time = event->ntime;	/* set up the cause stuff */
	n->t.cause = event->p.cause;

	npending--;

	  /* now calculate new value for each marked node.  Some nodes marked
	   * above may become unmarked by earlier calculations before we get
	   * to them in this loop...
	   */

	for( l = n->ngate; l != NULL; l = l->next )
	  {
	    t = l->xtor;
#ifdef USER_SUBCKT 
		if ( t->ttype == SUBCKT ) {
		    if (t->drain->nflags & VISITED)
			subckt_model_C(t);
		}
		else 
#endif
		{
	    	    if( t->source->nflags & VISITED )
		    	(*model)( t->source );
	    	    if( t->drain->nflags & VISITED )
		    	(*model)( t->drain );
		}
	  }

	if( (n->nflags & (INPUT | POWER_RAIL)) == INPUT )
	  {
	    for( l = n->nterm; l != NULL; l = l->next )
	      {
		nptr  other;

		t = l->xtor;
		other = other_node( t, n );
		if( other->nflags & VISITED )
		    (*model)( other );
	      }
	  }

	    /* see if we want to halt if this node changes value */
	brk_flag |= n->nflags;

	event = event->flink;
      }
    while( event != NULL );

    return( brk_flag );
  }


/*
 * Change the state of the nodes in the given input list to their new value,
 * setting their INPUT flag and enqueueing the event.
 */
private void SetInputs( listp, val )
  register iptr  *listp;
  register int   val;
  {
    register nptr  n;
    iptr           ip, last;

    for( ip = last = *listp; ip != NULL; ip = ip->next )
      {
	last = ip;

	n = ip->inode;

	n->npot = val;
	n->nflags &= ~INPUT_MASK;
	n->nflags |= INPUT;

		/* enqueue event so consequences are computed. */
	enqueue_input( n, val );

	if( n->curr->val != val or not (n->curr->inp) )
	    AddHist( n, val, 1, cur_delta, 0L, 0L );
      }

    if( last )
      {
	last->next = infree;
	infree = *listp;
      }
    *listp = NULL;
  }


private void MarkNOinputs()
  {
    register iptr  list;

    for( list = xinputs; list != NULL; list = list->next )
      {
	list->inode->nflags &= ~(INPUT_MASK | INPUT);
	list->inode->nflags |= VISITED;
      }
  }


	/* nodes which are no longer inputs */
private void EvalNOinputs()
  {
    nptr  n;
    iptr  list, last;

    for( list = last = xinputs; list != NULL; list = list->next )
      {
	cur_node = n = list->inode;
	AddHist( n, (int) n->curr->val, 0, cur_delta, 0L, 0L );
	if( n->nflags & VISITED )
	    (*model)( n );
	last = list;
      }
    if( last )
      {
	last->next = infree;
	infree = xinputs;
      }
    xinputs = NULL;
  }


public int step( stop_time )
  Ulong  stop_time;
  {
    evptr  evlist;
    long   brk_flag;
    int    ret_code = 0;

    /* look through input lists updating any nodes which just become inputs */

    MarkNOinputs();			/* nodes no longer inputs */
    SetInputs( &hinputs, HIGH );	/* HIGH inputs */
    SetInputs( &linputs, LOW );		/* LOW inputs */
    SetInputs( &uinputs, X );		/* X inputs */

    /* 
     * On the first call to step, make sure transistors with gates
     * of vdd and gnd are set up correctly.  Mark initial inputs first!
     */

    if (firstcall)
	if (VDD_node != NULL && GND_node != NULL)
	    init_vdd_gnd();

  try_again :
    /* process events until we reach specified stop time or events run out. */
    while( (evlist = get_next_event( stop_time )) != NULL )
      {
	MarkNodes( evlist );
	if( xinputs ) EvalNOinputs();

	brk_flag = EvalNodes( evlist );

	FreeEventList( evlist );	/* return event list to free pool */

	if( int_received )
	    goto done;
	if( brk_flag & (WATCHVECTOR | STOPONCHANGE | STOPVECCHANGE) )
	  {
	    if( brk_flag & (WATCHVECTOR | STOPVECCHANGE) )
		disp_watch_vec( brk_flag );
	    if( brk_flag & (STOPONCHANGE | STOPVECCHANGE) )
	      {
		ret_code = 1;
		goto done;
	      }
	  }
      }

    if( xinputs )
      {
	EvalNOinputs();
	goto try_again;
      }

    cur_delta = stop_time;
  done :
    if( analyzerON )
	UpdateWindow( cur_delta );
    return( ret_code );
  }


/* table to convert transistor type and gate node value into switch state
 * indexed by switch_state[transistor-type][gate-node-value].
 */
public	char  switch_state[NTTYPES][4] = 
  {
    OFF,	UNKNOWN,	UNKNOWN,	ON,	/* NCHAH */
    ON,		UNKNOWN,	UNKNOWN,	OFF,	/* PCHAN */
    WEAK,	WEAK,		WEAK,		WEAK,   /* RESIST */
    WEAK,	WEAK,		WEAK,		WEAK,   /* DEP */
  };


/* compute state of transistor.  If gate is a simple node, state is determined
 * by type of implant and value of node.  If gate is a list of nodes, then
 * this transistor represents a stack of transistors in the original network,
 * and we perform the logical AND of all the gate node values to see if
 * transistor is on.
 */
public
#define	 compute_trans_state( TRANS )					\
    ( ((TRANS)->ttype & GATELIST) ?					\
	ComputeTransState( TRANS ):					\
	switch_state[ BASETYPE( (TRANS)->ttype ) ][ (TRANS)->gate->npot ] )


public int ComputeTransState( t )
  register tptr  t;
  {
    register nptr  n;
    register tptr  l;
    register int   result;

    switch( BASETYPE( t->ttype ) )
      {
	case NCHAN :
	    result = ON;
	    for( l = (tptr) t->gate; l != NULL; l = l->scache.t )
	      {
		n = l->gate;
		if( n->npot == LOW )
		    return( OFF );
		else if( n->npot == X )
		    result = UNKNOWN;
	      }
	    return( result );

	case PCHAN :
	    result = ON;
	    for( l = (tptr) t->gate; l != NULL; l = l->scache.t )
	      {
		n = l->gate;
		if( n->npot == HIGH )
		    return( OFF );
		else if( n->npot == X )
		    result = UNKNOWN;
	      }
	    return( result );

	case DEP :
	case RESIST :
	    return( WEAK );

	default :
	    lprintf( stderr,
	      "**** internal error: unrecongized transistor type (0x%x)\n",
	      BASETYPE( t->ttype ) );
	    return( UNKNOWN );
      }
  }
