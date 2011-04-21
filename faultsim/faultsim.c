#include <stdio.h>
#include <string.h>  /* for strcmp() */

#include "defs.h"
#include "net.h"
#include "units.h"
#include "net_macros.h"
#include "globals.h"


#define	DEFAULT_PERCENT	20		/* default % of nodes to seed */

#define	N_SEED0		0x080000
#define	N_SEED1		0x100000
#define	N_OBVIOUS0	0x200000
#define	N_OBVIOUS1	0x400000
#define	FAULT_ME	0x800000


#define	PRIM_OUTP	DELETED

typedef struct Trigger *ptrig;

typedef struct Trigger
  {
    ptrig  next;	/* next trigger */
    long   period;	/* for sampler */
    long   offset;
    long   maxtime;
    hptr   samples;	/* list of sampling transitions */
    iptr   outp;	/* list of primary outputs to sample */
  } Trigger;


private struct
  {
    int    maybe;		/* may be detectable (node differs by X) */
    nptr   node;		/* node in which fault is detected */
    long   time;		/* time at which it is detected */
  } fault;


private	FILE	*f_fault;			/* file to send output */
private	ptrig	triggers = NULL;		/* list of triggers */
private	long	maxSampleTime;
private	tptr    stuckTrans = NULL;		/* trans. to fault a node */
private	int	pend_triggers;			/* # pending trigger events */
private	int	n_nodes_seeded;			/* number of nodes seeded */
private	int	num_detect, num_maybe, num_fail;

	/* forward declaration */
private tptr	init_fault_trans();


int add_trigger( nd, edge, delay )
  nptr  nd;
  int   edge;
  long  delay;
  {
    ptrig          t;
    struct Node    fake_nd;
    Ulong           tm;
    int            other = (edge == LOW) ? HIGH : LOW;
    register hptr  h;

    while( nd->nflags & ALIAS )
	nd = nd->nlink;
    fake_nd.curr = &fake_nd.head;
    fake_nd.head.next = last_hist;
    for( h = &nd->head; h != last_hist; h = h->next )
      {
	while( h != last_hist and (h->punt or h->val != other) ) h = h->next;
	while( h != last_hist and (h->punt or h->val != edge) ) h = h->next;
	tm = h->time + delay;
	if( h->val == edge and tm > 0 and tm <= cur_delta )
	    AddHist( &fake_nd, edge, 1, tm, 0L, 0L );
      }

    if( fake_nd.head.next == last_hist )
	return( 1 );

    t = (ptrig) Valloc( (int) sizeof( Trigger ), 1 );
    t->samples = fake_nd.head.next;
    t->period = t->offset = 0;
    t->outp = NULL;
    t->next = triggers;
    triggers = t;
    return( 0 );
  }


int add_sampler( period, offset )
  long  period, offset;
  {
    ptrig  t;

    maxSampleTime = cur_delta;
    t = (ptrig) Valloc( (int) sizeof( Trigger ), 1 );
    t->samples = NULL;
    t->period = period;
    t->offset = offset;
    t->outp = NULL;
    t->next = triggers;
    triggers = t;
    return( 0 );
  }


int add_prim_output( nd, flag )
  nptr  nd;
  int   *flag;
  {
    while( nd->nflags & ALIAS )
	nd = nd->nlink;

    if( nd->nflags & (POWER_RAIL | MERGED) )
	return( 1 );
    if( nd->nflags & PRIM_OUTP )
      {
	if( nd->n.thev != (Thev) triggers )
	  {
	    lprintf( stderr, "Too many trigger/sample for %s\n", pnode(nd) );
	    *flag |= 2;
	  }
      }
    else if( triggers != NULL )
      {
	iinsert( nd, &triggers->outp );
	nd->nflags |= PRIM_OUTP;
	nd->n.thev = (Thev) triggers;
	*flag |= 1;
      }
    return( 1 );
  }


void cleanup_fsim()
  {
    ptrig        t;
    iptr         i, it;
    struct Node  fake_nd;

    while( triggers != NULL )
      {
	if( triggers->samples != NULL )
	  {
	    fake_nd.head.next = triggers->samples;
	    FreeHistList( &fake_nd );
	  }
	for( i = triggers->outp; i != NULL; FreeInput( it ) )
	  {
	    it = i;
	    i->inode->nflags &= ~PRIM_OUTP;
	    i = i->next;
	  }
	t = triggers;
	triggers = triggers->next;
	Vfree( t );
      }
  }


#ifndef TIME_FAULTS
#  define FaultFail( ND, VAL ) \
    num_fail ++, \
    (f_fault == NULL) ? 0 : \
      fprintf( f_fault, "Fail\t%c  %s\n", vchars[ VAL ], pnode( ND ) )

#  define PrFault( TYPE, N, V ) \
    ( f_fault == NULL ) ? 0 : \
      fprintf( f_fault, "%s\t%c  %s [%.2f] %s\n", \
      TYPE, vchars[ V ], pnode( N ), d2ns( fault.time ), pnode( fault.node ) )

