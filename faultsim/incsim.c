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

/*
 * Incremental simulation
 */

#include <stdio.h>

#include "defs.h"
#include "net.h"
#include "units.h"
#include "globals.h"


/*
 * Set H to the next "effective" change in the history following Hp
 */
#define	NEXTH( H, Hp )					\
    for( (H) = (Hp)->next; (H)->punt; (H) = (H)->next )


/*
 * time at which history entry was enqueued
 */
#define	QTIME( H )		( (H)->time - (H)->t.r.delay )
#define	QPTIME( H )		( (H)->time - (H)->t.p.delay )
#define	PuntTime( H )		( (H)->time - (H)->t.p.ptime )


#define	IsDeviated( ND )	\
    ( (ND)->nflags & (DEVIATED | CHANGED) or (ND)->events != NULL )

#define	NeedUpdate( ND )	\
    ( ( (ND)->nflags & (ACTIVE_CL | STIM | POWER_RAIL) ) == 0 )


public	long	INC_RES = 0;		/* time resolution in ticks (.5 ns) */
public	nptr	inc_cause = (nptr) 1;	/* fake cause ptr for path command */

public	long    nevals = 0;		/* # of evaluations */
public	long    i_nevals = 0;		/* # of incremental evaluations */
private	long    o_nevals, o_nevent;

public	long	nreval_ev = 0;		/* number of various events */
public	long	npunted_ev = 0;
public	long	nstimuli_ev = 0;
public	long	ncheckpt_ev = 0;
public	long	ndelaychk_ev = 0;
public	long	ndelay_ev = 0;

private	void	(*curr_model)();	/* the current model used to eval */
private	hptr	modelp;			/* model history */

#ifdef FAULT_SIM

public	int	fault_mode = FALSE;	/* TRUE when doing faultsim */
private	int	stop_early;		/* TRUE when fault detected */
private	Ulong	old_cur_delta;		/* cur_delta before faultsim */
private	evptr	pending_evs;		/* pending events before faultsim */
extern	int	do_trigger();
extern	void	setup_triggers();

#define	DoingFault	( fault_mode != FALSE )
#define	NotDoingFault	( fault_mode == FALSE )
#else
#define	DoingFault	(0)
#define	NotDoingFault	(1)
#define	hchange		head
#endif


typedef struct			/* transistor stage built by GetConnList */
  {
    int   flags;			/* see below */
    nptr  nd_list;			/* the connection list */
    nptr  inp_list;			/* input nodes connected to list */
  } Stage, *pstg;

		/* stage flags */
#define	ALL_MERGED	0x1		/* all nodes in stage are converged */
#define	ONLY_INPUT	0x2		/* list consists of input node only */
#define	INP_TRANS	0x4		/* there are switching inputs */


/*
 * Update the state of a node from the history.  Set the INPUT flag as well.
 * Return a pointer to the next change in the history.
 */
private	hptr UpdateNode( nd )
  register nptr  nd;
  {
    register hptr cur, nxt;

    cur = nd->curr;
    if( cur->time > cur_delta )
	cur = (hptr) &(nd->head);

    NEXTH( nxt, cur );
    while( nxt->time <= cur_delta )
      {
	cur = nxt;
	NEXTH( nxt, nxt );
      }

    nd->curr = cur;
    nd->npot = cur->val;
    if( cur->inp )
	nd->nflags |= INPUT;
    else
	nd->nflags &= ~INPUT;

    return( nxt );
  }


/*
 * Activate a node: Set the ACTIVE_CL flag, schedule all pending transitions
 * as of cur_delta.  Punted events that were already punted (as of cur_delta)
 * remain in the history.  Pending punted events are rescheduled.  Punted
 * events between nd->curr and the next transition that were queued after
 * cur_delta are removed from the history and put in nd->t.punts.
 */
private void ActivateNode( nd )
  nptr  nd;
  {
    register hptr  h, p;
    hptr           *lastp;
    int            first;

#ifdef INCDEBUG
    if( nd->nflags & WATCHED )
	lprintf( stdout, "%.2f: Activate %s\n", d2ns(cur_delta), pnode(nd) );
#endif
    if( nd->nflags & STIM )
      {					/* don't dequeue if ntime == now */
	if( nd->c.event->ntime != cur_delta )
	    DequeueEvent( nd );
	nd->nflags &= ~STIM;
	NEXTH( p, nd->curr );
      }
    else
	p = UpdateNode( nd );

    (void) EnqueueHist( nd, p, CHECK_PNT );
    nd->nflags |= (ACTIVE_CL | WAS_ACTIVE);

    lastp = &(nd->t.punts);
    *lastp = NULL;
	/* look for any pending events as of cur_delta */
    first = TRUE;
    for( p = nd->curr, h = p->next; ; p = h, h = h->next )
      {
	if( h->punt )
	  {
	    if( PuntTime( h ) < cur_delta )	/* already punted, skip it */
		continue;

	    if( QPTIME( h ) <= cur_delta )	/* pending: enqueue it */
	      {
		if( QPTIME( h ) != cur_delta )	/* cur_delta: do not queue */
		    (void) EnqueueHist( nd, h, PUNTED );
		if( DoingFault )
		    continue;
		p->next = h->next;		/* and remove it from hist */
		h->next = freeHist;
		freeHist = h;
		h = p;
	      }
	    else if( first and NotDoingFault )
	      {			/* move if punted before next transition */
		p->next = h->next;
		h->next = NULL;
		*lastp = h;
		lastp = &(h->next);
		h = p;
	      }
	  }
	else
	  {
	    first = FALSE;
	    if( QTIME( h ) < cur_delta )	/* pending: enqueue it */
		(void) EnqueueHist( nd, h, REVAL );
	    else
		break;
	  }
      }
  }


/*
 * Define all possible cases due to an event on an active node, as follows:
 *	000 0: no edge, node remains converged (not deviated)
 *	001 1: no edge, node just converged
 *	010 2: no edge, node just deviated
 *	011 3: no edge, node remains deviated
 *	100 4: edge, node remains converged
 *	101 5: edge, node just converged
 *	110 6: edge, node just deviated
 *	111 7: edge, node remains deviated
 */

#define	O_DEV		0x1		/* node was previously deviated */
#define	N_DEV		0x2		/* node is now deviated */
#define	EDGE		0x4		/* a transistion ocurred */
#define	CHK		0x8		/* just dequeued a check_pnt event */
#define	SAME_T		0x10		/* event merges at the same time */
#define	N_ACTV		0x20		/* merge events: still active xtors */

#define	GetOdev( nd )		( (nd)->nflags & DEVIATED )

