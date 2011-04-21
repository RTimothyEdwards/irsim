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
#include <string.h>
#include <ctype.h>

#include "defs.h"
#include "net.h"
#include "globals.h"

/* useful subroutines for dealing with network */

public	char   vchars[] = "0XX1";

public	char  *ttype[ NTTYPES ] =
  {
    "n-channel",
    "p-channel",
    "depletion",
    "resistor"
#ifdef USER_SUBCKT
    ,
    "reserved",
    "subckt"
#endif
  };


#define	HASHSIZE		4387	/* hash table size */
#define	NBIT_HASH		14	/* number of bits for HASHSIZE */

private	nptr  hash[HASHSIZE];


public int GetHashSize()
  {
    return( HASHSIZE );
  }


/* hashing function used in interning symbols */
private	char  lcase[128] = 
  {
    0,		01,	02,	03,	04,	05,	06,	07,
    010,	011,	012,	013,	014,	015,	016,	017,
    020,	021,	022,	023,	024,	025,	026,	027,
    030,	031,	032,	033,	034,	035,	036,	037,
    ' ',	'!',	'"',	'#',	'$',	'%',	'&',	047,
    '(',	')',	'*',	'+',	',',	'-',	'.',	'/',
    '0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
    '8',	'9',	':',	';',	'<',	'=',	'>',	'?',
    '@',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
    'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
    'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
    'x',	'y',	'z',	'[',	0134,	']',	'^',	'_',
    '`',	'a',	'b',	'c',	'd',	'e',	'f',	'g',
    'h',	'i',	'j',	'k',	'l',	'm',	'n',	'o',
    'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
    'x',	'y',	'z',	'{',	'|',	'}',	'~',	0177
  };



private int sym_hash( name )
  register char  *name;
  {
    register int  hashcode = 0;

    do
	hashcode = (hashcode << 1) ^ (*name | 0x20);
    while( *(++name) );
    return( ((hashcode >= 0) ? hashcode : ~hashcode) % HASHSIZE );
  }


/*
 * Compare 2 strings, case doesn't matter.  Return value is an integer greater
 * than, equal to, or less than 0, according as 's1' is lexicographically
 * greater than, equal to, or less than 's2'.
 */
public int str_eql( s1, s2 )
  register char  *s1, *s2;
  {
    register int  cmp;

    while( *s1 )
      {
	if( (cmp = lcase[(int)*s1++] - lcase[(int)*s2++]) != 0 )
	    return( cmp );
      }
    return( 0 - *s2 );
  }


/* compare pattern with string, case doesn't matter.  "*" wildcard accepted */
public int str_match( p, s )
  register char  *p, *s;
  {
    while( 1 )
      {
	if( *p == '*' )
	  {
	    /* skip past multiple wildcards */
	    do
		p++;
	    while( *p == '*' );

	    /* if pattern ends with wild card, automatic match */
	    if( *p == 0 )
		return( 1 );

	    /* *p now points to first non-wildcard character, find matching
	     * character in string, then recursively match remaining pattern.
	     * if recursive match fails, assume current '*' matches more...
	     */
	    while( *s != 0 )
	      {
		while( lcase[(int)*s] != lcase[(int)*p] )
		    if( *s++ == 0 )
			return( 0 );
		if( str_match( p + 1, ++s ) )
		    return( 1 );
	      }

	    /* couldn't find matching character after '*', no match */
	    return( 0 );
	  }
	else if( *p == 0 )
	    return( *s == 0 );
	else if( lcase[(int)*p++] != lcase[(int)*s++] )
	    return( 0 );
      }
  }


/* find node in network */
public nptr find( name )
  register char  *name;
  {
    register nptr  ntemp;
    register int   cmp = 1;

    if( txt_coords and name[0] == '@' and name[1] == '=' )
	if( (ntemp = FindNode_TxtorPos( name )) != NULL )
	    return( ntemp );

    for( ntemp = hash[sym_hash( name )]; ntemp != NULL; ntemp = ntemp->hnext )
      {
	if( (cmp = str_eql( name, ntemp->nname )) >= 0 )
	    break;
      }
    return( (cmp == 0) ? ntemp : NULL );
  }


/*
 * Get node structure.  If not found, create a new one.
 * If create is TRUE a new node is created and NOT entered into the table.
 */
static int warnVdd = FALSE, warnGnd = FALSE ;

