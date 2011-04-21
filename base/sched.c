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
#include "ASSERT.h"


#define	TSIZE		16384	/* size of event array, must be power of 2 */
#define	TMASK		(TSIZE - 1)

typedef struct
  {
    evptr    flink, blink;	/* pointers in doubly-linked list */
  } evhdr;


public	int    debug = 0;	    /* if <>0 print lotsa interesting info */

public	Ulong   cur_delta;	    /* current simulated time */
public	nptr   cur_node;	    /* node that belongs to current event */
public	long   nevent;		    /* number of current event */

public	evptr  evfree = NULL;	    /* list of free event structures */
public	int    npending;	    /* number of pending events */
public	int    spending;	    /* number of scheduled events */

private	evhdr  ev_array[TSIZE];	    /* used as head of doubly-linked lists */

/*
 * Sets the parameter "list" to contain a pointer to the list of pending 
 * events at time "cur_delta + delta".  "end_of_list" contains a pointer to
 * the end of the list. If there are no events at the indicated time "list"
 * is set to NULL.  The function returns 0 if there are no events past the
 * "cur_delta + delta", otherwise it returns the delta at which the next
 * events are found;
 */

public Ulong pending_events( delta, list, end_of_list )
  Ulong   delta;
  evptr  *list, *end_of_list;
  {
    evhdr           *hdr;
    register evptr  ev;

    *list = NULL;

    delta += cur_delta;
    hdr = &ev_array[ delta & TMASK ];

    if( hdr->flink == (evptr) hdr or hdr->blink->ntime < delta )
	goto find_next;

    for( ev = hdr->flink; ev->ntime < delta; ev = ev->flink );
    if( ev->ntime != delta )
	goto find_next;

    *list = ev;
    if( hdr->blink->ntime == ev->ntime )
	*end_of_list = hdr->blink;
    else
      {
	for( delta = ev->ntime; ev->ntime == delta; ev = ev->flink );
	*end_of_list = ev->blink;
      }

  find_next:
     {
	register long  i, limit;
	Ulong time;

	time = MAX_TIME;
	for( i = ++delta, limit = i + TSIZE; i < limit; i++ )
	  {
	    ev = (evptr) &ev_array[ i & TMASK ];
	    if( ev != ev->flink )
	      {
		if( ev->blink->ntime < delta )
		    continue;
		for( ev = ev->flink; ev->ntime < delta; ev = ev->flink );
		if( ev->ntime < limit )
		  {
		    time = ev->ntime;
		    break;
		  }
		else if( ev->flink->ntime < time )
		    time = ev->flink->ntime;
	      }
	  }
	delta = ( time != MAX_TIME ) ? time - cur_delta : 0;
      }

    return( delta );
  }


/*
 * find the next event to be processed by scanning event wheel.  Return
 * the list of events to be processed at this time, removing it first
 * from the time wheel.
 */
public evptr get_next_event( stop_time )
  Ulong  stop_time;
  {
    register evptr  event;
    Ulong i, time, limit;

    if (npending == 0)
	return(NULL);

    time = MAX_TIME;
    for( i = cur_delta, limit = i + TSIZE; i < limit; i++ )
      {
	event = (evptr) &ev_array[ i & TMASK ];
	if( event != event->flink )
	  {
	    if( event->flink->ntime < limit )		/* common case */
		goto found;
	    if( event->flink->ntime < time )
		time = event->flink->ntime;
	  }
      }

    if( time != MAX_TIME )
	event = (evptr) &ev_array[ time & TMASK ];
    else
      {
	lprintf( stderr, "*** internal error: no events but npending set\n" );
	return( NULL );
      }

  found:
      {
	evptr  evlist = event->flink;

	time = evlist->ntime;

	if( time >= stop_time )
	    return( NULL );

	/* sanity check for incremental simulation mostly */
	ASSERT( time >= cur_delta )
	  {
	    lprintf( stderr, "time moving back %d -> %d\n", cur_delta,
	     evlist->ntime );
	  }

	cur_delta = time;			/* advance simulation time */

	if( event->blink->ntime != time )	/* check tail of list */
	  {
	    do
		event = event->flink;
	    while( event->ntime == time );

	    event = event->blink;		/* grab part of the list */
	    evlist->blink->flink = event->flink;
	    event->flink->blink = evlist->blink;
	    evlist->blink = event;
	    event->flink = NULL;
	  }
	else
	  {
	    event = evlist->blink;		/* grab the entire list */
	    event->blink->flink = NULL;
	    evlist->blink = event->blink;
	    event->flink = event->blink = (evptr) event;
	  }
	return( evlist );
      }
  }


