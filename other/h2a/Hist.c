#include <stdio.h>
#include "defs.h"
#include "net.h"
#include "globals.h"
#include "history.h"


public	hptr   freeHist = NULL;		/* list of free history entries */
public  hptr   last_hist;		/* pointer to dummy hist-entry that
					 * serves as tail for all nodes */
public	hptr	first_model;		/* first model entry */
public	Ulong	max_time;

private	char    inp_chars[] = " i";
private	char	node_name[256];
private	int	skip_it = 0;
extern	phist	*Sort();


private struct
  {
    phist  first;
    phist  *last;
    int    num;
    char   *sv_name;
  } theHistory = { NULL, &theHistory.first, 0, NULL };


private void NewHist( tm, delay, rtime, ptime, val, type, name )
  long  tm;
  int   delay, rtime, ptime, val, type;
  char  *name;
  {
    static phist  hfree = NULL;
    phist         new;

    if( (new = hfree) == NULL )
	new = (phist) MallocList( sizeof( History ), 1 );
    hfree = new->next;
    new->time = tm;
    new->delay = delay;
    new->rtime = rtime;
    new->ptime = ptime;
    new->name = (type == H_PEND) ? theHistory.sv_name : name;
    new->val = vchars[ val ];
    new->type = type;
    *theHistory.last = new;
    theHistory.last = &new->next;
    theHistory.num ++;
    if( type == H_FIRST or type == H_FIRST_INP )
	theHistory.sv_name = name;
  }


public void sortAndPrint()
  {
    phist     *data;
    long      maxv = 0;
    register phist  *arrp, p;

    if( theHistory.first == NULL )
      {
	if( only_name != NULL )
	    fprintf( stderr, "%s: NOT found\n", only_name );
	else
	    fprintf( stderr, "EMPTY HISTORY?\n" );
	exit( 0 );
      }
    *theHistory.last = NULL;
    data = (phist *) Valloc( (theHistory.num + 1) * sizeof( phist * ), 1 );
    for( arrp = data, p = theHistory.first; p != NULL; p = p->next )
      {
	*arrp++ = p;
	if( p->time > maxv ) maxv = p->time;
      }
    *arrp = NULL;

    data = Sort( theHistory.num, data, maxv );

    for( arrp = data; (p = *arrp) != NULL; arrp++ )
      {
	printf( "%10.1f  %c%c", d2ns( p->time ), p->val, " i ipP"[p->type] );
	if( p->type <= H_FIRST_INP )
	  {
	    if( do_delay )	printf( "      --" );
	    if( do_rtime )	printf( "      --" );
	  }
	else
	  {
	    if( do_delay )	printf( "%8.1f", d2ns( p->delay ) );
	    if( do_rtime )	printf( "%8.1f", d2ns( p->rtime ) );
	    if( p->type == H_PUNT and do_delay )
		printf( "  (%.1f)", d2ns( p->time - p->ptime ) );
	  }
	printf( "  %s\n", p->name );
      }
  }


public void init_hist()
  {
    static HistEnt  dummy;
    static HistEnt  dummy_model;

    max_time = MAX_TIME;

    last_hist = &dummy;
    dummy.next = last_hist;
    dummy.time = max_time;
    dummy.val = X;
    dummy.inp = 1;
    dummy.punt = 0;
    dummy.t.r.delay = dummy.t.r.rtime = 0;
  }

public void FreeHistList( node ) nptr node; {}


public void SetFirstHist( node, val, inp, time )
  nptr  node;
  int   val, inp;
  long  time;
  {
    if( only_name != NULL )
      {
	if( skip_it == 2 )		/* all_done */
	  {
	    if( do_sort ) sortAndPrint();
	    exit( 0 );
	  }
	skip_it = (str_eql( only_name, node->nname ) == 0) ? 2 : 1;
	if( skip_it == 1 )
	    return;
      }

    if( do_sort )
      {
	NewHist( time, 0, 0, 0, val, H_FIRST + inp, node->nname );
	return;
      }

    (void) strcpy( node_name, node->nname );

    if( not do_names )
	printf( "\n%s\n", node_name );

    printf( "%10.1f  %c%c", d2ns( time ), vchars[ val ], inp_chars[ inp ] );
    if( do_delay )
	printf( "      --" );
    if( do_rtime )
	printf( "      --" );
    if( do_names )
	printf( "  %s", node_name );
    putchar( '\n' );
  }


public void AddHist( node, val, inp, time, delay, rtime )
  nptr  node;
  int   val, inp;
  long  time, delay, rtime;
  {
    if( skip_it == 1 )
	return;

    if( do_sort )
      {
	NewHist( time, (int) delay, (int) rtime, 0, val, H_NORM + inp,
	  node->nname );
	return;
      }
    printf( "%10.1f  %c%c", d2ns( time ), vchars[ val ], inp_chars[ inp ] );
    if( do_delay )
	printf( "%8.1f", d2ns( delay ) );
    if( do_rtime )
	printf( "%8.1f", d2ns( rtime ) );
    if( do_names )
	printf( "  %s", node_name );
    putchar( '\n' );
  }


public void AddPunted( node, ev, delta )
  nptr   node;
  evptr  ev;
  long   delta;
  {
    if( skip_it == 1 )
	return;

    if( do_sort )
      {
	NewHist( ev->ntime, (int) ev->delay, (int) ev->rtime,
	  ev->ntime - delta, ev->eval, H_PUNT, node->nname );
	return;
      }
    printf( "%10.1f  %cp", d2ns( ev->ntime ), vchars[ ev->eval ] );
    if( do_delay )
	printf( "%8.1f", d2ns( ev->delay ) );
    if( do_rtime )
	printf( "%8.1f", d2ns( ev->rtime ) );
    if( do_delay )
	printf( "  (%.1f)", d2ns( delta ) );
    if( do_names )
	printf( "  %s", node_name );
    putchar( '\n' );
  }


public void enqueue_event( node, val, delay, rtime )
  nptr  node;
  int   val;
  long  delay, rtime;
  {
    if( skip_it == 1 )
	return;

    if( do_sort )
      {
	NewHist( cur_delta + delay, (int) delay, (int) rtime, 0, val, H_PEND,
	  node->nname );
	return;
      }
    printf( "%10.1f  %cP", d2ns( cur_delta + delay ), vchars[ val ] );
    if( do_delay )
	printf( "%8.1f", d2ns( delay ) );
    if( do_rtime )
	printf( "%8.1f", d2ns( rtime ) );
    if( do_names )
	printf( "  %s", node_name );
    putchar( '\n' );
  }