private	evptr    dev_evs;		/* deviate events */
private	evptr    mrg_evs;		/* merge events w/transition */
private	evptr    *last_mrg;		/* last merge event w/transition */
private	evptr    mrg0_evs;		/* last merge event */
private	evptr    chk_evs;		/* events with no transition */
private	evptr    inp_evs;		/* input events */
private	evptr    xinp_evs;		/* stop being an input events */
private	evptr    stim_evs;		/* stimulus events */
private	evptr    pend_evs;		/* pending events */


/*
 * Change the event type to reflect new state, add event to its corresponding
 * event list, set the oldpot field and the DEVIATED bit for the node.
 */
#define	Deviate_Edge( ND, EV, Oval, Chk )		\
  {							\
    (EV)->type = GetOdev( ND ) | N_DEV | Chk | EDGE;	\
    (EV)->nlink = dev_evs;				\
    dev_evs = (EV);					\
    (ND)->oldpot = Oval;				\
    (ND)->nflags |= DEVIATED;				\
  }							\


#define	Deviate( ND, EV, Oval, Chk )			\
  {							\
    (EV)->type = GetOdev( ND ) | N_DEV | Chk;		\
    (EV)->nlink = chk_evs;				\
    chk_evs = (EV);					\
    (ND)->oldpot = Oval;				\
    (ND)->nflags |= DEVIATED;				\
  }							\


#define	Deviate_SameVal( EV )				\
  {							\
    (EV)->type = N_DEV | O_DEV | EDGE;			\
    (EV)->nlink = dev_evs;				\
    dev_evs = (EV);					\
  }							\


/*
 * Change the event type to reflect new state, add event to its corresponding
 * event list and reset the DEVIATED bit.
 */
#define Merge_Edge( ND, EV, Chk )			\
  {							\
    if( (ND)->nflags & DEVIATED )			\
      {							\
	(EV)->type = (O_DEV | EDGE | Chk );		\
	*last_mrg = (EV);				\
	last_mrg = & (EV)->nlink;			\
      }							\
    else						\
      {							\
	(EV)->type = (Chk | EDGE);			\
	(EV)->nlink = mrg0_evs;				\
	mrg0_evs = (EV);				\
      }							\
    (ND)->nflags &= ~DEVIATED;				\
  }							\


#define Merge_SameEdge( ND, EV, Chk )			\
  {							\
    (EV)->type = GetOdev( ND ) | SAME_T | Chk | EDGE;	\
    (EV)->nlink = mrg0_evs;				\
    mrg0_evs = (EV);					\
    (ND)->nflags &= ~DEVIATED;				\
  }							\


#define Merge( ND, EV, Chk )				\
  {							\
    (EV)->type = GetOdev( ND ) | Chk;			\
    (EV)->nlink = chk_evs;				\
    chk_evs = (EV);					\
    (ND)->nflags &= ~DEVIATED;				\
  }							\


/*
 * Free all punted events currently in nd->t.punts and move all punted events
 * after nd->curr to nd->t.punts. 
 */
private void ReplacePunts( nd )
  nptr  nd;
  {
    register hptr  h, p;

    if( DoingFault )
	return;
    h = nd->t.punts;
    if( h != NULL )
      {
	for( p = h; p->next != NULL; p = p->next );
	p->next = freeHist;
	freeHist = h;
      }
    
    h = nd->curr;
    for( p = h; p->next->punt; p = p->next );
    if( p->punt )
      {
	nd->t.punts = h->next;
	h->next = p->next;
	p->next = NULL;
      }
    else
	nd->t.punts = NULL;
  }


/*
 * Update the state of a node due to a re-evaluation event.
 */
private void UpdateReval( e )
  register evptr  e;
  {
    register nptr   n;			/* node in question */
    register evptr  nxte;		/* next chk_point event (if any) */
    register long   d_t;		/* delta time between events */

    n = e->enode;
    nxte = n->c.event;

	/* no chk_point event or chk_point is too far in the future */
    if( nxte == NULL or (d_t = nxte->ntime - e->ntime) > INC_RES )
      {
	free_from_node( e, n );
	NewEdge( n, e );
	if( n->nflags & DEVIATED )
	  {
	    if( n->oldpot == e->eval )
		Merge_Edge( n, e, 0 )
	    else
		Deviate_SameVal( e )
	  }
	else
	    Deviate_Edge( n, e, n->npot, 0 );
	n->npot = e->eval;
      }
    else if( d_t != 0 )
      {
	free_from_node( e, n );

	switch( nxte->type )
	  {
	    case DELAY_CHK :
		ndelaychk_ev++;
		if( nxte->eval == e->eval )
		  {
		    n->curr = nxte->p.hist;	/* update curr pointer */
		    Merge_Edge( n, e, CHK );
		    ReplacePunts( n );
		  }
		else
		  {
		    DeleteNextEdge( n );
		    NewEdge( n, e );
		    Deviate_Edge( n, e, nxte->eval, CHK );
		  }
		DequeueEvent( n );
		n->npot = e->eval;
		break;

	    case DELAY_EV :
		if( nxte->eval == e->eval )
		    DelayEvent( e, d_t );		/* delay again */
		else
		  {
		    lprintf( stdout,
		      "Missed Glitch: %s => (%.2f %c) (%.2f %c)\n",
		      pnode( n ), d2ns( nxte->p.oldt ), "0XX1"[nxte->eval],
		      d2ns( e->ntime ), "0XX1"[e->eval] );
		    nxte->type = CHECK_PNT;
		    NewEdge( n, e );
		    Deviate_Edge( n, e, n->npot, 0 );
		    n->npot = e->eval;
		  }
		break;

	    case CHECK_PNT :
		if( n->nflags & DEVIATED )
		  {
		    if( n->oldpot == e->eval )
			Merge_Edge( n, e, 0 )
		    else
			Deviate_SameVal( e )

		    NewEdge( n, e );
		    n->npot = e->eval;
		  }
		else if( nxte->eval == e->eval )
		  {
		    register evptr  ne;

		    if( (ne = n->events) != NULL )
			while( ne->nlink != NULL )
			    ne = ne->nlink;

		    if( ne == NULL or ne->ntime > nxte->ntime )
		      {
			nxte->p.oldt = e->ntime;
			nxte->type = DELAY_EV;
			DelayEvent( e, d_t );
		      }
		    else
		      {
			NewEdge( n, e );
			Deviate_Edge( n, e, n->npot, 0 );
			n->npot = e->eval;
		      }
		  }
		else
		  {
		    NewEdge( n, e );
		    Deviate_Edge( n, e, n->npot, 0 );
		    n->npot = e->eval;
		  }
		break;

	    case INP_EV :
	    case XINP_EV :
		if( n->nflags & DEVIATED )
		  {
		    if( n->oldpot == e->eval )
			Merge_Edge( n, e, 0 )
		    else
			Deviate_SameVal( e );
		  }
		else
		    Deviate_Edge( n, e, n->npot, 0 );

		n->npot = e->eval;
		NewEdge( n, e );
		break;

	    default :
		lprintf( stderr, "Unexpected Event 0x(%x)\n", nxte->type );
	  }
      }
  }