/* remove event from node's list of pending events */
public
#define free_from_node( ev, nd )					\
  {									\
    if( (nd)->events == (ev) )						\
	(nd)->events = (ev)->nlink;					\
    else								\
      {									\
	register evptr  evp;						\
	for( evp = (nd)->events; evp->nlink != (ev); evp = evp->nlink );\
	evp->nlink = (ev)->nlink;					\
      }									\
  }


public
#define	FreeEventList( L )	(L)->blink->flink = evfree, evfree = (L)


/*
 * remove event from all structures it belongs to and return it to free pool
 */
public void free_event( event )
  register evptr  event;
  {
	/* unhook from doubly-linked event list */
    event->blink->flink = event->flink;
    event->flink->blink = event->blink;
    npending--;
    if (event->type == TIMED_EV) spending--;

	/* add to free storage pool */
    event->flink = evfree;
    evfree = event;

    if (event->type != TIMED_EV)
	free_from_node( event, event->enode );
  }

/* get event structure.  Allocate a bunch more if we've run out. */
#define NEW_EVENT( NEW )					\
  {								\
    if( ((NEW) = evfree) == NULL )				\
	(NEW) = (evptr) MallocList( sizeof( struct Event ), 1 );\
    evfree = (NEW)->flink;					\
  }


/*
 * Add an event to event list, specifying transition delay and new value.
 * 0 delay transitions are converted into unit delay transitions (0.01 ns).
 */
public void enqueue_event( n, newvalue, delta, rtime )
  register nptr  n;
  long           delta, rtime;
  {
    register evptr  marker, new;
    Ulong   etime;

	/* check against numerical errors from new_val routines */
    ASSERT( delta > 0 and delta < 60000 )
      {
	lprintf( stderr, "bad event @ %.2fns: %s ,delay=%ddeltas\n",
	  d2ns( cur_delta ), pnode( n ), delta );
	delta = rtime = 1;
      }

    NEW_EVENT( new );

	/* remember facts about this event */
    new->ntime = etime = cur_delta + delta;
    new->rtime = rtime;
    new->enode = n;
    new->p.cause = cur_node;
    new->delay = delta;
    if( newvalue == DECAY )		/* change value to X here */
      {
	new->eval = X;
	new->type = DECAY_EV;
      }
    else
      {
	new->eval = newvalue;
	new->type = REVAL;		/* for incremental simulation */
      }

	/* add the new event to the event list at the appropriate entry
	 * in event wheel.  Event lists are kept sorted by increasing
	 * event time.
	 */
    marker = (evptr) & ev_array[ etime & TMASK ];

	/* Check whether we need to insert-sort in the list */
    if( (marker->blink != marker) and (marker->blink->ntime > etime) )
      {
	do { marker = marker->flink; } while( marker->ntime <= etime );
      }

	/* insert event right before event pointed to by marker */
    new->flink = marker;
    new->blink = marker->blink;
    marker->blink->flink = new;
    marker->blink = new;
    npending++;

	/* 
	 * thread event onto list of events for this node, keeping it
	 * in sorted order
	 */
    if( (n->events != NULL) and (n->events->ntime > etime) )
      {
	for( marker = n->events; (marker->nlink != NULL) and
	  (marker->nlink->ntime > etime); marker = marker->nlink );
	new->nlink = marker->nlink;
	marker->nlink = new;
      }
    else
      {
	new->nlink = n->events;
	n->events = new;
      }
  }


