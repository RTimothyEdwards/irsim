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

/*
 * The routines in this file handle network input (from sim files).
 * The input routine "rd_network" replaces the work formerly done by presim.
 * This version differs from the former in the following:
 *  1. voltage drops across transistors are ignored (assumes transistors
 *     driven by nodes with voltage drops have the same resistance as those
 *     driven by a full swing).
 *  2. static power calculations not performed (only useful for nmos anyhow).
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>			/* for fabs() function */

#include "defs.h"
#include "net.h"
#include "globals.h"
#include "net_macros.h"

#define LSIZE		2000		/* size for input line buffer */
#define	MAXARGS		50		/* max. # of arguments per line */

#define DEFAULT_OUTPUT_RESISTANCE 500	/* output resistance used if no info given */

public	nptr   *VDD_node;		/* power supply nodes */
public	nptr   GND_node;
public  char   *simprefix = NULL;	/* if non-NULL, prefix nodes with this */

public	lptr   on_trans;		/* always on transistors */

public	int    nnodes;			/* number of actual nodes */
public	int    naliases;		/* number of aliased nodes */
public	int    ntrans[ NTTYPES ];	/* number of txtors indexed by type */

public	lptr   freeLinks = NULL;	/* free list of Tlist structs */
public	tptr   freeTrans = NULL;	/* free list of Transistor structs */
public	nptr   freeNodes = NULL;	/* free list of Node structs */

public	tptr   tcap = NULL;		/* list of capacitor-transistors */

private	int    lineno;			/* current input file line number */
private	char   *simfname;		/* current input filename */
private	int    nerrs = 0;		/* # of erros found in sim file */
private	int    isBinFile;		/* TRUE binary file, FALSE sim file */

public	tptr   rd_tlist;		/* list of transistors just read */

public
#define	MIN_CAP		0.00001		/* minimum node capacitance (in pf) */


#define	WARNING		0
#define	WARN		1
#define	MAX_ERRS	20
#define	FATAL		(MAX_ERRS + 1)


#define	MIT	0
#define LBL	1
#define	SU	2

private short int simFormat = MIT, offsLBL = 0 ;

private	char    bad_argc_msg[] = "Wrong number of args for '%c' (%d)\n";

#define	BAD_ARGC( CMD, ARGC, ARGV )			\
  {							\
    rsimerror( simfname, lineno, bad_argc_msg, CMD, ARGC );	\
    PrArgs( ARGC, ARGV );				\
    CheckErrs( WARN );					\
    return;						\
  }							\


private void PrArgs( argc, argv )
  int   argc;
  char  *argv[];
  {
    while( argc-- != 0 )
      (void) fprintf( stderr, "%s ", *argv++ );
    (void) fputs( "\n", stderr );
  }


private void CheckErrs( n )
  {
    nerrs += n;
    if( nerrs > MAX_ERRS )
      {
	if( n != FATAL )
	    (void) fprintf( stderr, "Too many errors in sim file <%s>\n",
	      simfname );
	exit( 1 );
      }
  }



/*
 * Traverse the transistor list and add the node connection-list.  We have
 * to be careful with ALIASed nodes.  Note that transistors with source/drain
 * connected VDD and GND nodes are not linked.
 */
private nptr connect_txtors()
  {
    register tptr  t, tnext;
    register nptr  gate, src, drn, nd_list;
    register int   type;
    register long  visited;

    visited = VISITED;
    nd_list = NULL;

    for( t = rd_tlist; t != NULL; t = tnext )
      {
	tnext = t->scache.t;
	for( gate = t->gate; gate->nflags & ALIAS; gate = gate->nlink);
	for( src = t->source; src->nflags & ALIAS; src = src->nlink );
	for( drn = t->drain; drn->nflags & ALIAS; drn = drn->nlink );

	t->gate = gate;
	t->source = src;
	t->drain = drn;
	
	/* if there are no devices in the design, connect by ttype instead */
	if( device_names == NULL )
          {
	    type = t->ttype;
	  }
	else
	  {	
	    type = device_names[t->ttype]->devtype;
	  }
	t->state = ( (type == DEP) || (type == RESIST) ) ? WEAK : UNKNOWN;
	t->tflags = 0;

	ntrans[ t->ttype ]++;
	if( src == drn or (src->nflags & drn->nflags & POWER_RAIL) )
	  {
	    t->flags |= TCAP;		/* transistor is just a capacitor */
	    LINK_TCAP( t );
	  }
	else
	  {
	    /* do not connect gate if ALWAYSON since they do not matter. */
	    if( (type == DEP) || (type == RESIST) )
	      {
		CONNECT( on_trans, t );
	      }
	    else
	      {
		CONNECT( t->gate->ngate, t );
	      }

	    /* Devices connected to the power rail are handled separately.  */
	    /* However, resistors tied to the power rails will not get	    */
	    /* updated otherwise, so makes those exceptions.		    */
	    if( not (src->nflags & POWER_RAIL) )
	      {
		CONNECT( src->nterm, t );
		LINK_TO_LIST( src, nd_list, visited );
	      }
	    else if( t->ttype == RESIST )
	      {
		CONNECT( t->gate->ngate, t );
	      }
	    if( not (drn->nflags & POWER_RAIL) )
	      {
		CONNECT( drn->nterm, t );
		LINK_TO_LIST( drn, nd_list, visited );
	      }
	    else if( t->ttype == RESIST )
	      {
		CONNECT( t->gate->ngate, t );
	      }
	  }
      }

    rd_tlist = NULL;	/* Reset the list */
    return( nd_list );
  }