public nptr RsimGetNode( name_in )
  register char  *name_in;
  {
    register nptr  n, *prev;
    register int   i, ispwrname = 0;
    int skip = 0;
    char *name = name_in;

    if (strcasecmp(name, "Vdd") == 0) ispwrname = 1;
    if (strcasecmp(name, "GND") == 0) ispwrname = 1;

    if ((simprefix != NULL) && (ispwrname == 0)) {
	/* Append the prefix to nodes.  This allows multiple files to be read	*/
	/* in as subcircuits and maintain individual node namespaces.  However,	*/
	/* nodes Vdd and GND remain global.					*/

	name = malloc(strlen(name_in) + strlen(simprefix) + 2);
	sprintf(name, "%s/%s", simprefix, name_in);
    }

    prev = &hash[ sym_hash( name ) ];
    for( i = 1, n = *prev; n != NULL; n = *(prev = &n->hnext) )
	if( (i = str_eql( name, n->nname )) >= 0 )
	    break;

    if( i == 0 )
      {
	if( strcmp( name, n->nname ) != 0 ) {
	    if ( strcasecmp( name, "Vdd" ) == 0 ) {
		skip = warnVdd ; warnVdd = TRUE ;
	    }
	    if ( strcasecmp( name, "GND" ) == 0 ) {
		skip = warnGnd ; warnGnd = TRUE ;
	    }
	    if ( skip == 0 ) 
	        lprintf( stderr, "Warning: Aliasing nodes '%s' and '%s'\n",
	          name, n->nname );
	}
	while( n->nflags & ALIAS )
	    n = n->nlink;
        if (name != name_in) free(name);
	return( n );
      }

	/* allocate new node from free storage */
    if( (n = (nptr) freeNodes) == NULL )
	n = (nptr) MallocList( sizeof( struct Node ), 1 );
    freeNodes = n->nlink;

    nnodes++;

    n->hnext = *prev;	/* insert node into hash table, after prev */
    *prev = n;

	/* initialize node entries */
    n->ngate = n->nterm = NULL;
    n->nflags = 0;
    n->ncap = MIN_CAP;
    n->vlow = LOWTHRESH;
    n->vhigh = HIGHTHRESH;
    n->c.time = 0;
    n->tplh = 0;
    n->tphl = 0;
    n->t.cause = NULL;
    n->nlink = NULL;
    n->events = NULL;
    n->npot = X;
    n->awpending = NULL ;

    n->head.next = last_hist;
    n->head.time = 0;
    n->head.val = X;
    n->head.inp = 0;
    n->head.punt = 0;
    n->head.t.r.rtime = n->head.t.r.delay = 0;
    n->curr = &(n->head);

	/* store all node-names as strings */
    i = strlen( name ) + 1;
    n->nname = Valloc( i, 1 );
    bcopy( name, n->nname, i );

    if (name != name_in) free(name);
    return( n );
  }


public nptr GetNewNode( name )
  char  *name;
  {
    register nptr  n;
    int            i;

    if( VDD_node != NULL and str_eql( name, pnode( VDD_node ) ) == 0 )
	return( VDD_node );
    if( GND_node != NULL and str_eql( name, pnode( GND_node ) ) == 0 )
	return( GND_node );

    if( (n = (nptr) freeNodes) == NULL )
	n = (nptr) MallocList( sizeof( struct Node ), 1 );
    freeNodes = n->nlink;

    nnodes++;

    n->hnext = n;	/* node NOT inserted in hash-table */

    n->ngate = n->nterm = NULL;
    n->nflags = 0;
    n->ncap = MIN_CAP;
    n->vlow = LOWTHRESH;
    n->vhigh = HIGHTHRESH;
    n->c.time = 0;
    n->tplh = 0;
    n->tphl = 0;
    n->t.cause = NULL;
    n->nlink = NULL;
    n->events = NULL;
    n->npot = X;

    n->head.next = last_hist;
    n->head.time = 0;
    n->head.val = X;
    n->head.inp = 0;
    n->head.punt = 0;
    n->head.t.r.rtime = n->head.t.r.delay = 0;
    n->curr = &(n->head);

	/* store all node-names as strings */
    i = strlen( name ) + 1;
    n->nname = Valloc( i, 1 );
    bcopy( name, n->nname, i );

    return( n );
  }