#define CopyEvent( DST, SRC )		\
  {					\
    (DST)->p.hist = (SRC)->p.hist;	\
    (DST)->ntime = (SRC)->ntime;	\
    (DST)->rtime = (SRC)->rtime;	\
    (DST)->delay = (SRC)->delay;	\
    (DST)->eval = (SRC)->eval;		\
  }					\


private void UpdateCheck_Pnt( e )
  register evptr  e;
  {
    register nptr   n;
    register evptr  nxte;
    register long   d_t = 0;

    n = e->enode;
    if( (nxte = n->events) != NULL )
      {
	while( nxte->nlink != NULL )
	    nxte = nxte->nlink;
	d_t = nxte->ntime - e->ntime;
      }

    if( nxte == NULL or d_t > INC_RES )
      {
	DeleteNextEdge( n );
	if( n->npot == e->eval )
	    Merge( n, e, CHK )
	else
	    Deviate( n, e, e->eval, CHK );
      }
    else if( d_t == 0 )
      {
	free_from_node( nxte, n );
	if( e->eval == nxte->eval )
	  {
	    n->curr = e->p.hist;
	    if( DoingFault )  n->hchange = *n->curr, n->curr = &n->hchange;
	    n->npot = e->eval;
	    n->curr->t.r.delay = nxte->delay;
	    if( e->rtime == nxte->rtime )
		Merge_SameEdge( n, e, CHK )
	    else
	      {
		n->curr->t.r.rtime = nxte->rtime;
		Merge_Edge( n, e, CHK );
	      }
	    ReplacePunts( n );
	  }
	else
	  {
	    DeleteNextEdge( n );
	    d_t = e->eval;		/* save old value temporarily */
	    CopyEvent( e, nxte );	/* this is the new value! */
	    NewEdge( n, e );
	    n->npot = e->eval;
	    Deviate_Edge( n, e, d_t, CHK );
	  }
      }
    else	/* 0 < d_t <= INC_RES */
      {
	if( n->nflags & DEVIATED )
	  {
	    DeleteNextEdge( n );
	    if( n->npot == e->eval )
		Merge( n, e, CHK )
	    else
		Deviate( n, e, e->eval, CHK );
	  }
	else
	  {
	    register hptr   h;

	    NEXTH( h, e->p.hist );
	    if( nxte->eval == e->eval and h->time > nxte->ntime )
	      {
		e->type = DELAY_CHK;
		DelayEvent( e, d_t );
	      }
	    else
	      {
		DeleteNextEdge( n );
		Deviate( n, e, e->eval, CHK );
	      }
	  }
      }
  }


private void UpdateDelay_Chk( e )
  register evptr  e;
  {
    register nptr   n;
    register evptr  nxte;
    register long   d_t = 0;

    n = e->enode;
    if( (nxte = n->events) != NULL )
      {
	while( nxte->nlink != NULL )
	    nxte = nxte->nlink;
	d_t = nxte->ntime - e->ntime;
      }

    if( nxte != NULL and d_t == 0 and nxte->eval == e->eval )
      {
	free_from_node( nxte, n );
	n->curr = e->p.hist;
	n->curr->t.r.delay = nxte->delay;
	if( e->rtime == nxte->rtime )
	    Merge_SameEdge( n, e, CHK )
	else
	  {
	    n->curr->t.r.rtime = nxte->rtime;
	    Merge_Edge( n, e, CHK );
	  }
	n->npot = e->eval;
	ReplacePunts( n );
      }
    else	/* event got punted (and maybe something else was queued) */
      {
	DeleteNextEdge( n );
	if( nxte != NULL and d_t == 0 )
	  {
	    free_from_node( nxte, n );
	    d_t = e->eval;
	    CopyEvent( e, nxte );
	    Deviate_Edge( n, e, d_t, CHK );
	    NewEdge( n, e );
	    n->npot = e->eval;
	  }
	else					/* nothing else was queued */
	    Deviate( n, e, e->eval, CHK );
      }
  }


private void UpdateDelay_Ev( e )
  register evptr  e;
  {
    register nptr   n;
    register evptr  nxte;
    register long   d_t = 0;

    n = e->enode;
    if( (nxte = n->events) != NULL )
      {
	while( nxte->nlink != NULL )
	    nxte = nxte->nlink;
	d_t = nxte->ntime - e->ntime;
      }

    if( nxte != NULL and d_t == 0 and nxte->eval == e->eval )
      {
	register hptr   h;

	free_from_node( nxte, n );
	NEXTH( h, n->curr );
	n->curr = h;
	Merge_SameEdge( n, e, CHK );
	n->npot = e->eval;
	ReplacePunts( n );
      }
    else		/* expected event was punted */
      {
	lprintf( stdout, "Missed Glitch: %s => (%.2f %c) -> (~%.2f %c)\n",
	  pnode( n ), d2ns( e->p.oldt ), "0XX1"[e->eval],
	  d2ns( (e->ntime + e->p.oldt)/2 ), "0XX1"[n->npot] );

	DeleteNextEdge( n );
	if( nxte != NULL and d_t == 0 )
	  {
	    free_from_node( nxte, n );
	    NewEdge( n, nxte );
	    d_t = e->eval;
	    CopyEvent( e, nxte );
	    Deviate_Edge( n, e, d_t, CHK );
	    n->npot = e->eval;
	  }
	else
	    Deviate( n, e, e->eval, CHK );
      }
  }