/*
 * node area and perimeter info (N sim command).
 */
private void node_info( targc, targv )
  int   targc;
  char  *targv[];
  {
    register nptr  n;

    if( targc != 8 )
	BAD_ARGC( 'N', targc, targv );

    n = RsimGetNode( targv[1] );

    n->ncap += 	atof( targv[4] ) * (CMA * LAMBDA2) +
		atof( targv[5] ) * (CPA * LAMBDA2) +
		atof( targv[6] ) * (CDA * LAMBDA2) +
		atof( targv[7] ) * 2.0 * (CDP * LAMBDA);
  }


/*
 * new format node area and perimeter info (M sim command).
 */
private void nnode_info( targc, targv )
  int   targc;
  char  *targv[];
  {
    register nptr  n;

    if( targc != 14 )
	BAD_ARGC( 'M', targc, targv );

    n = RsimGetNode( targv[1] );

    n->ncap +=	atof( targv[4] ) * (CM2A * LAMBDA2) +
		atof( targv[5] ) * 2.0 * (CM2P * LAMBDA) +
		atof( targv[6] ) * (CMA * LAMBDA2) +
		atof( targv[7] ) * 2.0 * (CMP * LAMBDA) +
		atof( targv[8] ) * (CPA * LAMBDA2) +
		atof( targv[9] ) * 2.0 * (CPP * LAMBDA) +
		atof( targv[10] ) * (CDA * LAMBDA) +
		atof( targv[11] ) * 2.0 * (CDP * LAMBDA) +
		atof( targv[12] ) * (CPDA * LAMBDA2) +
		atof( targv[13] ) * 2.0 * (CPDP * LAMBDA);
  }

private	int    AP_error = FALSE;

/*------------------------------------------------------*/
/* parse g=...,A_NUM,P_NUM attributes			*/
/* return values (area, perimeter) are in integer	*/
/* lambda^2 and lambda, respectively.			*/
/*------------------------------------------------------*/

private int parseAttr(str, a, p)
char *str;
int *a, *p;
{
   int l;
   char *s;

   if ( (l=strlen(str)) <= 2 ) {
   	*a = 0 ; 
   	*p = 0;
   	return FALSE;
   }

   for ( s = str+l*sizeof(char) ; *s != 'A' && s != str ; s--) ;
   if ( sscanf(s,"A_%d,P_%d", a, p ) != 2) 
    if ( sscanf(s,"a_%d,p_%d", a, p ) != 2) {
      rsimerror( simfname, lineno, "Bad area/perimeter attributes\n");
      return FALSE;
    }
   return TRUE;
}

/*--------------------------------------------------------------*/
/* Convert resistance in metric units to internal units (ohms).	*/
/* Default (unspecified) values are treated as ohms.		*/
/*--------------------------------------------------------------*/

private float rconvert( resstring )
  char *resstring;
{
   /* Convert string to floating-point, and stop at the	*/
   /* first non-numeric character.			*/

   double rval;
   char *pfix;

   rval = strtod((const char *)resstring, &pfix);

   while (*pfix != '\0' && isspace(*pfix)) pfix++;

   /* If no metric suffix is given, then interpret the answer	*/
   /* as ohms.  The return value is in ohms.			*/

   switch (*pfix) {
      case 'o': case '\0': break;			/* ohms */
      case 'k': case 'K': rval *= 1.0e3; break;		/* kiloOhms */
      case 'M': rval *= 1.0e6; break;			/* megaOhms */
      case 'G': rval *= 1.0e9; break;			/* gigaOhms */
      default:
	 rsimerror( simfname, lineno,
	    "Unknown resistor value suffix %s, assuming ohms\n", pfix);
	 break;
   }
   
   return (float)rval;
}