#else

#  define FaultFail( ND, VAL ) \
    num_fail ++, \
    (f_fault == NULL) ? 0 : \
      fprintf( f_fault, "Fail\t%c  (%s) %s\n", vchars[VAL], ustr, pnode(ND) )

#  define PrFault( TYPE, N, V ) \
    ( f_fault == NULL ) ? 0 : \
      fprintf( f_fault, "%s\t%c  %s [%.2f] (%s) %s\n", \
      TYPE, vchars[V], pnode(N), d2ns(fault.time), ustr, pnode(fault.node) )

#endif

#define FaultFound( ND, VAL )	num_detect++, PrFault( "Detect", ND, VAL )

#define FaultMaybe( ND, VAL )	num_maybe++, PrFault( "Maybe", ND, VAL )
				

private void StuckNode( nd, val )
  nptr  nd;
  int   val;
  {
    tptr  t;

    if( stuckTrans == NULL ) stuckTrans = init_fault_trans();

    t = stuckTrans;
    if( val == LOW )
      {
	t->ttype = NCHAN;	t->gate = VDD_node;	t->source = GND_node;
      }
    else	/* val == HIGH */
      {
	t->ttype = PCHAN;	t->gate = GND_node;	t->source = VDD_node;
      }

    t->drain = nd;
    CONNECT( t->gate->ngate, t );
    CONNECT( nd->nterm, t );
  }


private void UnStuckNode( nd )
  nptr  nd;
  {
    tptr  t = stuckTrans;

    DISCONNECT( t->gate->ngate, t );
    DISCONNECT( nd->nterm, t );
  }


private int do_fault( nd, val )
  nptr nd;
  int  val;
  {
    fault.maybe = FALSE;
    fault.node = NULL;

    StuckNode( nd, val );

#ifdef TIME_FAULTS
    uset_usage();    faultsim( nd );    get_usage( ustr );
#else
    faultsim( nd );
#endif

    if( fault.node != NULL )
	FaultFound( nd, val );
    else if( fault.maybe )
	FaultMaybe( nd, val );
    else
	FaultFail( nd, val );

    UnStuckNode( nd );
    return 1;
  }


private int should_seed( nd )
  nptr  nd;
  {
    register hptr  h;
    register int   zero, one;

    zero = one = 1;
    for( h = nd->head.next; h != last_hist; h = h->next )
      {
	if( h->inp )		return( 0 );	/* primary input */
	if( h->punt )		continue;
	if( h->val != LOW )	zero = 0;
	if( h->val != HIGH )	one = 0;
	if( zero + one == 0 )	break;
      }

    nd->nflags |= ( zero ) ? N_OBVIOUS0 : N_SEED0;
    nd->nflags |= ( one ) ? N_OBVIOUS1 : N_SEED1;

    return( 1 );
  }


private int seed_faults( p_seed )
  int  p_seed;
  {
    extern int   random();
    extern void  srandom();
    nptr         n, nd;
    int          can_seed, num_to_seed, num;

    lprintf( stdout, "seeding faults..." ); fflush( stdout );

    nd = GetNodeList();

    can_seed = 0;
    for( n = nd; n != NULL; n = n->n.next )
      {
	if( (n->nflags & (ALIAS | MERGED | POWER_RAIL | INPUT)) == 0 and
	  n->ngate != NULL )
	    can_seed += should_seed( n );
      }

    num_to_seed = (nnodes * p_seed) / 100;

    if( can_seed < num_to_seed )		/* seed all possible nodes */
      {
	for( n = nd; n != NULL; n = n->n.next )
	  {
	    if( n->nflags & (N_OBVIOUS0 | N_SEED0 | N_OBVIOUS1 | N_SEED1) )
		n->nflags |= FAULT_ME;
	  }
	lprintf( stdout, "done.  Only %d nodes to simulate\n", can_seed );
	return( can_seed );
      }
	
    num = 0;
    srandom( (long) nnodes );

  keep_seeding :
    for( n = nd; n != NULL; n = n->n.next )
      {
	if( (n->nflags & (N_OBVIOUS0 | N_SEED0 | N_OBVIOUS1 | N_SEED1)) == 0 )
	    continue;
	if( n->nflags & FAULT_ME )
	    continue;
	if( (random() & 100) <= p_seed )
	  {
	    num += 1;
	    n->nflags |= FAULT_ME;
	  }
	if( num >= num_to_seed )
	    break;
      }

    if( num < num_to_seed )
      {
	p_seed += (p_seed + 1) / 2;	/* increase probability */
	goto keep_seeding;
      }

    lprintf( stdout, "done.  %d nodes to simulate\n", can_seed );
    return( num_to_seed );
  }


