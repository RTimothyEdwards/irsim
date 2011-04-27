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
 * Event-driven timing simulation step for irsim.
 * 
 * Use diamond shape region in R-V plane for dc voltage computation.
 * Use 2-pole 1-zero model for pure charge sharing delay calculations.
 * Use 2-pole zero-at-origin model for driven spike analysis.
 * Details of the models can be found in Chorng-Yeong Chu's thesis:
 * "Improved Models for Switch-Level Simulation" also available as a
 * Stanford Technical Report CSL-TR-88-368.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defs.h"
#include "net.h"
#include "globals.h"
#ifdef DO_CACHE
#include "cache.h"
#endif


#define	SMALL		1E-15	    /* A small number */
#define	LARGE		1E15	    /* A large number */
#define	LIMIT		1E8	    /* R > LIMIT are considered infinite */
#define	NP_RATIO	0.7	    /* nmos-pmos ratio for spike analysis */


#define	MIN( a, b )		((a < b) ? a : b)
#define	MAX( a, b )		((a > b) ? a : b)


	/* combine 2 resistors in parallel */
#define	COMBINE( R1, R2 )	( (R1) * (R2) / ( (R1) + (R2) ) )
	/* combine 2 resistors in parallel, watch out for zero resistance */
#define	COMBINE_R( A, B )	( ((A) + (B) <= SMALL) ? 0 : COMBINE( A, B ) )

#define	IsWatched( N )		( (N)->nflags & WATCHED )
#define	SetDebug( FL, N )	\
	( ((debug & (FL)) == (FL) and IsWatched( N )) ? 1 : 0 )

		/* flags used in thevenin structure */
#define	T_DEFINITE	0x001	/* set for a definite rooted path	    */
#define	T_UDELAY	0x002	/* set if user specified delay encountered  */
#define	T_SPIKE		0x004	/* set if charge-sharing spike possible	    */
#define	T_DRIVEN	0x008	/* set if this branch is driven		    */
#define	T_REFNODE	0x010	/* set for the reference node in pure c-s   */
#define	T_XTRAN		0x020	/* set if connecting through an X trans.    */
#define	T_INT		0x040	/* set if we should consider input slope    */
#define	T_DOMDRIVEN	0x080	/* set if branch driven to dominant voltage */
#define T_CONFLICT	0x100	/* set if there are conflicting drivers	    */

public	int       tunitdelay = 0;	/* if <> 0, all transitions take
					 * 'tunitdelay' DELAY-units */
public	int       tdecay = 0;		/* if <> 0, undriven nodes decay to 
					 * X after 'tdecay' DELAY-units */
public  int	  settle = 0;		/* If driven by
					 * two nodes at opposite values,
					 * driven node settles to X after
					 * 'settle' DELAY-units.  Added by
					 * Tim, 10/6/06 */
public	char      withdriven;		/* TRUE if stage is driven by
					 *  some input */


typedef	struct
  {
    double  min;
    double  max;
  } Range;

	/* Parameters gathered during a tree walk */
	    /* resists are in ohms, caps in pf */
typedef struct thevenin
  {
    union
      {
	Thev  t;
	nptr  n;
      } link;			/* links these structures together	    */
    int      flags;		/* flags defined above			    */
    Range    Clow;		/* capacitance charged low		    */
    Range    Chigh;		/* capacitance charged high		    */
    Range    Rup;		/* resistance pulling up to Vdd		    */
    Range    Rdown;		/* resistance pulling down to GND	    */
    Range    Req;		/* resist. of present (parallel) xtor(s)    */
    Range    V;			/* normalized voltage range (0-1)	    */
    double   Rmin;		/* minimum resistance to any driver	    */
    double   Rdom;		/* minimum resistance to dominant driver    */
    double   Rmax;		/* maximum resistance to dominant driver    */
    double   Ca;		/* Adjusted non-switching capacitance 	    */
    double   Cd;		/* Adjusted total capacitance		    */
    double   tauD;		/* Elmore delay	(psec)			    */
    double   tauA;		/* 1st order time-constant (psec)	    */
    double   tauP;		/* 2nd order time-constant (psec)	    */
    double   Tin;		/* input transition = (input_tau) * Rin	    */
    short    tplh;		/* user specified low->high delay (DELTA)   */
    short    tphl;		/* user specified high->low delay (DELTA)   */
    char     final;		/* steady-state value calculated (H, L, X)  */
    char     tau_done;		/* if tau calculated, == dominant voltage   */
    char     taup_done;		/* if tauP calculated, == dominant voltage  */
  } thevenin;


typedef struct			/* one for each possible dominant voltage */
  {
    nptr  nd;			/* list of nodes driven to this potential */
    int   spike;		/* TRUE if this pot needs spike analysis */
  } Dominant;


typedef struct
  {
    double  ch_delay;		/* charging delay */
    double  dr_delay;		/* driven delay */
    float   peak;		/* spike peak */
    int     charge;		/* spike charge */
  } SpikeRec, *pspk;


private	Thev      thev_free = NULL;	/* Free list of thev structures */
private	int       inc_level;		/* 1 if debug and node is watched */
private	Dominant  dom_pot[ N_POTS ];	/* dominant voltage structure */

private	thevenin  init_thev;		/* pre-initialized thevenin structs */
private	thevenin  input_thev[ N_POTS ];


	/* forward references */
private	void	scheduleDriven(), schedulePureCS(), UndoConnList();
private	void	parallel_op(), CleanEvents();
private void	EnqueDecay( nptr, int);
private	int	ComputeDC( nptr );
private	Thev	get_dc_val(), series_op(), get_tau();
private	double	get_tauP();
private	pspk	ComputeSpike();
private	void	print_dc(), print_fval(), print_tau(), print_taup();
private	void	print_final(), print_spike(), print_spk();

public void linear_model( n )
  nptr    n;
  {
    int  i, changes;

    nevals++;

    for( i = LOW; i <= HIGH; i++ ) 
	dom_pot[i].nd = NULL, dom_pot[i].spike = FALSE;

    if( n->nflags & VISITED )
	BuildConnList( n );

    changes = ComputeDC( n );

    if( not changes )
	CleanEvents( n );
    else if( withdriven ) {
	scheduleDriven();
    }
    else {
        if( tdecay != 0)
	   EnqueDecay( n, tdecay );
	else
	   schedulePureCS( n );
    }
	
    UndoConnList( n );
  }


private void CleanEvents( n )
  register nptr  n;
  {
    register evptr  ev;

    do
      {
	while( (ev = n->events) != NULL )
	    PuntEvent( n, ev );
      }
    while( (n = n->nlink) != NULL );
  }