/*--------------------------------------------------------------*/
/* Convert length values (length, width, x, y) in metric units	*/
/* to internal units.  Default (unspecified) values are treated	*/
/* as lambda, using the lambda conversion factor.		*/
/* Return values are in centimicrons.  LAMBDA units are		*/
/* converted to centimicrons using the LAMBDACM factor.		*/
/*--------------------------------------------------------------*/

private float lconvert( lstring )
  char *lstring;
{
   /* Convert string to floating-point, and stop at the	*/
   /* first non-numeric character.			*/

   double lval;
   char *pfix;

   lval = strtod((const char *)lstring, &pfix);

   while (*pfix != '\0' && isspace(*pfix)) pfix++;

   /* If no metric suffix is given, then interpret the answer	*/
   /* as lambda.  The return value is in centimicrons.		*/

   switch (*pfix) {
      case 'n': case 'N': lval *= 0.1;   break;		/* nanometers */
      case 'u': case 'U': lval *= 100;   break;		/* microns */
      case 'm': case 'M': lval *= 1.0e5; break;		/* millimeters */
      case '\0': case 'l': lval *= LAMBDACM; break;	/* lambda */
      default:
	 rsimerror( simfname, lineno,
	    "Unknown length measure suffix %s, assuming lambda\n", pfix);
	 break;
   }
   return (float)lval;
}

/*
 * new transistor.  Implant specifies type.
 * AreaPos specifies the argument number that contains the area (if any).
 */
private void newtrans( implant, devidx, targc, targv )
  int   implant;
  int	devidx;
  int   targc;
  char  *targv[];
  {
    nptr           gate, src, drn;
    long           x, y;
    int		   width, length;
    double         cap;
    register tptr  t;
    unsigned int fetHasAP = FALSE ;
    unsigned int asrc, adrn, psrc, pdrn;

    if( implant == RESIST )
      {
	if( targc != 4 )
	    BAD_ARGC( 'r', targc, targv );

	gate = *VDD_node;
	src = RsimGetNode( targv[1] );
	drn = RsimGetNode( targv[2] );

	length = rconvert( targv[3] ) * LAMBDACM;
	width = 0;
      }
    else
      {
	if( targc < 4+offsLBL or targc > 11+offsLBL )
	    BAD_ARGC( targv[0][0], targc, targv );

	gate = RsimGetNode( targv[1] );
	src = RsimGetNode( targv[2] );
	drn = RsimGetNode( targv[3] );

	if( targc > 5+offsLBL )
	  {
	    length = (int)lconvert(targv[4 + offsLBL]);
	    width = (int)lconvert(targv[5 + offsLBL]);
	    if (width <= 0 or length <= 0)
	      {
		rsimerror( simfname, lineno,
		  "Bad transistor width=%d or length=%d\n", width, length );
		return;
	      }
	    if (targc > 7+offsLBL)
	      {
		x = (int)lconvert(targv[6+offsLBL]);
		y = (int)lconvert(targv[7+offsLBL]);
	      }
	  }
	else
	    width = length = (int)(2 * LAMBDACM);

	/* Compute capacitance in picoFarads.		*/
	/* CTGA is in pF/centimicron^2			*/
	/* length and width are in centimicrons		*/

	cap = (double)(length * width * CTGA);
      }

    NEW_TRANS( t );			/* create new transistor */

    t->ttype = implant;
    t->gate = gate;
    t->source = src;
    t->drain = drn;

    if( targc > 7+offsLBL )
      {
	t->x.pos = x;
	t->y.pos = y;
	EnterPos( t, TRUE );		/* Enter transistor position */
	if ( simFormat == SU && targc >= 9 ) { /* parse area and perimeter */
	  register int i ;

	  for ( fetHasAP = TRUE, i = 8 ; i < targc ; i++ ) {
	    if ( targv[i][0] == 's' )
	       fetHasAP = fetHasAP && parseAttr(targv[i], &asrc, &psrc);
	    else if ( targv[i][0] == 'd' )
	       fetHasAP = fetHasAP && parseAttr(targv[i], &adrn, &pdrn);
	   }
	}
      }
    else {
	EnterPos( t, FALSE );		/* Enter transistor position */
	if ( simFormat == SU && ! AP_error ) {
		rsimerror(  simfname, lineno, "no area/perim S/D attributes on fet\n");
		AP_error = TRUE;
	 }
	}

    t->scache.t = rd_tlist;		/* link it to the list */
    rd_tlist = t;

    t->r = requiv( devidx, width, length );

		/* update node capacitances  */
    gate->ncap += cap;

    if ( simFormat == SU ) {
       double capsrc = 0, capdrn =0 ;

       if ( fetHasAP ) {
          if ( implant == PCHAN ) {
	    capsrc = asrc * LAMBDA2 * CPDA + psrc * LAMBDA *CPDP;
	    capdrn = adrn * LAMBDA2 * CPDA + pdrn * LAMBDA *CPDP;
	   } 
	  else if ( implant == NCHAN || implant == DEP ) {
	    capsrc = asrc * LAMBDA2 * CDA + psrc * LAMBDA *CDP;
	    capdrn = adrn * LAMBDA2 * CDA + pdrn * LAMBDA *CDP;
          } 
	} 
       else if ( ! AP_error  ) {
	 lprintf( stderr, "Warning: Junction capacitances might be incorrect\n");
	 AP_error = TRUE;
       }
#ifdef NOTDEF
printf("LAMBDA=%f CDA=%f CDP=%f asrc=%d psrc=%d adrn=%d pdrn=%d capsrc=%lf capdrn=%lf\n", LAMBDA, CDA, CDP, asrc, psrc, adrn, pdrn, capsrc, capdrn);
#endif
       src->ncap += capsrc;
       drn->ncap += capdrn;
     }
    else if( config_flags & TDIFFCAP ) 
      {
	if ( implant == PCHAN ) {
		cap = CPTDW * width + CPTDE;
	}
	else if ( implant == NCHAN || implant == DEP )
		cap = CTDW * width + CTDE;
	else	cap = 0.0 ;
	src->ncap += cap;
	drn->ncap += cap;
      }
  }

