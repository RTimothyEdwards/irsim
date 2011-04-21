#ifndef _SUBCKT_H
#define _SUBCKT_H

#ifdef USER_SUBCKT 

#ifndef _LOCTYPES_H
#include "loctypes.h"
#endif

/*----------------------------------------------------------------------*/
/* User-defined subcircuits have a different set of logic values,	*/
/* allowing the output to be defined in various ways, like high-	*/
/* impedence, pull-up, pull-down, open drain, etc.			*/
/*----------------------------------------------------------------------*/

#define C_LOW           'l'     /* logic low */
#define C_X             'x'     /* unknown or intermediate value */
#define C_Z		'z'	/* high impedence state */
#define C_HI            'h'     /* logic high */

/*----------------------------------------------------------------------*/
/* Structure used to declare information about user-defined subcircuit	*/
/* library components.							*/
/*----------------------------------------------------------------------*/

typedef struct
{
    char	*name;			/* name of this sub-circuit */
    vfun	model;			/* function that models sub-circuit */
    ufun	init;			/* Function to call to init */
    short	ninputs;		/* Number of input nodes */
    short	noutputs;		/* Number of output nodes */
    float	*res;			/* pointer to list of output driver */
					/* "on" resistances */
} userSubCircuit;

/*----------------------------------------------------------------------*/
/* Structure that substitutes for a transistor record in the netlist	*/
/* at each subcircuit input node					*/
/*----------------------------------------------------------------------*/

typedef struct _SubcktT {
    userSubCircuit *subckt;
    nptr     *nodes;
    uptr     udata;          /* pointer to instance's private data */
    lptr     ndiode;
} SubcktT;

extern userSubCircuit subs[];

#endif

#ifndef _UNITS_H
#include "units.h"
#endif

#endif