private void EnqueDecay( n, decayval )
  register nptr  n;
  register int 	 decayval;
  {
    register evptr  ev;

    do
      {
	ev = n->events;
	if( (ev == NULL) ? (n->npot != X) : (ev->eval != X) )
	  {
	    if( (debug & DEBUG_EV) and IsWatched( n ) )
		lprintf( stdout, "  decay transition for %s @ %.2fns\n",
		  pnode( n ), d2ns( cur_delta + decayval ) );
	    enqueue_event( n, DECAY, (long) decayval, (long) decayval );
	 }
	n = n->nlink;
      }
    while( n != NULL );
  }


private void UndoConnList( n )
  register nptr  n;
  {
    register nptr  next;
    register lptr  l;
    register tptr  t;
#ifdef CL_STATS
    register int   num_trans = 0;

    for( next = n; next != NULL; next = next->nlink )
      {
	for( l = next->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( t->state != OFF )
		t->tflags |= CROSSED;
	  }
      }
#endif /* CL_STATS */

    do
      {
	next = n->nlink;
	n->nlink = NULL;

	n->n.thev->link.t = thev_free;
	thev_free = n->n.thev;

	for( l = n->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( t->state == OFF )
		continue;
	    
#ifdef CL_STATS
	    if( t->tflags & CROSSED )
		num_trans ++;
#endif /* CL_STATS */
	    if( not( t->tflags & (PBROKEN | BROKEN) ) )
	      {
		register Thev  r;

		if( (r = t->scache.r) != NULL )
		  {
		    r->link.t = thev_free;
		    thev_free = r;
		  }
		if( (r = t->dcache.r) != NULL )
		  {
		    r->link.t = thev_free;
		    thev_free = r;
		  }
	      }
	    t->scache.r = t->dcache.r = NULL;
	    t->tflags &= ~(CROSSED | BROKEN | PBROKEN | PARALLEL);
	  }
      }
    while( (n = next) != NULL );

#ifdef CL_STATS
    RecordConnList( num_trans );
#endif /* CL_STATS */
  }


/* 
 * Schedule the final value for node 'nd'.  Check to see if this final value
 * invalidates other events.  Since this event has more recent information
 * regarding the state of the network, delete transitions scheduled to come
 * after it.  Zero-delay transitions are avoided by turning them into unit
 * delays (1 delta).  Events scheduled to occur at the same time as this event
 * and driving the node to the same value are not punted.
 * Finally, before scheduling the final value, check that it is different
 * from the previously calculated value for the node.
 */
public void QueueFVal( nd, fval, tau, delay )
  nptr    nd;
  int     fval;
  double  tau, delay;
  {
    register evptr  ev;
    register Ulong  delta;
    int             queued = 0;

    delta = cur_delta + (Ulong) ps2d( delay );
    if( delta == cur_delta )			/* avoid zero delay */
	delta++;

    while( (ev = nd->events) != NULL and ev->ntime >= delta )
      {
	if( ev->ntime == delta and ev->eval == fval )
	    break;
	PuntEvent( nd, ev );
      }

    delta -= cur_delta;

    if( fval != ((ev == NULL) ? nd->npot : ev->eval) )
      {
	enqueue_event( nd, fval, (long) delta, (long) ps2d( tau ) );
	queued = 1;
      }

    if( (debug & DEBUG_EV) and IsWatched( nd ) )
	print_final( nd, queued, tau, delta );
  }


private void QueueSpike( nd, spk )
  nptr  nd;
  pspk  spk;
  {
    register evptr  ev;
    register Ulong  ch_delta, dr_delta;

    while( (ev = nd->events) != NULL )
	PuntEvent( nd, ev );

    if( spk == NULL )		/* no spike, just punt events */
      {
	return;
      }

    ch_delta = (Ulong) ps2d( spk->ch_delay );
    dr_delta = (Ulong) ps2d( spk->dr_delay );

    if( ch_delta == 0 )
	ch_delta = 1;
    if( dr_delta == 0 )
	dr_delta = 1;

    if( (debug & DEBUG_EV) and IsWatched( nd ) )
	print_spike( nd, spk, ch_delta, dr_delta );

    if( dr_delta <= ch_delta )		/* no zero delay spikes, done */
      {
	return;
      }

	/* enqueue spike and final value events */
    enqueue_event( nd, (int) spk->charge, (long) ch_delta, (long) ch_delta );
    enqueue_event( nd, (int) nd->npot, (long) dr_delta, (long) ch_delta );
  }


private void scheduleDriven()
  {
    register nptr  nd;
    register Thev  r;
    int            dom;
    double         tau, delay;

    for( dom = 0; dom < N_POTS; dom++ )
      {
	for( nd = dom_pot[ dom ].nd; nd != NULL; nd = r->link.n )
	  {
	    inc_level = SetDebug( DEBUG_TAU | DEBUG_TW, nd );

	    r = get_tau( nd, (tptr) NULL, dom, inc_level );

	    if( inc_level == 0 and SetDebug( DEBUG_TAU, nd ) )
		print_tau( nd, r, -1 );

	    r->tauA = r->Rdom * r->Ca;
	    r->tauD = r->Rdom * r->Cd;

	    if( r->flags & T_SPIKE )		/* deal with these later */
		continue;

	    if( nd->npot == r->final )		/* no change, just punt */
	      {
		evptr  ev;

		while( (ev = nd->events) != NULL )
		    PuntEvent( nd, ev );
		continue;
	      }
	    else if ((settle > 0) && (r->flags & T_CONFLICT)) {
		EnqueDecay(nd, settle);
		continue;
	    }
	    else if( tunitdelay )
	      {
		delay = tunitdelay;
		tau = 0.0;
	      }
	    else if( r->flags & T_UDELAY )
	      {
		switch( r->final )
		  {
		    case LOW :	tau = d2ps( r->tphl );			break;
		    case HIGH :	tau = d2ps( r->tplh );			break;
		    case X :	tau = d2ps( MIN( r->tphl, r->tplh ) );	break;
		  }
		delay = tau;
	      }
	    else
	      {
		if( r->final == X )
		    tau = r->Rmin * r->Ca;
		else if( r->flags & T_DEFINITE )
		    tau = r->Rmax * r->Ca;
		else
		    tau = r->Rdom * r->Ca;

		if( (r->flags & T_INT) and r->Tin > 0.5 )
		    delay = sqrt( tau * tau + d2ps( r->Tin ) * r->Ca );
		else
		    delay = tau;
	      }

	    QueueFVal( nd, (int) r->final, tau, delay );
	  }

	if( dom_pot[ dom ].spike )
	  {
	    for( nd = dom_pot[ dom ].nd; nd != NULL; nd = nd->n.thev->link.n )
	      {
		r = nd->n.thev;
		if( not (r->flags & T_SPIKE) )
		    continue;

		inc_level = SetDebug( DEBUG_TAUP | DEBUG_TW, nd );

		r->tauP = get_tauP( nd, (tptr) NULL, dom, inc_level );

		r->tauP *= r->Rdom / r->tauA;

		QueueSpike( nd, ComputeSpike( nd, r, dom ) );
	      }
	  }
      }
  }


