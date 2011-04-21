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
 * Incremental net changes are provides by the following commands:
 * 
 * alias node		'==' <node> <node_name>
 * alias node_ref	'=' <node> <node_ref>
 * create new node	'n' <node> <cap> <node_name>
 * eliminate node	'e' <node>
 * eliminate node	'E' <node_name>
 * change node cap	'C' <node> <delta_cap>
 * set node cap		'C' <node> '='<cap>
 * rename node		'r' <node> <node_name>
 * hierarchical rename	'h' <node> <node_name> [ <old_name> ]
 * connect nodes	'c' <node> <node>
 * break node		'b' <node> <new_node> <cap> <node_name>
 * 
 * add new transistor	'a' <type> <x> <y> <length> <width> <gate> <src> <drn>
 * delete transistor	'd' <x> <y>
 * exchange src/drn	'x' <x> <y>
 * change trans. size	's' <x> <y> <length> <width>
 * change position	'p' <x> <y> <new_x> <new_y>
 * move trans. terminal	'm' <x> <y> <new_gate> <new_src> <new_drn>
 * move trans. to node	'M' <x> <y> <new_node> ['g']['s']['d']
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>    /* **mdg** */
#include <time.h>

#include "defs.h"
#include "net.h"
#include "globals.h"
#include "net_macros.h"

/******** ALIAS MANAGMENT ******/

#define	ALIAS_TBL_SIZE		512
#define	LOG2_TBL_SIZE		9
#define	AliasTblIndex( N )	( (N) & (ALIAS_TBL_SIZE - 1) )
#define	AliasTblBucket( N )	( (N) >> LOG2_TBL_SIZE )
#define	SIZEOF( A )		( (int) sizeof( A ) )


typedef struct
  {
    nptr    **bucket;
    int     size;
  } Aliases;


private	Aliases	aliases;


private void InitAliasTbl()
  {
    int  i;

    aliases.size = 20;			/* default initial size */

    aliases.bucket = (nptr **) Valloc( SIZEOF( nptr * ) * aliases.size, 1 );
    for( i = 0; i < aliases.size; i++ )
	aliases.bucket[ i ] = NULL;
  }


void FreeAliasTbl()
  {
    int  i;

    if( aliases.bucket != NULL )
      {
	for( i = 0; i < aliases.size; i++ )
	  {
	    if( aliases.bucket[ i ] != NULL )
		Vfree( aliases.bucket[ i ] );
	  }
	Vfree( aliases.bucket );
      }
    aliases.bucket = NULL;
    aliases.size = 0;
  }


private void IncreaseAliasTbl( i )
  int   i;
  {
    Aliases  old;

    old = aliases;

    do { aliases.size *= 2; } while( i >= aliases.size );

    aliases.bucket = (nptr **) Valloc( SIZEOF( nptr * ) * aliases.size, 1 );
    for( i = 0; i < old.size; i++ ) aliases.bucket[ i ] = old.bucket[ i ];
    while( i < aliases.size ) aliases.bucket[ i++ ] = NULL;
    Vfree( old.bucket );
  }


private void EnterAlias( n, nd )
  register int  n;
  nptr          nd;
  {
    register int   i;
    register nptr  *tbl;

    i = AliasTblBucket( n );
    if( i >= aliases.size )
	IncreaseAliasTbl( i );

    if( (tbl = aliases.bucket[ i ]) == NULL )
      {
	register nptr  *t, *t_end;

	tbl = (nptr *) Valloc( SIZEOF( nptr ) * ALIAS_TBL_SIZE, 1 );
	aliases.bucket[ i ] = tbl;
	for( t = tbl, t_end = t + ALIAS_TBL_SIZE; t < t_end; *t++ = NULL );
      }

    tbl[ AliasTblIndex( n ) ] = nd;
  }


private nptr LookupAlias( n )
  register int  n;
  {
    register int   i;
    register nptr  *t, nd;

    i = AliasTblBucket( n );
    if( i >= aliases.size or (t = aliases.bucket[ i ]) == NULL )
	return( NULL );
    nd = t[ AliasTblIndex( n ) ];
    while( nd != NULL and (nd->nflags & ALIAS) )
	nd = nd->nlink;
    return( nd );
  }

/***** END OF ALIAS MANAGMENT ******/


#define LSIZE		2000		/* size for input line buffer */
#define	MAXARGS		20		/* max. # of arguments per line */

#define	streq( A, B )	( strcmp( (A), (B) ) == 0 )


    /* a capacitance change > CAP_THRESH % will consider the node changed */
#define	CAP_THRESH	(0.05)		/* default is now 5% */


private	int	lineno;			/* current input file line number */
private	char	*nu_fname;		/* current input filename */
private	int	num_errors;		/* number of errors in input file */
private	FILE	*nu_logf;		/* file for recording net changes */
private	int	context;		/* 0 for old transistor context */
private	int	num_deleted;		/* number of nodes deleted */
private	int	num_cap_set;		/* number of nodes with cap set */
private	nptr	ch_nlist;		/* list of nodes that were changed */
private	tptr	ch_tran;		/* list of changed transistors */
private	nptr	new_GND, new_VDD;
private	int	chg_VDD, chg_GND;