/*
 * accept a bunch of aliases for a node (= sim command).
 */
public void alias( targc, targv )
  int   targc;
  char  *targv[];
  {
    register nptr  n, m;
    register int   i;

    if( targc < 3 )
	BAD_ARGC( '=', targc, targv );

    n = RsimGetNode( targv[1] );

    for( i = 2; i < targc; i++ )
      {
	m = RsimGetNode( targv[i] );
	if( m == n )
	    continue;

	if( m->nflags & POWER_RAIL )
	    SWAP_NODES( m, n );

	if( m->nflags & POWER_RAIL )
	  {
	    // rsimerror( simfname, lineno, "Can't alias the power supplies\n" );
	    // continue;
	  }

	n->ncap += m->ncap;

	m->nlink = n;
	m->nflags |= ALIAS;
	m->ncap = 0.0;
	nnodes--;
	naliases++;
      }
  }


/*
 * node threshold voltages (t sim command).
 */
private void nthresh( targc, targv )
  int   targc;
  char  *targv[];
  {
    register nptr  n;

    if( targc != 4 )
	BAD_ARGC( 't', targc, targv );

    n = RsimGetNode( targv[1] );
    n->vlow = atof( targv[2] );
    n->vhigh = atof( targv[3] );
  }


/*
 * User delay for a node (D sim command).
 */
private void ndelay( targc, targv )
  int   targc;
  char  *targv[];
  {
    register nptr  n;

    if( targc != 4 )
	BAD_ARGC( 'D', targc, targv );

    n = RsimGetNode( targv[1] );
    n->nflags |= USERDELAY;
    n->tplh = ns2d( atof( targv[2] ) );
    n->tphl = ns2d( atof( targv[3] ) );
  }

private float cconvert( capstring )
  char *capstring;
{
   /* Convert string to floating-point, and stop at the	*/
   /* first non-numeric character.			*/

   double cval;
   char *pfix;

   cval = strtod((const char *)capstring, &pfix);

   while (*pfix != '\0' && isspace(*pfix)) pfix++;

   /* Convert metric suffixes to pF.  If no suffix is given, interprets  */
   /* the value as the default femtoFarads.  Return value is picoFarads. */

   switch (*pfix) {
      case 'm': cval *= 1.0e9;  break;				/* milliFarads */
      case 'u': case 'U': cval *= 1.0e6;  break;		/* microFarads */
      case 'n': case 'N': cval *= 1.0e3; break;			/* nanoFarads  */
      case 'p': case 'P': break;				/* picoFarads  */
      case 'f': case 'F': case '\0': cval *= 1.0e-3; break;	/* femtoFarads */
      case 'a': case 'A': cval *= 1.0e-6; break;		/* attoFarads  */
      default:
	 rsimerror( simfname, lineno,
	    "Unknown capacitance value suffix %s, assuming femtoFarads\n", pfix);
	 break;
   }
   return (float)cval;
}