private void schedulePureCS( nlist )
  nptr  nlist;
  {
    register nptr  nd;
    register Thev  r;
    int            dom;
    double         taup, tau, delay;

    r = nlist->n.thev;

    dom = r->final;
    r->flags |= T_REFNODE;

    taup = 0.0;
    for( nd = nlist; nd != NULL; nd = nd->nlink )
      {
	inc_level = SetDebug( DEBUG_TAU | DEBUG_TW, nd );

	r = get_tau( nd, (tptr) NULL, dom, inc_level );

	r->tauD = r->Rdom * r->Ca;

	switch( dom )
	  {
	    case LOW :
		r->tauA = r->Rdom * ( r->Ca - r->Cd * r->V.max );
		break;
	    case HIGH : 
		r->tauA = r->Rdom * ( r->Cd * ( 1 - r->V.min ) - r->Ca );
		break;
	    case X :			/* approximate Vf = 0.5 */
		r->tauA = r->Rdom * ( r->Ca - r->Cd * 0.5 ); 
		break;
	  }
	taup += r->tauA * nd->ncap;
      }

    r = nlist->n.thev;
    taup = taup / (r->Clow.min + r->Chigh.max);		/* tauP = tauP / CT */

    for( nd = nlist; nd != NULL; nd = nd->nlink )
      {
	r = nd->n.thev;
	if( r->final == nd->npot )		/* no change, no delay */
	    delay = tau = 0.0;
	else
	  {
	    switch( r->final )
	      {
		case LOW   : tau = (r->tauA - taup) / (1.0 - r->V.max);	break;
		case HIGH  : tau = (taup - r->tauA) / r->V.min;		break;
		case X     : tau = (r->tauA - taup) * 2.0;		break;
	      }
	    if( tau < 0.0 )
		tau = 0.0;
	    if( tunitdelay )
		delay = tunitdelay, tau = 0.0;
	    else
		delay = tau;
	  }

	QueueFVal( nd, (int) r->final, tau, delay );
      }
  }


/*
 * Compute the final value for each node on the connection list.
 * This routine will update V.min and V.max and add the node to the
 * corresponding dom_driver entry.  Return TRUE if any node changes value.
 */
private int ComputeDC( nlist )
  nptr  nlist;
  {
    register nptr  this;
    register Thev  r;
    int            anyChange = FALSE;

    for( this = nlist; this != NULL; this = this->nlink )
      {
	inc_level = SetDebug( DEBUG_DC | DEBUG_TW, this );

	this->n.thev = r = get_dc_val( this, (tptr) NULL, inc_level );

	if( withdriven )
	  {
	    if( r->Rdown.min >= LIMIT )
		r->V.min = 1;
	    else
		r->V.min = r->Rdown.min / ( r->Rdown.min + r->Rup.max );
	    if( r->Rup.min >= LIMIT )
		r->V.max = 0;
	    else
		r->V.max = r->Rdown.max / ( r->Rdown.max + r->Rup.min );
	  }
	else		/* use charge/total charge if undriven */
	  {
	    r->V.min = r->Chigh.min / ( r->Chigh.min + r->Clow.max );
	    r->V.max = r->Chigh.max / ( r->Chigh.max + r->Clow.min );
	  }

	if( r->V.min >= this->vhigh )
	    r->final = HIGH;
	else if( r->V.max <= this->vlow )
	    r->final = LOW;
	else {
	    r->final = X;
	    if ((this->npot != X) && withdriven && (settle > 0))
		r->flags |= T_CONFLICT;
	}

	if( withdriven )
	  {
	    /*
	     * if driven and indefinite, driven value must equal 
	     * charging value otherwise the final value is X
	     */
	    if( r->final != X and not (r->flags & T_DEFINITE) )
	      {
		char  cs_val;

		if( r->Chigh.min >= this->vhigh * (r->Chigh.min + r->Clow.max) )
		    cs_val = HIGH;
		else if( r->Chigh.max <= this->vlow * (r->Chigh.max + r->Clow.min) )
		    cs_val = LOW;
		else
		    cs_val = X;			/* always X */

		if( cs_val != r->final )
		    r->final = X;
	      }

	    r->link.n = dom_pot[ (int)r->final ].nd;	/* add it to list */
	    dom_pot[ (int)r->final ].nd = this;

	    /* possible spike if no transition and opposite charge exists */
	    if( r->final == this->npot and (
	      (r->final == LOW and r->Chigh.min > SMALL) or
	      (r->final == HIGH and r->Clow.min > SMALL) ) )
	      {
		r->flags |= T_SPIKE;
		dom_pot[ (int)r->final ].spike = TRUE;
		anyChange = TRUE;
	      }
	  }

	if( r->final != this->npot )
	    anyChange = TRUE;

	if( SetDebug( DEBUG_DC, this ) )
	    print_fval( this, r );
      }
    return( anyChange );
  }



#define NEW_THEV( T )						\
  {								\
    if( ((T) = thev_free) == NULL )				\
	(T) = (Thev) MallocList( sizeof( thevenin ), 1 );	\
    thev_free = T->link.t;					\
  }


/*
 * Compute the parametes used to calculate the final value (Chigh, Clow,
 * Rup, Rdown) by doing a depth-first traversal of the tree rooted at node
 * 'n'.  The traversal is done by a recursive walk through the tree. Note that
 * the stage is already a simple tree; loops are broken by BuildConnList.  As
 * a side effect also compute Req of the present transistor and all other
 * transistors in parallel with it, and any specified user delays.
 * The parameters are:
 *
 * n      : the node whose dc parameters we want.
 * tran   : the transistor that leads to 'n' (NULL if none).
 * level  : level of recursion if we are debugging this node, else 0.
 */