private	void	nu_error(char *, ...);	/* varargs must declare parameters */

	/* value stored in tflags, indicating how transistor changed */
#define	T_CH_POS	0x1
#define	T_CHK_TERM	0x2
#define	T_MOV_GATE	0x4
#define	T_MOV_SRC	0x8
#define	T_MOV_DRN	0x10
#define	T_ADDED		( T_CH_POS | T_MOV_GATE | T_MOV_SRC | T_MOV_DRN )

#define	N_CAP_SET	(STIM)


#define	TRAN_CHANGE( T, WHAT )			\
  {						\
    if( (T)->tflags == 0 )			\
      {						\
	(T)->dcache.t = ch_tran;		\
	(T)->scache.t = ch_tran->scache.t;	\
	ch_tran->scache.t->dcache.t = (T);	\
	ch_tran->scache.t = (T);		\
      }						\
    (T)->tflags |= (WHAT);			\
  }


#define	CAP_CHANGE( ND, LIST, GCAP )		\
  {						\
    if( not ((ND)->nflags & VISITED) )		\
      {						\
	(ND)->n.next = (LIST);			\
	LIST = (ND);				\
	(ND)->nflags |= VISITED;		\
	(ND)->c.cap = (ND)->ncap;		\
      }						\
    (ND)->ncap += (GCAP);			\
  }						\


#define	NODE_CHANGE( ND, LIST )			\
  {						\
    if( not ((ND)->nflags & VISITED) )		\
      {						\
	(ND)->n.next = (LIST);			\
	LIST = (ND);				\
      }						\
    (ND)->nflags |= (VISITED | CHANGED);	\
  }						\


#define Error( A )	\
  {			\
    nu_error A ;	\
    return;		\
  }


private	char	bad_arg_msg[] = "Wrong # of arguments for '%s' expect %s\n";
private	char	bad_alias_msg[] = "Illegal alias number (%d)\n";
private	char	no_node_msg[] = "can not find node %s\n";
private	char	no_alias_msg[] = "Non-existent node alias (%d)\n";
private	char	no_trans_msg[] = "can not find transistor @ %d,%d\n";


#define	FindNode( ND, ARG ) 	\
  {				\
    int  Nnum = atoi( ARG );	\
    if( Nnum < 0 )		\
	Error( (bad_alias_msg, Nnum) ); \
    if( ((ND) = LookupAlias( Nnum )) == NULL ) \
	Error( (no_alias_msg, Nnum) ); \
  }


#define	ff2pf( C )	( (C) * 0.001 )		/* ff to pf */


private void alias_node( ac, av )
  int   ac;
  char  *av[];
  {
    int   nnum;
    nptr  nd;

    if( ac != 3 )	Error( (bad_arg_msg, av[0], "3") );

    nnum = atoi( av[1] );
    if( nnum < 0 )	Error( (bad_alias_msg, nnum) );

    nd = (av[0][1] != '=') ? FindNode_TxtorPos( av[2] ) : RsimGetNode( av[2] );
    if( nd == NULL )
	Error( (no_node_msg, av[2]) );

    EnterAlias( nnum, nd );
  }


private void new_node( ac, av )
  int   ac;
  char  *av[];
  {
    nptr    nd;
    int     n;
    double  cap;
    
    if( ac != 4 )	Error( (bad_arg_msg, av[0], "4") );

    cap = ff2pf( atoi( av[2] ) );
    n = atoi( av[1] );
    if( n < 0 )		Error( (bad_alias_msg, n) );

    nd = GetNewNode( av[3] );
    nd->ncap = cap;
    NODE_CHANGE( nd, ch_nlist );

    EnterAlias( n, nd );
  }


private void eliminate_node( ac, av )
  int   ac;
  char  *av[];
  {
    nptr  nd;

    if( ac != 2 )		Error( (bad_arg_msg, av[0], "2") );

    if( av[0][0] == 'E' )
      {
	nd = find( av[1] );
	if( nd == NULL )	Error( (no_node_msg, av[1]) );
	while( nd->nflags & ALIAS )
	    nd = nd->nlink;
      }
    else
      {
	FindNode( nd, av[1] );
      }

    if( nd->nflags & POWER_RAIL )		/* never delete Vdd or Gnd */
	return;
    n_delete( nd );
    NODE_CHANGE( nd, ch_nlist );
    nd->nflags |= DELETED;
    nnodes--;
    num_deleted += 1;
  }


private void change_cap( ac, av )
  int   ac;
  char  *av[];
  {
    nptr    nd;
    double  cap;

    if( ac != 3 )	Error( (bad_arg_msg, av[0], "3") );

    FindNode( nd, av[1] );

    if( av[2][0] == '=' )
      {
	cap = ff2pf( atoi( &av[2][1] ) );
	CAP_CHANGE( nd, ch_nlist, 0 );
	nd->ncap = cap;
	nd->nflags |= N_CAP_SET;
	num_cap_set ++;
      }
    else
      {
	cap = ff2pf( atoi( av[2] ) );
	CAP_CHANGE( nd, ch_nlist, cap );
      }
  }


