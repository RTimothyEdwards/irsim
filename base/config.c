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
#include <math.h>

#include "defs.h"
#include "net.h"
#include "globals.h"


/*
 * electrical parameters used for deriving capacitance info for charge
 * sharing.  Default values aren't for any particular process, but are
 * self-consistent.
 *	Area capacitances are all in pfarads/sq-micron units.
 *	Perimeter capacitances are all in pfarads/micron units.
 */
public	double  CM2A = .00000;	/* 2nd metal capacitance -- area */
public	double  CM2P = .00000;	/* 2nd metal capacitance -- perimeter */
public	double  CMA = .00003;	/* 1st metal capacitance -- area */
public	double  CMP = .00000;	/* 1st metal capacitance -- perimeter */
public	double  CPA = .00004;	/* poly capacitance -- area */
public	double  CPP = .00000;	/* poly capacitance -- perimeter */
public	double  CDA = .00010;	/* n-diffusion capacitance -- area */
public	double  CDP = .00060;	/* n-diffusion capacitance -- perimeter */
public	double  CPDA = .00010;	/* p-diffusion capacitance -- area */
public	double  CPDP = .00060;	/* p-diffusion capacitance -- perimeter */
public	double  CGA = .00040;	/* gate capacitance -- area */
				/* the following are computed from above */
public	double	CTDW;		/* xtor diff-width capacitance -- perimeter */
public	double	CPTDW;		
public	double	CTDE;		/* xtor diff-extension cap. -- perimeter */
public	double	CPTDE;	
public	double	CTGA;		/* xtor gate capacitance -- area */

public	double  LAMBDA = 2.5;	  /* microns/lambda */
public	double  LAMBDA2 = 6.25;	  /* LAMBDA**2 */
public	double	LAMBDACM = 250;	  /* centi-microns/lambda */
public	double  LOWTHRESH = 0.3;  /* low voltage threshold, normalized units */
public	double  HIGHTHRESH = 0.8; /* high voltage threshold,normalized units */
public	double  DIFFEXT = 0;	  /* width of source/drain diffusion */

public	int	config_flags = 0;

public  DevRec	**device_names;    /* Subcircuit-to-device naming */

public  long    nttypes = 0;       /* Number of transistor types */

/* values of config_flags */

public
#define	TDIFFCAP	0x01	/* set if DIFFPERIM or DIFFEXTF are true    */

#define	CNTPULLUP	0x02	/* set if capacitance from gate of pullup   */
				/* should be included.			    */

#define	DIFFPERIM	0x04	/* set if diffusion perimeter does not 	    */
				/* include sources/drains of transistors.   */

#define	SUBPAREA	0x08	/* set if poly over xistor doesn't make a   */
				/* capacitor.				    */

#define	DIFFEXTF	0x10	/* set if we should add capacitance due to  */
				/* diffusion-extension of source/drain.	    */

#define CONFIG_LOADED	0x20	/* set if the config file has been loaded.  */


typedef struct	/* table translating parameters to its associated names */
  {
    char      *name;
    int       flag;
    double    *dptr;
  } pTable;

private	pTable  parms[] = 
  {
    "capm2a",	    0x0,	&CM2A,
    "capm2p",	    0x0,	&CM2P,
    "capma",	    0x0,	&CMA,
    "capmp",	    0x0,	&CMP,
    "cappa",	    0x0,	&CPA,
    "cappp",	    0x0,	&CPP,
    "capda",	    0x0,	&CDA,
    "capdp",	    0x0,	&CDP,
    "cappda",	    0x0,	&CPDA,
    "cappdp",	    0x0,	&CPDP,
    "capga",	    0x0,	&CGA,
    "lambda",	    0x0,	&LAMBDA,
    "lowthresh",    0x0,	&LOWTHRESH,
    "highthresh",   0x0,	&HIGHTHRESH,
    "diffperim",    DIFFPERIM,	NULL,
    "cntpullup",    CNTPULLUP,	NULL,
    "subparea",	    SUBPAREA,	NULL,
    "diffext",	    DIFFEXTF,	&DIFFEXT,
    NULL,	    0x0,	NULL
  };


#define	LSIZE		500	/* max size of parameter file input line */
#define	MAXARGS		10	/* max number of arguments in line */

private	int     lineno;		/* current line number */
private	char    *currfile;	/* current input file */
private	int     nerrs;		/* errors found in config file */
private	int     maxerr;
private	char	*ttype_drop[ NTTYPES ];

	/* forward references */
