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
 * The routines in this file handle binary network files input/output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defs.h"
#include "net.h"
#include "globals.h"
#include "rsim.h"
#include "bin_io.h"
#include "net_macros.h"


private	FILE	*fnet;			/* file for reading/writing network */
private	tptr	brd_tlist;		/* list of transistor just read */
private	double	inet_CTGA;		/* transistor capacitances */
private	double	inet_CTDE;
private	double	inet_CTDW;
private	long	inet_lambda;
private	void	adjust_transistors();

private	char	netHeader[] = "<<inet>>";
private	char	endAscii[] = "<end>\n";

#define	HEADER_LEN		(sizeof( netHeader ) - 1)

	/* define number of bytes used to write/read network parameter */
#define	NB_NDINDEX	sizeof(pointertype)
#define	NB_NCAP		sizeof(double)
#define	NB_TSIZE	sizeof(int)
#define	NB_COORD	sizeof(int)
#define	NB_VTHRESH	2
#define	NB_TP		2
#define	NB_STRLEN	2
#define	NB_FLAGS	1
#define	NB_TTYPE	1

	/* node flags we are interested in preserving */
#define	SAVE_FLAGS	(POWER_RAIL | ALIAS | USERDELAY)

	/* scale factors for floating-point numbers */
#define	VTH_SCALE		1000.0
#define	CAP_SCALE		16384.0    /* power of 2 to minimize error */


typedef struct		/* format of node records in net file */
  {
    union
      {
	char  cap[ NB_NCAP ];
	char  alias[ NB_NDINDEX ];
      } n;
    char  flags[ NB_FLAGS ];
    char  slen[ NB_STRLEN ];
  } File_Node;

#define	Size_File_Node		( NB_NDINDEX + NB_FLAGS + NB_STRLEN )


typedef struct		/* format of node user-delays in net file */
  {
    char  tphl[ NB_TP ];
    char  tplh[ NB_TP ];
  } File_Delay;

#define	Size_File_Delay		( NB_TP + NB_TP )


typedef struct		/* format of node thresholds in net file */
  {
    char  vhigh[ NB_VTHRESH ];
    char  vlow[ NB_VTHRESH ];
  } File_Thresh;

#define	Size_File_Thresh 	( NB_VTHRESH + NB_VTHRESH )


typedef struct		/* format of transistor records in net file */
  {
    char  width[ NB_TSIZE ];
    char  length[ NB_TSIZE ];
    char  ttype[ NB_TTYPE ];
    char  gate[ NB_NDINDEX ];
    char  src[ NB_NDINDEX ];
    char  drn[ NB_NDINDEX ];
    char  x[ NB_COORD ];
    char  y[ NB_COORD ];
  } File_Trans;

#define	Size_File_Trans	( NB_TSIZE + NB_TSIZE + NB_TTYPE + \
		NB_NDINDEX + NB_NDINDEX + NB_NDINDEX + NB_COORD + NB_COORD )


private tptr wr_nodes()
  {
    register nptr  n;
    register tptr  txlist;
    register lptr  l;
    File_Node      node;
    File_Delay     delays;
    File_Thresh    thresh;
    int            slen;
    double         cap;
    float          vhigh = HIGHTHRESH, vlow = LOWTHRESH;

    txlist = NULL;

    for( n = GetNodeList(); n != NULL; n = n->n.next )
      {
	cap = n->ncap;
	for( l = n->nterm; l != NULL; l = l->next )
	  {
	    register tptr  t = l->xtor;

	    if( not (t->tflags & ACTIVE_T) )
	      {
		t->scache.t = txlist;
		txlist = t;
		t->tflags |= ACTIVE_T;
	      }
	    if( t->ttype & GATELIST )
	      cap += (StackCap( t ) / 2.0);
	  }
	
	slen = strlen( pnode( n ) ) + 1;
	PackBytes( node.slen, slen, NB_STRLEN );
	node.flags[0] = n->nflags & SAVE_FLAGS;
	if( n->vhigh != vhigh or n->vlow != vlow )
	    node.flags[0] |= WATCHED;
	if( n->nflags & ALIAS )
	    PackBytes( node.n.alias, Node2index( n->nlink ), NB_NDINDEX )
	else
	    PackBytes( node.n.cap, cap * CAP_SCALE, NB_NCAP );
	(void) Fwrite( &node, Size_File_Node, fnet );
	(void) Fwrite( pnode( n ), slen, fnet );

	if( n->nflags & USERDELAY )
	  {
	    PackBytes( delays.tphl, n->tphl, NB_TP );
	    PackBytes( delays.tplh, n->tplh, NB_TP );
	    (void) Fwrite( &delays, Size_File_Delay, fnet );
	  }
	if( node.flags[0] & WATCHED )
	  {
	    PackBytes( thresh.vhigh, n->vhigh * VTH_SCALE, NB_VTHRESH );
	    PackBytes( thresh.vlow, n->vlow * VTH_SCALE, NB_VTHRESH );
	    (void) Fwrite( &thresh, Size_File_Thresh, fnet );
	  }
      }

    return( txlist );
  }


