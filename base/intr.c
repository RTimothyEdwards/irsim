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
#include <signal.h>

#include "defs.h"

#ifdef SYS_V
#    ifndef hpux
#	define	signal( SIG, HAND )	sigset( SIG, HAND )
#    endif
#endif


public	int    int_received = 0;


private void int_handler()
  {
    if( int_received == 1 )
	(void) fprintf( stderr, "\nok ... wait a second\n" );
    if( int_received <= 1 )
	int_received++;

#ifdef SYS_V
    (void) signal( SIGINT, int_handler );
#endif
  }


private void bye_bye()
  {
    extern void TerminateAnalyzer();

    TerminateAnalyzer();
    exit( 0 );
  }


public void InitSignals()
  {
    (void) signal( SIGQUIT, SIG_IGN );		    /* ignore quit */
    (void) signal( SIGINT, int_handler );
    (void) signal( SIGHUP, bye_bye );		    /* for magic's sake */
  }