/*
 * add capacitance to a node (c sim command).
 */
private void ncap( targc, targv )
  int   targc;
  char  *targv[];
  {
    register nptr n, m;
    float         cap;

    if( targc == 3 )
      {
	n = RsimGetNode( targv[1] );
	n->ncap += cconvert( targv[2] );
      }
    else if( targc == 4 )		/* two terminal caps	*/
      {
	cap = cconvert( targv[3] );
	n = RsimGetNode( targv[1] );
	m = RsimGetNode( targv[2] );
	if( n != m )			/* add cap to both nodes */
	  {
	    if( m != GND_node )	m->ncap += cap;
	    if( n != GND_node )	n->ncap += cap;
	  }
	else if( n == GND_node )	/* same node, only GND makes sense */
	    n->ncap += cap;
      }
    else
	BAD_ARGC( 'c', targc, targv );
  }

/*
 * new subcircuit device.  Determine from the device_list record what
 * kind of a device this is and call the appropriate procedure.
 */
private void newdevice( targc, targv )
  int   targc;
  char  *targv[];
  {
    static int lastn = -1;
    static int lastp = -1;
    int i, idx = -1, nargc, type;
    char **nargv;
    int length, width;
    double value;
    char svalue[128];

    /* Normally, the vast majority of devices in a .sim file will be
     * one type of nFET and one type of pFET, so record the indexes
     * in lastn and lastp and check those records first, for
     * efficiency.
     */
    if (lastn >= 0)
	if (!strcmp(targv[targc - 1], device_names[lastn]->devname))
	    idx = lastn;

    if ((idx == -1) && (lastp >= 0))
	if (!strcmp(targv[targc - 1], device_names[lastp]->devname))
	    idx = lastp;

    if (idx == -1)
    {
        for (i = 0; device_names[i] != NULL; i++) 
	{
	    if ((i == lastn) || (i == lastp)) continue;
	    if (!strcmp(targv[targc - 1], device_names[i]->devname))
	    {
		idx = i;
		break;
	    }
	}
    }

#ifdef USER_SUBCKT
    if (idx == -1)
    {
	newsubckt(targc, targv);
	return;
    }
#endif

    if (idx == -1)
    {
	rsimerror( simfname, lineno, "Unknown device name." );
	return;
    }

    /* The following expects that all transistors are 4-terminal devices
     * (SPICE-compatible).  Other devices may or may not have a substrate
     * terminal (both ways are SPICE-compatible);  the substrate terminal
     * for these devices is ignored.  For nFET and pFET transistors, the
     * substrate is ignored but is used to set the power and ground net
     * names.
     */

    type = device_names[idx]->devtype;
    switch (type)
    {
	case NFET:
	    lastn = idx;
	    if (targc >= 10)
	    {
		nargc = targc - 2;
		nargv = (char **)malloc(nargc * sizeof(char *));
		nargv[0] = targv[0];
		nargv[1] = targv[1];    /* Gate */
		nargv[2] = targv[2];    /* Source */
		nargv[3] = targv[3];    /* Drain */
		nargv[4] = targv[targc - 5] + 2;    /* Length */
		nargv[5] = targv[targc - 4] + 2;    /* Width */
		nargv[6] = targv[targc - 3] + 2;    /* X position */
		nargv[7] = targv[targc - 2] + 2;    /* Y position */
		newtrans(NCHAN, idx, nargc, nargv);
		free(nargv);
	    }
	    else
		rsimerror( simfname, lineno, "Incorrect number of arguments "
			" for nFET subcircuit device." );
	    break;

	case PFET:
	    lastp = idx;
	    if (targc >= 10)
	    {
		nargc = targc - 2;
		nargv = (char **)malloc(nargc * sizeof(char *));
		nargv[0] = targv[0];
		nargv[1] = targv[1];    /* Gate */
		nargv[2] = targv[2];    /* Source */
		nargv[3] = targv[3];    /* Drain */
		nargv[4] = targv[targc - 5] + 2;    /* Length */
		nargv[5] = targv[targc - 4] + 2;    /* Width */
		nargv[6] = targv[targc - 3] + 2;    /* X position */
		nargv[7] = targv[targc - 2] + 2;    /* Y position */
		newtrans(PCHAN, idx, nargc, nargv);
		free(nargv);
	    }
	    else
		rsimerror( simfname, lineno, "Incorrect number of arguments "
			" for nFET subcircuit device." );
	    break;

	case CAPACITOR:
	    if (targc >= 8)
	    {
		nargc = 4;
		nargv = (char **)malloc(nargc * sizeof(char *));
		nargv[0] = targv[0];
		nargv[1] = targv[1];    /* Top */
		nargv[2] = targv[2];    /* Bottom */

		length = rconvert( targv[targc - 5] + 2 ) * LAMBDACM;
		width = rconvert( targv[targc - 4] + 2 ) * LAMBDACM;
		value = (double)(length * width) * device_names[idx]->devvalue;
		sprintf(svalue, "%g", value);
		nargv[3] = svalue;
		ncap(nargc, nargv);
		free(nargv);
	    }
	    else
		rsimerror( simfname, lineno, "Incorrect number of arguments "
			" for capacitor subcircuit device." );
	    break;

	case RESISTOR:
	    if (targc >= 8)
	    {
		nargc = 4;
		nargv = (char **)malloc(nargc * sizeof(char *));
		nargv[0] = targv[0];
		nargv[1] = targv[1];    /* Terminal 1 */
		nargv[2] = targv[2];    /* Terminal 2 */

		length = rconvert( targv[targc - 5] + 2 ) * LAMBDACM;
		width = rconvert( targv[targc - 4] + 2 ) * LAMBDACM;
		/* Compute resistance argument from device length and width
		 * and the resistance per square value in the parameter file.
		 */
		value = ((double)length / (double)width)
			    * device_names[idx]->devvalue;
		sprintf(svalue, "%g", value);
		nargv[3] = svalue;
		newtrans(RESIST, idx, targc, targv);
		free(nargv);
	    }
	    else
		rsimerror( simfname, lineno, "Incorrect number of arguments "
			" for capacitor subcircuit device." );
	    break;

	case DIODE:
	    if (targc >= 8)
	    {
		nargc = 4;
		nargv = (char **)malloc(nargc * sizeof(char *));
		nargv[0] = targv[0];
		nargv[1] = targv[1];    /* Cathode */
		nargv[2] = targv[2];    /* Anode */

		/* Treat the diode as an equivalent capacitance.
		 * Compute capacitance argument from device area
		 * and the capacitance per unit area in the parameter file.
		 */
		length = rconvert( targv[targc - 5] + 2 ) * LAMBDACM;
		width = rconvert( targv[targc - 4] + 2 ) * LAMBDACM;
		value = (double)(length * width) * device_names[idx]->devvalue;
		sprintf(svalue, "%g", value);
		nargv[3] = svalue;
		ncap(nargc, nargv);
		free(nargv);
	    }
	    else
		rsimerror( simfname, lineno, "Incorrect number of arguments "
			" for capacitor subcircuit device." );
	    break;
    }
  }