/* insert node into hash table.  Keep table entry sorted in ascending order */
public void n_insert( nd )
  register nptr  nd;
  {
    register nptr  n, *prev;
    register char  *name;
    register int   i = 1;

    name = pnode( nd );
    prev = &hash[ sym_hash( name ) ];
    for( n = *prev; n != NULL; n = *(prev = &n->hnext) )
      {
	i = str_eql( name, n->nname );
	if( i >= 0 )
	    break;
      }
    if( i == 0 )
      {
	if( n != nd )
	    lprintf( stderr, "n_insert: multiple node '%s'\n", pnode( nd ) );
	return;
      }
    nd->hnext = *prev;
    *prev = nd;
  }


/*
 * Delete node from hash table, and free its name.
 */
public void n_delete( nd )
  register nptr  nd;
  {
    register nptr  n, *prev;

    prev = &hash[ sym_hash( pnode( nd ) ) ];
    for( n = *prev; n != NULL ; n = *(prev = &n->hnext) )
      {
	if( n == nd )
	  {
	    Vfree( n->nname );
	    n->nname = NULL;
	    *prev = n->hnext;
	    n->hnext = n;		/* mark node not in hash-table */
	    return;
	  }
      }
  }


/*
 * Visit each node in network, calling function passed.
 * If 'fun' returns anything <> 0 then terminate prematurely.
 */
public void walk_net( fun, arg )
  int   (*fun)();
  char  *arg;
  {
    register int   index;
    register nptr  n;

    for( index = 0; index < HASHSIZE; index++ )
	for( n = hash[index]; n; n = n->hnext )
	    if( (*fun)( n, arg ) )
		return;
  }


/*
 * Visit each node in network, calling function passed with arguments:
 *	node	-> the current node.
 *	index	-> number that identifies node in the hash-table.
 *	arg	-> whatever argument the caller provided.
 * If 'fun' returns FALSE then terminate prematurely.
 */
public void walk_net_index( fun, arg )
  int   (*fun)();
  char  *arg;
  {
    register Uint  mi, ma;
    register nptr  n;

    for( ma = 0; ma < HASHSIZE; ma++ )
	for( n = hash[ ma ], mi = 0; n; n = n->hnext, mi++ )
	  {
	    Ulong  val = (mi << NBIT_HASH) | ma;
	    if( (*fun)( n, val, arg ) )
		return;
	  }
  }


/*
 * Return a list of all nodes in the network.
 */
public nptr GetNodeList()
  {
    register int   index;
    register nptr  n, *last;
    struct Node    head;

    last = &(head.n.next);
    for( index = 0; index < HASHSIZE; index++ )
      {
	for( n = hash[index]; n != NULL; n = n->hnext )
	  {
	    *last = n;
	    last = &(n->n.next);
	  }
      }
    *last = NULL;
    return( head.n.next );
  }


/*
 * Return the node corresponding to the index number (returned by Node2index).
 */
public nptr Index2node( index )
  pointertype index;
  {
    register nptr  n;
    register unsigned  ma, mi;

    ma = index & ((1 << NBIT_HASH) - 1);
    mi = index >> NBIT_HASH;
    if( ma >= HASHSIZE )
	return( NULL );
    for( n = hash[ ma ]; n != NULL and mi != 0; n = n->hnext, mi-- );
    return( n );
  }


/*
 * Return a number that identifies the node in the name hash-table.
 */
public pointertype Node2index( nd )
  nptr  nd;
  {
    register nptr      n;
    register unsigned  mi, ma;
    pointertype        val;

    if( nd != NULL )
      {
	ma = sym_hash( pnode( nd ) );
	for( n = hash[ ma ], mi = 0; n != NULL; n = n->hnext, mi++ )
	  {
	    if( n == nd )
		goto got_it;
	  }
      }
    ma = HASHSIZE;
    mi = 0;

  got_it :
    val = (mi << NBIT_HASH) | ma;
    return( val );
  }


/* visit each node in network, calling function passed as arg with any node
 * whose name matches pattern
 */
public int match_net( pattern, fun, arg )
  char  *pattern;
  int   (*fun)();
  char  *arg;
  {
    register int   index;
    register nptr  n;
    int            total = 0;

    for( index = 0; index < HASHSIZE; index++ )
	for( n = hash[index]; n; n = n->hnext )
	    if( str_match( pattern, pnode( n ) ) )
		total += (*fun)( n, arg );

    return( total );
  }


/* return pointer to asciz name of node */

public
#define	pnode( NODE )	( (NODE)->nname )


#ifndef TCL_IRSIM	/* TCL has much better hash routines */

/* initialize hash table */
public void init_hash()
  {
    register int  i;

    for( i = 0; i < HASHSIZE; i += 1 )
	hash[i] = NULL;
  }
#endif