/* same as enqueue_event, but assumes 0 delay and rise/fall time */
public void enqueue_input( n, newvalue )
  register nptr  n;
  {
    register evptr  marker, new;
    register Ulong  etime;

	/* Punt any pending events for this node. */
    while( n->events != NULL )
	free_event( n->events );

    NEW_EVENT( new );

	/* remember facts about this event */
    new->ntime = etime = cur_delta;
    new->rtime = new->delay = 0;
    new->enode = new->p.cause = n;
    new->eval = newvalue;
    new->type = REVAL;			/* anything, doesn't matter */

     /* Add new event to HEAD of list at appropriate entry in event wheel */

    marker = (evptr) & ev_array[ etime & TMASK ];
    new->flink = marker->flink;
    new->blink = marker;
    marker->flink->blink = new;
    marker->flink = new;
    npending++;

	/* thread event onto (now empty) list of events for this node */
    new->nlink = NULL;
    n->events = new;
  }


/*
 * Initialize event structures
 */
public void init_event()
  {
    register int    i;
    register evhdr  *event;

    for( i = 0, event = &ev_array[0]; i < TSIZE; i += 1, event += 1 )
      {
	event->flink = event->blink = (evptr) event;
      }
    npending = 0;
    spending = 0;
    nevent = 0;
  }


public void PuntEvent( node, ev )
  nptr   node;
  evptr  ev;
  {
    if( node->nflags & WATCHED )
	lprintf( stdout,
	  "    punting transition of %s -> %c scheduled for %2.2fns\n",
	  node->nname, vchars[ev->eval], d2ns( ev->ntime ) );

    ASSERT( node == ev->enode )
      {
	lprintf( stderr, "bad punt @ %d for %s\n", cur_delta, node->nname );
	node->events = NULL;
	return;
      }

    if( ev->type != DECAY_EV )		/* don't save punted decay events */
	AddPunted( ev->enode, ev, cur_delta );
    free_event( ev );
  }


public void requeue_events( evlist, thread )
  evptr  evlist;
  int    thread;
  {
    register Ulong   etime;
    register evptr  ev, next, target;

    npending = 0;
    spending = 0;
    for( ev = evlist; ev != NULL; ev = next )
      {
	next = ev->flink;

	npending++;
	etime = ev->ntime;
	target = (evptr) & ev_array[ etime & TMASK ];

	if( (target->blink != target) and (target->blink->ntime > etime) )
	  {
	    do { target = target->flink; } while( target->ntime <= etime );
	  }

	ev->flink = target;
	ev->blink = target->blink;
	target->blink->flink = ev;
	target->blink = ev;

	if (ev->type == TIMED_EV)
	    spending++;
	else if (thread)
	  {
	    register nptr   n = ev->enode;
	    register evptr  marker;

	    if( (n->events != NULL) and (n->events->ntime > etime) )
	      {
		for( marker = n->events; (marker->nlink != NULL) and
	  	(marker->nlink->ntime > etime); marker = marker->nlink );
		ev->nlink = marker->nlink;
		marker->nlink = ev;
	      }
	    else
	      {
		ev->nlink = n->events;
		n->events = ev;
	      }
	  }
      }
  }

	/* Incremental simulation routines */

/*
 * Back the event queues up to time 'btime'.  This is the opposite of
 * advancing the simulation time.  Mark all pending events as PENDING,
 * and re-enqueue them according to their creation-time (ntime - delay).
 */

