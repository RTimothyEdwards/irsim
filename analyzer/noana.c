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


extern void lprintf();


typedef long  TimeType;


public int AddNode( nd, flag )
  nptr  nd;
  int   *flag;
  {
    return( 1 );
  }

public int AddVector( vec, flag )
  bptr  vec;
  int   *flag;
  {
    return( 1 );
  }

public void DisplayTraces( isMapped )  int  isMapped;  {}

public void StopAnalyzer()  {}

public void RestartAnalyzer( first_time, last_time, same_hist )
  Ulong  first_time, last_time;
  int   same_hist;
  {}

public void ClearTraces() {}

public void RemoveVector( b )  bptr  b; {}

public void RemoveNode( n )  nptr  n; {}

public void RemoveAllDeleted()  {}

public void UpdateWindow( endT ) TimeType  endT;  {}

public void TerminateAnalyzer() {}

public int InitDisplay( fname, display_unit )
  char  *fname;
  char  *display_unit;
  {
    (void) lprintf( stdout, "No analyzer in this version\n" );
    return( FALSE );
  }

public void InitTimes( firstT, stepsize, lastT, reInit )
  TimeType  firstT, stepsize, lastT;
  {}