private void wr_trans( t )
  register tptr  t;
  {
    File_Trans  trans;
    pointertype ndindx;

    if( t->ttype & ORED )
      {
	for( t = t->tlink; t != NULL; t = t->scache.t ) wr_trans( t );
	return;
      }

    ndindx = Node2index( t->gate );
    PackBytes( trans.gate, ndindx, NB_NDINDEX );

    ndindx = Node2index( t->source );
    PackBytes( trans.src, ndindx, NB_NDINDEX );

    ndindx = Node2index( t->drain );
    PackBytes( trans.drn, ndindx, NB_NDINDEX );

    PackBytes( trans.length, t->r->length, NB_TSIZE );
    PackBytes( trans.width, t->r->width, NB_TSIZE );

    trans.ttype[0] = t->ttype & ~(ORED | ORLIST | STACKED | GATELIST);

    if( t->tlink != t )
      {
	trans.ttype[0] |= GATELIST;
	PackBytes( trans.x, t->x.pos, NB_COORD );
	PackBytes( trans.y, t->y.pos, NB_COORD );
      }

    (void) Fwrite( &trans, Size_File_Trans, fnet );
  }


private void wr_txtors( tlist )
  tptr  tlist;
  {
    register tptr  t, next;

    for( t = tlist; t != NULL; t = next )
      {
	next = t->scache.t;

	t->tflags &= ~ACTIVE_T;
	t->scache.t = NULL;

	if( t->ttype & GATELIST )
	  {
	    for( t = (tptr) t->gate; t != NULL; t = t->scache.t )
		wr_trans( t );
	  }
	else
	    wr_trans( t );
      }

    for( t = tcap->dcache.t; t != tcap; t = t->dcache.t )
	wr_trans( t );
  }


private void WriteAscii( f )
  FILE  *f;
  {
    (void) fprintf( f, "lambda %ld\n", LAMBDACM );
    (void) fprintf( f, "CTGA %.6f\n", CTGA * CM_M2 );
    (void) fprintf( f, "CTDW %.6f\n", CTDW * CM_M );
    (void) fprintf( f, "CTDE %.6f\n", CTDE );

	/* add extensions here */
    (void) fprintf( f, endAscii );
  }


private int ReadAscii( f, line )
  FILE  *f;
  char  *line;
  {
    int  nl = 0;

    inet_CTGA = CTGA;
    inet_CTDE = CTDE;
    inet_CTDW = CTDW;
    inet_lambda = LAMBDACM;

    do
      {
	if( fgetline( line, 200, f ) == NULL )
	    return( -1 );
	nl++;
	if( strcmp( line, endAscii ) == 0 )
	    return( 0 );
	if( strncmp( line, "lambda ", 7 ) == 0 )
	    inet_lambda = atoi( line + 7 );
	if( strncmp( line, "CTGA ", 5 ) == 0 )
	    inet_CTGA = atof( line + 5 ) / CM_M2;
	else if( strncmp( line, "CTDW ", 5 ) == 0 )
	    inet_CTDW = atof( line + 5 ) / CM_M;
	else if( strncmp( line, "CTDE ", 5 ) == 0 )
	    inet_CTDE = atof( line + 5 );

	/* add extensions here */
      }
    while( nl < 30 );

    (void) fprintf( stderr, "inet file seems bad\n" );
    return( -1 );
  }