/*
 * parse input line into tokens, filling up carg and setting targc
 */
private int parse_line( line, carg )
  register char  *line;
  register char  **carg;
  {
    register char  ch;
    register int   targc;

    targc = 0;
    while( ch = *line++ )
      {
	if( ch <= ' ' )
	    continue;
	targc++;
	*carg++ = line - 1;
	while( *line > ' ' )
	    line++;
	if( *line )
	    *line++ = '\0';
      }
    *carg = 0;
    return( targc );
  }


private	int    R_error = FALSE;
private	int    A_error = FALSE;

private int input_sim (simfile, has_param_file)
  char  *simfile;
  int   has_param_file;
{
    FILE  *fin;
    char  line[LSIZE];
    char  *targv[MAXARGS];	/* tokens on current command line */
    int   targc;		/* number of args on command line */
    long  offset;		/* state of previously opened file */
    int   olineno;

    /* Test for file with and without extension */

    if ((fin = fopen(simfile, "r")) == NULL)
    {
	char *tmpfname = (char *)malloc(5 + strlen(simfile));

	strcpy(tmpfname, simfile);
	strcat(tmpfname, ".sim");
	fin = fopen(tmpfname, "r");
	free(tmpfname);
	if (fin == NULL)
	{
	    lprintf( stderr, "cannot open '%s' for sim input\n", simfile );
	    return FALSE;
	}
    }
    simfname = simfile;
    lineno = 0;

    while (fgetline(line, LSIZE, fin) != NULL)
    {
	lineno++;
	if ((lineno > 1) && (has_param_file < 0))
	{
	    /* Attempt to load a default config file */
	    has_param_file = config("scmos100");
	    if (has_param_file < 0)
	    {
	        lprintf( stderr, "No prm file specified and unable to load default!\n");
		return FALSE;
	    }
	    else 
	        lprintf( stderr, "config file unknown; using default scmos100.prm\n");
	}
	targc = parse_line(line, targv);

	if (targv[0] == NULL)
	    continue;

	switch (targv[0][0])
	{
	    case '@':
		if (targc != 2)
		{
		    rsimerror(simfname, lineno, bad_argc_msg, '@', targc);
		    CheckErrs(WARN);
		    break;
		}
		offset = ftell(fin);
		olineno = lineno;
		(void) fclose(fin);
		(void) input_sim(targv[1]);
		if ((fin = fopen(simfile, "r")) == NULL)
		{
		    rsimerror(simfname, lineno, "can't re-open sim file '%s'\n",
				simfile);
		    CheckErrs(WARN);
		    return FALSE;
		}
		(void) fseek(fin, offset, 0);
		simfname = simfile;
		lineno = olineno;
		break;

	    case '|':
		if (lineno > 1) break;
		if (targc > 2) {
		    double lmbd = atof(targv[2]) / 100.0;

		    if ((targc > 4) && (has_param_file < 0))
		    {
			char *prmname = malloc(strlen(targv[2]) + strlen(targv[4]) + 1);

			sprintf(prmname, "%s%s", targv[4], targv[2]);
			has_param_file = config( prmname );
			free(prmname);
			if (has_param_file < 0)
			{
			    lprintf(stderr, "Could not load parameter file %s%s.prm\n",
					targv[4], targv[2]);
			    return FALSE;
			}
			else
			    lprintf(stdout, "Parameter file %s%s.prm determined from "
					"header line\n", targv[4], targv[2]);
		    }

		    if (lmbd != LAMBDA)
		    {
			/* Only flag a warning if the difference is	*/
			/* more than is accounted for by roundoff error	*/

			if (fabs(lmbd - LAMBDA)/LAMBDA > 0.01)
			{
			    rsimerror(simfname, lineno, "WARNING: sim "
				"file lambda (%g) != config lambda"
				" (%g)\n", lmbd, LAMBDA);
			    rsimerror( simfname, lineno, "WARNING: Using "
				"the config lambda (%g)\n", LAMBDA);
			}
		    }
		}
		if (targc > 6)
		{
		    if (!strcasecmp(targv[6], "MIT")) simFormat = MIT;
		    else if (!strcasecmp(targv[6], "LBL")) simFormat = LBL;
		    else if (!strcasecmp(targv[6], "SU")) simFormat = SU;
		    else {
		        rsimerror(simfname, lineno, "Unknown format '%s' "
				"assuming MIT\n", targv[6]);
		        simFormat = MIT ;
		    }
	            offsLBL = (simFormat == LBL) ? 1 : 0;  /* skip bulk */
		    if (simFormat == SU)
		    {
			if (CDA == 0.0 || CDP == 0.0 || CPDA == 0.0 || CPDP == 0.0)
			    lprintf(stderr, "Warning: SU format and area/perim"
					" cap values are zero\n");
			else
			    lprintf(stderr, "info: SU format --> using S/D attrs"
					" to compute junction parasitics\n");
		    }
		} 
		break;

	    case 'e':
	    case 'n':
		newtrans(NCHAN, 0, targc, targv);
		break;

	    case 'p':
		newtrans(PCHAN, 1, targc, targv);
		break;

	    case 'd':
		newtrans(DEP, 0, targc, targv);
		break;

	    case 'r':
		newtrans(RESIST, 2, targc, targv);
		break;

	    case 'x':
		newdevice(targc, targv);
		break;
	    case 'N':
		node_info(targc, targv);
		break;

	    case 'M':
		nnode_info(targc, targv);
		break;

	    case 'c':
	    case 'C':
	    	ncap(targc, targv);
		break;

	    case '=':
	    	alias(targc, targv);
		break;

	    case 't':
	    	nthresh(targc, targv);
		break;

	    case 'D': 
	    	ndelay(targc, targv);
		break;

	    case 'R': 
		if (!R_error)	/* only warn about this 1 time */
		{
		    lprintf(stderr,
			"%s: Ignoring lumped-resistance ('R' construct)\n",
			simfname);
		    R_error = TRUE;
		}
		break;
	    case 'A':
		if (!A_error)	/* only warn about this 1 time */
		{
		    lprintf(stderr,
			"%s: Ignoring attribute-line ('A' construct)\n",
			simfname);
		    A_error = TRUE;
		}
		break;

	    case '<': 
		if (lineno == 1 && rd_netfile(fin, line))
		{
		    (void) fclose(fin);
		    return TRUE;
		}
		/* fall through if rd_netfile returns FALSE */

	    default:
		rsimerror(simfname, lineno, "Unrecognized input line (%s)\n",
		  targv[0]);
		CheckErrs(WARN);
	}
    }
    (void) fclose (fin);
    lprintf(stdout, "\nRead %s lambda:%.2lfu format:%s\n", simfile, LAMBDA, 
			simFormat == MIT ? "MIT" : ((simFormat == LBL) ?
			"LBL" : "SU"));
    return FALSE;
}