private	void    insert();
private	void    makedevice();


private int ParseLine( line, args )
  register char  *line;
  char           **args;
  {
    register char  c;
    int            ac = 0;

    for( ; ; )
      {
	while( (c = *line) <= ' ' and c != '\0' )
	    line++;
	if( c == '\0' or c == ';' )
	    break;
	*args++ = line;
	ac++;
	while( (c = *line) > ' ' and c != ';' )
	    line++;
	*line = '\0';
	if( c == '\0' or c == ';' )
	    break;
	line++;
      }
    *line = '\0';
    *args = NULL;
    return( ac );
  }

/*
 * info on resistance vs. width and length are stored first sorted by
 * width, then by length.
 */
struct length
  {
    struct length    *next;	/* next element with same width */
    long             l;		/* length of this channel in centimicrons */
    double           r;		/* equivalent resistance/square */
  };

struct width
  {
    struct width     *next;	/* next width */
    long             w;		/* width of this channel in centimicrons */
    struct length    *list;	/* list of length structures */
  } *resistances[ R_TYPES ][ NTTYPES ];

/*
 * copy the linked list in 'from' to 'to'
 */
private void resist_copy( from, to )
  register struct width  **from;
  register struct width  **to;
  {
    register struct width   *p, *q, *wnew;
    register struct length  *l, *m, *lnew;

    if (from == NULL)
      {
 	 lprintf( stderr, "Warning: missing required default device resistance\n");
         return;
      }

    if (to == NULL)
      {
         lprintf(stderr, "Device has no resistances array!\n");
         return;
      }

    for( p = *from, q = NULL; p != NULL; q = wnew, p = p->next )
      {
	/* Allocate new width record */
	wnew = (struct width *) Valloc( sizeof( struct width ), 1 );
	wnew->next = NULL;
	wnew->w = p->w;
	if( q == NULL )
	    *to = wnew;
        else
	    q->next = wnew;

	/* Copy length records */
	for( l = p->list, m = NULL; l != NULL; m = lnew, l = l->next)
	  {
	    lnew = (struct length *) Valloc( sizeof( struct length ), 1 );
  	    lnew->next = NULL;
	    lnew->r = l->r;
	    lnew->l = l->l;
  	    if( m == NULL )
		wnew->list = lnew;
	    else
		m->next = lnew;
	  }	    
      }
  }

/* Read in the parameters (.prm) file.  Return 0 on success, -1 on failure */