private Thev get_dc_val( n, tran, level )
  nptr  n;
  tptr  tran;
  int   level;
  {
    register lptr  l;
    register tptr  t;
    register nptr  other;
    register Thev  r;
    Thev           cache, *pcache;

    NEW_THEV( r );

    if( n->nflags & INPUT )
      {
	*r = input_thev[ n->npot ];
	return( r );
      }

    *r = init_thev;
    switch( n->npot )
      {
	case LOW :   r->Clow.min = r->Clow.max = n->ncap;	break;
	case X :     r->Clow.max = r->Chigh.max = n->ncap;	break;
	case HIGH :  r->Chigh.min = r->Chigh.max = n->ncap;	break;
      }

    for( l = n->nterm; l != NULL; l = l->next )
      {
	t = l->xtor;

	/* ignore path going back or through a broken loop */
	if( t == tran or t->state == OFF or (t->tflags & (BROKEN | PBROKEN)) )
	    continue;

	if( n == t->source )
	  {
	    other = t->drain;	pcache = &(t->dcache.r);
	  }
	else
	  {
	    other = t->source;	pcache = &(t->scache.r);
	  }

	/*
	 * if cache is not empty use the value found there, otherwise
	 * compute what is on the other side of the transistor and 
	 * transmit the result through a series operation.
	 */
	if( (cache = *pcache) == NULL )
	  {
	    cache = series_op( get_dc_val( other, t, level+inc_level ), t );
	    *pcache = cache;
	  }
	parallel_op( r, cache );
      }

    if( n->nflags & USERDELAY )		/* record user delays, if any */
      {
	r->tplh = n->tplh; r->tphl = n->tphl;
	r->flags |= T_UDELAY;
      }

    if( level != 0 )
	print_dc( n, r, level );

    return( r );
  }


/*
 * The following macros set Req to the appropriate dynamic resistance.
 * If the transistor state is UNKNOWN then also set the T_XTRAN flag.
 */
#define	GetReq( R, T, TYPE )			\
  {						\
    if( ( (T)->tflags & PARALLEL ) )		\
	get_parallel( R, T, TYPE );		\
    else					\
      {						\
	(R)->Req.min = (T)->r->dynres[TYPE];	\
	if( (T)->state == UNKNOWN )		\
	    (R)->flags |= T_XTRAN;		\
	else					\
	    (R)->Req.max = (T)->r->dynres[TYPE];\
      }						\
  }						\


#define	GetMinR( R, T )				\
  {						\
    if( ( (T)->tflags & PARALLEL ) )		\
	get_min_parallel( R, T );		\
    else					\
      {						\
	(R)->Req.min = MIN( (T)->r->dynhigh, (T)->r->dynlow );	\
	if( (T)->state == UNKNOWN )		\
	    (R)->flags |= T_XTRAN;		\
	else					\
	    (R)->Req.max = (R)->Req.min;	\
      }						\
  }						\

/*
 * Do the same as GetReq but deal with parallel transistors.
 */
private void get_parallel( r, t, restype )
  Thev           r;
  register tptr  t;
  int            restype;
  {
    register Resists  *rp = t->r;
    double            gmin, gmax;

    gmin = 1.0 / rp->dynres[ restype ];
    gmax = (t->state == UNKNOWN ) ? 0.0 : gmin;

    for( t = par_list( t ); t != NULL; t = t->dcache.t )
      {
	rp = t->r;
	gmin += 1.0 / rp->dynres[ restype ];
	if( t->state != UNKNOWN )
	    gmax += 1.0 / rp->dynres[ restype ];
      }
    r->Req.min = 1.0 / gmin;
    if( gmax == 0.0 )
	r->flags |= T_XTRAN;
    else
	r->Req.max = 1.0 / gmax;
  }


/*
 * Do the same as get_parallel but use the minimum dynamic resistance.
 */
void get_min_parallel( r, t )
  Thev           r;
  register tptr  t;
  {
    register Resists  *rp = t->r;
    double            gmin, gmax, tmp;

    gmin = 1.0 / MIN( rp->dynlow, rp->dynhigh );
    gmax = (t->state == UNKNOWN ) ? 0.0 : gmin;

    for( t = par_list( t ); t != NULL; t = t->dcache.t )
      {
	rp = t->r;
	tmp = 1.0 / MIN( rp->dynlow, rp->dynhigh );
	gmin += tmp;
	if( t->state != UNKNOWN )
	    gmax += tmp;
      }
    r->Req.min = 1.0 / gmin;
    if( gmax == 0.0 )
	r->flags |= T_XTRAN;
    else
	r->Req.max = 1.0 / gmax;
  }


/*
 * Add transistor 't' in series with thevenin struct 'r'.  As a side effect
 * set Req for 't'.  The midpoint voltage is used to determine whether to
 * use the dynamic-high or dynamic-low resistance.  If the branch connecting
 * to 't' is not driven by an input then use the charge information.  The
 * current estimates of both resistance or capacitance is used.
 */
private Thev series_op( r, t )
  register Thev  r;
  register tptr  t;
  {
    double  up_min, down_min;

    if( not (r->flags & T_DRIVEN) )
      {
	if( r->Chigh.min > r->Clow.max )
	    GetReq( r, t, R_HIGH )
	else if( r->Chigh.max < r->Clow.min )
	    GetReq( r, t, R_LOW )
	else
	    GetMinR( r, t )
	return( r );		/* no driver, so just set Req */ 
      }

    if( r->Rdown.min > r->Rup.max )
	GetReq( r, t, R_HIGH )
    else if( r->Rdown.max < r->Rup.min )
	GetReq( r, t, R_LOW )
    else
	GetMinR( r, t )

    up_min = r->Rup.min;
    down_min = r->Rdown.min;

    if( up_min < LIMIT )
	r->Rup.min += r->Req.min * ( 1.0 + up_min / r->Rdown.max );
    if( down_min < LIMIT )
	r->Rdown.min += r->Req.min * ( 1.0 + down_min / r->Rup.max );
    if( r->flags & T_XTRAN )
      {
	r->flags &= ~T_DEFINITE;
	r->Rup.max = r->Rdown.max = LARGE;
      }
    else
      {
	if( r->Rup.max < LIMIT )
	    r->Rup.max += r->Req.max * ( 1.0 + r->Rup.max / down_min );
	if( r->Rdown.max < LIMIT )
	    r->Rdown.max += r->Req.max * ( 1.0 + r->Rdown.max / up_min );
      }
    return( r );
  }


