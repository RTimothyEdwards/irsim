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
 *	Routines to write and read history dump files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>   /* for strncmp() */

#include "defs.h"
#include "units.h"
#include "ASSERT.h"
#include "bin_io.h"
#include "globals.h"


private	char	fh_header[] = "#HDUMP#\n";	/* first line of dump file */
private	int	dumph_version = 3;


	/* define the number of bytes used to read/write history */
#define	NB_HEADER	( sizeof( fh_header ) - 1 )
#define	NB_NUMBER	4
#define	NB_NDINDEX	sizeof(pointertype)
#define	NB_TIME		sizeof(Ulong)
#define	NB_PVAL		1
#define	NB_RTIME	2
#define	NB_DELAY	2
#define	NB_EVAL		1
#define	NB_VERSION	2

#define	MAGIC_NUM( DELTA, NNODES ) ( ((DELTA) ^ (NNODES)) & 0xffff )

typedef struct
  {
    char  header[ NB_HEADER ];
    char  hsize[ NB_NUMBER ];
    char  nnodes[ NB_NUMBER ];
    char  cur_delta[ NB_TIME ];
    char  magic[ NB_NUMBER ];
    char  version[ NB_VERSION ];
    char  time0[ NB_TIME ];
  } File_Head;

#define	Size_File_Head	( NB_HEADER + NB_NUMBER + NB_NUMBER + NB_TIME + \
			NB_NUMBER + NB_VERSION + NB_TIME )


typedef struct			/* format of history header, one per node */
  {
    char  node[ NB_NDINDEX ];	/* node for which history follows */
    char  time[ NB_TIME ];	/* initial time (usually 0) */
    char  pval[ NB_PVAL ];	/* initial packed-value */
  } Node_Head;

#define	Size_Node_Head		( NB_NDINDEX + NB_TIME + NB_PVAL )


typedef struct		/* format of each history entry */
  {
    char  time[ NB_TIME ];	/* time of this change */
    char  rtime[ NB_RTIME ];	/* input rise time */
    char  delay[ NB_DELAY ];	/* associated delay */
    char  pval[ NB_PVAL ];	/* packed-value (inp, val, punt) */
    char  ptime[ NB_DELAY ];	/* punt time (only punted events) */
  } File_Hist;

#define	Size_File_Hist		( NB_TIME + NB_RTIME + NB_DELAY + NB_PVAL )
#define	Size_Ptime		( NB_DELAY )
#define	Size_PuntFile_Hist	( Size_File_Hist + Size_Ptime )


	/* marks end of history.  Must be same size as Size_File_Hist! */
typedef struct
  {
    char  mark[ NB_TIME ];		/* marker, same for all nodes */
    char  npend[ NB_RTIME ];		/* # of pending events */
    char  dummy1[ NB_DELAY ];		/* just fill the space */
    char  dummy2[ NB_PVAL ];
  } EndHist;


typedef struct		/* format for pending events */
  {
    char  cause[ NB_NDINDEX ];
    char  time[ NB_TIME ];
    char  delay[ NB_DELAY ];
    char  rtime[ NB_RTIME ];
    char  eval[ NB_EVAL ];
  } File_Pend;

#define	Size_File_Pend	(NB_NDINDEX + NB_TIME + NB_DELAY + NB_RTIME + NB_EVAL)


	/* macros to pack/unpack a value/inp/punt triplet from/to a byte */

#define	PACK_VAL( H )		( (H->inp << 5) | (H->punt << 4) | H->val )
#define	GET_INP( PV )		( (PV) >> 5 & 1 )
#define	GET_VAL( PV )		( (PV) & 0x7 )
#define	IS_PUNT( PV )		( (PV) & 0x10 )


private	EndHist	h_end =
  {
    { '\0', '\0', '\0', '\040' },		/* mark */
    { '\0', '\0' },				/* npend */
    { '\0', '\0' },				/* dummy1 */
    { '\n' }					/* dummy2 */
  };