private void update_nodes( e )
  evptr  e;
  {
    nptr   n;

	/* clear event lists */

    dev_evs = chk_evs = mrg_evs = mrg0_evs = inp_evs = xinp_evs =
    stim_evs = pend_evs = NULL;
    last_mrg = &mrg_evs;

    do
      {
#ifdef STATS
	{ extern int ev_hgm; if( ev_hgm ) IncHistEvCnt( (Uint) e->type ); }
#endif /* STATS */
	switch( e->type )
	  {
	    case PUNTED :
		npunted_ev++;
		UpdateReval( e );
		break;

	    case REVAL :
	    case DECAY_EV :
		nreval_ev++;
		UpdateReval( e );
		break;

	    case DELAY_CHK :
		ndelaychk_ev++;
		UpdateDelay_Chk( e );
		break;

	    case DELAY_EV :
		ndelay_ev++;
		UpdateDelay_Ev( e );
		break;

	    case CHECK_PNT :
		ncheckpt_ev++;
		UpdateCheck_Pnt( e );
		break;

	    case XINP_EV :
		ncheckpt_ev++;
		n = e->enode;
		n->curr = e->p.hist;
		n->nflags &= ~(INPUT | ACTIVE_CL);
		e->nlink = xinp_evs;
		xinp_evs = e;
		ReplacePunts( n );
		break;

	    case INP_EV :
		ncheckpt_ev++;
		n = e->enode;
		while( n->events and n->events->ntime != cur_delta )
		    free_event( n->events );
		ReplacePunts( n );
		n->curr = e->p.hist;
		n->npot = e->eval;
		n->nflags |= INPUT;
		n->nflags &= ~DEVIATED;
		e->nlink = inp_evs;
		inp_evs = e;
		break;

	    case STIM_INP :
		nstimuli_ev++;
		n = e->enode;
		n->npot = e->eval;
		n->curr = e->p.hist;
		n->nflags |= INPUT;
		e->nlink = stim_evs;
		stim_evs = e;
		break;

	    case STIM_XINP :
	      {
		register hptr  h;

		nstimuli_ev++;
		n = e->enode;
		n->curr = e->p.hist;
		n->nflags &= ~INPUT;
		NEXTH( h, n->curr );
		if( not EnqueueHist( n, h, STIMULI ) )
		    n->nflags &= ~STIM;
		break;
	      }

	    case STIMULI :
		nstimuli_ev++;
		n = e->enode;
		n->npot = e->eval;
		n->curr = e->p.hist;
		e->nlink = stim_evs;
		stim_evs = e;
		break;

	    case PENDING :
		e->nlink = pend_evs;
		pend_evs = e;
		break;
#ifdef notdef
	    case PUNTED :
		npunted_ev++;
		n = e->enode;
		    /* else: punt along with chk_pnt, the later handles it */
		if( n->c.event == NULL or n->c.event->ntime != e->ntime )
		  {
		    int  oval;

		    free_from_node( e, n );
		    NewEdge( n, e );
		    oval = (n->nflags & DEVIATED) ? n->oldpot : n->npot;
		    Deviate_Edge( n, e, oval, 0 );
		    n->npot = e->eval;
		  }
		break;
#endif
	    case CHNG_MODEL :
		curr_model = model_table[ modelp->val ];
		modelp = modelp->next;
		if( modelp != NULL )
		    (void) EnqueueOther( CHNG_MODEL, (Ulong) modelp->time );
		break;

#ifdef FAULT_SIM
	    case TRIGGER_EV :
		stop_early = do_trigger( e );
		break;
#endif
	    default:
		lprintf( stderr,
		  "update_nodes: bad event (%x) @ delta=%d for node %s\n",
		   e->type, cur_delta, pnode( e->enode ) );
	  }
	npending -= 1;
      }
    while( (e = e->flink) != NULL );

    *last_mrg = mrg0_evs;	/* join the 2 lists, order is important */
  }


/*
 * Run through the various event lists and update the state of all transistors
 * controlled by the node that triggered the event.  Also mark all nodes that
 * need to be considered for evaluation as VISITED, this doesn't imply they
 * will be evaluated.
 */
private void UpdateTransistors()
  {
    register evptr  e;
    register lptr   l;
    register tptr   t;

    for( e = dev_evs; e != NULL; e = e->nlink )
      {
	if( (e->type & O_DEV) == 0 )		/* just deviated */
	  {
	    for( l = e->enode->ngate; l != NULL; l = l->next )
	      {
		t = l->xtor;
	    	if( t->tflags & ACTIVE_T )
		    t->state = compute_trans_state( t );
		t->source->nflags |= VISITED;	/* always mark src/drn */
		t->drain->nflags |= VISITED;
	      }
	  }
	else					/* was already deviated */
	  {
	    for( l = e->enode->ngate; l != NULL; l = l->next )
	      {
		t = l->xtor;
		if( t->tflags & ACTIVE_T )
		  {
		    t->state = compute_trans_state( t );
		    t->source->nflags |= VISITED;
		    t->drain->nflags |= VISITED;
		  }
	      }
	  }
      }

    for( e = mrg_evs; e != NULL; e = e->nlink )
      {
	for( l = e->enode->ngate; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( t->tflags & ACTIVE_T )
	      {
		t->state = compute_trans_state( t );
		t->source->nflags |= VISITED;
		t->drain->nflags |= VISITED;
	      }
	  }
      }

    for( e = chk_evs; e != NULL; e = e->nlink )
      {
	if( (e->type & (N_DEV | O_DEV) ) == N_DEV )	/* just deviated */
	  {
	    for( l = e->enode->ngate; l != NULL; l = l->next )
	      {
		t = l->xtor;
	    	if( t->tflags & ACTIVE_T )
		    t->state = compute_trans_state( t );
		t->source->nflags |= VISITED;	/* always mark src/drn */
		t->drain->nflags |= VISITED;
	      }
	  }
      }

    for( e = inp_evs; e != NULL; e = e->nlink )
      {
	for( l = e->enode->ngate; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( t->tflags & ACTIVE_T )
	      {
		t->state = compute_trans_state( t );
		t->source->nflags |= VISITED;
		t->drain->nflags |= VISITED;
	      }
	  }
      }

    for( e = xinp_evs; e != NULL; e = e->nlink )
	e->enode->nflags |= VISITED;

    for( e = stim_evs; e != NULL; e = e->nlink )
      {
	for( l = e->enode->ngate; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( t->tflags & ACTIVE_T )
	      {
		t->state = compute_trans_state( t );
		t->source->nflags |= VISITED;
		t->drain->nflags |= VISITED;
	      }
	  }
      }

    VDD_node->nflags &= ~VISITED;
    GND_node->nflags &= ~VISITED;
  }


#define hash_terms(T)     ((pointertype)((T)->source) ^ (pointertype)((T)->drain))


/*
 * Build the connection list for node 'n'.  Determine (1) if all nodes in the
 * list are converged or whether this list is adjacent to a transistor whose
 * gate is deviated, (2) if there are any inactive nodes (or transistors) in
 * the connection list, and (3) whether any inputs (transistor gates) to the
 * connection list have a transition at the current time.  As a side effect
 * set the 'withdriven' flag.  The algorithm to construct the connection list
 * is the same as that used by BuildConnList.
 *
 * We must be careful not to report as an input transition nodes that stop
 * being inputs (i.e. hist->delay == 0 and hist->inp == 0).
 */
#define IsCurrTransition( H )			\
  ( (H)->time == cur_delta and ((H)->inp == 1 or (H)->t.r.delay != 0) )