/* make oldr = (oldr || newr), but watch out for infinte resistances. */
#define	DoParallel( oldr, newr )	\
  {					\
    if( oldr > LIMIT )			\
	oldr = newr;			\
    else if( newr < LIMIT )		\
	oldr = COMBINE( oldr, newr );	\
  }					\


/*
 * Combine the new resistance block of the tree walk with the current one.
 * Accumulate the low and high capacitances, the user-specified
 * delay (if any), and the driven flag of the resulting structure.
 */
private void parallel_op( r, new )
  Thev  r, new;
  {
    r->Clow.max += new->Clow.max;
    r->Chigh.max += new->Chigh.max;
    if( not (new->flags & T_XTRAN) )
      {
	r->Clow.min += new->Clow.min;
	r->Chigh.min += new->Chigh.min;
      }

    /*
     * Accumulate the minimum user-specified delay only if the new block
     * has some drive associated with it.
     */
    if( (new->flags & (T_DEFINITE | T_UDELAY)) == (T_DEFINITE | T_UDELAY) )
     {
	if( r->flags & T_UDELAY )
	  {
	    r->tplh = MIN( r->tplh, new->tplh );
	    r->tphl = MIN( r->tphl, new->tphl );
	  }
	else
	  {
	    r->tplh = new->tplh;
	    r->tphl = new->tphl;
	    r->flags |= T_UDELAY;
	  }
     }

    if( not (new->flags & T_DRIVEN) )
	return;				/* undriven, just update caps */

    r->flags |= T_DRIVEN;		/* combined result is driven */

    DoParallel( r->Rup.min, new->Rup.min );
    DoParallel( r->Rdown.min, new->Rdown.min );

    if( r->flags & new->flags & T_DEFINITE )	/* both definite blocks */
      {
	DoParallel( r->Rup.max, new->Rup.max );
	DoParallel( r->Rdown.max, new->Rdown.max );
      }
    else if( new->flags & T_DEFINITE )		/* only new is definite */
      {
	r->Rup.max = new->Rup.max;
	r->Rdown.max = new->Rdown.max;
	r->flags |= T_DEFINITE;			    /* result is definite */
      }
    else				/* new (perhaps r) is indefinite */
      {
	if( new->Rup.max < r->Rup.max )	r->Rup.max = new->Rup.max;
	if( new->Rdown.max < r->Rdown.max ) r->Rdown.max = new->Rdown.max;
      }
  }


/*
 * Determine the input time-constant (input-slope * rstatic).  We are only
 * interseted in transistors that just turned on (its gate has a transition
 * at time == cur_delta).  We must be careful not to report as a transition
 * nodes that stop being inputs (hist->delay == 0 and hist->inp == 0).
 */
#define IsCurrTransition( H )			\
  ( (H)->time == cur_delta and ((H)->inp == 1 or (H)->t.r.delay != 0) )

/*
 * Return TRUE if we should consider the input slope of this transistor.  As
 * a side-effect, return the input time constant in 'ptin'.
 */
private int GetTin( t, ptin )
  register tptr  t;
  double         *ptin;
  {
    register hptr  h;
    int            is_int = FALSE;

    if( t->state != ON )
	return( FALSE );

    if( (t->ttype & GATELIST) == 0 )
      {
	h = t->gate->curr;
	if( IsCurrTransition( h ) )
	  {
	    *ptin = h->t.r.rtime * t->r->rstatic;
	    is_int = TRUE;
	  }
      }
    else
      {
	double  tmp = 0.0;

	for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
	  {
	    h = t->gate->curr;
	    if( IsCurrTransition( h ) )
	      {
		is_int = TRUE;
		tmp += h->t.r.rtime * t->r->rstatic;
	      }
	  }
	*ptin = tmp;
      }
    return( is_int );
  }

#define	InputTau( T, PR )	\
  ( ((T)->tflags & PARALLEL) ? parallel_GetTin( T, PR ) : GetTin( T, PR ) )

private int parallel_GetTin( t, itau )
  register tptr  t;
  double         *itau;
  {
    double  tin, tmp = 0.0;
    int     is_int;

    is_int = GetTin( t, &tin );

    for( t = par_list( t ); t != NULL; t = t->dcache.t )
      {
	if( GetTin( t, &tmp ) )
	  {
	    tin = (is_int) ? COMBINE_R( tin, tmp ) : tmp;
	    is_int = TRUE;
	  }
	*itau = tin;
      }
    return( is_int );
  }


/*
 * Compute the parameters needed to calculate the 1st order time-constants
 * (Rmin, Rdom, Rmax, Ca, Cd, Tin) by doing a depth-first traversal of the
 * tree rooted at node 'n'.  The parameters are gathered by performing a
 * recursive tree walk similar to ComputeDC.  As a side effect, the tauP
 * field will contain the multiplication factor to move a capacitor across
 * a transistor using 'current distribution', this field may be required
 * later when computing tauP.  The parameters are:
 *
 * n      : the node whose time-constant parameters we want.
 * dom    : the value of the dominant driver for this stage.
 * tran   : the transistor that leads to 'n' (NULL if none).
 * level  : level of recursion if we are debugging this node, else 0.
 *
 * This routine can be called more than once if the stage is dominated by
 * more than 1 potential, hence the tau_done flag keeps track of the potential
 * for which the parameters stored in the cache were computed.  If the flag
 * value and the current dominant potential do not match, we go ahead and
 * recompute the values.
 */
