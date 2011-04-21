/*--------------------------------------------------------------*/
/* This file (user.c) is a placeholder that defines the array	*/
/* of information about user-defined subcircuits.  To compile	*/
/* a different set of user subcircuits, replace "user.c" in the */
/* Makefile with the name of the source code file.  Files	*/
/* "flop_mux.c" and "dll.c" are provided as examples.		*/
/*--------------------------------------------------------------*/

#include <stdio.h>

#ifndef _GLOBALS_H
#include "globals.h"
#endif

#include "subckt.h"

#define	DEBUG 0
#define	dprintf if (DEBUG) lprintf

userSubCircuit subs[] =
{
    { NULL,	NULL,	   NULL,      0, 0, NULL}
};

/*--------------------------------------------------------------*/
/* End of user.c empty placeholder library			*/
/*--------------------------------------------------------------*/