public evptr back_sim_time( btime, is_inc )
  Ulong  btime;
  int   is_inc;
  {
    evptr           tmplist;
    register int    nevents;
    register evptr  ev, next;
    register evhdr  *hdr, *endhdr;

    nevents = 0;
    tmplist = NULL;

	/* first empty out the time wheel onto the temporary list */
    for( endhdr = &ev_array[ TSIZE ], hdr = ev_array; hdr != endhdr ; hdr++ )
      {
	for( ev = hdr->flink; ev != (evptr) hdr; ev = next )
	  {
	    next = ev->flink;

	    ev->blink->flink = ev->flink;	/* remove event */
	    ev->flink->blink = ev->blink;
	    if( is_inc )
		free_from_node( ev, ev->enode );

	    if( (not is_inc) and ev->ntime - ev->delay >= btime )
	      {
		free_from_node( ev, ev->enode );
		ev->flink = evfree;
		evfree = ev;
	      }
	    else
	      {
		ev->flink = tmplist;		/* move it to tmp list */
		tmplist = ev;

		nevents++;
	      }
	  }
      }

    if( not is_inc )
      {
	requeue_events( tmplist, FALSE );
	return( NULL );
      }

    if( is_inc != TRUE )	/* only for fault simulation (is_inc == 2) */
      {
	npending = 0;
	return( tmplist );
      }

	/* now move the temporary list to the time wheel */
    for( ev = tmplist; ev != NULL; ev = next )
      {
	register Ulong   etime;
	register evptr  target;

	next = ev->flink;

	ev->ntime -= ev->delay;
	if (ev->type != TIMED_EV) ev->type = PENDING;
	etime = ev->ntime;
	target = (evptr) & ev_array[ etime & TMASK ];

	if( (target->blink != target) and (target->blink->ntime > etime) )
	  {
	    do { target = target->flink; } while( target->ntime <= etime );
	  }

	ev->flink = target;
	ev->blink = target->blink;
	target->blink->flink = ev;
	target->blink = ev;
      }

    npending = nevents;
    return( NULL );
  }


/*
 * Enqueue event type 'type' form history entry 'hist' for node 'nd'.
 * Note that events with type > THREAD are not threaded onto a node's list
 * of pending events, since we don't want the new_val routines to look at
 * this event, instead we keep track of these events in the c.event event
 * of every node.  Return FALSE if this history entry is the sentinel
 * (last_hist), otherwise return TRUE.
 */
public int EnqueueHist( nd, hist, type )
  nptr  nd;
  hptr  hist;
  int   type;
  {
    register evptr  marker, new;
    register Ulong   etime;

    if( hist == last_hist )			/* never queue this up */
      {
	nd->c.event = NULL;
	return( FALSE );
      }

    NEW_EVENT( new );

	/* remember facts about this event */
    new->ntime = etime = hist->time;
    new->eval = hist->val;
    new->enode = nd;
    new->p.hist = hist;
    if( hist->punt )
      {
	new->delay = hist->t.p.delay;
	new->rtime = hist->t.p.rtime;
      }
    else
      {
	new->delay = hist->t.r.delay;
	new->rtime = hist->t.r.rtime;
      }

    marker = (evptr) & ev_array[ etime & TMASK ];

	/* Check whether we need to insert-sort in the list */
    if( (marker->blink != marker) and (marker->blink->ntime > etime) )
      {
	do { marker = marker->flink; } while( marker->ntime <= etime );
      }

	/* insert event right before event pointed to by marker */
    new->flink = marker;
    new->blink = marker->blink;
    marker->blink->flink = new;
    marker->blink = new;
    npending++;

    if( hist->inp )
	type |= IS_INPUT;
    else if( new->delay == 0 )
	type |= IS_XINPUT;
    new->type = type;

    if( type > THREAD )
      {
	nd->c.event = new;
	return( TRUE );
      }

    if( (nd->events != NULL) and (nd->events->ntime > etime) )
      {
	for( marker = nd->events; (marker->nlink != NULL) and
	  (marker->nlink->ntime > etime); marker = marker->nlink );
	new->nlink = marker->nlink;
	marker->nlink = new;
      }
    else
      {
	new->nlink = nd->events;
	nd->events = new;
      }
    return( TRUE );
  }

/*
 * Find a scheduled event in the event queue and return a pointer to it.
 */