private Thev get_tau( n, tran, dom, level )
  nptr  n;
  tptr  tran;
  int   dom;
  int   level;
  {
    register Thev  r, cache;
    register lptr  l;
    register tptr  t;
    register nptr  other;

    if( tran == NULL )
	r = n->n.thev;
    else
	r = (tran->source == n) ? tran->scache.r : tran->dcache.r;

    r->tau_done = dom;

    if( n->nflags & INPUT )
      {
	r->Tin = r->Rmin = r->Ca = r->Cd = 0.0;
	if( n->npot == dom )
	  {
	    r->Rdom = r->Rmax = 0.0;
	    r->flags |= T_DOMDRIVEN;
	  }
	else
	  {
	    r->flags &= ~(T_DOMDRIVEN | T_INT);
	    if( dom == X )
		r->Rdom = r->Rmax = 0.0;
	    else
		r->Rdom = r->Rmax = LARGE;
	  }
	return( r );
      }

    if( n->n.thev->flags & T_REFNODE )	    /* reference node in pure CS */
      {
	r->Rmin = r->Rdom = r->Rmax = 0.0;
	r->Ca = r->Cd = 0.0;
	return( r );
      }

    r->Rmin = r->Rdom = r->Rmax = LARGE;
    r->Cd = n->ncap;
    if( dom == X )			/* assume X nodes are charged high */
	r->Ca = (n->npot == LOW) ? 0.0 : n->ncap;
    else
	r->Ca = (n->npot == dom) ? 0.0 : n->ncap;

    r->Tin = 0.0;
    r->flags &= ~(T_DOMDRIVEN | T_INT);

    for( l = n->nterm; l != NULL; l = l->next )
      {
	t = l->xtor;
	if( t->state == OFF or t == tran or (t->tflags & (BROKEN | PBROKEN)) )
	    continue;

	if( n == t->source )
	  {
	    other = t->drain;	cache = t->dcache.r;
	  }
	else
	  {
	    other = t->source;	cache = t->scache.r;
	  }

	if( cache->tau_done != dom )
	  {
	    double  oldr;

	    cache = get_tau( other, t, dom, level + inc_level );
	    /* Only use input slope for xtors on the dominant (driven) path */
	    if( (cache->flags & T_DOMDRIVEN) and InputTau( t, &oldr ) )
	      {
		cache->flags |= T_INT;
		cache->Tin += oldr;
	      }

	    oldr = cache->Rdom;

	    cache->Rmin += cache->Req.min;
	    cache->Rdom += cache->Req.min;
	    if( cache->flags & T_XTRAN )
		cache->Rmax = LARGE;
	    else
		cache->Rmax += cache->Req.max;

	    /* Exclude capacitors if the other side of X transistor == dom */
	    if( (cache->flags & T_XTRAN) and other->npot == dom )
		cache->tauP = cache->Ca = cache->Cd = 0.0;
	    else if( oldr > LIMIT )
		cache->tauP = 1.0;
	    else
	      {
		cache->tauP = oldr / cache->Rdom;
		cache->Ca *= cache->tauP;
		cache->Cd *= cache->tauP;
	      }
	  }

	r->Ca += cache->Ca;
	r->Cd += cache->Cd;

	r->Rmin = COMBINE( r->Rmin, cache->Rmin );
	if( r->Rdom > LIMIT )
	  {
	    r->Rdom = cache->Rdom;
	    r->Rmax = cache->Rmax;
	  }
	else if( cache->Rdom < LIMIT )
	  {
	    r->Rdom = COMBINE( r->Rdom, cache->Rdom );
	    r->Rmax = COMBINE( r->Rmax, cache->Rmax );
	  }

	if( cache->flags & T_DOMDRIVEN )
	    r->flags |= T_DOMDRIVEN;	/* at least 1 dominant driven path */

	if( cache->flags & T_INT )
	  {
	    if( r->flags & T_INT )
		r->Tin = COMBINE_R( r->Tin, cache->Tin );
	    else
	      {
		r->Tin = cache->Tin;
		r->flags |= T_INT;
	      }
	  }
      }

    if( level > 0 )
	print_tau( n, r, level );

    return( r );
  }


/*
 * Calculate the 2nd order time constant (tauP) for the net configuration
 * as seen through node 'n'.  The net traversal and the parameters are
 * similar to 'get_tau'.  Note that at this point we have not have calculated
 * tauA for nodes not driven to the dominant potential, hence we need to
 * compute those by first calling get_tau.  This routine will update the tauP
 * entry as well as the taup_done flag.
 */
private double get_tauP( n, tran, dom, level )
  nptr  n;
  tptr  tran;
  int   dom;
  int   level;
  {
    register lptr  l;
    register tptr  t;
    register Thev  r;
    nptr           other;
    double         taup;

    if( n->nflags & INPUT )
	return( 0.0 );

    r = n->n.thev;
    if( r->tau_done != dom )		/* compute tauA for the node */
      {
	r = get_tau( n, (tptr) NULL, dom, 0 );
	r->tauA = r->Rdom * r->Ca;
	r->tauD = r->Rdom * r->Cd;
      }

    taup = r->tauA * n->ncap;

    for( l = n->nterm; l != NULL; l = l->next )
      {
	t = l->xtor;
	if( t->state == OFF or t == tran or (t->tflags & (BROKEN | PBROKEN)) )
	    continue;

	if( t->source == n )
	    other = t->drain,	r = t->dcache.r;
	else
	    other = t->source,	r = t->scache.r;

	if( r->taup_done != dom )
	  {
	    r->tauP *= get_tauP( other, t, dom, level + inc_level );
	    r->taup_done = dom;
	  }
	taup += r->tauP;
      }
    if( level > 0 )
	print_taup( n, level, taup );

    return( taup );
  }


#include "spiketbl.c"

/*
 * Compute the size of spike.  If the spike is too small return NULL, else
 * fill in the appropriate structure and return a pointer to it.  In order
 * to determine in which table to lookup the spike peak we look at the
 * conductivity of all ON transistors connected to node 'nd'; the type with
 * the largest conductivity determines whether it is mostly an nmos or pmos
 * network.  This simple scheme should work for most simple nets.
 */