private int DumpNodeHist( nd, ndindx, fp )
  nptr   nd;
  Ulong ndindx;
  FILE   *fp;
  {
    register hptr  h;
    Node_Head      header;
    File_Hist      hist;

    if( nd->nflags & (POWER_RAIL | ALIAS | MERGED) )
	return( 0 );

    PackBytes( header.node, ndindx, NB_NDINDEX );

    h = &(nd->head);
    PackBytes( header.time, h->time, NB_TIME );
    PackBytes( header.pval, PACK_VAL( h ), NB_PVAL );
    if( Fwrite( &header, Size_Node_Head, fp ) <= 0 )
	goto abort;
    
    for( h = h->next; h != last_hist; h = h->next )
      {
	PackBytes( hist.time, h->time, NB_TIME );
	PackBytes( hist.pval, PACK_VAL( h ), NB_PVAL );
	if( h->punt )
	  {
	    PackBytes( hist.delay, h->t.p.delay, NB_DELAY );
	    PackBytes( hist.rtime, h->t.p.rtime, NB_RTIME );
	    PackBytes( hist.ptime, h->t.p.ptime, NB_DELAY );
	    if( Fwrite( &hist, Size_PuntFile_Hist, fp ) <= 0 )
		goto abort;
	  }
	else
	  {
	    PackBytes( hist.delay, h->t.r.delay, NB_DELAY );
	    PackBytes( hist.rtime, h->t.r.rtime, NB_RTIME );
	    if( Fwrite( &hist, Size_File_Hist, fp ) <= 0 )
		goto abort;
	  }
      }

    if( nd->events != NULL )
      {
	register evptr  ev;
	register int    n;
	File_Pend       pending;

	for( n = 0, ev = nd->events; ev != NULL; ev = ev->nlink, n++ );
	PackBytes( h_end.npend, n, NB_RTIME );
	if( Fwrite( &h_end, Size_File_Hist, fp ) <= 0 )
	    goto abort;
	for( ev = nd->events; ev != NULL; ev = ev->nlink )
	  {
	    ndindx = Node2index( ev->p.cause );
	    PackBytes( pending.cause, ndindx, NB_NDINDEX );
	    PackBytes( pending.time, ev->ntime, NB_TIME );
	    PackBytes( pending.delay, ev->delay, NB_DELAY );
	    PackBytes( pending.rtime, ev->rtime, NB_RTIME );
	    PackBytes( pending.eval, ev->eval, NB_EVAL );
	    if( Fwrite( &pending, Size_File_Pend, fp ) <= 0 )
		goto abort;
	  }
      }
    else
      {
	PackBytes( h_end.npend, 0, NB_RTIME );
	if( Fwrite( &h_end, Size_File_Hist, fp ) <= 0 )
	    goto abort;
      }

    return( 0 );

  abort:
    lprintf( stderr, "can't write to file, history dump aborted\n" );
    return( 1 );
  }


private int WriteHistHeader( fd )
  FILE *fd;
  {
    File_Head  fh;
    long       mag;

    mag = MAGIC_NUM( cur_delta, nnodes );

    bcopy( fh_header, fh.header, NB_HEADER );
    PackBytes( fh.hsize, GetHashSize(), NB_NUMBER );
    PackBytes( fh.nnodes, nnodes, NB_NUMBER );
    PackBytes( fh.cur_delta, cur_delta, NB_TIME );
    PackBytes( fh.magic, mag, NB_NUMBER );
    PackBytes( fh.version, dumph_version, NB_VERSION );
    PackBytes( fh.time0, sim_time0, NB_TIME );
    if( Fwrite( &fh, Size_File_Head, fd ) <= 0 )
	return( -1 );
    return( 0 );
 }


