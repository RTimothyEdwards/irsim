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

#ifndef _RSIM_H
#define _RSIM_H

#ifndef _NET_H
#include "net.h"
#endif


/* front end for mos simulator -- Chris Terman (6/84)	*/
/* substantial changes: Arturo Salz (88) 		*/
/* split definitions into rsim.h: Tim Edwards (02)	*/

#define	LSIZE		2000	/* max size of command line (in chars) */
#define	MAXARGS		100	/* maximum number of command-line arguments */
#define	CMDTBLSIZE	64	/* size of command hash-table */
#define	MAXCOL		80	/* maximum width of print line */

#define	ITERATOR_START	'{'
#define	ITERATOR_END	'}'

#define	SIZEOF( X )	( (int) sizeof( X ) )

typedef struct _Cmd
  {
    char         *name;			/* name of this command */
    int          (*handler)();		/* handler for this command */
    short        nmin, nmax;		/* min and max number of arguments */
    char         *help;			/* command description */
    struct _Cmd  *next;			/* list of commands in bucket */
  } Command;

typedef struct _Path
  {
    struct _Path  *next;
    char          name[1];
  } Path;


typedef struct sequence *sptr;

typedef struct sequence
  {
    sptr    next;			/* next vector in linked list */
    int     which;			/* 0 => node; 1 => vector */
    union
      {
	nptr  n;
	bptr  b;
      } ptr;			/* pointer to node/vector */
    int     vsize;			/* size of each value */
    int     nvalues;			/* number of values specified */
    char    values[1];			/* array of values */
  } sequence;

/* variable external references */

extern	Command	*cmdtbl[];		/* command hash-table */
extern  int     contline;
extern	int	analyzerON;		/* set when analyzer is running */
extern	Ulong	sim_time0;		/* starting time (see flush_hist) */
extern	FILE	*logfile;		/* log file of transactions */

extern char	*filename;
extern char	*first_file;
extern int	lineno;
extern int	stack_txtors;
extern int	debug;
extern int	targc;

#ifdef	POWER_EST
extern	FILE	*caplogfile;		/* log file of cap transitions */
extern	double	toggled_cap;		/* indicative of total power of chip */
extern  float   vsupply;		/* supply voltage for pwr estimation */
extern  float   capstarttime;
extern  float   capstoptime;
extern  float	captime;
extern  float	powermult;		/* to do power estimate in milliWatts */
extern  int	pstep;			/* Bool - end of step power display */
extern  float   step_cap_x_trans;	/* Stepwise C*trans count */
#endif /* POWER_EST */

/* function prototype forward references */

#ifdef TCL_IRSIM
extern void Usage( char *, ... );
#else
extern void Usage();
#endif

extern char *BaseName();

extern int cmdfile();
extern int docmdpath();
extern int start_analyzer();

extern void parse_line();
extern int  exec_cmd();
extern int  expand();
extern int  input();
extern void shift_args();

#endif /* _RSIM_H */
