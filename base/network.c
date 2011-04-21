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
#include "globals.h"

/* event driven mosfet simulator. Chris Terman (2/80) */

public	iptr  hinputs = NULL;	/* list of nodes to be driven high */
public	iptr  linputs = NULL;	/* list of nodes to be driven low */
public	iptr  uinputs = NULL;	/* list of nodes to be driven X */
public	iptr  xinputs = NULL;	/* list of nodes to be removed from input */

public	iptr  infree = NULL;	/* unused input list cells */

public	iptr  *listTbl[8];

public
#define	FreeInput( X )	(X)->next = infree, infree = (X)


public void init_listTbl()
  {
    int  i;

    for( i = 0; i < 8; listTbl[ i++ ] = NULL );
    listTbl[ INPUT_NUM( H_INPUT ) ] = &hinputs;
    listTbl[ INPUT_NUM( L_INPUT ) ] = &linputs;
    listTbl[ INPUT_NUM( U_INPUT ) ] = &uinputs;
    listTbl[ INPUT_NUM( X_INPUT ) ] = &xinputs;
  }


#define	pvalue( node_name, node )	\
    lprintf( stdout, "%s=%c ", (node_name), "0XX1"[(node)->npot] )


private void pgvalue( t )
  register tptr  t;
  {
    register nptr  n;
    static char    *states[] = { "OFF", "ON", "UKNOWN", "WEAK" };

    if( debug )
	lprintf( stdout, "[%s] ", states[t->state] );

    if( t->ttype & GATELIST )
      {
	lprintf( stdout, "( " );
	for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
	  {
	    n = t->gate;
	    pvalue( pnode( n ), n );
	  }

	lprintf( stdout, ") " );
      }
    else
      {
	n = t->gate;
	pvalue( pnode( n ), n );
      }
  }


private void pr_one_res( s, r )
  char     *s;
  double  r;
  {
    if( r < 1e-9 or r > 100e9 )
        (void) sprintf( s, "%2.1e", r );
    else
      {
	int  e = 3;

	if( r >= 1000.0 )
	    do { e++; r *= 0.001; } while( r >= 1000.0 );
	else if( r < 1 and r > 0 )
	    do { e--; r *= 1000; } while( r < 1.0 );
	(void) sprintf( s, "%.1f%c", r, "num\0KMG"[ e ] );
      }
  }


private void pr_t_res( fp, r )
  FILE     *fp;
  Resists  *r;
  {
    char    buf[3][15];

    pr_one_res( buf[0], r->rstatic );
    pr_one_res( buf[1], r->dynhigh );
    pr_one_res( buf[2], r->dynlow );
    lprintf( fp, "[%s, %s, %s]", buf[0], buf[1], buf[2] );
  }


private void ptrans( t )
  register tptr  t;
  {
    lprintf( stdout, "%s ", ttype[ BASETYPE( t->ttype ) ] );
    if( BASETYPE( t->ttype ) != RESIST )
	pgvalue( t );

    pvalue( pnode( t->source ), t->source );
    pvalue( pnode( t->drain ), t->drain );
    pr_t_res( stdout, t->r );
    if( t->tlink != t and (treport & REPORT_TCOORD) )
	lprintf( stdout, " <%d,%d>\n", t->x.pos, t->y.pos );
    else
	lprintf( stdout, "\n" );
  }


public void idelete( n, list )
  register nptr  n;
  register iptr  *list;
  {
    register iptr  p = *list;
    register iptr  q;

    if( p == NULL )
	return;
    else if( p->inode == n )
      {
	*list = p->next;
	FreeInput( p );
      }
    else
      {
	for( q = p->next; q != NULL; p = q, q = p->next )
	    if( q->inode == n )
	      {
		p->next = q->next;
		FreeInput( q );
		return;
	      }
      }
  }


public void iinsert( n, list )
  nptr  n;
  iptr  *list;
  {
    register iptr  p = infree;

    if( p == NULL )
	p = infree = (iptr) MallocList( sizeof( struct Input ), 1 );
    infree = p->next;

    p->next = *list;
    *list = p;
    p->inode = n;
  }


public void iinsert_once( n, list )
  nptr  n;
  iptr  *list;
  {
    register iptr  p;

    for( p = *list; p != NULL; p = p->next )
	if( p->inode == n )
	    return;

    iinsert( n, list );
  }


private	int clear_input( n )
  register nptr  n;
  {
    if( not( n->nflags & POWER_RAIL ) )
	n->nflags &= ~INPUT;
    return( 0 );
  }