public int config( cname )
  char  *cname;
  {
    register pTable  *p;
    FILE             *cfile;
    char             prm_file[256];
    char             line[LSIZE];
    char             *targv[MAXARGS];
    char             tmpbuff[NTTYPES * 22];
    int              targc;
    int		     i;

    nerrs = 0;

    for( targc = 0; targc < NTTYPES; targc++ )
      {
	ttype_drop[ targc ] = &tmpbuff[ targc * 22 ];
	(void) sprintf( ttype_drop[ targc ], "%s-with-drop", ttype[ targc ] );
      }

    if( *cname != '/' )		/* not full path specified */
      {
	Fstat  *stat;
	int	flag = TRUE ;
	
	stat = FileStatus( cname );
	if( not stat->read )
	  {
	    sprintf( prm_file, "%s/irsim/prm/%s", cad_lib, cname );
again:
	    stat = FileStatus( prm_file );
	    if( stat->read )
	        cname = prm_file;
	    else
	      {
		(void) strcat( prm_file, ".prm" );
		stat = FileStatus( prm_file );
		if( stat->read )
		    cname = prm_file;
		else if ( flag ) {
			flag = FALSE ;
	    		sprintf( prm_file, "%s/irsim/%s", cad_lib, cname );
			goto again;
		}
	      }
	  }
      }
    currfile = cname;

    lineno = 0;
    if( (cfile = fopen( cname, "r" )) == NULL )
      {
	lprintf(stderr,"can't open electrical parameters file <%s>\n", cname);
	return -1;
      }

    *line = '\0';
    fgetline(line, LSIZE, cfile);
    maxerr = 15;

    while (*line == ';') {
       if (fgetline(line, LSIZE, cfile) == NULL) {
          lprintf(stderr, "Unexpected end of file\n");
	  *line = '\0';
          break;
       }
       if ((*line == ';') && (strstr(line, "onfiguration") != NULL)) break;
    }
    if (*line != ';') {
       if (*line != '\0') lprintf(stderr, "Unexpected first line: %s\n", line);
       rewind(cfile);
       maxerr = 1;
    }

    /* Initialize table of devices */
    device_names = (DevRec **)malloc(sizeof(DevRec *));
    device_names[0] = (DevRec *)NULL;

    /* Insert default devices */
    makedevice("nfet", "n-channel", (float)0.0);
    makedevice("pfet", "p-channel", (float)0.0);
    makedevice("resistor", "resistor", (float)0.0);

    while( fgetline( line, LSIZE, cfile ) != NULL )
      {
	lineno++;
	targc = ParseLine( line, targv );
	if( targc == 0 )
	    continue;
	if( str_eql( "resistance", targv[0] ) == 0 )
	  {
	    if( targc >= 6 )
		insert( targv[1], targv[2], targv[3], targv[4], targv[5] );
	    else
	      {
		rsimerror( currfile, lineno, "syntax error in resistance spec\n" );
		nerrs++;
	      }
	    continue;
	  }
	else if( str_eql( "device", targv[0] ) == 0 )
	  {
	    if( targc >= 4 )
		makedevice( targv[1], targv[2], (float)atof(targv[3]) );
	    else if( targc >= 3 )
		makedevice( targv[1], targv[2], (float)0.0 );
	    else
	      {
		rsimerror( currfile, lineno, "syntax error in device\n" );
		nerrs++;
	      }
	    continue;
	  }
	else
	  {
	    for( p = parms; p->name != NULL; p++ )
	      {
		if( str_eql( p->name, targv[0] ) == 0 )
		  {
		    if( p->dptr != NULL )
			*(p->dptr) = atof( targv[1] );
		    if( p->flag != 0 and atof( targv[1] ) != 0.0 )
			config_flags |= p->flag;
		    break;
		  }
	      }
	    if( p->name == NULL )
	      {
		rsimerror( currfile, lineno,
		  "unknown electrical parameter: (%s)\n", targv[0] );
		nerrs++;
	      }
	  }
	if( nerrs >= maxerr )
	  {
	    if( maxerr == 1 )
		lprintf( stderr,
		  "I think %s is not an electrical parameters file\n", cname );
	    else
		lprintf( stderr, "Too many errors in '%s'\n", cname );
	    return -1;
	  }
      }
    LAMBDA2 = LAMBDA * LAMBDA;
    LAMBDACM = LAMBDA * CM_M;
    CTGA = ((config_flags & SUBPAREA) ? (CGA - CPA) : CGA) / (CM_M2);
    switch( config_flags & (DIFFEXTF | DIFFPERIM) )
      {
	case 0 :
	    CTDE = CTDW = 0.0; CPTDE = CPTDW = 0.0;	
	    break;
	case DIFFPERIM :
	    config_flags |= TDIFFCAP;
	    CTDE = CPTDE = 0.0;
	    CTDW = -(CDP / CM_M);
	    CPTDW = -(CPDP / CM_M);
	    break;
	case DIFFEXTF :
	    config_flags |= TDIFFCAP;
	    CTDE = (2 * DIFFEXT * LAMBDA * CDP);
	    CPTDE = (2 * DIFFEXT * LAMBDA * CPDP);
	    CTDW = (CDP + DIFFEXT * LAMBDA * CDA) / CM_M;
	    CPTDW = (CPDP + DIFFEXT * LAMBDA * CPDA) / CM_M;
	    break;
	case (DIFFEXTF | DIFFPERIM) :
	    config_flags |= TDIFFCAP;
	    CTDE = (2 * DIFFEXT * LAMBDA * CDP);
	    CPTDE = (2 * DIFFEXT * LAMBDA * CPDP);
	    CTDW = (DIFFEXT * LAMBDA * CDA) / CM_M;
	    CPTDW = (DIFFEXT * LAMBDA * CPDA) / CM_M;
	    break;
      }

    if( config_flags & CNTPULLUP )
	lprintf( stderr, "warning: cntpullup is not supported\n" );
   
    /* Check if transistor devices have been given resistance values */
    for( i = 0; i < nttypes; i++ )
      {
	if(( i > 2 ) && ( device_names[i]->devtype < 2 ))	// only transistors
	  {
	    if( device_names[i]->devinit != R_SET )
	      {
 	        lprintf( stderr, "Warning: missing required resistance for device %s\n", 
		   device_names[i]->devname );
		lprintf( stdout, "Setting default resistance value\n");
	        if( device_names[i]->devtype == NFET )
		  {
		    resist_copy(resistances[STATIC][0], resistances[STATIC][i]);
		    resist_copy(resistances[DYNHIGH][0], resistances[DYNHIGH][i]);
		    resist_copy(resistances[DYNLOW][0], resistances[DYNLOW][i]);
		  }	  
		else if( device_names[i]->devtype == PFET )
		  { 
		    resist_copy(resistances[STATIC][1], resistances[STATIC][i]);
		    resist_copy(resistances[DYNHIGH][1], resistances[DYNHIGH][i]);
		    resist_copy(resistances[DYNLOW][1], resistances[DYNLOW][i]);
	 	  }
		device_names[i]->devinit |= R_SET;
	      }
	  }
      }
          
    (void) fclose( cfile );
    config_flags |= CONFIG_LOADED;
    return 0;
  }