private pspk ComputeSpike( nd, r, dom )
  nptr           nd;
  register Thev  r;
  int            dom;
  {
    int              rtype, tab_indx, alpha, beta;
    float            nmos, pmos;
    static SpikeRec  spk;
    register lptr    l;

    if( r->tauP <= SMALL )		/* no capacitance, no spike */
      {
	if( (debug & DEBUG_SPK) and IsWatched( nd ) )
	    lprintf( stdout, " spike( %s ) ignored (taup=0)\n" );
	return( NULL );
      }

    rtype = (dom == LOW) ? R_LOW : R_HIGH;
    nmos = pmos = 0;
    for( l = nd->nterm; l != NULL; l = l->next )
      {
	register tptr  t;

	t = l->xtor;
	if( t->state == OFF or (t->tflags & BROKEN) )
	    continue;
	if( BASETYPE( t->ttype ) == PCHAN )
	    pmos += 1.0 / t->r->dynres[ rtype ];
	else
	    nmos += 1.0 / t->r->dynres[ rtype ];
      }
    if( nmos > NP_RATIO * (pmos + nmos) )		/* mostly nmos */
	tab_indx = (rtype == R_LOW) ? NLSPKMIN : NLSPKMAX;
    else if( pmos > NP_RATIO * (pmos + nmos) )		/* mostly pmos */
	tab_indx = (rtype == R_LOW) ? NLSPKMAX : NLSPKMIN;
    else
	tab_indx = LINEARSPK;

    alpha = (int) ( SPIKETBLSIZE * r->tauA / (r->tauA + r->tauP - r->tauD) );
    if( alpha < 0 )
	alpha = 0;
    else if( alpha > SPIKETBLSIZE )
	alpha = SPIKETBLSIZE;

    beta = (int) ( SPIKETBLSIZE * (r->tauD - r->tauA) / r->tauD );
    if( beta < 0 )
	beta = 0;
    else if( beta > SPIKETBLSIZE )
	beta = SPIKETBLSIZE;

    spk.peak = spikeTable[ tab_indx ][ beta ][ alpha ];
    spk.ch_delay = delayTable[ beta ][ alpha ];

    if( dom == LOW )
      {
	if( spk.peak <= nd->vlow )		/* spike is too small */
	    goto no_spike;
	else
	    spk.charge = (spk.peak >= nd->vhigh) ? HIGH : X;
      }
    else	/* dom == HIGH */
      {
	if( spk.peak <= 1.0 - nd->vhigh )
	    goto no_spike;
	else
	    spk.charge = (spk.peak >= 1.0 - nd->vlow) ? LOW : X;
      }

    spk.ch_delay *= r->tauA * r->tauD / r->tauP;

    if( r->Rmax < LARGE )
	spk.dr_delay = r->Rmax * r->Ca;
    else
	spk.dr_delay = r->Rdom * r->Ca;

    if( (debug & DEBUG_SPK) and IsWatched( nd ) )
	print_spk( nd, r, tab_indx, dom, alpha, beta, &spk, TRUE );
    return( &spk );

  no_spike :
    if( (debug & DEBUG_SPK) and IsWatched( nd ) )
	print_spk( nd, r, tab_indx, dom, alpha, beta, &spk, FALSE );
    return( NULL );
  }


/*
 * Initialize pre-initialized thevenin structs.  I want to get it right
 * and this is much safer than letting the compiler initialize it.
 */
public void InitThevs()
  {
    register Thev  t;

    init_thev.link.n	= NULL;
    init_thev.flags	= 0;
    init_thev.Clow.min	= 0.0;
    init_thev.Clow.max	= 0.0;
    init_thev.Chigh.min	= 0.0;
    init_thev.Chigh.max	= 0.0;
    init_thev.Rup.min	= LARGE;
    init_thev.Rup.max	= LARGE;
    init_thev.Rdown.min	= LARGE;
    init_thev.Rdown.max	= LARGE;
    init_thev.Req.min	= LARGE;
    init_thev.Req.max	= LARGE;
    init_thev.V.min	= 1.0;
    init_thev.V.max	= 0.0;
    init_thev.Rmin	= LARGE;
    init_thev.Rdom	= LARGE;
    init_thev.Rmax	= LARGE;
    init_thev.Ca	= 0.0;
    init_thev.Cd	= 0.0;
    init_thev.tauD	= 0.0;
    init_thev.tauA	= 0.0;
    init_thev.tauP	= 0.0;
    init_thev.Tin	= SMALL;
    init_thev.tplh	= 0;
    init_thev.tphl	= 0;
    init_thev.final	= X;
    init_thev.tau_done	= N_POTS;
    init_thev.taup_done	= N_POTS;

    t =	&input_thev[ LOW ];
    t->link.n		= NULL;
    t->flags		= T_DEFINITE | T_DRIVEN;
    t->Clow.min		= 0.0;
    t->Clow.max		= 0.0;
    t->Chigh.min	= 0.0;
    t->Chigh.max	= 0.0;
    t->Rup.min		= LARGE;
    t->Rup.max		= LARGE;
    t->Rdown.min	= SMALL;
    t->Rdown.max	= SMALL;
    t->Req.min		= LARGE;
    t->Req.max		= LARGE;
    t->V.min		= 0.0;
    t->V.max		= 0.0;
    t->Rmin		= SMALL;
    t->Rdom		= LARGE;
    t->Rmax		= LARGE;
    t->Ca		= 0.0;
    t->Cd		= 0.0;
    t->tauD		= 0.0;
    t->tauA		= 0.0;
    t->tauP		= 0.0;
    t->Tin		= SMALL;
    t->tplh		= 0;
    t->tphl		= 0;
    t->final		= LOW;
    t->tau_done		= N_POTS;
    t->taup_done	= N_POTS;

    t = &input_thev[ HIGH ];
    t->link.n		= NULL;
    t->flags		= T_DEFINITE | T_DRIVEN;
    t->Clow.min		= 0.0;
    t->Clow.max		= 0.0;
    t->Chigh.min	= 0.0;
    t->Chigh.max	= 0.0;
    t->Rup.min		= SMALL;
    t->Rup.max		= SMALL;
    t->Rdown.min	= LARGE;
    t->Rdown.max	= LARGE;
    t->Req.min		= LARGE;
    t->Req.max		= LARGE;
    t->V.min		= 1.0;
    t->V.max		= 1.0;
    t->Rmin		= SMALL;
    t->Rdom		= LARGE;
    t->Rmax		= LARGE;
    t->Ca		= 0.0;
    t->Cd		= 0.0;
    t->tauD		= 0.0;
    t->tauA		= 0.0;
    t->tauP		= 0.0;
    t->Tin		= SMALL;
    t->tplh		= 0;
    t->tphl		= 0;
    t->final		= HIGH;
    t->tau_done		= N_POTS;
    t->taup_done	= N_POTS;

    t = &input_thev[ X ];
    t->link.n		= NULL;
    t->flags		= T_DEFINITE | T_DRIVEN;
    t->Clow.min		= 0.0;
    t->Clow.max		= 0.0;
    t->Chigh.min	= 0.0;
    t->Chigh.max	= 0.0;
    t->Rup.min		= SMALL;
    t->Rup.max		= LARGE;
    t->Rdown.min	= SMALL;
    t->Rdown.max	= LARGE;
    t->Req.min		= LARGE;
    t->Req.max		= LARGE;
    t->V.min		= 1.0;
    t->V.max		= 0.0;
    t->Rmin		= SMALL;
    t->Rdom		= LARGE;
    t->Rmax		= LARGE;
    t->Ca		= 0.0;
    t->Cd		= 0.0;
    t->tauD		= 0.0;
    t->tauA		= 0.0;
    t->tauP		= 0.0;
    t->Tin		= SMALL;
    t->tplh		= 0;
    t->tphl		= 0;
    t->final		= X;
    t->tau_done		= N_POTS;
    t->taup_done	= N_POTS;

    input_thev[ X+1 ] = input_thev[ X ];
  }


/*
 * printing routines for debug mode
 */

