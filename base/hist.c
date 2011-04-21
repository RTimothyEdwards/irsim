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

#include "defs.h"
#include "net.h"
#include "ASSERT.h"
#include "globals.h"

#ifdef FAULT_SIM
extern	int	fault_mode;
#define	DoingFault	(fault_mode != 0)
#else
#define	DoingFault	(0)
#define	hchange	head		/* for the compiler's sake */
#endif


public	hptr   freeHist = NULL;		/* list of free history entries */
public  hptr   last_hist;		/* pointer to dummy hist-entry that
					 * serves as tail for all nodes */
public	int    num_edges = 0;
public	int    num_punted = 0;
public	int    num_cons_punted = 0;

public	hptr   first_model;		/* first model entry */
private	hptr   curr_model;		/* ptr. to current model entry */

private	char   mem_msg[] = "*** OUT OF MEMORY:Will stop collecting history\n";

public void init_hist()
  {
    static HistEnt  dummy;
    static HistEnt  dummy_model;

    last_hist = &dummy;
    dummy.next = last_hist;
    dummy.time = MAX_TIME;
    dummy.val = X;
    dummy.inp = 1;
    dummy.punt = 0;
    dummy.t.r.delay = dummy.t.r.rtime = 0;

    dummy_model.time = 0;
    dummy_model.val = model_num;
    dummy_model.inp = 0;
    dummy_model.punt = 0;
    dummy_model.next = NULL;
    first_model = curr_model = &dummy_model;
  }


#define	NEW_HIST( NEW, ACTnoMEM )					\
  {									\
    if( ((NEW) = freeHist) == NULL )					\
      {									\
	if( ((NEW) = (hptr) MallocList( sizeof( HistEnt ), 0 )) == NULL ) \
	  {								\
	    lprintf( stderr, mem_msg );					\
	    sm_stat |= OUT_OF_MEM;					\
	    ACTnoMEM;							\
	  }								\
      }									\
    freeHist = (NEW)->next;						\
  }									\


public void SetFirstHist( node, value, inp, time )
  nptr  node;
  int   value, inp;
  Ulong  time;
  {
    node->head.time = time;
    node->head.val = value;
    node->head.inp = inp;
    node->head.t.r.rtime = node->head.t.r.delay = 0;
  }


/*
 * Add a new entry to the history list.  Update curr to point to this change.
 */
public void AddHist( node, value, inp, time, delay, rtime )
  register nptr  node;
  int            value;
  int            inp;
  Ulong          time;
  long           delay, rtime;
  {
    register hptr  newh, curr;

    num_edges++;

    curr = node->curr;

    if( sm_stat & OUT_OF_MEM )
        return;

    while( curr->next->punt )		/* skip past any punted events */
	curr = curr->next;

    NEW_HIST( newh, return );

    newh->next = curr->next;
    newh->time = time;
    newh->val = value;
    newh->inp = inp;
    newh->punt = 0;
    newh->t.r.delay = delay;
    newh->t.r.rtime = rtime;
    node->curr = curr->next = newh;
  }


#define	PuntTime( H )		( (H)->time - (H)->t.p.ptime )


/*
 * Add a punted event to the history list for the node.  Consecutive punted
 * events are kept in punted-order, so that h->ptime < h->next->ptime.
 * Adding a punted event does not change the current pointer, which always
 * points to the last "effective" node change.
 */
public void AddPunted( node, ev, tim )
  register nptr  node;
  evptr          ev;
  Ulong          tim;
  {
    register hptr  newp, h;

    h = node->curr;

    num_punted++;
    if( (sm_stat & OUT_OF_MEM) or DoingFault )
        return;

    NEW_HIST( newp, return );		/* allocate the punted event itself */

    newp->time = ev->ntime;
    newp->val = ev->eval;
    newp->inp = 0;
    newp->punt = 1;
    newp->t.p.delay = (Uint)(ev->delay);
    newp->t.p.rtime = (Uint)(ev->rtime);
    newp->t.p.ptime = (Uint)(newp->time - tim);

    if( h->next->punt )		/* there are some punted events already */
      {
	num_cons_punted++;
	do { h = h->next; } while( h->next->punt );
      }

    newp->next = h->next;
    h->next = newp;
  }


/*
 * Free up a node's history list
 */
public void FreeHistList( node )
  register nptr  node;
  {
    register hptr  h, next;

    if( (h = node->head.next) == last_hist )		/* nothing to do */
    	return;

    while( (next = h->next) != last_hist )		/* find last entry */
	h = next;

    h->next = freeHist;				/* link list to free list */
    freeHist = node->head.next;

    node->head.next = last_hist;
    node->curr = &(node->head);

    sm_stat &= ~OUT_OF_MEM;
  }



public void NoMoreIncSim()
  {
    lprintf( stderr, "*** can't continue incremetal simulation\n" );
    exit( 1 );		/* do something here ? */
  }


/*
 * Add a new model entry, recording the time of the change.
 */
public void NewModel( nmodel )
  int  nmodel;
  {
    if( curr_model->time != cur_delta )
      {
	hptr newh;

	NEW_HIST( newh, NoMoreIncSim() );

	newh->next = NULL;
	newh->time = cur_delta;
	newh->val = nmodel;
	curr_model->next = newh;
	curr_model = newh;
      }
    else
	curr_model->val = nmodel;
  }



#define	QTIME( H )		( (H)->time - (H)->t.r.delay )