public void wr_netfile( fname )
  char  *fname;
  {
    tptr  txlist;

    if( (fnet = fopen( fname, "w" )) == NULL )
      {
	(void) fprintf( stderr, "can't open file '%s'\n", fname );
	return;
      }

    (void) fprintf( fnet, "%s\n", netHeader );
    (void) fprintf( fnet, "%d %d\n", GetHashSize(), nnodes + naliases );

    WriteAscii( fnet );
    txlist = wr_nodes();
    wr_txtors( txlist );
    (void) fclose( fnet );
  }


private void PrematureEof()
  {
    (void) fprintf( stderr, "premature eof in inet file\n" );
    exit( 1 );
  }


private void rd_nodes( nname, n_nodes )
  char  *nname;
  int   n_nodes;
  {
    File_Node    node;
    File_Delay   delays;
    File_Thresh  thresh;
    pointertype  tmp;
    int          slen;
    nptr         n;
    nptr         aliases;

    aliases = NULL;
    while( n_nodes-- != 0 )
      {
	if( Fread( &node, Size_File_Node, fnet ) != Size_File_Node )
	    PrematureEof();

	UnpackBytes( node.slen, slen, NB_STRLEN );
	if( Fread( nname, slen, fnet ) != slen )
	    PrematureEof();

	n = RsimGetNode( nname );

	n->nflags = node.flags[0];
	if( n->nflags & ALIAS )
	  {
	    UnpackBytes( node.n.alias, tmp, NB_NDINDEX );
	    n->c.time = tmp;

	    n->n.next = aliases;
	    aliases = n;
	  }
	else
	  {
	    UnpackBytes( node.n.cap, tmp, NB_NCAP );
	    n->ncap = tmp * (1/CAP_SCALE);
	    if( n->ncap < MIN_CAP )
		n->ncap = MIN_CAP;
	  }

	if( n->nflags & USERDELAY )
	  {
	    if( Fread( &delays, Size_File_Delay, fnet ) != Size_File_Delay )
		PrematureEof();
	    UnpackBytes( delays.tphl, n->tphl, NB_TP );
	    UnpackBytes( delays.tplh, n->tplh, NB_TP );
	  }
	if( n->nflags & WATCHED )
	  {
	    n->nflags &= ~WATCHED;
	    if( Fread( &thresh, Size_File_Thresh, fnet ) != Size_File_Thresh )
		PrematureEof();
	    UnpackBytes( thresh.vhigh, tmp, NB_VTHRESH );
	    n->vhigh = tmp * (1/VTH_SCALE);
	    UnpackBytes( thresh.vlow, tmp, NB_VTHRESH );
	    n->vlow = tmp * (1/VTH_SCALE);
	  }
      }

    VDD_node->nflags |= INPUT;		/* these bits get thrashed above */
    GND_node->nflags |= INPUT;

    for( n = aliases; n != NULL; n = n->n.next )
      {
	n->nlink = Index2node( n->c.time );
	n->c.time = 0;
	nnodes--;
	naliases++;
      }
  }


private void rd_txtors()
  {
    File_Trans  trans;
    pointertype ndindx;
    int         width, length;
    tptr        t, *last;

    brd_tlist = NULL;
    last = &brd_tlist;
    while( Fread( &trans, Size_File_Trans, fnet ) == Size_File_Trans )
      {
	NEW_TRANS( t );

	UnpackBytes( trans.gate, ndindx, NB_NDINDEX );
	t->gate = Index2node( ndindx );

	UnpackBytes( trans.src, ndindx, NB_NDINDEX );
	t->source = Index2node( ndindx );

	UnpackBytes( trans.drn, ndindx, NB_NDINDEX );
	t->drain = Index2node( ndindx );

	UnpackBytes( trans.width, width, NB_TSIZE );
	UnpackBytes( trans.length, length, NB_TSIZE );

	t->ttype = trans.ttype[0];

	if( t->ttype & GATELIST )
	  {
	    t->ttype &= ~GATELIST;
	    UnpackBytes( trans.x, t->x.pos, NB_COORD );
	    UnpackBytes( trans.y, t->y.pos, NB_COORD );
	    EnterPos( t, TRUE );
	  }
	else
	    EnterPos( t, FALSE );

	t->r = requiv( (int) t->ttype, width, length );

	ntrans[ BASETYPE( t->ttype ) ] += 1;

	*last = t;
	last = &(t->scache.t);
      }
    *last = NULL;
  }