private void ChangeNodeName( nd, str )
  nptr  nd;
  char  *str;
  {
    int  len;

    if( nd->nflags & POWER_RAIL )
      {
	if( str_eql( str, nd->nname ) == 0 )
	    return;
	else if( nd == GND_node )
	    chg_GND = TRUE;
	else
	    chg_VDD = TRUE;
      }

    len = strlen( str ) + 1;

    n_delete( nd );

    nd->nname = Valloc( len, 1 );
    bcopy( str, nd->nname, len );

    CAP_CHANGE( nd, ch_nlist, 0 );

    if( str_eql( str, "Vdd" ) )
	new_VDD = nd;
    else if( str_eql( str, "Gnd" ) )
	new_GND = nd;
  }


private void rename_node( ac, av )
  int   ac;
  char  *av[];
  {
    nptr  nd;

    if( ac != 3 )	Error( (bad_arg_msg, av[0], "3") );

    FindNode( nd, av[1] );

    if( not streq( nd->nname, av[2] ) )
	ChangeNodeName( nd, av[2] );
  }


/*
 * Return TRUE if 'name1' is "better" than 'name2', FALSE otherwise.
 */
private int BestNodeName( name1, name2 )
  char *name1, *name2;
  {
    register int nslashes1, nslashes2;
    register char *np1, *np2;

    for( nslashes1 = 0, np1 = name1; *np1; )
      {
	if( *np1++ == '/' )  nslashes1++;
     }

    for( nslashes2 = 0, np2 = name2; *np2; )
      {
	if( *np2++ == '/' )  nslashes2++;
      }

    --np1;
    --np2;

	/* chose label over generated name */
    if( *np1 != '#' && *np2 == '#' )	return( TRUE );
    if( *np1 == '#' && *np2 != '#' )	return( FALSE );

    /* either both are labels or generated names */
    /* check pathname components */
    if( nslashes1 < nslashes2 )		return( TRUE );
    if( nslashes1 > nslashes2 )		return( FALSE );

    /* same # of pathname components; check length */
    if( np1 - name1 < np2 - name2 )	return( TRUE );
    if( np1 - name1 > np2 - name2 )	return( FALSE );

    /* same # of pathname components; same length; use lex ordering */
    return( (strcmp( name1, name2 ) > 0) ? TRUE : FALSE );
  }



private void hier_rename_node( ac, av )
  int   ac;
  char  *av[];
  {
    nptr  nd;

    if( ac < 3 or ac > 4 )
	Error( (bad_arg_msg, av[0], "3-4") );

    FindNode( nd, av[1] );

    if( ac == 3 )
      {
	if( BestNodeName( av[2], nd->nname ) )
	    ChangeNodeName( nd, av[2] );
      }
    else if( streq( nd->nname, av[ 3 ] ) )
	ChangeNodeName( nd, av[2] );
  }


private void connect_nodes( ac, av )
  int   ac;
  char  *av[];
  {
    int            t_terms;
    register nptr  nd1, nd2;
    register lptr  l;

    if( ac != 3 )	Error( (bad_arg_msg, av[0], "3") );

    FindNode( nd1, av[1] );
    FindNode( nd2, av[2] );

    if( nd1 == nd2 )
	return;

    if( (nd1->nflags & nd2->nflags) & POWER_RAIL )
	Error( ("Warning: attempt to short Vdd & Gnd\n") );

    if( (nd2->nflags & POWER_RAIL) or BestNodeName( nd2->nname, nd1->nname ) )
	SWAP_NODES( nd1, nd2 );

    if( nd2->ngate != NULL )
      {
	register lptr  last = NULL;

	for( l = nd2->ngate; l != NULL; l = l->next )
	  {
	    l->xtor->gate = nd1;
	    last = l;
	  }
	last->next = nd1->ngate;
	nd1->ngate = nd2->ngate;
      }

    if( nd2->nterm != NULL )
      {
	register lptr  last = NULL;
	register tptr  t;

	for( l = nd1->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    TRAN_CHANGE( t, T_CHK_TERM );
	  }
	for( l = nd2->nterm; l != NULL; l = l->next )
	  {
	    t = l->xtor;
	    if( t->source == nd2 ) t->source = nd1;
	    if( t->drain == nd2 ) t->drain = nd1;
	    TRAN_CHANGE( t, T_CHK_TERM );
	    last = l;
	  }

	if( nd1->nflags & POWER_RAIL )
	    last->next = freeLinks,	freeLinks = nd2->nterm;
	else
	    last->next = nd1->nterm,	nd1->nterm = nd2->nterm;

	t_terms = TRUE;
      }
    else
	t_terms = FALSE;

    for( l = on_trans; l != NULL; l = l->next )
      {
	if( l->xtor->gate == nd2 ) l->xtor->gate = nd1;
      }

    if( tcap->scache.t != tcap )
      {
	register tptr  t;

	for( t = tcap->scache.t; t != tcap; t = t->scache.t )
	  {
	    if( t->gate == nd2 )	t->gate = nd1;
	    if( t->source == nd2 )	t->source = nd1;
	    if( t->drain == nd2 )	t->drain = nd1;
	  }
      }

    n_delete( nd2 );

    nd2->ngate = nd2->nterm = NULL;
    nd2->nlink = nd1;
    nd2->nflags = (ALIAS | DELETED);
    num_deleted += 1;
    nnodes--;

    NODE_CHANGE( nd2, ch_nlist );

    CAP_CHANGE( nd1, ch_nlist, nd2->ncap );
    nd2->ncap = 0.0;
    if( t_terms )
	nd1->nflags |= CHANGED;
  }