private int ReadHistHead( fd, pNewTime, pTime0 )
  FILE  *fd;
  Ulong  *pNewTime, *pTime0;
  {
    File_Head  fh;
    int        n_nodes, hsize, n_version;
    long       magic;
    Ulong      newTime, time0;

    if( Fread( &fh, Size_File_Head, fd ) != Size_File_Head )
      {
	lprintf( stderr, "ReadHist: can't read file\n" );
	return( -1 );
      }

    UnpackBytes( fh.version, n_version, NB_VERSION );
    UnpackBytes( fh.hsize, hsize, NB_NUMBER );
    UnpackBytes( fh.nnodes, n_nodes, NB_NUMBER );
    UnpackBytes( fh.cur_delta, newTime, NB_TIME );
    UnpackBytes( fh.magic, magic, NB_NUMBER );
    UnpackBytes( fh.time0, time0, NB_TIME );

    if( strncmp( fh_header, fh.header, NB_HEADER ) != 0 )
      {
	lprintf( stderr, "ReadHist: not a history dump file\n" );
	return( -1 );
      }

    if( n_version != dumph_version )
      {
	lprintf( stderr, "ReadHist: Incompatible version: %d\n", n_version );
	return( -1 );
      }

    if( (nnodes != n_nodes and nnodes != 0) or hsize != GetHashSize() or
      magic != MAGIC_NUM( newTime, n_nodes ) )
      {
	lprintf( stderr, "ReadHist: incompatible or bad history dump\n" );
	return( -1 );
      }

    *pNewTime = newTime;
    *pTime0 = time0;
    return( 0 );
  }


public void DumpHist( fname )
  char  *fname;
  {
    FILE  *fp;

    if( (fp = fopen( fname, "w" )) == NULL )
      {
	lprintf( stderr, "can not open file '%s'\n", fname );
	return;
      }
    
    if( WriteHistHeader( fp ) )
      {
	lprintf( stderr, "can't write to file '%s'\n", fname );
	(void) fclose( fp );
	return;
      }

    walk_net_index( DumpNodeHist, fp );

    (void) fclose( fp );
  }