public int rd_netfile( f, line )
  FILE  *f;
  char  *line;
  {
    int   hash_size, n_nodes;

    if( strncmp( line, netHeader, HEADER_LEN ) != 0 )
	return( FALSE );

    if( fgetline( line, 200, f ) == NULL )
	PrematureEof();

    if( sscanf( line, "%d %d", &hash_size, &n_nodes ) != 2 )
      {
	(void) fprintf( stderr, "bad format for net file\n" );
	exit( 1 );
      }

    if( hash_size != GetHashSize() )
      {
	(void) fprintf( stderr, "Incompatible net file version\n" );
	exit( 1 );
      }

    if( ReadAscii( f, line ) ) PrematureEof();

    fnet = f;
    rd_nodes( line, n_nodes );
    rd_txtors();

    adjust_transistors( brd_tlist );

    return( TRUE );
  }


private void adjust_transistors( tlist )
  tptr  tlist;
  {
    double         adj_g, adj_w, adj_e;
    register tptr  t;

#    define	ABS( X )	( ((X) < 0) ? -(X) : (X) )
#    ifndef CAP_ADJUST_LIMIT
#	define	CAP_ADJUST_LIMIT	0.0000005
#    endif
#    define	G_CAP_ADJUST_LIMIT	(CAP_ADJUST_LIMIT / CM_M2)
#    define	W_CAP_ADJUST_LIMIT	(CAP_ADJUST_LIMIT / CM_M)

    adj_g = CTGA - inet_CTGA;
    adj_w = CTDW - inet_CTDW;
    adj_e = CTDE - inet_CTDE;

    if( LAMBDACM != inet_lambda and LAMBDACM > 0 and inet_lambda > 0 )
      {
	double  adj_size = (double) LAMBDACM / (double) inet_lambda;

	for( t = tlist; t != NULL; t = t->scache.t )
	  {
	    t->r->length *= adj_size;    t->r->width *= adj_size;
	  }
      }

    if( ABS( adj_g ) > G_CAP_ADJUST_LIMIT )
      {
	for( t = tlist; t != NULL; t = t->scache.t )
	    t->gate->ncap += adj_g * t->r->length * t->r->width;
      }

    if( ABS( adj_w ) > W_CAP_ADJUST_LIMIT or ABS( adj_e ) > CAP_ADJUST_LIMIT )
      {
	for( t = tlist; t != NULL; t = t->scache.t )
	  {
	    t->source->ncap += adj_w * t->r->width + adj_e;
	    t->drain->ncap += adj_w * t->r->width + adj_e;
	  }
      }
  }


/*
 * Traverse the transistor list and add the node connection-list.  We have
 * to be careful with ALIASed nodes.  Note that transistors with source/drain
 * connected to VDD and GND nodes are not linked.
 */
public nptr bin_connect_txtors()
  {
    register tptr  t, tnext;
    register nptr  nd_list = NULL;

    for( t = brd_tlist; t != NULL; t = tnext )
      {
	tnext = t->scache.t;
	t->state = ( t->ttype & ALWAYSON ) ? WEAK : UNKNOWN;
	t->tflags = 0;

	if( t->ttype & TCAP )
	  {
	    LINK_TCAP( t );
	  }
	else
	  {
	    if( t->ttype & ALWAYSON )
	      {
		CONNECT( on_trans, t );
	      }
	    else
	      {
		CONNECT( t->gate->ngate, t );
	      }

	    if( not (t->source->nflags & POWER_RAIL) )
	      {
		CONNECT( t->source->nterm, t );
		LINK_TO_LIST( t->source, nd_list, VISITED );
	      }
	    if( not (t->drain->nflags & POWER_RAIL) )
	      {
		CONNECT( t->drain->nterm, t );
		LINK_TO_LIST( t->drain, nd_list, VISITED );
	      }
	  }
      }
    return( nd_list );
  }