private int seed_fault( nd )
  nptr  nd;
  {
    if( nd->nflags & FAULT_ME )
      {
	n_nodes_seeded ++;

	if( nd->nflags & N_OBVIOUS0 )	FaultFail( nd, LOW );
	if( nd->nflags & N_SEED0 )	do_fault( nd, LOW );

	if( nd->nflags & N_OBVIOUS1 )	FaultFail( nd, HIGH );
	if( nd->nflags & N_SEED1 )	do_fault( nd, HIGH );
      }
    nd->nflags &= ~(N_SEED0 | N_SEED1 | N_OBVIOUS0 | N_OBVIOUS1 | FAULT_ME);
    return( int_received );
  }


private int clear_node_flags( nd )
  nptr  nd;
  {
    nd->nflags &= ~(N_SEED0 | N_SEED1 | N_OBVIOUS0 | N_OBVIOUS1 | FAULT_ME);
    return( 0 );
  }


void exec_fsim( fname, p_seed )
 char  *fname;
 int   p_seed;
  {
    FILE    *log_save = logfile;
    int     nodes_to_test;
    double  total;

    if( triggers == NULL )
      {
	lprintf( stderr, "No triggers defined.  Aborted\n" );
	return;
      }

    if( p_seed <= 0 )	p_seed = DEFAULT_PERCENT;

    if( fname == NULL )
	fname = "fsim.out";
    if( strcmp( fname, "/dev/null" ) == 0 )	/* for timing: no i/o */
	f_fault = NULL;
    else if( (f_fault = fopen( fname, "w" )) == NULL )
      {
	lprintf( stderr, "Cannot open '%s'.  Aborted\n" );
	return;
      }

    nodes_to_test = seed_faults( p_seed );

    num_detect = num_maybe = num_fail = n_nodes_seeded = 0;

    init_faultsim();

    walk_net( seed_fault, (char *) 0 );

    logfile = f_fault;

    if( int_received )
      {
	lprintf( stdout, "** Interrupted ** => seeded %d nodes of %d (%g%%)\n",
	  n_nodes_seeded, nodes_to_test,
	  100.0 * n_nodes_seeded / nodes_to_test );
	walk_net( clear_node_flags, (char *) 0 );
      }

    total = num_detect + num_maybe + num_fail;

    lprintf( stdout,
      "----\n%.0f faults\n%d detected  (%d probably)\n%d undetected\n",
      total, num_detect, num_maybe, num_fail );

    if( total == 0.0 )
      {
	total = 1;
	num_detect = 1;
      }
    total *= 0.01;

    lprintf( stdout, "fault coverage: %.2f%% (%.2f%%)\n",
       num_detect / total, (num_detect + num_maybe) / total );

    if( f_fault != NULL and f_fault != stdout )
	(void) fclose( f_fault );

    logfile = log_save;

    end_faultsim();
  }


void setup_triggers()
  {
    ptrig  t;
    evptr  ev;

    pend_triggers = 0;
    for( t = triggers; t != NULL; t = t->next )
      {
	if( t->samples != NULL )
	  {
	    ev = EnqueueOther( TRIGGER_EV, (long) t->samples->time );
	    ev->enode = (nptr) t;
	    ev->p.hist = t->samples;
	  }
	else
	  {
	    Ulong  tm = t->offset ? t->offset : t->offset + t->period;
	    ev = EnqueueOther( TRIGGER_EV, tm );
	    ev->enode = (nptr) t;
	  }
	pend_triggers ++;
      }
  }


int do_trigger( ev )
  evptr  ev;
  {
    ptrig          t = (ptrig) ev->enode;
    hptr           h;
    register iptr  i;

    for( i = t->outp; i != NULL; i = i->next )
      {
	register nptr  n = i->inode;

	if( n->nflags & DEVIATED )
	  {
	    if( n->npot != X and n->oldpot != X )
	      {
		fault.time = cur_delta;
		fault.node = n;
		return( TRUE );
	      }
	    else if( not fault.maybe )
	      {
		fault.time = cur_delta;
		fault.node = n;
		fault.maybe = TRUE;
	      }
	  }
      }
    if( t->samples != NULL )
      {
	h = ev->p.hist->next;
	if( h != last_hist )
	  {
	    ev = EnqueueOther( TRIGGER_EV, (Ulong) h->time );
	    ev->enode = (nptr) t;
	    ev->p.hist = h;
	  }
	else
	    pend_triggers --;
      }
    else
      {
	if( ev->ntime + t->period <= maxSampleTime )
	  {
	    ev = EnqueueOther( TRIGGER_EV, ev->ntime + t->period );
	    ev->enode = (nptr) t;
	  }
	else
	    pend_triggers --;
      }

    return( (pend_triggers <= 0) ? TRUE : FALSE );
  }


private tptr init_fault_trans()
  {
    static Resists       smallR = { { 0.00001, 0.00001 }, 0.00001, 0, 0 };
    static struct Trans  tran;

    tran.scache.r = tran.dcache.r = NULL;
    tran.tflags = 0;
    tran.n_par = 0;
    tran.state = ON;
    tran.r = &smallR;
    tran.tlink = &tran;
    tran.x.pos = tran.x.pos = 0;
    return( &tran );
  }