private pstg GetConnList( n )
  register nptr  n;
  {
    register nptr  next, this, other, *inext;
    register lptr  l;
    register tptr  t;
    register hptr  h;
    register int   status;
    int            n_par = 0;
    static Stage   stage;

    stage.nd_list = n;
    stage.inp_list = NULL;

    status = ALL_MERGED;
    withdriven = FALSE;

    n->nflags &= ~VISITED;
    inext = &(stage.inp_list);

    if( NeedUpdate( n ) )
	(void) UpdateNode( n );
    if( IsDeviated( n ) )
	status &= ~ALL_MERGED;

    if( n->nflags & INPUT )
      {
	stage.flags = ALL_MERGED | ONLY_INPUT;
	return( &stage );
      }

    next = this = n->nlink = n;
    do
      {
	for( l = this->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( not (t->tflags & ACTIVE_T) )
	      {
		if( t->ttype & GATELIST )
		  {
		    for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
			if( NeedUpdate( t->gate ) )
			    (void) UpdateNode( t->gate );
		    t = l->xtor;
		  }
		else if( NeedUpdate( t->gate ) )
		    (void) UpdateNode( t->gate );

		t->state = compute_trans_state( t );
	      }
	    else if( status & ALL_MERGED )
	      {
		if( t->ttype & GATELIST )
		  {
		    for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
			if( t->gate->nflags & DEVIATED )
			  {
			    status &= ~ALL_MERGED;
			    break;
			  }
		    t = l->xtor;
		  }
		else if( t->gate->nflags & DEVIATED )
		    status &= ~ALL_MERGED;
	      }

	    if( t->state == OFF )
		continue;

	    if( t->tflags & CROSSED )
	      {
		t->tflags &= ~CROSSED;
		continue;
	      }

	    t->scache.r = t->dcache.r = NULL;

	    other = other_node( t, this );

	    if( NeedUpdate( other ) )
		(void) UpdateNode( other );
	    if( IsDeviated( other ) )
		status &= ~ALL_MERGED;

	    if( other->nflags & INPUT )
	      {
		withdriven = TRUE;
		if( other->nlink == NULL and 
		  not (other->nflags & (ACTIVE_CL | POWER_RAIL)) )
		  {
		    *inext = other;
		    inext = &(other->nlink);
		    *inext = other;		/* anything other than NULL */
		  }
	      }
	    else
	      {
		t->tflags |= CROSSED;
		if( other->nlink == NULL )
		  {
		    other->nflags &= ~VISITED;
		    other->nlink = n;
		    next->nlink = other;
		    next = other;
		    other->n.tran = t;
		  }
		else if( model_num != LIN_MODEL )
		    goto is_trans;
		else if( hash_terms( other->n.tran ) == hash_terms( t ) )
		  {
		    register tptr  tran = other->n.tran;

		    if( tran->tflags & PARALLEL )
			t->dcache.t = par_list( tran );
		    else
		      {
			if( n_par >= MAX_PARALLEL )
			  {
			    WarnTooManyParallel();
			    t->tflags |= PBROKEN;
			    continue;
			  }
			tran->n_par = n_par++;
			tran->tflags |= PARALLEL;
		      }
		    par_list( tran ) = t;
		    t->tflags |= PBROKEN;
		  }
		else
		  {
		    t->tflags |= BROKEN;
		    continue;
		  }
	      }
	is_trans :
	    if( t->ttype & GATELIST )
	      {
		for( t = (tptr) t->gate; t; t = t->scache.t )
		  {
		    h = t->gate->curr;
		    if( IsCurrTransition( h ) )
		      {
			t = l->xtor;
			status |= INP_TRANS;
			break;
		      }
		  }
		t = l->xtor;
	      }
	    else
	      {
		h = t->gate->curr;
		if( IsCurrTransition( h ) )
		    status |= INP_TRANS;
	      }
	  }		/* end of for each nterm */
      }
    while( (this = this->nlink) != n );

    next->nlink = NULL;
    *inext = NULL;
    stage.flags = status;

    return( &stage );
  }


/*
 * Activate the gate of a transistor to act as stimuli.  First check
 * that the node in question is not part of an active cluster or is
 * already acting as stimuli.
 */
#define	StimulateGate( GATE )				\
  {							\
    if( NeedUpdate( GATE ) )				\
      {							\
	register hptr nextH = UpdateNode( GATE );	\
	if( EnqueueHist( GATE, nextH, STIMULI ) )	\
	    (GATE)->nflags |= STIM;			\
      }							\
  }


/*
 * Activate all nodes and transistors in the connection list of node 'nd',
 * scheduling pending events on the outputs and activating any inputs as
 * stimuli.  This routine assumes that the connection list has already been
 * built GetConnList.  Also activate all INPUT nodes connected to the stage.
 * The routine returns TRUE if any of the nodes in the connection list has
 * any pending punted events, else FALSE;
 */
private void ActivateStage( stg )
  pstg  stg;
  {
    register nptr  n;

    n = stg->nd_list;
    do
      {
	register lptr  l;
	register tptr  t;

	if( not (n->nflags & ACTIVE_CL) )
	    ActivateNode( n );

	for( l = n->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( t->tflags & ACTIVE_T )
		continue;
	    t->tflags |= ACTIVE_T;
	    if( t->ttype & GATELIST )
	      {
		for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
		    StimulateGate( t->gate );
	      }
	    else
		StimulateGate( t->gate );
	  }
      }
    while( (n = n->nlink) != NULL );

    n = stg->inp_list;		/* traverse inputs connected to stage */
    while( n != NULL )
      {
	register nptr  next;

	if( not (n->nflags & (ACTIVE_CL | POWER_RAIL)) )
	    (void) ActivateNode( n );

	next = n->nlink;
	n->nlink = NULL;
	n = next;
      }
  }


/*
 * Merge any 'future' punted events from previous runs with the current
 * history for the node.  Note that events created prior to cur_delta are
 * simply discarded (if they belong in the history the should have been
 * recreated and re-punted).
 */
private void MergePunts( nd )
  nptr  nd;
  {
    register hptr  h, p, nx;

	/* skip past any punted events already in the history */
    for( nx = nd->curr; nx->next->punt; nx = nx->next );

    p = nd->t.punts;
    do
      {
	h = p;
	p = p->next;
	if( QPTIME( h ) < cur_delta )		/* in the past, free it */
	  {
	    h->next = freeHist;
	    freeHist = h;
	  }
	else					/* in the future, merge */
	  {
	    h->next = nx->next;
	    nx->next = h;
	    nx = h;
	  }
      }
    while( p != NULL );

    nd->t.punts = NULL;
  }


/*
 * Deactivate all nodes on the connection list and destroy the list.  If a
 * node has any gate connections then turn it into a stimulus node.  Also
 * deactivate any non-OFF transistor on the connection list and move any
 * punted events from the previous simulation that have yet to be enqueued
 * back into the history.  We can't deactivate OFF transistors because some
 * of them may be boundary transistors.
 * For all transistors in the stage, clear the src/drn caches and any flags
 * that may have been set when building stage.
 */
