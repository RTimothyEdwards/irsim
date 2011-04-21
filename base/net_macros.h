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

#ifndef _NET_MACROS_H
#define _NET_MACROS_H

/* 
 * Auxiliary net macros.
 */
 
#include <stdio.h>

#ifndef _NET_H
#include "net.h"
#endif

/*
 * Swap the 2 nodes
 */
#define	SWAP_NODES( N1, N2 )		SWAP( nptr, N1, N2 )


/*
 * if FLAG is not set in in NODE->nflags, Link NODE to the head of LIST
 * using the temporary entry (n.next) in the node structure.  This is
 * used during net read-in/change to build lists of affected nodes.
 */
#define	LINK_TO_LIST( NODE, LIST, FLAG )	\
  {						\
    if( ((NODE)->nflags & (FLAG)) == 0 )	\
      {						\
	(NODE)->nflags |= (FLAG);		\
	(NODE)->n.next = (LIST);		\
	LIST = (NODE);				\
      }						\
  }						\



/*
 * Allocate a new "Tlist" pointer.
 */
#define	NEW_LINK( LP )						\
  {								\
    if( (LP = freeLinks) == NULL )				\
	LP = (lptr) MallocList( sizeof( struct Tlist ), 1 );	\
    freeLinks = (LP)->next;					\
  }


/*
 * Return "Tlist" pointer LP to free pool.
 */
#define	FREE_LINK( LP )			\
  {					\
    (LP)->next = freeLinks;		\
    freeLinks = (LP);			\
  }


/*
 * Allocate a new Transistor.
 */
#ifndef USER_SUBCKT
#define	NEW_TRANS( T )						\
  {								\
    if( (T = freeTrans) == NULL )				\
	T = (tptr) MallocList( sizeof( struct Trans ), 1 );	\
    freeTrans = (tptr) (T)->gate;				\
  }
#else 
#define	NEW_TRANS( T )						\
  {								\
    if( (T = freeTrans) == NULL )				\
	T = (tptr) MallocList( sizeof( struct Trans ), 1 );	\
    T->subptr = NULL ;                                          \
    freeTrans = (tptr) (T)->gate;				\
  }
#define NEW_SUBCKT( S ) \
  { \
   S = ( SubcktT *) Falloc( sizeof (SubcktT) );\
   bzero( S , sizeof(SubcktT) );\
  }
#endif


/*
 * Return transistor record T to free pool.
 */
#define	FREE_TRANS( T )			\
  {					\
    (T)->gate = (nptr) freeTrans;	\
    freeTrans = (T);			\
  }					\


/*
 * Add transistor T to the list of transistors connected to that list.
 * The transistor is added at the head of the list.
 */
#define CONNECT( LIST, T )	\
  {				\
    register lptr  newl;	\
				\
    NEW_LINK( newl );		\
    newl->xtor = (T);		\
    newl->next = (LIST);	\
    LIST = newl;		\
  }


/*
 * Transistors that have their drain/source shorted are NOT connected
 * to the network, they are instead linked as a doubly linked list
 * using the scache/dcache fields.
 */
#define LINK_TCAP( T )			\
  {					\
    (T)->dcache.t = tcap;		\
    (T)->scache.t = tcap->scache.t;	\
    tcap->scache.t->dcache.t = (T);	\
    tcap->scache.t = (T);		\
    tcap->x.pos ++;			\
  }


#define	UNLINK_TCAP( T )			\
  {						\
    (T)->dcache.t->scache.t = (T)->scache.t;	\
    (T)->scache.t->dcache.t = (T)->dcache.t;	\
    (T)->ttype &= ~TCAP;			\
    tcap->x.pos --;				\
  }


/*
 * Replace the first ocurrence of transistor OLD by NEW on LIST.
 */
#define	REPLACE( LIST, OLD, NEW )			\
  {							\
    register lptr  lp;					\
							\
    for( lp = (LIST); lp != NULL; lp = lp->next )	\
      {							\
	if( lp->xtor == OLD )				\
	  {						\
	    lp->xtor = NEW;				\
	    break;					\
	  }						\
      }							\
  }							\


/*
 * Remove the entry for transistor T from LIST, and return it to the
 * free pool.
 */
#define	DISCONNECT( LIST, T )			\
  {						\
    register lptr  li, *lip;			\
						\
    lip = &(LIST);				\
    while( (li = *lip) != NULL )		\
      {						\
	if( li->xtor == (T) )			\
	  {					\
	    *lip = li->next;			\
	    FREE_LINK( li );			\
	    break;				\
	  }					\
	lip = &(li->next);			\
      }						\
  }						

#endif /* _NET_MACROS_H */