private void break_node( ac, av )
  int   ac;
  char  *av[];
  {
    int     nnum;
    double  cap;
    nptr    nd, new_nd;

    if( ac != 5 )	Error( (bad_arg_msg, av[0], "5") );

    FindNode( nd, av[1] );
    CAP_CHANGE( nd, ch_nlist, 0.0 );

    nnum = atoi( av[2] );
    if( nnum < 0 )	Error( (bad_alias_msg, nnum) );

    cap = ff2pf( atoi( av[3] ) );
    new_nd = GetNewNode( av[4] );
    new_nd->ncap = cap;
    EnterAlias( nnum, new_nd );
    NODE_CHANGE( new_nd, ch_nlist );
  }


private void add_new_trans( ac, av )
  int   ac;
  char  *av[];
  {
    struct Trans  tran;
    tptr          t;
    int           length, width;
    double        cap;

    if( ac != 9 )	Error( (bad_arg_msg, av[0], "9") );

    switch( av[1][0] )
      {
	case 'n' :	tran.ttype = NCHAN;	break;
	case 'p' :	tran.ttype = PCHAN;	break;
	case 'd' :	tran.ttype = DEP;	break;
	default :
	    nu_error( "unknown transistor type (%s)\n", av[1] );
	    return;
      }

    tran.x.pos = atoi(av[2]);
    tran.y.pos = atoi(av[3]);
    length = (int)(atof(av[4]) * LAMBDACM);
    width = (int)(atof(av[5]) * LAMBDACM);

    FindNode( tran.gate, av[6] );
    FindNode( tran.source, av[7] );
    FindNode( tran.drain, av[8] );

    tran.tflags = tran.n_par = 0;
    tran.state = (tran.ttype & ALWAYSON) ? WEAK : UNKNOWN;

    ntrans[ tran.ttype ]++;

    NEW_TRANS( t );
    *t = tran;
    t->r = requiv( (int) tran.ttype, width, length );

    t->tlink = t;

    cap = length * width * CTGA;
    CAP_CHANGE( t->gate, ch_nlist, cap );

    if( (config_flags & TDIFFCAP) and (cap = CTDW * width + CTDE) != 0.0 )
      {
	CAP_CHANGE( t->source, ch_nlist, cap );
	CAP_CHANGE( t->drain, ch_nlist, cap );
      }
    if( t->source != t->drain )
      {
	NODE_CHANGE( t->source, ch_nlist );
	NODE_CHANGE( t->drain, ch_nlist );
      }

    TRAN_CHANGE( t, T_ADDED );
  }


#define	FindTxtor( T, X_STR, Y_STR, X, Y )	\
  {						\
    X = atoi( X_STR );				\
    Y = atoi( Y_STR );				\
    if( ((T) = FindTxtorPos( X, Y )) == NULL )	\
      {						\
	nu_error( no_trans_msg, X, Y ); 	\
	return;					\
      }						\
  }


private void delete_trans( ac, av )
  int   ac;
  char  *av[];
  {
    tptr    t;
    long    x, y;
    double  cap, gcap;

    if( ac != 3 )	Error( (bad_arg_msg, av[0], "3") );

    FindTxtor( t, av[1], av[2], x, y );

    DeleteTxtorPos( t );

    gcap = -(t->r->width * t->r->length * CTGA);
    cap = (config_flags & TDIFFCAP) ? -(CTDW * t->r->width + CTDE) : 0.0;

    if( t->ttype & TCAP )
      {
	if( cap != 0.0 )
	  {
	    CAP_CHANGE( t->source, ch_nlist, cap );
	    CAP_CHANGE( t->drain, ch_nlist, cap );
	  }
	UNLINK_TCAP( t );
      }
    else
      {
	if( t->ttype & STACKED ) DestroyStack( t->dcache.t );
	if( t->ttype & ORLIST ) UnParallelTrans( t );

	if( t->ttype & ALWAYSON )
	  {
	    DISCONNECT( on_trans, t );
	  }
	else
	  {
	    DISCONNECT( t->gate->ngate, t );
	  }
	DISCONNECT( t->source->nterm, t );
	DISCONNECT( t->drain->nterm, t );

	NODE_CHANGE( t->source, ch_nlist );
	NODE_CHANGE( t->drain, ch_nlist );
	if( cap != 0.0 )
	  {
	    t->source->ncap += cap;
	    t->drain->ncap += cap;
	  }
	if( t->tflags != 0 )	/* it is in changed list -> remove it */
	  {
	    t->dcache.t->scache.t = t->scache.t;
	    t->scache.t->dcache.t = t->dcache.t;
	  }
      }
    CAP_CHANGE( t->gate, ch_nlist, gcap );
    ntrans[ BASETYPE( t->ttype ) ] -= 1;
    FREE_TRANS( t );
  }