private void DeactivateStage( stg, skipnd )
  pstg  stg;
  nptr  skipnd;
  {
    register nptr  n, next;

    for( n = stg->nd_list; n != NULL; n = next )
      {
	register lptr  l;
	register tptr  t;

	next = n->nlink;
	n->nlink = NULL;

#ifdef INCDEBUG
	if( n->nflags & WATCHED )
	    lprintf(stdout, "%.2f: Deactivate %s\n",d2ns(cur_delta),pnode(n));
#endif
	if( n->nflags & ACTIVE_CL )
	  {
	    if( n->c.event != NULL and n->c.event->ntime > cur_delta )
		DequeueEvent( n );
	    n->nflags &= ~ACTIVE_CL;
	    if( n->t.punts != NULL )
		MergePunts( n );
	  }
	if( n->ngate != NULL and not (n->nflags & STIM) and n != skipnd )
	  {
	    hptr  h;
	    NEXTH( h, n->curr );
	    if( EnqueueHist( n, h, STIMULI ) )
		n->nflags |= STIM;
	  }

	for( l = n->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    t->dcache.r = NULL;		/* may be set for parallel xtors */

	    if( (t->tflags & ACTIVE_T) and (t->state != OFF or
	      (other_node( t, n )->nflags & INPUT))  )
		t->tflags = 0;
	    else
		t->tflags &= ~(BROKEN | PBROKEN | PARALLEL);
	  }
      }

    for( n = stg->inp_list; n != NULL; n = next )
      {
	next = n->nlink;
	n->nlink = NULL;
      }
  }



private void UndoStage( stg )
  pstg  stg;
  {
    register nptr  n, next;
    register lptr  l;
    register tptr  t;

    for( n = stg->nd_list; n != NULL; n = next )
      {
	next = n->nlink;
	n->nlink = NULL;

	for( l = n->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    t->dcache.r = NULL;
	    t->tflags &= ~(BROKEN | PBROKEN | PARALLEL);
	  }
      }

    for( n = stg->inp_list; n != NULL; n = next )
      {
	next = n->nlink;
	n->nlink = NULL;
      }
  }


/*
 * Evaluate the source/drain of all active transistors controlled by 'nd'.
 * Activate/Deactivate clusters as required.  Return TRUE if there are any
 * active transistors controlled by this node, FALSE otherwise.
 * If force is TRUE then evaluate even if all nodes in the clusters are
 * merged.
 */
private int EvalSrcDrn( nd, force )
  nptr    nd;
  int     force;
  {
    register lptr  l;
    register int   found;
    register nptr  nterm;
    pstg           stg;

    found = FALSE;
    cur_node = nd;
    for( l = nd->ngate; l != NULL; l = l->next )
      {
	tptr  t = l->xtor;

	if( not( t->tflags & ACTIVE_T ) )
	    continue;

	found = TRUE;
	nterm = t->source;
	if( nterm->nflags & VISITED )
	  {
	    stg = GetConnList( nterm );
	    if( stg->flags & ONLY_INPUT )
	      {
		if( not (nterm->nflags & (ACTIVE_CL | POWER_RAIL)) and 
		  nd->nflags & DEVIATED )
		    (void) ActivateNode( nterm );
	      }
	    else if( (stg->flags & ALL_MERGED) and not force )
		DeactivateStage( stg, (nptr) NULL );
	    else
	      {
		ActivateStage( stg );
		(*curr_model)( nterm );
	      }
	  }

	nterm = t->drain;
	if( nterm->nflags & VISITED )
	  {
	    stg = GetConnList( nterm );
	    if( stg->flags & ONLY_INPUT )
	      {
		if( not (nterm->nflags & (ACTIVE_CL | POWER_RAIL)) and 
		  nd->nflags & DEVIATED )
		    ActivateNode( nterm );
	      }
	    else if( (stg->flags & ALL_MERGED) and not force )
		DeactivateStage( stg, (nptr) NULL );
	    else
	      {
		ActivateStage( stg );
		(*curr_model)( nterm );
	      }
	  }
      }
    return( found );
  }


/*
 * Evaluate the source/drain of ALL transistors controlled by 'nd', which just
 * deviated.  Different things should be done if a transition caused the node
 * to deviate or a lack of transition.
 */
private void EvalJustDeviated( nd, has_trans )
  nptr  nd;
  int   has_trans;
  {
    pstg  stg;
    lptr  l;

    cur_node = nd;
    for( l = nd->ngate; l != NULL; l = l->next )
      {
	register nptr  nterm;
	register tptr  t;

	t = l->xtor;
	nterm = t->source;
	if( nterm->nflags & VISITED )
	  {
	    stg = GetConnList( nterm );
	    if( stg->flags & ONLY_INPUT )
	      {
		if( not (nterm->nflags & (ACTIVE_CL | POWER_RAIL)) )
		    ActivateNode( nterm );
	      }
	    else
	      {
		ActivateStage( stg );
		if( has_trans or (stg->flags & INP_TRANS) )
		    (*curr_model)( nterm );
		else
		    UndoStage( stg );
	      }
	  }

	nterm = t->drain;
	if( nterm->nflags & VISITED )
	  {
	    stg = GetConnList( nterm );
	    if( stg->flags & ONLY_INPUT )
	      {
		if( not (nterm->nflags & (ACTIVE_CL | POWER_RAIL)) )
		    ActivateNode( nterm );
	      }
	    else
	      {
		ActivateStage( stg );
		if( has_trans or (stg->flags & INP_TRANS) )
		    (*curr_model)( nterm );
		else
		    UndoStage( stg );
	      }
	  }
				/* may happen if src/drn are inputs now */
	if( not (t->tflags & ACTIVE_T) )
	  {
	    t->tflags |= ACTIVE_T;
	    if( t->ttype & GATELIST )
	      {
		for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
		    StimulateGate( t->gate );
		t = l->xtor;
	      }
	    t->state = compute_trans_state( t );
	  }
      }
  }