private int rd_hist( fd, pnlist )
  FILE  *fd;
  nptr  *pnlist;
  {
    Node_Head     head;
    File_Hist     hist;
    EndHist       *pe;
    int           inp, val, n, pval, delay, rtime;
    pointertype   ndindx;
    nptr          nd, ndlist;
    Ulong         etime;
    struct Event  ev;

    pe = (EndHist *) &hist;
    ndlist = NULL;
    while( Fread( &head, Size_Node_Head, fd ) == Size_Node_Head )
      {
	UnpackBytes( head.node, ndindx, NB_NDINDEX );
	if( (nd = Index2node( ndindx )) == NULL )
	  {
	    lprintf( stderr, "history read aborted: could not find node\n" );
	    *pnlist = ndlist;
	    return( -1 );
	  }

	if( nd->nflags & (POWER_RAIL | ALIAS) )
	  {
	    lprintf( stderr, "warning: %s should not be in history\n",
	      pnode( nd ) );
	  }

	UnpackBytes( head.time, etime, NB_TIME );
	UnpackBytes( head.pval, pval, NB_PVAL );
	val = GET_VAL( pval );
	inp = GET_INP( pval );
	SetFirstHist( nd, val, inp, etime );

	nd->n.next = ndlist;
	ndlist = nd;

	if( nd->head.next != last_hist )
	    FreeHistList( nd );

	while( TRUE )
	  {
	    if( Fread( &hist, Size_File_Hist, fd ) != Size_File_Hist )
		goto badfile;
	    if( bcmp( pe->mark, h_end.mark, NB_TIME ) == 0 )
		break;
	    if( nd->nflags & (POWER_RAIL | ALIAS) )
		continue;
	    UnpackBytes( hist.time, etime, NB_TIME );
	    UnpackBytes( hist.delay, delay, NB_DELAY );
	    UnpackBytes( hist.rtime, rtime, NB_RTIME );
	    UnpackBytes( hist.pval, pval, NB_PVAL );
	    val = GET_VAL( pval );
	    inp = GET_INP( pval );

	    ASSERT( delay < 60000 and delay >= 0 )
	      {
		lprintf( stderr, "Error: Corrupted history entry:\n" );
		lprintf( stderr, "\t%s time=%.2f delay=%.2f value=%c\n",
		  pnode( nd ), d2ns( etime ), d2ns( delay ), vchars[val] );
	      }
	    if( IS_PUNT( pval ) )
	      {
		if( Fread( hist.ptime, Size_Ptime, fd ) != Size_Ptime )
		    goto badfile;
		ev.eval = val;
		ev.ntime = etime;
		ev.delay = delay;
		ev.rtime = rtime;
		UnpackBytes( hist.ptime, delay, NB_DELAY );
		etime -= delay;
	      	AddPunted( nd, &ev, etime );
	      }
	    else
		AddHist( nd, val, inp, etime, (long) delay, (long) rtime );
	  }

	if( not (nd->nflags & POWER_RAIL) )
	  {
	    nd->npot = nd->curr->val;
	    if( nd->curr->inp )
		nd->nflags |= INPUT;
	  }

	while( nd->events != NULL )	/* get rid of any pending events */
	    free_event( nd->events );

	UnpackBytes( pe->npend, n, NB_RTIME );
	while( n != 0 )
	  {
	    File_Pend  pend;

	    if( Fread( &pend, Size_File_Pend, fd ) != Size_File_Pend )
		goto badfile;
	    UnpackBytes( pend.cause, ndindx, NB_NDINDEX );
	    UnpackBytes( pend.time, etime, NB_TIME );
	    UnpackBytes( pend.delay, delay, NB_DELAY );
	    UnpackBytes( pend.rtime, rtime, NB_RTIME );
	    UnpackBytes( pend.eval, val, NB_EVAL );
	    ASSERT( delay < 60000 and delay >= 0 )
	      {
		lprintf( stdout, "Error: Corrupted history entry:\n" );
		lprintf( stdout, "\t%s time=%.2f delay=%.2f value=%c [pnd]\n",
		  pnode( nd ), d2ns( etime ), d2ns( delay ), vchars[val] );
		n--;
		continue;
	      }

	    cur_node = Index2node( ndindx );
	    cur_delta = etime - delay;			/* fake the delay */
	    enqueue_event( nd, val, (long) delay, (long) rtime );
	    n--;
	  }
      }

    *pnlist = ndlist;
    return( 0 );

  badfile:
    lprintf( stderr, "premature eof on history file\n" );
    *pnlist = ndlist;
    return( -1 );
  }


private void fix_transistors( nd )
  register nptr  nd;
  {
    register lptr  l;

    while( nd != NULL )
      {
	for( l = nd->ngate; l != NULL; l = l->next )
	    l->xtor->state = compute_trans_state( l->xtor );
	nd = nd->n.next;
      }
    for( l = VDD_node->ngate; l != NULL; l = l->next )
	l->xtor->state = compute_trans_state( l->xtor );
    for( l = GND_node->ngate; l != NULL; l = l->next )
	l->xtor->state = compute_trans_state( l->xtor );
  }


public void ReadHist( fname )
  char  *fname;
  {
    FILE       *fd;
    Ulong       newTime, time0;
    nptr       ndlist;

    if( (fd = fopen( fname, "r" )) == NULL )
      {
	lprintf( stderr, "can not open file '%s'\n", fname );
	return;
      }

    if( ReadHistHead( fd, &newTime, &time0 ) )
      {
	(void) fclose( fd );
	return;
      }

    ClearInputs();

    if( rd_hist( fd, &ndlist ) )
      {
	nptr  n;

	for( n = ndlist; n != NULL; n = n->n.next )
	  {
	    FreeHistList( n );			/* undo any work done */
	    while( n->events != NULL )
		free_event( n->events );
	  }
      }
    else
      {
	sim_time0 = time0;
	cur_delta = newTime;
	if( cur_delta > 0 )
	    NoInit();
	if( VDD_node != NULL )
	    fix_transistors( ndlist );
      }

    (void) fclose( fd );
  }