private void init_counts()
{
    register int  i;

    for( i = 0; i < NTTYPES; i++ )
	ntrans[i] = 0;
    nnodes = naliases = 0;
}


public int rd_network( simfile, prefix, has_param_file )
  char  *simfile;
  char  *prefix;
  int   has_param_file;
  {
    static int      firstcall = 1;

    if( firstcall )
      {
	rd_tlist = NULL;
	init_counts();
	init_listTbl();

	if( power_net_name == NULL )
	  {
	    lprintf( stdout, "Using default name \"Vdd\" for power net.\n" );
	    power_net_name = (char **)malloc( sizeof(char*) * power_net_name_size );
	    *power_net_name = strdup( "Vdd" );
	    *( power_net_name + 1 ) = NULL;
	  }
	if( VDD_node == NULL ) 
	  {
	    VDD_node = (nptr*)malloc( 2 * sizeof(nptr) );
	    *( VDD_node + 1 ) = NULL;
	  }
	*VDD_node = RsimGetNode( power_net_name );
	(*VDD_node)->npot = HIGH;
	(*VDD_node)->nflags |= (INPUT | POWER_RAIL);
	(*VDD_node)->head.inp = 1;
	(*VDD_node)->head.val = HIGH;
	(*VDD_node)->head.punt = 0;
	(*VDD_node)->head.time = 0;
	(*VDD_node)->head.t.r.rtime = (*VDD_node)->head.t.r.delay = 0;
	(*VDD_node)->head.next = last_hist;
	(*VDD_node)->curr = &((*VDD_node)->head);

	if (ground_net_name == NULL)
	  {
	    lprintf(stdout, "Using default name \"Gnd\" for ground net.\n");
	    ground_net_name = strdup( "Gnd" );
	  }

	GND_node = RsimGetNode( ground_net_name );
	GND_node->npot = LOW;
	GND_node->nflags |= (INPUT | POWER_RAIL);
	GND_node->head.inp = 1;
	GND_node->head.val = LOW;
	GND_node->head.punt = 0;
	GND_node->head.time = 0;
	GND_node->head.t.r.rtime = GND_node->head.t.r.delay = 0;
	GND_node->head.next = last_hist;
	GND_node->curr = &(GND_node->head);

	NEW_TRANS( tcap );
	tcap->scache.t = tcap->dcache.t = tcap;
	tcap->x.pos = 0;

	firstcall = 0;
      }
    nerrs = 0;

    simprefix = prefix;
    isBinFile = input_sim( simfile, has_param_file );
    simprefix = (char *)NULL;

    if (nerrs > 0)
#ifndef TCL_IRSIM
	exit( 1 );
#else
    {
	lprintf(stderr, "Errors occurred on reading input file %s\n", simfile);
	return 1;
    }
    else
	return 0;
#endif
  }


public void pTotalNodes()
  {
    lprintf( stdout, "%d nodes", nnodes );
    if( naliases != 0 )
	lprintf( stdout, ", %d aliases", naliases );
    lprintf( stdout, "; " );
  }


public void pTotalTxtors()
  {
    int  i;

    lprintf( stdout, "transistors:" );
    for( i = 0; i < NTTYPES; i++ )
	if( ntrans[i] != 0 )
	    lprintf( stdout, " %s=%d", ttype[i], ntrans[i] );
    if( tcap->x.pos != 0 )
	lprintf( stdout, " shorted=%d", tcap->x.pos );
    lprintf( stdout, "\n" );
  }


public void ConnectNetwork()
  {
    nptr  ndlist;

    pTotalNodes();

    ndlist = ( isBinFile ) ? bin_connect_txtors() : connect_txtors();

    make_parallel( ndlist );
    make_stacks( ndlist );

    pTotalTxtors();
    pParallelTxtors();
    pStackedTxtors();
  }