/* linear interpolation, assume that x1 < x <= x2 */
#define	interp( x, x1, y1, x2, y2 )  \
  ( (((double) (x - x1)) / ((double) (x2 - x1))) * (y2 - y1) + y1 )


/*
 * given a list of length structures, sorted by incresing length return
 * resistance of given channel.  If no exact match, return result of
 * linear interpolation using two closest channels.
 */
private double lresist( list, l, size )
  register struct length  *list;
  long                    l;
  double                  size;
  {
    register struct length  *p, *q;

    for( p = list, q = NULL; p != NULL; q = p, p = p->next )
      {
	if( p->l == l or( p->l > l and q == NULL ) )
	    return( p->r * size );
	if( p->l > l )
	    return( size * interp( l, q->l, q->r, p->l, p->r ) );
      }
    if( q != NULL )
	return( q->r *size );
    return( 1E4 * size );
  }


/*
 * given a pointer to the width structures for a particular type of
 * channel compute the resistance for the specified channel.
 */
private double wresist( list, w, l )
  register struct width  *list;
  int                    w, l;
  {
    register struct width  *p, *q;
    double                 size = ((double) l) / ((double) w);
    double                 temp;

    for( p = list, q = NULL; p != NULL; q = p, p = p->next )
      {
	if( p->w == w or( p->w > w and q == NULL ) )
	    return( lresist( p->list, l, size ) );
	if( p->w > w )
	  {
	    temp = lresist( q->list, l, size );
	    return( interp( w, q->w, temp, p->w, lresist( p->list, l, size ) ) );
	  }
      }
    if( q != NULL )
	return( lresist( q->list, l, size ) );
    return( 1E4 * size );
  }

typedef struct ResEntry  *resptr;

typedef struct ResEntry
  {
    resptr    r_next;
    Resists   r;
  } ResEntry;

#define	RES_TAB_SIZE	67

/*
 * Compute equivalent resistance given width, length and type of transistor.
 * for all contexts (STATIC, DYNHIGH, DYNLOW).  Place the result on the
 * transistor 
 */
public Resists *requiv( type, width, length )
  int  type;
  int  width, length;
  {
    static resptr    *res_htab[ NTTYPES ];
    static resptr    freeResist;
    resptr           *rtab;
    register resptr  r;
    unsigned         n;

    rtab = res_htab[ type ];
    if( rtab == NULL )
      {
	rtab = (resptr *) Valloc( RES_TAB_SIZE * sizeof( resptr * ), 1 );
	for( n = 0; n < RES_TAB_SIZE; rtab[ n++ ] = NULL );
	res_htab[ type ] = rtab;
      }
    n = ((unsigned) (length * 110133 + width)) % RES_TAB_SIZE;
    for( r = rtab[ n ]; r != NULL; r = r->r_next )
      {
	if( r->r.length == length and r->r.width == width ) return( &r->r );
      }

    if( (r = freeResist) == NULL )
	r = (resptr) MallocList( sizeof( ResEntry ), 1 );
    freeResist = r->r_next;
    r->r_next = rtab[n];
    rtab[n] = r;

    r->r.length = (Uint)length;
    r->r.width = (Uint)width;

    if( type == RESIST )
      {
	r->r.dynlow = r->r.dynhigh = r->r.rstatic = (float) length / LAMBDACM;
      }
    else
      {
	r->r.rstatic = wresist( resistances[ STATIC ][type], width, length );
	r->r.dynlow = wresist( resistances[ DYNLOW ][type], width, length );
	r->r.dynhigh = wresist( resistances[ DYNHIGH ][type], width, length );
      }
    return( &r->r );
  }