public void ClearInputs()
  {
    register int   i;
    register iptr  p, next;
    register nptr  n;

    for( i = 0; i < 5; i++ )
      {
	if( listTbl[ i ] == NULL )
	    continue;
	for( p = *listTbl[ i ]; p != NULL; p = next )
	  {
	    next = p->next;
	    n = p->inode;
	    FreeInput( p );
	    if( not( n->nflags & POWER_RAIL ) )
		n->nflags &= ~(INPUT_MASK | INPUT);
	  }
	*(listTbl[ i ]) = NULL;
      }
    walk_net( clear_input, (char *) 0 );
  }


/*
 * set/clear input status of node and add/remove it to/from corresponding list.
 */
public int setin( n, which )
  register nptr  n;
  char           *which;
  {
    char locwhich = *which;

    if (locwhich == '!') 	/* Toggle the bit value */
    {
	if (n->npot == HIGH)
	    locwhich = 'l';
	else if (n->npot == LOW)
	    locwhich = 'h';
    }

    while( n->nflags & ALIAS )
	n = n->nlink;

    if( n->nflags & (POWER_RAIL | MERGED) )	/* Gnd, Vdd, or merged node */
      {
	if( (n->nflags & MERGED) or "lxuh"[ n->npot ] != locwhich )
	    lprintf( stdout, "Can't drive `%s' to `%c'\n",
	     pnode( n ), locwhich );
      }
    else
      {
	iptr  *list = listTbl[ INPUT_NUM( n->nflags ) ];

#	define	WASINP( N, P )	( ((N)->nflags & INPUT) && (N)->npot == (P) )

	switch( locwhich )
	  {
	    case 'h' :
		if( list != NULL and list != &hinputs )
		  {
		    n->nflags = n->nflags & ~INPUT_MASK;
		    idelete( n, list );
		  }
		if( not (list == &hinputs or WASINP( n, HIGH )) )
		  {
		    n->nflags = (n->nflags & ~INPUT_MASK) | H_INPUT;
		    iinsert( n, &hinputs );
		  }
		break;

	    case 'l' :
		if( list != NULL and list != &linputs )
		  {
		    n->nflags = n->nflags & ~INPUT_MASK;
		    idelete( n, list );
		  }
		if( not (list == &linputs or WASINP( n, LOW )) )
		  {
		    n->nflags = (n->nflags & ~INPUT_MASK) | L_INPUT;
		    iinsert( n, &linputs );
		  }
		break;

	    case 'u' :
		if( list != NULL and list != &uinputs )
		  {
		    n->nflags = n->nflags & ~INPUT_MASK;
		    idelete( n, list );
		  }
		if( not (list == &uinputs or WASINP( n, X )) )
		  {
		    n->nflags = (n->nflags & ~INPUT_MASK) | U_INPUT;
		    iinsert( n, &uinputs );
		  }
		break;

	    case 'x' :
		if( list == &xinputs )
		    break;
		if( list )
		  {
		    n->nflags = n->nflags & ~INPUT_MASK;
		    idelete( n, list );
		  }
		if( n->nflags & INPUT )
		  {
		    n->nflags = (n->nflags & ~INPUT_MASK) | X_INPUT;
		    iinsert( n, &xinputs );
		  }
		break;
	    default :
		return( 0 );
	  }
      }
    return( 1 );
  }


private int wr_value( n, fp )
  register nptr  n;
  FILE           *fp;
  {
    if( not (n->nflags & (ALIAS | POWER_RAIL)) )
      {
	int ch = ((n->nflags & INPUT) ? '4' : '0') + n->npot;
	(void) putc( ch, fp );
      }
    return( 0 );
  }


public int wr_state( fname )
  char  *fname;
  {
    FILE  *fp;

    if( (fp = fopen( fname, "w" )) == NULL )
	return( 1 );

    (void) fprintf( fp, "%d\n", nnodes );
    walk_net( wr_value, (char *) fp );
    (void) fclose( fp );
    return( 0 );
  }


typedef struct
  {
    FILE  *file;
    int   errs;
    int   restore;
  } StateFile;


private int rd_stvalue( n, st )
  register nptr  n;
  StateFile      *st;
  {
    int   ch, inp;

    if( n->nflags & (ALIAS | POWER_RAIL) )
	return( 0 );

    FreeHistList( n );
    while( n->events != NULL )		/* remove any pending events */
	free_event( n->events );

    if( st->file == NULL )
      {
	ch = X;
	st->errs ++;
      }
    else
      {
	ch = getc( st->file );
	if( ch == EOF )
	  {
	    ch = X;
	    st->errs ++;
	    (void) fclose( st->file );
	    st->file = NULL;
	  }
	else if( ch < '0' or ch > '7' or ch == '2' or ch == '6' )
	  {
	    st->errs ++;
	    ch = X;
	  }
	else if( st->restore and ch >= '4' )
	  {
	    ch = ch - '4';
	    inp = 1;
	  }
	else
	  {
	    ch = (ch - '0') & (LOW | X | HIGH);
	    inp = 0;
	  }
      }

    if( n->nflags & MERGED )
	return( 0 );

    if( inp )
	n->nflags |= INPUT;

    n->head.val = ch;
    n->head.inp = inp;

    if( ch != n->npot )
      {
	register lptr  l;
	register tptr  t;

	n->npot = ch;
	for( l = n->ngate; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    t->state = compute_trans_state( t );
	  }
      }
    return( 0 );
  }


