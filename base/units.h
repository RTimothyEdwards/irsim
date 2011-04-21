/* 
 * units.h --- Defines the time size of internal units by declaring
 * 		their relationship to nanoseconds and picoseconds.
 *		Previously these definitions were in "net.h".
 */

#ifndef _UNITS_H
#define _UNITS_H

/* Conversion macros between various time units */

#define PSEC_TIME	/* 1 delta = 1 ps */

#ifdef PSEC_TIME
#define	d2ns( D )		( (D) * 0.001 )		/* deltas to ns */
#define	d2ps( D )		( (D) * 1.0 )		/* deltas to ps */
#define	ns2d( N )		( (N) * 1000.0 )	/* ns to deltas */
#define	ps2d( P )		( (P) * 1.0 )		/* ps to deltas */
#define DEFLT_STEP		10000
#else
#define d2ns( D )               ( (D) * 0.1 )           /* deltas to ns */
#define d2ps( D )               ( (D) * 100.0 )         /* deltas to ps */
#define ns2d( N )               ( (N) * 10.0 )          /* ns to deltas */
#define ps2d( P )               ( (P) * 0.01 )          /* ps to deltas */
#define DEFAULT_STEP		100
#endif
#define	ps2ns( P )		( (P) * 0.001 )		/* ps to ns */

#endif /* _UNITS_H */