private char *get_indent( i )
  int  i;
  {
    static char   indent[] = ".........................";
    static char   spaces[] = "                          ";
    static int    last_c = 0;

    if( i >= sizeof( indent ) )
	i = sizeof( indent ) - 1;
    else
	i++;

    indent[ i ] = '\0';
    lprintf( stdout, " %s", indent );
    indent[ i ] = ' ';
    spaces[ last_c ] = ' ';
    last_c = i + 1;
    spaces[ last_c ] = '\0';
    return( spaces );
  }


private char *r2ascii( s, r )
  char    *s;
  double  r;
  {
    if( r >= LIMIT )
	(void) strcpy( s, " - " );
    else if( r > 1.0 )
      {
	int  exp;

	for( exp = 0; r >= 1000.0; exp++, r *= 0.001 );
	(void) sprintf( s, "%.1f%c", r, " KMG"[ exp ] );
      }
    else
	(void) sprintf( s, "%g", r );

    return( s );
  }


private void print_dc( n, r, level )
  nptr  n;
  Thev  r;
  int   level;
  {
    char  cbuf[4][20];
    char  *indent;

    indent = get_indent( level );
    lprintf( stdout, "compute_dc( %s )\n%s", pnode( n ), indent );

    if( not withdriven )
	lprintf( stdout, "pure cs:" );
    else
	lprintf( stdout, "%sefinite", (r->flags & T_DEFINITE) ? "D" : "Ind");

    lprintf( stdout, "  Rup=[%s, %s]  Rdown=[%s, %s]\n",
      r2ascii( cbuf[0], r->Rup.min ), r2ascii( cbuf[1], r->Rup.max ),
      r2ascii( cbuf[2], r->Rdown.min ), r2ascii( cbuf[3], r->Rdown.max ) );

    lprintf( stdout, "%sClow=[%.2f, %.2f]  Chigh=[%.2f, %.2f]\n",
      indent, r->Clow.min, r->Clow.max, r->Chigh.min, r->Chigh.max );
  }


private void print_fval( n, r )
  nptr  n;
  Thev  r;
  {
    lprintf( stdout, " final_value( %s )  V=[%.2f, %.2f]  => %c",
      pnode( n ), r->V.min, r->V.max, vchars[ (int)r->final ] );
    lprintf( stdout,  (r->flags & T_SPIKE) ? "  (spk)\n" : "\n" );
  }


private void print_tau( n, r, level )
  nptr  n;
  Thev  r;
  int   level;
  {
    char  cbuf[3][20];
    char  *indent;

    indent = get_indent( level );
    lprintf( stdout, "compute_tau( %s )\n%s", pnode( n ), indent );

    lprintf( stdout, "{Rmin=%s  Rdom=%s  Rmax=%s}",
      r2ascii( cbuf[0], r->Rmin ), r2ascii( cbuf[1], r->Rdom ),
      r2ascii( cbuf[2], r->Rmax ) );

    lprintf( stdout, "  {Ca=%.2f  Cd=%.2f}\n", r->Ca, r->Cd );

    lprintf( stdout, "%stauA=%.2f  tauD=%.2f ns, RTin=",
      indent, ps2ns( r->Rdom * r->Ca ), ps2ns( r->Rdom * r->Cd ) );

    if( r->flags & T_INT )
	lprintf( stdout, "%.2f ohm*ns\n", d2ns( r->Tin ) );
    else
	lprintf( stdout, "-\n" );
  }


private void print_taup( n, level, taup )
  nptr    n;
  int     level;
  double  taup;
  {
    (void) get_indent( level );
    lprintf( stdout, "tauP( %s ) = %.2f ns\n", pnode( n ), ps2ns( taup ) );
  }


private void print_final( nd, queued, tau, delay )
  nptr    nd;
  int     queued;
  double  tau;
  Ulong   delay;
  {
    Thev   r;
    Ulong  dtau;	/* tau in deltas */

    r = nd->n.thev;
    dtau = ps2d( tau );

    lprintf( stdout, " [event %s->%c @ %.2f] ",
      pnode( cur_node ), vchars[ cur_node->npot ], d2ns( cur_delta ) );
 
    lprintf( stdout, (queued ? "causes %stransition for" : "%sevaluates"), 
	(withdriven ? "" : "CS ") );

    lprintf( stdout, " %s: %c -> %c", pnode( nd ),
      vchars[ nd->npot ], vchars[ (int)r->final ] );
    lprintf( stdout, " (tau=%.2fns, delay=%.2fns)\n",
      d2ns( dtau ), d2ns( delay ) );
  }


private void print_spike( nd, spk, ch_delay, dr_delay )
  nptr    nd;
  pspk    spk;
  Ulong   ch_delay, dr_delay;
  {
    lprintf( stdout, "  [event %s->%c @ %.2f] causes ",
      pnode( cur_node ), vchars[ cur_node->npot ], d2ns( cur_delta ) );
    if( dr_delay <= ch_delay )
	lprintf( stdout, "suppressed " );
 
    lprintf( stdout, "spike for %s: %c -> %c -> %c", pnode( nd ),
      vchars[ nd->npot ], vchars[ spk->charge ], vchars[ nd->npot ] );
    lprintf( stdout, " (peak=%.2f delay: ch=%.2fns, dr=%.2fns)\n",
      spk->peak, d2ns( ch_delay ), d2ns( dr_delay ) );
  }


private void print_spk( nd, r, tab, dom, alpha, beta, spk, is_spk )
  nptr  nd;
  Thev  r;
  int   tab, dom, alpha, beta;
  pspk  spk;
  int   is_spk;
  {
    char  *net_type;

    lprintf( stdout, " spike_analysis( %s ):", pnode( nd ) );
    if( tab == LINEARSPK )
	net_type = "n-p mix";
    else if( tab == NLSPKMIN )
	net_type = (dom == LOW) ? "nmos" : "pmos";
    else
	net_type = (dom == LOW) ? "pmos" : "nmos";

    lprintf( stdout, " %s driven %s\n",
       net_type, (dom == LOW) ? "low" : "high" );
    lprintf( stdout, "{tauA=%.2f  tauD=%.2f  tauP=%.2f} ns  ",
      ps2ns( r->tauA ), ps2ns( r->tauD ), ps2ns( r->tauP ) );
    lprintf( stdout, "alpha=%d  beta=%d => peak=%.2f",
      alpha, beta, spk->peak );
    if( is_spk )
	lprintf( stdout, " v=%c\n", vchars[ spk->charge ] );
    else
	lprintf( stdout, " (too small)\n" );
  }
