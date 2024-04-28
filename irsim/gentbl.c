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

/* generate lookup tables for switch level simulation */

/* number of states desired for logic level */
#define	NVALUES			4

/* total number of logic states we need */
#define	NSPECTRUM		(2 * NVALUES + 1)

/* (NSPECTRUM choose 2) + NSPECTRUM */
#define	NINTERVAL		((NSPECTRUM *(NSPECTRUM - 1)) / 2 + NSPECTRUM)

char  *names[NSPECTRUM] = 
  {
    "DH", "WH", "CH", "cH", "Z", "cL", "CL", "WL", "DL"
  };

FILE  *out;


/* name of interval */
char *pinterval( int high, int low )
  {
    static char  temp[100];

    if( high != low )
	sprintf( temp, "%s%s", names[high], names[low] );
    else
	sprintf( temp, "%s", names[high] );
    return( temp );
  }


/* return strength of value */
int strength( register int i )
  {
    if( (i -= NVALUES) < 0 )
	i = -i;
    return( i );
  }

#define min( A, B )			( ((A) < (B)) ? (A) : (B) )
#define max( A, B )			( ((A) > (B)) ? (A) : (B) )

/* find the enclosing interval */
char *span( int ihigh,int ilow,int jhigh, int jlow )
  {
    return( pinterval( min( ihigh, jhigh ), max( ilow, jlow ) ) );
  }


/* merge two intervals using least-upper bound operation */
char *merge( int ihigh, int ilow, int jhigh, int jlow )
  {
    register int  ahigh, alow;

    if( strength( ihigh ) > strength( jhigh ) )
	ahigh = ihigh;
    else if( strength( ihigh ) < strength( jhigh ) )
	ahigh = jhigh;
    else if( ihigh < jhigh )
	ahigh = ihigh;
    else
	ahigh = jhigh;

    if( strength( ilow ) > strength( jlow ) )
	alow = ilow;
    else if( strength( ilow ) < strength( jlow ) )
	alow = jlow;
    else if( ilow > jlow )
	alow = ilow;
    else
	alow = jlow;

    return( pinterval( ahigh, alow ) );
  }


/* convert interval to use weak values */
char *weak( int i, int j )
  {
    if( i == 0 )
	i = 1;
    else if( i == (NSPECTRUM - 1) )
	i = NSPECTRUM - 2;
    if( j == 0 )
	j = 1;
    else if( j == (NSPECTRUM - 1) )
	j = NSPECTRUM - 2;
    return( pinterval( i, j ) );
  }


int main(void)
  {
    register int  i, j, k, ii, jj, interval2;

    if( (out = fopen( "stables.c", "w" )) == NULL )
      {
	fprintf( stderr, "cannot open stables.c for output\n" );
	exit( 1 );
      }

    fprintf( out, "/* DO NOT EDIT: THIS FILE IS GENERATED USING gentbl */\n");
    fprintf( out, "/* names for each value interval */\n" );
    fprintf( out, "char *node_values[%d] = {\n", NINTERVAL+1 );
    fprintf( out, "\t\"EMPTY\",\n" );
    for( i = 0; i < NSPECTRUM; i += 1 )
	for( j = i; j < NSPECTRUM; j += 1 )
	    fprintf( out, "\t\"%s\",\n", pinterval( i, j ) );
    fprintf( out, "};\n\n" );

    fprintf( out, "/* index for each value interval */\n" );
    fprintf( out, "#define EMPTY\t%d\n\n", 0 );
    for( i = 0, k = 1; i < NSPECTRUM; i += 1 )
	for( j = i; j < NSPECTRUM; j += 1, k += 1 )
	    fprintf( out, "#define %s\t%d\n", pinterval( i, j ), k );
    fprintf( out, "\n" );

    fprintf( out, "/* conversion between interval and logic value */\n" );
    fprintf( out, "char logic_state[%d] = {\n", NINTERVAL+1 );
    fprintf( out, "  0,\t/* EMPTY */\n" );
    for( i = 0; i < NSPECTRUM; i += 1 )
	for( j = i; j < NSPECTRUM; j += 1 )
	    if( i < NVALUES && j < NVALUES )
		fprintf( out, "  HIGH,\t/* %s */\n", pinterval( i, j ) );
	    else if( i > NVALUES && j > NVALUES )
		fprintf( out, "  LOW,\t/* %s */\n", pinterval( i, j ) );
	    else
		fprintf( out, "  X,\t/* %s */\n", pinterval( i, j ) );
    fprintf( out, "};\n\n" );

    fprintf( out, "/* transmit interval through switch */\n" );
    fprintf( out, "char transmit[%d][4] = {\n", NINTERVAL+1 );
    fprintf( out, "  0,\t0,\t0,\t0,\t/* EMPTY */\n" );
    for( i = 0; i < NSPECTRUM; i += 1 )
	for( j = i; j < NSPECTRUM; j += 1 )
	  {
	    fprintf( out, "  Z," );			  /* off switch */
	    fprintf( out, "\t%s,", pinterval( i, j ) );	  /* on switch */
	    fprintf( out, "\t%s,", span( i, j, NVALUES, NVALUES ) );
							  /* unknown switch */
	    fprintf( out, "\t%s,", weak( i, j ) );	  /* weak switch */
	    fprintf( out, "\t/* %s */\n", pinterval( i, j ) );
	  }
    fprintf( out, "};\n\n" );

	/* compute smallest power of two greater than NINTERVAL */
    for( interval2 = 1; interval2 < NINTERVAL; interval2 <<= 1 );

    fprintf( out, "/* result of shorting two intervals */\n" );
    fprintf( out, "char smerge[%d][%d] = {\n", NINTERVAL+1, NINTERVAL+1 );
    fprintf( out, "\n/* EMPTY */\n" );
    for( i = 0; i <= NINTERVAL; i +=1 )
      {
	fprintf( out, "  0   ," );			/* EMPTY */
	if( i % 8 == 7 )
	    fprintf( out, "\n" );
      }

    for( i = 0; i < NSPECTRUM; i += 1 )
	for( j = i; j < NSPECTRUM; j += 1 )
	  {
	    fprintf( out, "\n/* %s */\n", pinterval( i, j ) );
	    fprintf( out, "  0   ," );
	    for( ii = 0, k = 1; ii < NSPECTRUM; ii += 1 )
		for( jj = ii; jj < NSPECTRUM; jj += 1, k += 1 )
		  {
		    fprintf( out, "  %-4s,", merge( i, j, ii, jj ) );
		    if( k % 8 == 7 )
			fprintf( out, "\n" );
		  }
	  }
    fprintf( out, "\n};\n" );
    exit(0);
  }