private void exchange_terms( ac, av )
  int   ac;
  char  *av[];
  {
    tptr  t;
    long  x, y;

    if( ac != 3 )	Error( (bad_arg_msg, av[0], "3") );

    FindTxtor( t, av[1], av[2], x, y );

    SWAP_NODES( t->source, t->drain );
  }


private void change_tsize( ac, av )
  int   ac;
  char  *av[];
  {
    tptr     t;
    long     x, y;
    int	     length, width;
    double   cap;
    Resists  *old_r;

    if( ac != 5 )	Error( (bad_arg_msg, av[0], "5") );

    FindTxtor( t, av[1], av[2], x, y );

    length = (int)(atof( av[3] ) * LAMBDACM);
    width = (int)(atof( av[4] ) * LAMBDACM);

    cap = (width * length - t->r->length * t->r->width) * CTGA;
    CAP_CHANGE( t->gate, ch_nlist, cap );

    if( config_flags & TDIFFCAP )
      {
	cap = CTDW * (width - t->r->width) + CTDE;
	CAP_CHANGE( t->source, ch_nlist, cap );
	CAP_CHANGE( t->drain, ch_nlist, cap );
      }

    old_r = t->r;
    t->r = requiv( (int) t->ttype, width, length );

    if( old_r->dynlow != t->r->dynlow or old_r->dynhigh != t->r->dynhigh )
      {
	NODE_CHANGE( t->source, ch_nlist );
	NODE_CHANGE( t->drain, ch_nlist );
      }
  }


private void change_tposition( ac, av )
  int   ac;
  char  *av[];
  {
    tptr  t;
    long  x, y;

    if( ac != 5 )	Error( (bad_arg_msg, av[0], "5") );

    FindTxtor( t, av[1], av[2], x, y );

    DeleteTxtorPos( t );

    t->x.pos = atoi( av[1] );
    t->y.pos = atoi( av[2] );

    TRAN_CHANGE( t, T_CH_POS );
  }


private void move_trans_terms( ac, av )
  int   ac;
  char  *av[];
  {
    long    x, y, was_tcap = FALSE;
    tptr    t;
    nptr    gate, src, drn;
    double  cap;

#    define IsDot( STR )	((STR)[0] == '.' and (STR)[1] == '\0' )

    if( ac != 6 )	Error( (bad_arg_msg, av[0], "6") );

    FindTxtor( t, av[1], av[2], x, y );
    
    if( IsDot( av[3] ) )    gate = t->gate;	else FindNode( gate, av[3] );
    if( IsDot( av[4] ) )    src = t->source;	else FindNode( src, av[4] );
    if( IsDot( av[5] ) )    drn = t->drain;	else FindNode( drn, av[5] );

    if( gate == t->gate and src == t->source and drn == t->drain )
	return;

    if( t->ttype & STACKED )	DestroyStack( t->dcache.t );
    if( t->ttype & ORLIST )	UnParallelTrans( t );
    if( t->ttype & TCAP )
      {
	UNLINK_TCAP( t );
	was_tcap = TRUE;
      }

    if( gate != t->gate )
      {
	if( gate->nflags & MERGED )
	  {
	    DestroyStack( gate->t.tran );
	    NODE_CHANGE( gate, ch_nlist );
	  }
	cap = (t->r->width * t->r->length) * CTGA;
	CAP_CHANGE( gate, ch_nlist, cap );
	CAP_CHANGE( t->gate, ch_nlist, -cap );

	if( not (t->ttype & ALWAYSON) )
	  {
	    DISCONNECT( t->gate->ngate, t );
	    TRAN_CHANGE( t, T_MOV_GATE );
	  }
	t->gate = gate;
      }

    cap = (config_flags & TDIFFCAP) ? t->r->width * CTDW + CTDE : 0.0;

    if( t->source != src )
      {
	if( was_tcap )
	  {
	    CAP_CHANGE( t->source, ch_nlist, -cap );
	  }
	else
	  {
	    DISCONNECT( t->source->nterm, t );
	    NODE_CHANGE( t->source, ch_nlist );
	    t->source->ncap -= cap;
	  }
	src->ncap += cap;
	t->source = src;
	TRAN_CHANGE( t, T_MOV_SRC );
      }

    if( t->drain != drn )
      {
	if( was_tcap )
	  {
	    CAP_CHANGE( t->drain, ch_nlist, -cap );
	  }
	else
	  {
	    DISCONNECT( t->drain->nterm, t );
	    NODE_CHANGE( t->drain, ch_nlist );
	    t->drain->ncap -= cap;
	  }
	drn->ncap += cap;
	t->drain = drn;
	TRAN_CHANGE( t, T_MOV_DRN );
      }

    NODE_CHANGE( src, ch_nlist );
    NODE_CHANGE( drn, ch_nlist );
  }


private void SwitchContext()
  {
    register tptr  t;

    if( context )
	return;

    context = 1;

    for( t = ch_tran->scache.t; t != ch_tran; t = t->scache.t )
      {
	if( t->tflags & T_CH_POS )
	  {
	    EnterPos( t, TRUE );
	    t->tflags &= ~T_CH_POS;
	  }
      }
  }