private void EvalInputs()
  {
    register evptr  ev;
    register nptr   n;

    for( ev = inp_evs; ev != NULL; ev = ev->nlink )
      {
	register nptr   other;
	register lptr   l;
	register tptr   t;
	int             act_tran = FALSE;

	n = cur_node = ev->enode;
	n->nflags &= ~ACTIVE_CL;	/* assume it won't be active */

	for( l = n->nterm; l != NULL; l = l->next )
	  {
	    pstg  stg;

	    t = l->xtor;
	    other = other_node( t, n );
	    stg = GetConnList( other );
	    if( stg->flags & ALL_MERGED )
	      {
		if( not (stg->flags & ONLY_INPUT) )
		    DeactivateStage( stg, (nptr) NULL );
		else if( t->tflags & ACTIVE_T )
		    act_tran = TRUE;
	      }
	    else
	      {
		ActivateStage( stg );
		(*curr_model)( other );
	      }
	  }
	if( act_tran == TRUE and not (n->nflags & (POWER_RAIL | ACTIVE_CL)) )
	  {
	    register hptr  h;

	    NEXTH( h, n->curr );
	    if( EnqueueHist( n, h, CHECK_PNT ) )
		n->nflags |= ACTIVE_CL;
	  }
      }

    for( ev = inp_evs; ev != NULL; ev = ev->nlink )
      {
	int   n_active;

	n = cur_node = ev->enode;
	n_active = EvalSrcDrn( n, FALSE );

	/* 
	 * if node is not active anymore and it controls any 
	 * active transistors turn it into a stimulus node
	 */
	if( n_active == TRUE and not (n->nflags & ACTIVE_CL) )
	  {
	    register hptr  h;

	    NEXTH( h, n->curr );
	    if( EnqueueHist( n, h, STIMULI ) )
		n->nflags |= STIM;
	  }
      }
  }


private void EvalStimulus()
  {
    evptr  ev;
    int    n_active;
    nptr   nd;

    for( ev = stim_evs; ev != NULL; ev = ev->nlink )
      {
	nd = ev->enode;
	n_active = EvalSrcDrn( nd, FALSE );

	if( not n_active )			/* no active transistors */
	    nd->nflags &= ~STIM;
	else if( nd->nflags & STIM )		/* enqueue next transition */
	  {
	    register hptr  h;

	    NEXTH( h, nd->curr );
	    if( not EnqueueHist( nd, h, STIMULI ) )
		nd->nflags &= ~STIM;
	  }
      }
  }


private void EvalXinputs()
  {
    register evptr  ev;
    register nptr   n;
    pstg            stg;

    for( ev = xinp_evs; ev != NULL; ev = ev->nlink )
      {
	n = cur_node = ev->enode;
	if( n->nflags & VISITED )
	  {
	    stg = GetConnList( n );
	    if( stg->flags & ALL_MERGED )
		DeactivateStage( stg, (nptr) NULL );
	    else
	      {
		ActivateStage( stg );
		(*curr_model)( n );
	      }
	  }
      }
  }


private void EvalEventList()
  {
    register evptr  e;
    register nptr   n;
    int             status;
    pstg            stg;
    
    for( e = dev_evs; e != NULL; e = e->nlink )
      {
	n = e->enode;
	if( (e->type & O_DEV) == 0 )
	    EvalJustDeviated( n, TRUE );
	else
	    (void) EvalSrcDrn( n, FALSE );

	  /* if the node is still active schedule its next CHECK_PNT event */
	if( (n->nflags & ACTIVE_CL) and (e->type & CHK) )
	  {
	    hptr  h;

	    NEXTH( h, n->curr );
	    (void) EnqueueHist( n, h, CHECK_PNT );
	  }
      }

    for( e = mrg_evs; e != NULL; e = e->nlink )
      {
	status = ( (e->type & (SAME_T | O_DEV)) == O_DEV ) ? TRUE : FALSE;
	    
	if( EvalSrcDrn( e->enode, status ) )
	    e->type |= N_ACTV;
      }

    for( e = mrg_evs; e != NULL; e = e->nlink )
      {
	n = e->enode;
	stg = GetConnList( n );
	if( stg->flags & ALL_MERGED )
	    DeactivateStage( stg, (e->type & N_ACTV) ? (nptr) NULL : n );
	else
	    UndoStage( stg );

	if( (n->nflags & ACTIVE_CL) and (e->type & CHK) )
	  {
	    hptr  h;

	    NEXTH( h, n->curr );
	    (void) EnqueueHist( n, h, CHECK_PNT );
	  }
      }

    for( e = chk_evs; e != NULL; e = e->nlink )
      {
	n = e->enode;
	switch( e->type & (EDGE| O_DEV | N_DEV) )
	  {
	    case 0 :
		lprintf( stderr, "warning: case 0 time=%.2f for %s\n",
		  d2ns( cur_delta ), pnode( n ) );
		break;

	    case O_DEV :	/* Node just merged but no transition */
		stg = GetConnList( n );
		if( stg->flags & ALL_MERGED )
		    DeactivateStage( stg, (nptr) NULL );
		else
		    UndoStage( stg );
		break;

	    case N_DEV :	/* Node just deviated but no transition */
		n = e->enode;
		EvalJustDeviated( n, FALSE );
		break;

	    case O_DEV | N_DEV :/* Node stays deviated but no transition */
		break;		/* -- do nothing -- */

 	    default:
		lprintf( stderr, "bad chk event (0x%2x) @ t=%.2f\n", e->type,
		  d2ns( cur_delta ) );
	  }

	if( (n->nflags & ACTIVE_CL) and (e->type & CHK) )
	  {
	    hptr  h;

	    NEXTH( h, n->curr );
	    (void) EnqueueHist( n, h, CHECK_PNT );
	  }
      }
  }



/*
 * Events pending from a previous simulation run.  If the node is active at
 * this point, simply drop the event (the node is being resimulated) otherwise
 * re-schedule the pending event (as if it had been simulated).
*/
private void EvalPending()
  {
    register evptr  ev;
    register nptr   n;

    for( ev = pend_evs; ev != NULL; ev = ev->nlink )
      {
	n = ev->enode;
	if( not (n->nflags & ACTIVE_CL) )
	    enqueue_event( n, (int) ev->eval, (long) ev->delay, (long) ev->rtime );
      }
  }


private void incstep( stop_time )
  Ulong  stop_time;
  {
    evptr  evlist;
    Ulong   step_t, refresh;

    refresh = (stop_time < 10) ? 1 : stop_time / 10;
    step_t = cur_delta + refresh;
    if( DoingFault ) step_t = MAX_TIME;		/* never refresh */

    while( (evlist = get_next_event( stop_time )) != NULL )
      {
	update_nodes( evlist );
	UpdateTransistors();

	EvalEventList();

	if( inp_evs )	    EvalInputs();
	if( xinp_evs )	    EvalXinputs();
	if( stim_evs )	    EvalStimulus();
	if( pend_evs )	    EvalPending();

	FreeEventList( evlist );

	if( cur_delta >= step_t )
	  {
	    do
	      {
		lprintf( stdout, "time = %d.0\n", (int) d2ns( step_t ) );
		(void) fflush( stdout );
		step_t += refresh;
	      }
	    while( cur_delta >= step_t );
	    if( analyzerON )
		UpdateWindow( cur_delta - 1 );
	  }
#ifdef FAULT_SIM
	if( stop_early or int_received )
	    return;
#endif
      }
    cur_delta = stop_time;
#ifndef FAULT_SIM
    if( analyzerON )
	UpdateWindow( cur_delta );
#endif
  }


