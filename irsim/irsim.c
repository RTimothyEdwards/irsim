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

#include "defs.h"
#include "net.h"
#include "globals.h"
#include "rsim.h"
#ifdef USER_SUBCKT
#include "../usersubckt/subckt.h"
#endif

extern void InitCmdPath();
extern public void init_commands();
extern int finput( char *name);

/* VARARGS1 */
public void Usage( char *msg, char *s1 )
  {
    (void) fprintf( stderr, msg, s1 );
    (void) fprintf( stderr, "usage:\n irsim " );
    (void) fprintf( stderr, "[-s] prm_file {sim_file ..} [-cmd_file ..]\n" );
    (void) fprintf( stderr, "\t-s\t\tstack series transistors\n" );
    (void) fprintf( stderr, "\tprm_file\telectrical parameters file\n" );
    (void) fprintf( stderr, "\tsim_file\tsim (network) file[s]\n" );
    (void) fprintf( stderr, "\tcmd_file\texecute command file[s]\n" );
    exit( 1 );
  }

/* Main routine for irsim */

public int main( int argc, char *argv[] )
  {
    int  i, arg1, has_param_file;

    InitSignals();
    InitUsage();
    InitThevs();
    InitCAD();
    InitCmdPath();
    init_hist();
    (void) fprintf( stdout, "*** IRSIM %s.%s *** @ %s\n",
		IRSIM_VERSION, IRSIM_REVISION, IRSIM_DATE);
    (void) fflush( stdout );

    filename = "*initialization*";

    for( arg1 = 1; arg1 < argc; arg1++ )
      {
	if( argv[arg1][0] == '-' )
	  {
	    switch( argv[arg1][1] )
	      {
		case 's' :			/* stack series transistors */
		    stack_txtors = TRUE;
		    break;
		default :
		    Usage( "unknown switch: %s\n", argv[arg1] );
	      }
	  }
	else
	  break;
      }

	/* read in the electrical configuration file */
    if( arg1 < argc )
	has_param_file = config( argv[arg1++] );
    else
	has_param_file = -1;


	/* Read network files (sim files) */
    for( i = arg1; i < argc; i += 1 )
      {
	if( argv[i][0] != '-' and argv[i][0] != '+' )
	  {
	    rd_network( argv[i], (char *)NULL, has_param_file );
	    if( first_file == NULL )
		first_file = BaseName( argv[i] );
	  }
      }

    if( first_file == NULL )
      {
	(void) fprintf( stderr, "No network, no work. Bye...\n" );
	exit( 1 );
      }

    ConnectNetwork();	/* connect all txtors to corresponding nodes */
    InitTimes(sim_time0, stepsize, cur_delta, 0);

    init_commands();	/* set up command table */

    init_event();

	/* search for -filename for command files to process. */
    filename = "command line";
    lineno = 1;
    for( i = arg1; i < argc; i++ )
	if( argv[i][0] == '-' and not finput( &argv[i][1] ) )
	    rsimerror( filename, lineno, "cannot open %s for input\n",
	       &argv[i][1] );

	/* finally (assuming we get this far) read commands from user */
    debug = 0;
    filename = "tty";
    lineno = 0;
    (void) input( stdin );

    TerminateAnalyzer();
    exit( 0 );
    /* NOTREACHED */
  }