public evptr FindScheduled(idx)
   short idx;
{
    register evptr  ev, next;
    register evhdr  *hdr, *endhdr;

    for (endhdr = &ev_array[TSIZE], hdr = ev_array; hdr != endhdr; hdr++)
    {
	for (ev = hdr->flink; ev != (evptr) hdr; ev = next)
	{
	    next = ev->flink;
	    if ((ev->type == TIMED_EV) && (ev->rtime == idx))
	    {
		return ev;
	    }
	}
    }
    return NULL;
}

/*
 * Remove a scheduled event from the event queue
 */

public void DequeueScheduled(idx)
   short idx;
{
    register evptr  ev;

    ev = FindScheduled(idx);
    if (ev) free_event(ev);
}

/*
 * Remove a pending event from the event queue and from the node
 * that generated it.
 */

public void DequeueEvent( nd )
  register nptr nd;
  {
    register evptr  ev;

    ev = nd->c.event;
    ev->blink->flink = ev->flink;
    ev->flink->blink = ev->blink;
    ev->flink = evfree;
    evfree = ev;
    nd->c.event = NULL;

    npending--;
  }


public void DelayEvent( ev, delay )
  evptr  ev;
  long   delay;
  {
    register evptr  marker, new;
    register nptr   nd;
    Ulong   etime;

    nd = ev->enode;
    NEW_EVENT( new );
    
	/* remember facts about this event */
    *new = *ev;
    new->delay += delay;
    new->ntime += delay;

    etime = new->ntime;

    marker = (evptr) & ev_array[ etime & TMASK ];

	/* Check whether we need to insert-sort in the list */
    if( (marker->blink != marker) and (marker->blink->ntime > etime) )
      {
	do { marker = marker->flink; } while( marker->ntime <= etime );
      }

	/* insert event right before event pointed to by marker */
    new->flink = marker;
    new->blink = marker->blink;
    marker->blink->flink = new;
    marker->blink = new;
    npending++;

    if( new->type > THREAD )
      {
	nd->c.event = new;
	return;
      }

    if( (nd->events != NULL) and (nd->events->ntime > etime) )
      {
	for( marker = nd->events; (marker->nlink != NULL) and
	  (marker->nlink->ntime > etime); marker = marker->nlink );
	new->nlink = marker->nlink;
	marker->nlink = new;
      }
    else
      {
	new->nlink = nd->events;
	nd->events = new;
      }
  }


public evptr EnqueueOther( type, time )
  int   type;
  Ulong  time;
  {
    register evptr  marker, new;
    Ulong   etime;

    NEW_EVENT( new );

    new->ntime = etime = time;
    new->type = type;

    if (new->type == TIMED_EV) spending++;

    marker = (evptr) & ev_array[ etime & TMASK ];

	/* Check whether we need to insert-sort in the list */
    if( (marker->blink != marker) and (marker->blink->ntime > etime) )
      {
	do { marker = marker->flink; } while( marker->ntime <= etime );
      }

	/* insert event right before event pointed to by marker */
    new->flink = marker;
    new->blink = marker->blink;
    marker->blink->flink = new;
    marker->blink = new;
    npending++;
    return( new );
  }


/*
 * Remove any events that may be left from the incremental simulation.
 */
public void rm_inc_events( all )
  int  all;
  {
    register int    nevents;
    register evptr  ev, next;
    register evhdr  *hdr, *endhdr;

    nevents = 0;

    for( endhdr = &ev_array[ TSIZE ], hdr = ev_array; hdr != endhdr; hdr++ )
      {
	for( ev = hdr->flink; ev != (evptr) hdr; ev = next )
	  {
	    next = ev->flink;
	    if( all or ev->type >= THREAD )
	      {
		ev->blink->flink = next;		/* remove event */
		ev->flink->blink = ev->blink;
		ev->flink = evfree;
		evfree = ev;
		if( ev->type <= THREAD )
		    free_from_node( ev, ev->enode );
	      }
	    else
		nevents++;
	  }
      }
    npending = nevents;
  }
