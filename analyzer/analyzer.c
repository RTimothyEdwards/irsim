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
#include <string.h>  /* for strlen() */

#include "globals.h"
#include "ana.h"
#include "ana_glob.h"

private	int    numAdded;


public char *SetName( n )
  register char  *n;
  {
    int   i;

    i = strlen( n );
    if( i > max_name_len )
      {
	i -= max_name_len;
	n += i;
      }
    return( n );
  }


private void AddTrace( t )
  Trptr  t;
  {
    if( traces.first == NULL )
      {
	t->next = t->prev = NULL;
	traces.first = traces.last = t;
      }
    else
      {
	t->next = NULL;
	t->prev = traces.last;
	traces.last->next = t;
	traces.last = t;
      }
    numAdded++;
  }

public int OffsetNode( nd, flag )
  nptr  nd;
  int	*flag;
{
   /* do nothing */
   return 1;
}

public int OffsetVector( vec, flag )
  bptr  vec;
  int	*flag;
{
   /* do nothing */
   return 1;
}

public int AddNode( nd, flag )
  nptr  nd;
  int   *flag;
  {
    Trptr  t;
    
    while( nd->nflags & ALIAS )
	nd = nd->nlink;

    if( nd->nflags & MERGED )
      {
	fprintf( stderr, "can't watch node %s\n", nd->nname );
	return( 1 );
      }
    if( (t = (Trptr) Valloc( sizeof( TraceEnt ), 0 )) == NULL )
      {
	fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
	  nd->nname );
	return( 0 );
      }
    t->name = SetName( nd->nname );
    t->len = strlen( t->name );
    t->bdigit = 1;
    t->vector = FALSE;
    t->n.nd = nd;
    t->cache[0].wind = t->cache[0].cursor = &(nd->head);
    AddTrace( t );
    return( 1 );
  }


public int AddVector( vec, flag )
  bptr  vec;
  int   *flag;
  {
    Trptr  t;
    int    n;

    n = vec->nbits;
    t = (Trptr) Valloc( sizeof( TraceEnt ) + sizeof( Cache ) * (n - 1), 0 );
    if( t == NULL )
      {
	fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
	  vec->name );
	return( 0 );
      }
    t->name = SetName( vec->name );
    t->len = strlen( t->name );
    if( *flag != 0 )
	t->bdigit = *flag;
    else
	t->bdigit = ( n > 5 ) ? 5 : 1;	/* > 5 bits defaults to decimal fmt */
    t->vector = TRUE;
    t->n.vec = vec;
    for( n--; n >= 0; n-- )
	t->cache[n].wind = t->cache[n].cursor = &(vec->nodes[n]->head);
    AddTrace( t );
    return( 1 );
  }



public void DisplayTraces( isMapped )
  int  isMapped;
  {
    int  change;

    DisableInput();
   
    traces.total += numAdded;
    numAdded = 0;

    if( not isMapped )				/* only the first time */
      {
	FlushTraceCache();
	XMapWindow( display, window );
      }
    else if( not (windowState.iconified or windowState.tooSmall) )
      {
	change = WindowChanges();

	if( change & NTRACE_CHANGE )
	  {
	    RedrawNames( namesBox );
	    DrawCursVal( cursorBox );
	    if( change & WIDTH_CHANGE )
	      {
		DrawScrollBar( FALSE );
		RedrawTimes();
	      }
	    DrawTraces( tims.start, tims.end );
	  }
      }

    EnableInput();
  }


public void StopAnalyzer()
  {
    DisableAnalyzer();
  }


public void RestartAnalyzer( first_time, last_time, same_hist )
  Ulong  first_time, last_time;
  int   same_hist;
  {
    register Trptr  t;
    register int    i, n;

    printf("restarting analyzer\n");
    for( t = traces.first, i = traces.total; i != 0; i--, t = t->next )
      {
	if( t->vector )
	  {
	    for( n = t->n.vec->nbits - 1; n >= 0; n-- )
	      {
		t->cache[n].wind = t->cache[n].cursor = 
		  &(t->n.vec->nodes[n]->head);
	      }
	  }
	else
	    t->cache[0].wind = t->cache[0].cursor = &(t->n.nd->head);
      }

    InitTimes( first_time, tims.steps / DEF_STEPS, last_time, 1);
    if( same_hist )
	UpdateTraceCache( 0 );
    else
	FlushTraceCache();
    EnableAnalyzer( /* same_hist */ );
  }


public void RemoveTrace( t )
  Trptr  t;
  {
    traces.total--;
    if( t == traces.first )
      {
	traces.first = t->next;
	if( t->next )
            t->next->prev = NULL;
        else
            traces.last = NULL;
      }
    else
      {
        t->prev->next = t->next;
        if( t->next )
            t->next->prev = t->prev;
        else
            traces.last = t->prev;
      }
    
    if( selectedTrace == t )
        selectedTrace = NULL;
    Vfree( t );
  }


public void ClearTraces()
  {
    int  change, wasTooSmall;

    DisableInput();
   
    while( traces.total != 0 )
	RemoveTrace( traces.first );


    if( windowState.iconified )
      {
	EnableInput();
	return;
      }

    wasTooSmall = windowState.tooSmall;

    change = WindowChanges();
    if( windowState.tooSmall )
      {
	RedrawSmallW();
	EnableInput();
	return;
      }

    if( wasTooSmall )
      {
	RedrawBanner();
	RedrawText();
	DrawCursVal( cursorBox );
      }
    RedrawNames( namesBox );
    DrawScrollBar( wasTooSmall );
    RedrawTimes();
    DrawTraces( tims.start, tims.end );

    EnableInput();
  }


public void UpdateWinRemove()
  {
    int  change;

    change = WindowChanges();

    if( change & RESIZED )
        return;

    DisableInput();

    if( not (change & NTRACE_CHANGE) )          /* no trace became visible */
        SetSignalPos();

    if( change & WIDTH_CHANGE )
      {
        DrawScrollBar( FALSE );
        RedrawTimes();
      }

    RedrawNames( namesBox );
    DrawCursVal( cursorBox );
    DrawTraces( tims.start, tims.end );

    EnableInput();
  }


public void RemoveVector( b )
  bptr  b;
  {
    Trptr  t, tmp;
    int    i = FALSE;

    for( t = traces.first; t != NULL; )
      {
	if( t->vector and (t->n.vec == b) )
	  {
	    tmp = t->next;
	    RemoveTrace( t );
	    t = tmp;
	    i = TRUE;
	  }
	else
	    t = t->next;
      }
    if( i )
	UpdateWinRemove();
  }



public void RemoveNode( n )
  nptr  n;
  {
    Trptr  t, tmp;
    int    i = FALSE;

    for( t = traces.first; t != NULL; )
      {
	if( not t->vector and (t->n.nd == n) )
	  {
	    tmp = t->next;
	    RemoveTrace( t );
	    t = tmp;
	    i = TRUE;
	  }
	else
	    t = t->next;
      }
    if( i )
	UpdateWinRemove();
  }


public void RemoveAllDeleted()
  {
    Trptr  t, tmp;
    int    i = FALSE;

    for( t = traces.first; t != NULL; )
      {
	if( (t->vector and (t->n.vec->traced & DELETED)) or
	  (not t->vector and (t->n.nd->nflags & DELETED)) )
	  {
	    tmp = t->next;
	    RemoveTrace( t );
	    t = tmp;
	    i = TRUE;
	  }
	else
	    t = t->next;
      }
    if( i )
	UpdateWinRemove();
  }