private int fix_inc_nodes( nd )
  register nptr  nd;
  {
    register hptr  h;
    register lptr  l;

    if( nd->nflags & (ALIAS | MERGED) )
	return( 0 );

    if( (nd->nflags & ACTIVE_CL) and nd->t.punts != NULL )  /* shouldn't be */
      {
	for( h = nd->t.punts; h->next != NULL; h = h->next );
	h->next = freeHist;
	freeHist = nd->t.punts;
      }

    if( nd->nflags & (WAS_ACTIVE | CHANGED) )
	nd->t.cause = inc_cause;

    nd->nflags &= ~(VISITED|CHANGED|DEVIATED|STIM|ACTIVE_CL|WAS_ACTIVE);

    NEXTH( h, nd->curr );
    if( h != last_hist )		/* inconsistent ? */
      {
	register hptr  p;

	do
	  {
	    p = h;
	    NEXTH( h, h );
	  } while( h != last_hist );
	nd->curr = h = p;
      }
    else
	h = nd->curr;

    nd->c.time = h->time;
    nd->npot = h->val;
    if( h->inp )
	nd->nflags |= INPUT;
    else
	nd->nflags &= ~INPUT;

    for( l = nd->ngate; l != NULL; l = l->next )	/* fix transistors */
      {
	register tptr  t = l->xtor;
	t->state = compute_trans_state( t );
	t->tflags &= ~ACTIVE_T;
      }
    for( l = on_trans; l != NULL; l = l->next )
	l->xtor->tflags &= ~ACTIVE_T;

    return( 0 );
  }


/*
 * Start evaluating/activating the changed nodes.  This will schedulle other
 * events to get things rolling.  Special care is needed for changed nodes
 * that became inputs at time 0; we must ensure that their adjacent (src/drn)
 * transistors have the proper state, so things work out when the node
 * becomes undriven (if at all) and revaluated properly,
 */
private void startup_isim( n )
  nptr n;			/* the changed node */
  {
    pstg  stg;

    stg = GetConnList( n );
    ActivateStage( stg );
    if( stg->flags & INP_TRANS )	/* transition at time 0 => evaluate */
	(*curr_model)( n );
    else if( stg->flags & ONLY_INPUT )	/* node is an input at time 0 */
      {
	register lptr  l;
	register tptr  t;

	for( l = n->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    t->state = compute_trans_state( t );
	  }
	UndoStage( stg );
      }
    else				/* no transitions at time 0, undo */
	UndoStage( stg );
  }


/*
 * Main incremental simulation routine.  Move back to time 0, changing all
 * pending events to PENDING type.  Activate the clusters of all the
 * nodes in 'changed nodes' list and start simulating until we reach the
 * current time.  Make sure that the whole network is in a consistent state
 * before returning.
 */
public void incsim( ch_list )
  iptr  ch_list;
  {
    Ulong  stop_time;

    o_nevals = nevals;
    nevals = i_nevals;
    o_nevent = nevent;
    sm_stat |= INCR_SIM;

    stop_time = cur_delta;
    cur_delta = sim_time0;

    (void) back_sim_time( cur_delta, TRUE );

    modelp = first_model;			/* initialize the model */
    curr_model = model_table[ modelp->val ];
    modelp = modelp->next;
    if( modelp != NULL )
	(void) EnqueueOther( CHNG_MODEL, (Ulong) modelp->time );

    if( ch_list != NULL )
      {
	register iptr  ip;

	if( stop_time != 0 )		/* work to do ? */
	  {
	    for( ip = ch_list; ip != NULL; ip = ip->next )
		ip->inode->nflags |= VISITED;	/* set VISITED on all nodes */
	  }
	else
	  {
	    while( (ip = ch_list) != NULL )
	      {
		ch_list = ch_list->next;
		ip->inode->nflags &= ~(VISITED);
		FreeInput( ip );
	      }
	  }

	while( (ip = ch_list) != NULL )
	  {
	    ch_list = ch_list->next;
	    if( ip->inode->nflags & VISITED )
		startup_isim( ip->inode );
	    FreeInput( ip );
	  }
      }

    incstep( stop_time );

    rm_inc_events( FALSE );

    walk_net( fix_inc_nodes, (char *) 0 );

    sm_stat &= ~INCR_SIM;
    i_nevals = nevals;
    nevals = o_nevals;
    nevent = o_nevent;
  }


#ifdef FAULT_SIM
public void init_faultsim()
  {
    o_nevals = nevals;
    nevals = i_nevals;
    o_nevent = nevent;
    sm_stat |= INCR_SIM;
    old_cur_delta = cur_delta;
    cur_delta = sim_time0;
    pending_evs = back_sim_time( cur_delta, TRUE + TRUE );
    fault_mode = TRUE;
  }


public void end_faultsim()
  {
    walk_net( fix_inc_nodes, (char *) 0 );
    cur_delta = old_cur_delta;
    requeue_events( pending_evs, TRUE );
    fault_mode = FALSE;
    sm_stat &= ~INCR_SIM;
    i_nevals = nevals;
    nevals = o_nevals;
    nevent = o_nevent;
  }


private int fix_fault_nodes( nd )
  register nptr  nd;
  {
    register lptr  l;

    if( not (nd->nflags & (ALIAS | MERGED)) )
      {
	nd->nflags &= ~(VISITED|CHANGED|DEVIATED|STIM|ACTIVE_CL|WAS_ACTIVE);

	if( nd->curr == &nd->hchange )
	  {
	    register hptr  h;

	    NEXTH( h, nd->curr );
	    nd->curr = (h == last_hist) ? &nd->head : h;
	  }

	    /* only mark transistors as non-activate for next faultsim */
	for( l = nd->ngate; l != NULL; l = l->next )
	  {
	    register tptr  t = l->xtor;
	    t->tflags &= ~ACTIVE_T;
	  }
	for( l = on_trans; l != NULL; l = l->next )
	    l->xtor->tflags &= ~ACTIVE_T;
      }
    return( 0 );
  }


public void faultsim( n )
  nptr  n;
  {
    Ulong   stop_time;

    stop_time = old_cur_delta;

    cur_delta = sim_time0;
    stop_early = 0;

    modelp = first_model;			/* initialize the model */
    curr_model = model_table[ modelp->val ];
    modelp = modelp->next;
    if( modelp != NULL )
	(void) EnqueueOther( CHNG_MODEL, (Ulong) modelp->time );

    setup_triggers();

    n->nflags |= (VISITED | CHANGED);
    startup_isim( n );

    incstep( stop_time );

    rm_inc_events( TRUE );

    walk_net( fix_fault_nodes, (char *) 0 );
  }
#endif