private void linsert( list, l, resist )
  register struct length  **list;
  long                    l;
  double                  resist;
  {
    register struct length  *p, *q, *lnew;

    for( p = *list, q = NULL; p != NULL; q = p, p = p->next )
      {
	if( p->l == l )
	  {
	    p->r = resist;
	    return;
	  }
	if( p->l > l )
	    break;
      }
    lnew = (struct length *) Valloc( sizeof( struct length ), 1 );
    lnew->next = p;
    lnew->l = l;
    lnew->r = resist;
    if( q == NULL )
	*list = lnew;
    else
	q->next = lnew;
  }


/* add a new data point to the interpolation array */
private void winsert( list, w, l, resist )
  register struct width  **list;
  long                   w, l;
  double                 resist;
  {
    register struct width   *p, *q, *wnew;
    register struct length  *lnew;

    for( p = *list, q = NULL; p != NULL; q = p, p = p->next )
      {
	if( p->w == w )
	  {
	    linsert( &p->list, l, resist );
	    return;
	  }
	if( p->w > w )
	    break;
      }
    wnew = (struct width *) Valloc( sizeof( struct width ), 1 );
    lnew = (struct length *) Valloc( sizeof( struct length ), 1 );
    wnew->next = p;
    wnew->list = lnew;
    wnew->w = w;
    if( q == NULL )
	*list = wnew;
    else
	q->next = wnew;
    lnew->next = NULL;
    lnew->l = l;
    lnew->r = resist;
  }


/* interpret resistance specification command */
private void insert( type, context, w, l, r )
  char  *type, *context, *w, *l, *r;
  {
    register int  c, t;
    long          width, length;
    double        resist;
    int 	  init_mask = 0;

    width = atof( w ) * CM_M;
    length = atof( l ) * CM_M;
    resist = atof( r );
    if( width <= 0 or length <= 0 or resist <= 0 )
      {
	rsimerror( currfile, lineno, "bad w, l, or r in config file\n" );
	nerrs++;
	return;
      }

    if( str_eql( context, "static" ) == 0 )
      {
	c = STATIC;
	init_mask = STATIC_MASK;
      }
    else if( str_eql( context, "dynamic-high" ) == 0 )
      {
	c = DYNHIGH;
	init_mask = DYNHIGH_MASK;
      }
    else if( str_eql( context, "dynamic-low" ) == 0 )
      {
	c = DYNLOW;
	init_mask = DYNLOW_MASK;
      }
    else if( str_eql( context, "power" ) == 0 )
	c = POWER;
    else
      {
	rsimerror( currfile, lineno, "bad resistance context in config file\n" );
	nerrs++;
	return;
      }

    for( t = 0; t < nttypes; t++ )
      {
	if( str_eql( device_names[t]->devname, type ) == 0 )
	  {
	    if( c == POWER )
		return;
	    winsert( &resistances[c][t], width, length, resist*width/length );
	    device_names[t]->devinit |= init_mask;
	    return;
	  }
	else if( str_eql( ttype_drop[t], type ) == 0 )
	    return;
      }
    
    rsimerror( currfile, lineno, "bad resistance transistor type\n" );
    nerrs++;
  }

/* interpret device record (conversion from subcircuit to device) */

private void makedevice( type, name, value )
  char  *type, *name;
  float value;
  {
    int i, j, typeidx;
    DevRec **devlist, *newdev;

    if (!strcmp(type, "nfet"))
	typeidx = NFET;
    else if (!strcmp(type, "pfet"))
	typeidx = PFET;
    else if (!strcmp(type, "capacitor"))
	typeidx = CAPACITOR;
    else if (!strcmp(type, "resistor"))
	typeidx = RESISTOR;
    else if (!strcmp(type, "diode"))
	typeidx = DIODE;
    else
    {
	rsimerror( currfile, lineno, "bad device type\n" );
	nerrs++;
	return;
    }
    
    newdev = (DevRec *)malloc(sizeof(DevRec));
    newdev->devname = strdup(name);
    newdev->devtype = typeidx;
    newdev->devvalue = value;
    newdev->fettype = ++nttypes;
    newdev->devinit = 0;

    /* This is inefficient but there will only be a small number of devices
     * so it doesn't really matter how efficient it is.
     */
    device_names = (DevRec **)realloc(device_names, nttypes * sizeof(DevRec *));
    device_names[nttypes - 1] = newdev;
  }