public char *rd_state( fname, restore )
  char  *fname;
  int   restore;
  {
    char       rline[25];
    StateFile  st;

    if( (st.file = fopen( fname, "r" )) == NULL )
	return( "can not read state file\n" );

    (void) fgetline( rline, 25, st.file );
    if( atoi( rline ) != nnodes )
      {
	(void) fclose( st.file );
	return( "bad node count in state file\n" );
      }

    ClearInputs();

    if( analyzerON )
	StopAnalyzer();

    sim_time0 = cur_delta = 0;

    st.errs = 0;
    st.restore = restore;
    walk_net( rd_stvalue, (char *) &st );

    NoInit();

    if( analyzerON )
	RestartAnalyzer( sim_time0, cur_delta, FALSE );

    if( st.file == NULL )
      {
	(void) sprintf( fname, "premature EOF in state file (%d errors)\n",
	  st.errs );
	return( fname );
      }

    (void) fclose( st.file );

    if( st.errs != 0 )
      {
	(void) sprintf( fname, "%d errors found in state file\n", st.errs );
	return( fname );
      }
    return( NULL );
  }


public int info( n, which )
  register nptr  n;
  char           *which;
  {
    register tptr  t;
    register lptr  l;
    char           *name;

    if( n == NULL )
	return( 0 );

    if( int_received )
	return( 1 );

    name = pnode( n );
    while( n->nflags & ALIAS )
	n = n->nlink;

    if( n->nflags & MERGED )
      {
	lprintf( stdout, "%s => node is inside a transistor stack\n", name );
	return( 1 );
      }

    pvalue( name, n );
    if( n->nflags & INPUT )
	lprintf( stdout, "[NOTE: node is an input] " );
    lprintf( stdout, "(vl=%.2f vh=%.2f) ", n->vlow, n->vhigh );
    if( n->nflags & USERDELAY )
	lprintf( stdout, "(tplh=%d, tphl=%d) ", n->tplh, n->tphl );
    lprintf( stdout, "(%5.4f pf) ", n->ncap );

    if ((*which == '?') || !strcmp(which, "querysource"))
      {
	lprintf( stdout, "is computed from:\n" );
	for( l = n->nterm; l != NULL and int_received == 0; l = l->next )
	  {
	    t = l->xtor;
	    lprintf( stdout, "  " );
	    if( debug == 0 )
	      {
		char  *drive = NULL;
		nptr  rail;

		rail = (t->drain->nflags & POWER_RAIL) ? t->drain : t->source;
		if( BASETYPE( t->ttype ) == NCHAN and rail == GND_node )
		    drive = "pulled down by ";
		else if( BASETYPE( t->ttype ) == PCHAN and rail == VDD_node )
		    drive = "pulled up by ";
		else if( BASETYPE( t->ttype ) == DEP and rail == VDD_node and
		  other_node( t, rail ) == t->gate )
		    drive = "pullup ";
		else
		    ptrans( t );

		if( drive != NULL )
		  {
		    lprintf( stdout, drive );
		    pgvalue( t );
		    pr_t_res( stdout, t->r );
		    if( t->tlink != t and (treport & REPORT_TCOORD) )
			lprintf( stdout, " <%d,%d>\n", t->x.pos, t->y.pos );
		    else
			lprintf( stdout, "\n" );
		  }
	      }
	    else
		ptrans( t );
	  }
      }
    else
      {
	lprintf( stdout, "affects:\n" );
	for( l = n->ngate; l != NULL and int_received == 0; l = l->next )
	    ptrans( l->xtor );
      }

    if( int_received )
	lprintf( stdout, "-- interrupted\n" );

    if( n->events != NULL )
      {
	register evptr  e;

	lprintf( stdout, "Pending events:\n" );
	for( e = n->events; e != NULL; e = e->nlink )
	    lprintf( stdout, "   transition to %c at %2.2fns\n",
	      "0XX1"[e->eval], d2ns( e->ntime ) );
      }

    return( 1 );
  }