/*
 * Add a new change to the history list of node 'nd' caused by event 'ev'.
 * Skip past any punted events already in the history and update nd->curr to
 * point to the new change.
 */
public void NewEdge( nd, ev )
  nptr   nd;
  evptr  ev;
  {
    register hptr  p, h, newh;

    for( p = nd->curr, h = p->next; h->punt; p = h, h = h->next );

    if( DoingFault )
	p = newh = &nd->hchange;
    else
	NEW_HIST( newh, NoMoreIncSim() );

    newh->time = ev->ntime;
    newh->val = ev->eval;
    newh->inp = 0;		/* always true in incremental simulation */
    newh->punt = 0;
    newh->t.r.delay = ev->delay;
    newh->t.r.rtime = ev->rtime;

    p->next = newh;
    newh->next = h;

    nd->curr = newh;		/* newh becomes the current value */
  }


/*
 * Delete the next effective change (following nd->curr) from the history.
 * Punted events before the next change (in nd->t.punts) can now be freed.
 * Punted events following the deleted edge are moved to nd->t.punts.
 */
public void DeleteNextEdge( nd )
  nptr  nd;
  {
    register hptr  a, b, c;

    if( DoingFault )
      {
	if( nd->t.punts != NULL ) lprintf( stderr, "non-null punts\n" );
	
	if( nd->curr != &nd->hchange )
	  {
	    nd->hchange = *nd->curr;
	    nd->curr = &nd->hchange;
	  }
	
	a = nd->curr;
	for( b = a->next; b->punt; a = b, b = b->next );

	nd->hchange.next = b->next;	/* b->next => punt before next edge */
	nd->t.punts = NULL;
	return;
      }

    if( (a = nd->t.punts) != NULL )	/* remove previously punted events */
      {
	for( b = a; b->next != NULL; b = b->next );
	b->next = freeHist;
	freeHist = a;
      }

    a = nd->curr;
    for( b = a->next; b->punt; a = b, b = b->next );
    for( c = b->next; c->punt; b = c, c = c->next );
    c = a->next;			/* c => next edge */
    a->next = b->next;
    a = c->next;			/* a => first punted event after c */

    c->next = freeHist;			/* free the next effective change */
    freeHist = c;

    if( a->punt )			/* move punted events from hist */
      {
	nd->t.punts = a;
	b->next = NULL;
      }
    else
	nd->t.punts = NULL;
  }


#define	NEXTH( H, P )	for( (H) = (P)->next; (H)->punt; (H) = (H)->next )

public void FlushHist( ftime )
  register Ulong  ftime;
  {
    register nptr  n;
    register hptr  h, p, head;

    for( n = GetNodeList(); n != NULL; n = n->n.next )
      {
	head = &(n->head);
	if( head->next == last_hist or (n->nflags & ALIAS) )
	    continue;
	p = head;
	NEXTH( h, p );
	while( h->time < ftime )
	  {
	    p = h;
	    NEXTH( h, h );
	  }
	head->val = p->val;
	head->time = p->time;
	head->inp = p->inp;
	while( p->next != h )
	    p = p->next;
	if( head->next != h )
	  {
	    p->next = freeHist;
	    freeHist = head->next;
	    head->next = h;
	  }
	if( n->curr->time < ftime )
	  {
	    n->curr = head;
	  }
      }
  }


public int backToTime( nd )
  register nptr  nd;
  {
    register hptr  h, p;

    if( nd->nflags & (ALIAS | MERGED) )
	return( 0 );

    h = &(nd->head);
    NEXTH( p, h );
    while( p->time < cur_delta )
      {
	h = p;
	NEXTH( p, p );
      }
    nd->curr = h;

	/* queue pending events */
    for( p = h, h = p->next; ; p = h, h = h->next )
      {
	Ulong  qtime;

	if( h->punt )
	  {
	    if( PuntTime( h ) < cur_delta )	/* already punted, skip it */
		continue;

	    qtime = h->time - h->t.p.delay;	/* pending, enqueue it */
	    if( qtime < cur_delta )
	      {
		Ulong tmp = cur_delta;
		cur_delta = qtime;
		enqueue_event( nd, (int) h->val, (long) h->t.p.delay,
		  (long) h->t.p.rtime );
		cur_delta = tmp;
	      }
	    p->next = h->next;
	    h->next = freeHist;
	    freeHist = h;
	    h = p;
	  }
	else
	  {
	    qtime = QTIME( h );
	    if( qtime < cur_delta )		/* pending, enqueue it */
	      {
		Ulong tmp = cur_delta;
		cur_delta = qtime;
		enqueue_event( nd, (int) h->val, (long) h->t.r.delay,
		  (long) h->t.r.rtime );
		cur_delta = tmp;

		p->next = h->next;		/* and free it */
		h->next = freeHist;
		freeHist = h;
		h = p;
	      }
	    else
		break;
	  }
      }

    p->next = last_hist;
    p = h;
	/* p now points to the 1st event in the future (to be deleted) */
    if( p != last_hist )
      {
	while( h->next != last_hist )
	    h = h->next;
	h->next = freeHist;
	freeHist = p;
      }

    h = nd->curr;
    nd->npot = h->val;
    nd->c.time = h->time;
    if( h->inp )
	nd->nflags |= INPUT;

    if( nd->ngate != NULL )		/* recompute transistor states */
      {
	register lptr  l;
	register tptr  t;

	for( l = nd->ngate; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    t->state = compute_trans_state( t );
	  }
      }
    return( 0 );
  }