private void move_trans_to_node( ac, av )
  int   ac;
  char  *av[];
  {
    long    x, y, was_tcap = FALSE;
    tptr    t;
    nptr    nd, gate, src, drn;
    char    *s;
    double  cap;

    SwitchContext();

    if( ac != 5 )	Error( (bad_arg_msg, av[0], "5") );

    FindTxtor( t, av[1], av[2], x, y );

    FindNode( nd, av[3] );
    
    gate = t->gate;
    src = t->source;
    drn = t->drain;
    for( s = av[4]; *s != '\0'; s++ )
      {
	if( *s == 'g' )		gate = nd;
	else if( *s == 's' )	src = nd;
	else if( *s == 'd' )	drn = nd;
	else Error( ("Unknown terminal '%c'\n", *s) );
      }

    if( gate == t->gate and src == t->source and drn == t->drain )
	return;

    if( t->ttype & STACKED )	DestroyStack( t->dcache.t );
    if( t->ttype & ORLIST )	UnParallelTrans( t );
    if( t->ttype & TCAP )
      {
	UNLINK_TCAP( t );
	was_tcap = TRUE;
      }

    if( gate != t->gate )
      {
	if( t->ttype & ALWAYSON  )
	  {
	    DISCONNECT( on_trans, t );
	  }
	else
	  {
	    DISCONNECT( t->gate->ngate, t );
	  }
	t->gate = gate;
	TRAN_CHANGE( t, T_MOV_GATE );
	cap = (t->r->width * t->r->length) * CTGA;
	gate->ncap += cap;
      }

    cap = (config_flags & TDIFFCAP) ? t->r->width * CTDW + CTDE : 0.0;

    if( t->source != src )
      {
	if( not was_tcap )
	  {
	    DISCONNECT( t->source->nterm, t );
	  }
	t->source = src;
	src->ncap += cap;
	TRAN_CHANGE( t, T_MOV_SRC );
      }

    if( t->drain != drn )
      {
	if( not was_tcap )
	  {
	    DISCONNECT( t->drain->nterm, t );
	  }
	t->drain = drn;
	drn->ncap += cap;
	TRAN_CHANGE( t, T_MOV_DRN );
      }
  }


private void change_thresh( ac, av )
  int   ac;
  char  *av[];
  {
    nptr   nd;
    float  vlow, vhigh;

    if( ac != 4 )	Error( (bad_arg_msg, av[0], "4") );
    nd = find( av[1] );
    while( nd->nflags & ALIAS ) nd = nd->nlink;
    if( nd == NULL )	Error( (no_node_msg, av[1]) );
    vlow = atof( av[2] );
    vhigh = atof( av[3] );
    if( nd->vlow != vlow or nd->vhigh != vhigh )
      {
	nd->vlow = vlow; nd->vhigh = vhigh;
	NODE_CHANGE( nd, ch_nlist );
      }
  }


private void ndelay( ac, av )
  int   ac;
  char  *av[];
  {
    nptr   nd;
    long   tplh, tphl;

    if( ac != 4 )	Error( (bad_arg_msg, av[0], "4") );
    nd = find( av[1] );
    while( nd->nflags & ALIAS ) nd = nd->nlink;
    if( nd == NULL )	Error( (no_node_msg, av[1]) );
    tplh = ns2d( atof( av[2] ) );
    tphl = ns2d( atof( av[3] ) );

    if( not (nd->nflags & USERDELAY) or nd->tplh != tplh or nd->tphl != tphl )
      {
	nd->tplh = tplh; nd->tphl = tphl;
	NODE_CHANGE( nd, ch_nlist );
      }
  }


/*
 * parse input line into tokens, filling up carg and setting targc
 */
private int parse_line( line, carg )
  register char  *line;
  register char  **carg;
  {
    register char  ch;
    register int   targc;

    targc = 0;
    while( ch = *line++ )
      {
	if( ch <= ' ' )
	    continue;
	targc++;
	*carg++ = line - 1;
	while( *line > ' ' )
	    line++;
	if( *line )
	    *line++ = '\0';
      }
    *carg = 0;
    return( targc );
  }


