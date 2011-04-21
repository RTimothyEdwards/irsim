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
#include "net_macros.h"


public	nptr   VDD_node;		/* power supply nodes */
public	nptr   GND_node;

public	lptr   on_trans;		/* always on transistors */

public	int    nnodes;			/* number of actual nodes */
public	int    naliases;		/* number of aliased nodes */
public	int    ntrans[ NTTYPES ];	/* number of txtors indexed by type */

public	lptr   freeLinks = NULL;	/* free list of Tlist structs */
public	tptr   freeTrans = NULL;	/* free list of Transistor structs */
public	nptr   freeNodes = NULL;	/* free list of Node structs */

public	tptr   tcap = NULL;		/* list of capacitor-transistors */


public
#define	MIN_CAP		0.0005		/* minimum node capacitance (in pf) */


private void init_counts()
  {
    register int  i;

    for( i = 0; i < NTTYPES; i++ )
	ntrans[i] = 0;
    nnodes = naliases = 0;
  }


public int rd_network( simfile )
  FILE  *simfile;
  {
    static int  firstcall = 1;
    char        line[ 200 ];

    if( firstcall )
      {
	init_hash();
	init_counts();
	init_listTbl();

	VDD_node = GetNode( "Vdd" );
	VDD_node->npot = HIGH;
	VDD_node->nflags |= (INPUT | POWER_RAIL);
	VDD_node->head.inp = 1;
	VDD_node->head.val = HIGH;
	VDD_node->head.punt = 0;
	VDD_node->head.time = 0;
	VDD_node->head.t.r.rtime = VDD_node->head.t.r.delay = 0;
	VDD_node->head.next = last_hist;
	VDD_node->curr = &(VDD_node->head);

	GND_node = GetNode( "Gnd" );
	GND_node->npot = LOW;
	GND_node->nflags |= (INPUT | POWER_RAIL);
	GND_node->head.inp = 1;
	GND_node->head.val = LOW;
	GND_node->head.punt = 0;
	GND_node->head.time = 0;
	GND_node->head.t.r.rtime = GND_node->head.t.r.delay = 0;
	GND_node->head.next = last_hist;
	GND_node->curr = &(GND_node->head);

	NEW_TRANS( tcap );
	tcap->scache.t = tcap->dcache.t = tcap;
	tcap->x.pos = 0;

	firstcall = 0;
      }

    if( fgetline( line, 200, simfile ) == NULL )
      {
	lprintf( stderr, "can't read input file\n" );
	exit( 1 );
      }
    if( rd_netfile( simfile, line ) == FALSE )
	exit( 1 );
  }


public void pTotalNodes()
  {
    lprintf( stdout, "%d nodes", nnodes );
    if( naliases != 0 )
	lprintf( stdout, ", %d aliases", naliases );
    lprintf( stdout, "; " );
  }


public void pTotalTxtors()
  {
    int  i;

    lprintf( stdout, "transistors:" );
    for( i = 0; i < NTTYPES; i++ )
	if( ntrans[i] != 0 )
	    lprintf( stdout, " %s=%d", ttype[i], ntrans[i] );
    if( tcap->x.pos != 0 )
	lprintf( stdout, " shorted=%d", tcap->x.pos );
    lprintf( stdout, "\n" );
  }


public void ConnectNetwork()
  {
#ifdef notdef
    nptr  ndlist;

    pTotalNodes();
    ndlist = ( isBinFile ) ? bin_connect_txtors() : connect_txtors();
    pTotalTxtors();
    make_parallel( ndlist, (long)(stack_txtors ? 0 : VISITED) );
    pParallelTxtors();
    if( stack_txtors )
      {
	make_stacks( ndlist );
	pStackedTxtors();
      }
#endif
  }


#define	SIZE2LAMBDA( X )	( (double) (X) / LAMBDACM )
#define	CAP_LIM			( 2 * MIN_CAP )
#define	CAP2FF( C )		( (int) (((C) + (MIN_CAP / 2)) * 1000.0) )


public void wr_sim( fout )
  FILE  *fout;
  {
    register tptr  t;
    register nptr  n;
    tptr           tlist;
    nptr           nlist;
    char           capfmt[50];

    if( use_GND and GND_node->nname[0] == 'G' )		/* may be a number */
	(void) strcpy( GND_node->nname, "GND" );

    nlist = GetNodeList();
    GetInetParms( &tlist, &LAMBDACM );

    for( t = tlist; t != NULL; t = t->scache.t )
      {
	int  type;

	type = BASETYPE( t->ttype );

	if( type == RESIST )
	  {
	    (void) fprintf( fout, "r %s %s %.2f\n",
	      pnode( t->source ), pnode( t->drain ), t->r->rstatic );
	  }
	else
	  {
	    (void) fprintf( fout, "%c %s %s %s %.2f %.2f", ttype[type][0],
	      pnode( t->gate ), pnode( t->source ), pnode( t->drain ),
	      SIZE2LAMBDA( t->r->length ), SIZE2LAMBDA( t->r->width ) );
	    if( t->tlink != t )
		(void) fprintf( fout, " %ld %ld", t->x, t->y );
	    (void) fputc( '\n', fout );
	  }
	t->gate->nflags |= VISITED;
	t->source->nflags |= VISITED;
	t->drain->nflags |= VISITED;
      }

    if( do_sim_cap and two_term_caps )
      {
	(void) sprintf( capfmt, "C %%s %s %%d\n", GND_node->nname );

	for( n = nlist; n != NULL; n = n->n.next )
	  {
	    if( not (n->nflags & VISITED) or n->ncap >= CAP_LIM )
		(void) fprintf( fout, capfmt, pnode( n ), CAP2FF( n->ncap ) );
	  }
      }
    else if( do_sim_cap )
      {
	for( n = nlist; n != NULL; n = n->n.next )
	  {
	    if( not (n->nflags & VISITED) or n->ncap >= CAP_LIM )
		(void) fprintf( fout, "C %s %.3f\n", pnode( n ), n->ncap );
	  }
     }

    for( n = nlist; n != NULL; n = n->n.next )
      {
	if( n->nflags & ALIAS )
	    (void) fprintf( fout, "= %s %s\n", pnode( n ), pnode( n->nlink ));
      }

    if( do_delays )
      {
	for( n = nlist; n != NULL; n = n->n.next )
	  {
	    if( n->nflags & USERDELAY )
		(void) fprintf( fout, "D %s %.2f %.2f\n", pnode( n ),
		  d2ns( n->tplh ), d2ns( n->tphl ) );
	  }
      }

    if( do_thresholds )
      {
	float  vhigh = HIGHTHRESH, vlow = LOWTHRESH;

	for( n = nlist; n != NULL; n = n->n.next )
	  {
	    if( n->vlow != vlow or n->vhigh != vhigh )
		(void) fprintf( fout, "t %s %g %g\n", pnode( n ),
		  n->vlow, n->vhigh );
	  }
      }
  }