private void input_changes( fin )
  FILE  *fin;
  {
    char  line[ LSIZE ];
    char  *targv[ MAXARGS ];
    int   targc;

    VDD_node->nflags |= VISITED;	/* never add these to 'change' list */
    GND_node->nflags |= VISITED;
    new_GND = new_VDD = NULL;
    chg_GND = chg_VDD = 0;

    InitAliasTbl();

    while( fgetline( line, LSIZE, fin ) != NULL )
      {
	lineno++;
	if( nu_logf )
	    (void) fputs( line, nu_logf );
	targc = parse_line( line, targv );
	if( targc == 0 )
	    continue;

	switch( targv[0][0] )
	  {
	    case '=' :	alias_node( targc, targv );		break;
	    case 'n' :	new_node( targc, targv );		break;
	    case 'e' :	eliminate_node( targc, targv );		break;
	    case 'E' :	eliminate_node( targc, targv );		break;
	    case 'C' :	change_cap( targc, targv );		break;
	    case 'r' :	rename_node( targc, targv );		break;
	    case 'h' :	hier_rename_node( targc, targv );	break;
	    case 'c' :	connect_nodes( targc, targv );		break;
	    case 'b' :	break_node( targc, targv );		break;
	    case 'a' :	add_new_trans( targc, targv );		break;
	    case 'd' :	delete_trans( targc, targv );		break;
	    case 'x' :	exchange_terms( targc, targv );		break;
	    case 's' :	change_tsize( targc, targv );		break;
	    case 'p' :	change_tposition( targc, targv );	break;
	    case 'm' :	move_trans_terms( targc, targv );	break;
	    case 'M' :	move_trans_to_node( targc, targv );	break;
	    case 't' :	change_thresh( targc, targv );		break;
	    case 'D' :	ndelay( targc, targv );			break;
	    case '|' :						break;
	    default :
		nu_error( "Unrecognized command (%s)\n", targv[0] );
	  }
      }

    (void) fclose( fin );
    if( nu_logf )
	(void) fclose( nu_logf );

    FreeAliasTbl();

    VDD_node->nflags &= ~(VISITED | CHANGED);	/* restore flag */
    GND_node->nflags &= ~(VISITED | CHANGED);
  }


/*
 * Mark any alias that points to a deleted node as deleted.
 */
private int mark_del_alias( n )
  register nptr  n;
  {
    register nptr  nd;
    register int   has_del = FALSE;

    if( n->nflags & DELETED )
	return( 0 );

    for( nd = n; nd->nflags & ALIAS; nd = nd->nlink )
	if( nd->nflags & DELETED )
	    has_del = TRUE;

    if( has_del )		/* deleted node(s) in alias list, resolve */
	n->nlink = nd;

    if( nd->nflags & DELETED )	/* points to a deleted node, delete as well */
      {
	n->nflags |= DELETED;
	n->nflags &= ~ALIAS;
	NODE_CHANGE( n, ch_nlist );
	naliases--;
      }
    return( 0 );
  }


private nptr rm_nodes()
  {
    register nptr  *nlast, node;
    nptr           nlist;

    if( num_deleted == 0 )
	return( ch_nlist );

    if( naliases > 0 )
	walk_net( mark_del_alias, (char *) 0 );

    rm_del_from_lists();

    nlist = NULL;
    nlast = &nlist;

    for( node = ch_nlist; node != NULL; node = node->n.next )
      {
	if( not (node->nflags & DELETED) )
	  {
	    *nlast = node;
	    nlast = &node->n.next;
	    continue;
	  }

	while( node->events )		/* remove all pending events */
	    free_event( node->events );

	if( IsInList( node->nflags ) )
	    idelete( node, listTbl[INPUT_NUM( node->nflags )] );

	FreeHistList( node );

	if( node->hnext != node )
	    n_delete( node );

	node->nlink = freeNodes;	/* return it to free pool */
	freeNodes = node;
      }
    *nlast = NULL;

    return( nlist );
  }


private void conn_ch_trans( tlist )
  tptr  tlist;
  {
    register tptr  t;
    tptr           tnext;

    for( t = tlist->scache.t; t != tlist; t = tnext )
      {
	tnext = t->scache.t;

	if( t->tflags & T_CH_POS )
	    EnterPos( t, TRUE );

	if( t->tflags & (T_MOV_GATE | T_MOV_SRC | T_MOV_DRN) )
	  {
	    nptr  src = t->source, drn = t->drain;

	    if( src == drn or (src->nflags & drn->nflags & POWER_RAIL) )
	      {
		if( not (t->tflags & T_MOV_SRC) )
		    DISCONNECT( src->nterm, t );
		if( not (t->tflags & T_MOV_DRN) )
		    DISCONNECT( drn->nterm, t );
		if( t->ttype & ALWAYSON )
		    DISCONNECT( on_trans, t )
		else if( t->tflags & T_MOV_GATE )
		    DISCONNECT( t->gate->ngate, t );

		t->ttype |= TCAP;
		LINK_TCAP( t );
	      }
	    else
	      {
		if( t->tflags & T_MOV_GATE )
		  {
		    if( t->ttype & ALWAYSON )
			CONNECT( on_trans, t )
		    else
			CONNECT( t->gate->ngate, t );
		  }
		if( (t->tflags & T_MOV_SRC) and not (src->nflags & POWER_RAIL) )
		    CONNECT( src->nterm, t );
		if( (t->tflags & T_MOV_DRN) and not (drn->nflags & POWER_RAIL) )
		    CONNECT( drn->nterm, t );
	      }
	  }
	else if( t->tflags & T_CHK_TERM )
	  {
	    nptr  src = t->source, drn = t->drain;

	    if( src == drn or (src->nflags & drn->nflags & POWER_RAIL) )
	      {
		DISCONNECT( src->nterm, t );
		DISCONNECT( drn->nterm, t );
		if( t->ttype & ALWAYSON )
		    DISCONNECT( on_trans, t )
		else
		    DISCONNECT( t->gate->ngate, t );

		t->ttype |= TCAP;
		LINK_TCAP( t );
	      }
	  }
	t->tflags = 0;
      }
  }


private void chk_power()
  {
    if( new_GND == NULL and chg_GND == 0 )
	;
    else
      {
	fprintf( stderr, "Error: changed Gnd, cannot deal with this yet!\n" );
	exit( 1 );
      }

    if( new_VDD == NULL and chg_VDD == 0 )
	;
    else
      {
	fprintf( stderr, "Error: changed Vdd, cannot deal with this yet!\n" );
	exit( 1 );
      }
  }


private void add_tran_cap( t )
  register tptr  t;
  {
    if( t->gate->nflags & N_CAP_SET )
	t->gate->ncap += t->r->width * t->r->length * CTGA;
	
    if( config_flags & TDIFFCAP )
      {
	if( t->source->nflags & N_CAP_SET )
	    t->source->ncap += t->r->width * CTDW + CTDE;
	if( t->drain->nflags & N_CAP_SET )
	    t->drain->ncap += t->r->width * CTDW + CTDE;
      }
  }


#define CHK_CAP( ND )	if( (ND)->ncap < MIN_CAP ) (ND)->ncap = MIN_CAP
#define	ABS( X )	( ((X) < 0) ? -(X) : (X) )


private iptr changed_nodes()
  {
    register nptr  n;
    iptr           ilist = NULL;

    for( n = ch_nlist; n != NULL; n = n->n.next )
      {
	CHK_CAP( n );

	if( n->hnext == n )
	    n_insert( n );

	n->nflags &= ~N_CAP_SET;

	if( not (n->nflags & CHANGED) )
	  {
	    double deltac = ABS( n->c.cap - n->ncap );
	    if( deltac < CAP_THRESH * n->c.cap )
	      {
		n->c.time = n->curr->time;
		continue;
	      }
	    n->nflags |= CHANGED;
	  }

	if( n->nflags & MERGED )
	  {
	    register nptr  stack;

	    stack = n->t.tran->source;
	    if( not (stack->nflags & (CHANGED | POWER_RAIL)) )
	      {
		iinsert( stack, &ilist );
		stack->nflags |= CHANGED;
	      }
	    stack = n->t.tran->drain;
	    if( not (stack->nflags & (CHANGED | POWER_RAIL)) )
	      {
		iinsert( stack, &ilist );
		stack->nflags |= CHANGED;
	      }
	    n->nflags &= ~CHANGED;
	    continue;
	  }

	if( n->nflags & POWER_RAIL )
	    continue;

	iinsert( n, &ilist );
      }
    return( ilist );
  }


public iptr rd_changes( fname, logname )
  char  *fname;
  char  *logname;
  {
    FILE          *fin;
    iptr          ilist = NULL;
    struct Trans  dummyt;
    extern char   *getenv();

    ch_tran = dummyt.scache.t = dummyt.dcache.t = &dummyt;
    ch_nlist = NULL;
    context = num_cap_set = num_deleted = num_errors = lineno = 0;
    nu_fname = fname;

    if( (fin = fopen( fname, "r" )) == NULL )
      {
	lprintf( stderr, "can not open '%s' for net changes\n", fname );
	return( NULL );
      }

    if( logname == NULL )
	nu_logf = NULL;
    else if( (nu_logf = fopen( logname, "a" )) == NULL )
	lprintf( stderr, "warning: can't open logfile %s\n", logname );
    else
      {
	time_t ltime = time( 0L );
	(void) fprintf( nu_logf, "| changes @ %s", ctime( &ltime ) );
      }

    if( analyzerON )	StopAnalyzer();

    input_changes( fin );

    chk_power();
    ch_nlist = rm_nodes();
    conn_ch_trans( ch_tran );
    if( num_cap_set ) walk_trans( add_tran_cap, (char *) 0 );
    make_parallel( ch_nlist );
    make_stacks( ch_nlist );

    pTotalNodes();
    pTotalTxtors();
    pParallelTxtors();
    pStackedTxtors();

    ilist = changed_nodes();

    if( analyzerON )	RestartAnalyzer( sim_time0, sim_time0, FALSE );

    if( num_errors != 0 )
	lprintf( stderr, "%s contains %d errors\n", fname, num_errors,
	  (nu_logf != NULL or logfile != NULL) ? " listed in logfile" : "" );

    if( getenv( "RSIM_CHANGED" ) != NULL )
      {
	iptr  ip;

	if( ilist != NULL ) lprintf( stdout, "changed nodes:\n" );
	for( ip = ilist; ip != NULL; ip = ip->next )
	    lprintf( stdout, "  %s\n", pnode( ip->inode ) );
      }

    return( ilist );
  }

private void nu_error(char *fmt, ...)
  {
    va_list  args;
    char     *errstr = "| error";
    FILE     *fp;

    if (nu_logf != NULL)	fp = nu_logf;
    else if (logfile != NULL)	fp = logfile;
    else			fp = stderr, errstr++;

    va_start(args, fmt);

    (void) fprintf(fp, "%s:%s, line %d: ", errstr, nu_fname, lineno);
    (void) vfprintf(fp, fmt, args);
    va_end(args);
    num_errors++;
  }
