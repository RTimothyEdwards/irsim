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
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#ifdef TCL_IRSIM
#include <tk.h>
#endif

#include "defs.h"
#include "net.h"
#include "globals.h"

#include "rsim.h"

public Command	*cmdtbl[ CMDTBLSIZE ];	/* command hash-table */

private	Bits	*blist = NULL;		/* list of vectors */

private	sptr	slist = NULL;		/* list of sequences */
private	int	maxsequence = 0;	/* longest sequence defined */


public  int     contline = 0;
private	sptr	xclock = NULL;		/* vectors which make up clock */
private	int	maxclock = 0;		/* longest clock sequence defined */
private short	scheduletag = 0;	/* index tag for scheduled events */

private	iptr	wlist = NULL;		/* list of nodes to be displayed */
private	iptr	wvlist = NULL;		/* list of vectors to be displayed */

public	char	*filename;		/* current input file */
public	int	lineno = 0;		/* current line number */
private	int	column = 0;		/* current output column */
private	int	stopped_state = FALSE;	/* have we stopped ? */
					
#ifndef TCL_IRSIM
private	Path	*cmdpath = NULL;	/* search path for cmd files */
#endif

public	Ulong	stepsize = DEFLT_STEP;	/* simulation step, in Delta's */
private	int	dcmdfile = 0;		/* display commands read from file */
private	int	ddisplay = 1;		/* if <>0 run "d" at end of step */

#ifdef TCL_IRSIM
private char	*tcldproc = NULL;	/* if <>NULL "d" command executes */
					/* this Tcl procedure		  */

char **targv;				/* array of command line arguments, */
					/* passed from Tcl.		    */
#else
public	char	*targv[ MAXARGS ];	/* array of tokens on command line */
#endif
public	int	targc;			/* number of args on command line */

public	char	wildCard[ MAXARGS ];	/* set if corresponding arg has '*' */
private	char	plus_minus[] = "+";	/* see apply() below */
private	char	potchars[] = "luxh.";	/* set of potential characters */
private char    not_in_stop[] = "Can't do that while stopped, try \"C\"\n";

public	char	*first_file = NULL;	/* basename of network file read-in */

public	int	analyzerON = FALSE;	/* set when analyzer is running */

public	Ulong	sim_time0 = 0;		/* starting time (see flush_hist) */
public	FILE	*logfile = NULL;	/* log file of transactions */

public char	**power_net_name = NULL;    /* Net name for power */
public char	*ground_net_name = NULL;   /* Net name for ground */

public int      power_net_name_size = 0;   /* Number of power net names */

#ifdef	POWER_EST
public	FILE	*caplogfile = NULL;	/* log file of cap transitions */
public	double	toggled_cap = 0;	/* indicative of total power of chip */
public  float   vsupply = 5.0;		/* supply voltage for pwr estimation */
public  float   capstarttime = 0.0;
public  float   capstoptime = 0.0;
public  float	captime = 0.0;
public  float	powermult = 0.0;	/* to do power estimate in milliWatts */
public  float   power = 0.0; 		/* power estimate logged after powquery */
public  int	pstep = 0;		/* Bool - end of step power display */
public  int	phist = 0;		/* Bool - create the histogram */
public  float   step_cap_x_trans = 0;	/* Stepwise C*trans count */
public  float   step_pow_x_trans = 0;	/* Stepwise total power */

#endif /* POWER_EST */

private int undefseq( /* p, list, lmax */ );
private int clockit( /* n */ );

#ifdef TCL_IRSIM
extern Tcl_Interp *irsiminterp;
extern int check_interrupt();
#endif

/* Histogram storage variables */
private float min = -1;
private float max = -1;
private int size = -1;
private long histpstepstart = -1;
 
#define	HashCmd( NM )	( ((NM)[0] + (((NM)[1]) << 1) ) & (CMDTBLSIZE - 1) )

/* 
 * Parse line into tokens, filling up targv and WildCard, and setting 'targc'
 */
public void parse_line( line, bufsize )
  register char  *line;
  int            bufsize;
  {
    char           *extra;
    register int   i;
    register char  ch;
    char           wc;			/* wild card indicator */

	/* extra storage comes out of unused portion of line buffer */
    i = strlen( line ) + 1;
    bufsize -= i;
    extra = &line[i];
    targc = 0;
    while( i = *line++ )
      {
	    /* skip past white space */
	if( i <= ' ' )
	    continue;

	    /* found start of new argument */
	if( targc == 0 and i == '|' )
	  {
	    targc = 0;		/* comment line, stop now */
	    return;
	  }

	    /* remember where argument begins */
	if( targc >= MAXARGS )
	  {
	    rsimerror( filename, lineno, "too many arguments in command\n" );
	    targc = 0;
	    return;
	  }
	else
	    targv[targc++] = --line;

	  /* skip past text of argument, terminate with null character.
	   * While scanning argument remember if we see a "{" which marks
	   * the possible beginning of an iteration expression.
	   */

	wc = FALSE;
	i = 0;
	while( (ch = *line) > ' ' )
	  {
	    if( ch == '*' )
		wc = TRUE;
	    else if( ch == ITERATOR_START )
		i = 1;
	    line++;
	  }
	*line++ = '\0';

	  /* if the argument might contain one or more iterators, process
	   * it more carefully...
	   */
	if( i == 1 )
	  {
	    if( expand( targv[--targc], &extra, &bufsize, wc ) )
	      {
		targc = 0;
		return;
	      }
	  }
	else
	    wildCard[targc - 1] = wc;
      }
  }


/* 
 * Given a text string, expand any iterators it contains.  For example, the
 * string "out.{1:10}" expands into ten arguments "out.1", ..., "out.10".
 * The string can contain multiple iterators which will be expanded
 * independently, e.g., "out{1:10}{1:20:2}" expands into 100 arguments.
 * Buffer and bufsize describe a byte buffer which can be used for expansion.
 * Return 0 if expansion succeeds, 1 otherwise.
 */
int expand( string, buffer, bufsize, wc )
  char  	*string;
  char          **buffer;
  int           *bufsize;
  char          wc;
  {
    register char  *p;
    char           prefix[100], index[256];
    int            start, stop, nstep, length;

	/* copy string until we reach beginning of iterator */
    p = prefix;
    length = 0;
    while( *string )
      {
	if( *string == ITERATOR_START )
	  {
	    *p = 0;
	    goto gotit;
	  }
	*p++ = *string++;
      }
    *p = 0;

	/* if we get here, there was no iterator in the string, so save what
	 * we have as another argument.
	 */
    length = strlen( prefix ) + 1;
    if( length > *bufsize )
      {
	rsimerror( filename, lineno, "too many arguments in command\n" );
	return( 1 );
      }
    (void) strcpy( *buffer, prefix );
    wildCard[targc] = wc;
    targv[targc++] = *buffer;
    *bufsize -= length;
    *buffer += length;
    return( 0 );

	/* gobble down iterator */
  gotit :
    start = 0;
    stop = 0;
    nstep = 0;
    for( string += 1; *string >= '0' and *string <= '9'; string += 1 )
	start = start * 10 + *string - '0';
    if( *string != ':' )
    if( *string != ':' )
	goto err;
    for( string += 1; *string >= '0' and *string <= '9'; string += 1 )
	stop = stop * 10 + *string - '0';
    if( *string == ITERATOR_END )
	goto done;
    if( *string != ':' )
	goto err;
    for( string += 1; *string >= '0' and *string <= '9'; string += 1 )
	nstep = nstep * 10 + *string - '0';
    if( *string == ITERATOR_END )
	goto done;

  err :
    rsimerror( filename, lineno, "syntax error in name iterator" );
    return( 1 );

  done :	/* suffix starts just past '}' which terminates iterator */
    string += 1;

	/* figure out correct step size */
    if( nstep == 0 )
	nstep = 1;
    else if( nstep < 0 )
	nstep = -nstep;
    if( start > stop )
	nstep = -nstep;

	/* expand the iterator */
    while( (nstep > 0 and start <= stop) or (nstep < 0 and start >= stop) )
      {
	(void) sprintf( index, "%s%d%s", prefix, start, string );
	if( expand( index, buffer, bufsize, wc ) )
	    return( 1 );
	start += nstep;
      }
    return( 0 );
  }


private	int applyStart = 1;

/*
 * Apply given function to each argument on the command line.
 * Arguments are checked first to ensure they are the name of a node or
 * vector; wild-card patterns are allowed as names.
 * Either 'fun' or 'vfunc' is called with the node/vector as 1st argument:
 *	'fun' is called if name refers to a node.
 *	'vfun' is called if name refers to a vector.  If 'vfun' is NULL
 *	then 'fun' is called on each node of the vector.
 * The parameter (2nd argument) passed to the specified function will be:
 *	If 'arg' is the special constant 'plus_minus' then
 *	    if the name is preceded by a '-' pass a pointer to '-'
 *	    otherwise pass a pointer to '+'.
 *	else 'arg' is passed as is.
 */

private void apply (fun, vfunc, arg)
  int  (*fun)(), (*vfunc)();
  char  *arg;
{
    register char *flag;
    register bptr b;
    register int  i, j, found;
    char *p;

#ifdef TCL_IRSIM
    /* Check for case in which arguments are passed as a list. */
    int start, argc;
    char **argv;

    if (targc == applyStart + 1)
    {
	Tcl_SplitList(irsiminterp, targv[applyStart], &argc, (CONST char ***)&argv);
	if (argc > 1)
	    start = 0;
	else
	{
	    Tcl_Free((char *)argv);
	    argv = targv;
	    argc = targc;
	    start = applyStart;
	}
    }
    else
    {
	argv = targv;
	argc = targc;
	start = applyStart;
    }

    for (i = start; i < argc; i++)
    {
	p = argv[i];

#else

    for (i = applyStart; i < targc; i++)
    {
	p = targv[i];

#endif

	if (arg == plus_minus)
	{
	    if (*p == '-')
	    {
		flag = p;
		p++;
	    }
	    else
		flag = plus_minus;
	}
	else
	    flag = arg;

	found = 0;
	if (wildCard[i])
	{
	    for (b = blist; b != NULL; b = b->next)
		if (str_match(p, b->name ))
		{
		    if (vfunc != NULL)
			(*vfunc)(b, flag);
		    else
			for (j = 0; j < b->nbits; j++)
			    (*fun)(b->nodes[j], flag);
		    found = 1;
		}
	    found += match_net(p, fun, flag);
	}
	else
	{
	    nptr n = find(p);

	    if (n != NULL)
		found += (*fun)(n, flag);
	    else
	    {
		for (b = blist; b != NULL; b = b->next)
		    if (str_eql(p, b->name) == 0)
		    {
			if (vfunc != NULL)
			    (*vfunc)(b, flag);
			else
			    for (j = 0; j < b->nbits; j++)
				(*fun)(b->nodes[j], flag);
			found = 1;
			break;
		    }
	    }
	}
	if (found == 0)
	    rsimerror(filename, lineno, "%s: No such node or vector\n", p);
    }

#ifdef TCL_IRSIM
    if (argv != targv) Tcl_Free((char *)argv);
#endif

}

/*
 * map a character into one of the potentials that a node can be set/compared
 */

public int ch2pot( ch )
  char  ch;
{
    register int  i;
    register char *s;

    for (i = 0, s = "0ux1lUXhLUXH"; s[i] != '\0'; i++)
	if (s[i] == ch)
	    return (i & (N_POTS - 1));

    rsimerror(filename, lineno, "%c: unknown node value\n", ch);
    return (N_POTS);
}

#ifndef TCL_IRSIM

/*
 * Open an input stream, and process commands from it.
 * Returns 0 if file could not be opened, 1 otherwise.
 */
public int finput( name )
  char  *name;
  {
    FILE  *fp = NULL;
    int   olineno;
    char  *ofname;
    char  pathname[256];

    if( *name == '/' )				/* absolute path */
	fp = fopen( name, "r" );
    else
      {
	Path  *p;
	for( p = cmdpath; p != NULL; p = p->next )
	  {
	    (void) sprintf( pathname, "%s/%s", p->name, name );
	    if( (fp = fopen( pathname, "r" )) != NULL )
		break;
	  }
      }
    if( fp == NULL )
	return( 0 );

    (void) strcpy( pathname, name );
    ofname = filename;
    olineno = lineno;
    filename = pathname;
    lineno = 0;
    (void) input( fp );
    (void) fclose( fp );
    filename = ofname;
    lineno = olineno;
    return( 1 );
  }

/*
 * Execute a builtin command or read commands from a '.cmd' file.
 */
public int exec_cmd()
  {
    static char       missing_args[] = "missing arguments to \"%s\"\n";
    static char       many_args[] = "too many arguments for \"%s\"\n";
    register Command  *cmd;
    char              *arg = targv[0];

	/* search command table, dispatch to handler, if any */
    for( cmd = cmdtbl[ HashCmd( arg ) ]; cmd != NULL; cmd = cmd->next )
      {
	if( strcmp( arg, cmd->name ) == 0 )
	  {
	    if( targc < cmd->nmin )
		rsimerror( filename, lineno, missing_args, cmd->name );
	    else if( targc > cmd->nmax )
		rsimerror( filename, lineno, many_args, cmd->name );
	    else
		return( (*cmd->handler)() );
	    return( 0 );
	  }
      }

	/* no built-in command found, try for a command file */
    if( targc == 1 )
      {
	(void) strcat( arg, ".cmd" );
	if( finput( arg ) )
	    return( 0 );
	arg[ strlen( arg ) - 4 ] = '\0';
      }

    rsimerror( filename, lineno, "unrecognized command: %s\n", arg );

    return( 0 );
  }


/* 
 * Read and execute commands read from input stream.
 */
int input( in )
  FILE  *in;
  {
    static char  line[ LSIZE ];		/* static: use only 1 line buffer */
    int          ret;

    while( 1 )
      {
	  /* output prompt and flush output */
	if( in == stdin )
	    (void) fputs( "irsim> ", stdout );
	(void) fflush( stdout );

#ifdef HAVE_PTHREADS
	/* Unlock analyzer display */
        EnableInput();
#endif

	    /* read command */
	if( fgetline( line, LSIZE, in ) == NULL )
	  {
	    if( (in != stdin) or not isatty( (int) fileno( stdin ) ) or
	      freopen( "/dev/tty", "r", stdin ) == NULL )
		return( -1 );
	    (void) fputc( '\n', stdout );
	    continue;
	  }

	if( in == stdin )
	  {
	    if( logfile != NULL )
		(void) fputs( line, logfile );
	  }
	else if( dcmdfile )
	    (void) fprintf( stdout, "%s:%d> %s", filename, lineno, line );

#ifdef HAVE_PTHREADS
	/* Lock analyzer display */
        DisableInput();
#endif

	    /* convert line into tokens */
	lineno += 1;
	lineno += contline;
	parse_line( line, LSIZE );
	if( targc != 0 and (ret = exec_cmd()) != 0 )
	    return( ret );

	    /* if user typed ^C stop reading and return to top level */
	if( int_received )
	  {
	    if( in == stdin )
		int_received = 0;
	    else
	      {
		(void) fprintf( stderr, "interrupt <%s @ %d>...",
		  filename, lineno );
		return( 1 );
	      }
	    (void) fputc( '\n', stderr );
	  }
      }
  }


/*
 * Read and process a command file (@ command).
 */
int cmdfile()
  {
    if( not finput( targv[1] ) )
	rsimerror( filename, lineno, "cannot open %s for input\n", targv[1] );

    return( 0 );
  }


/*
 * Set or Print the search path for command files.
 */
int docmdpath()
  {
    Path  *p, **last;
    int   i;

    if( targc == 1 )
      {
    	/* echo current cmdpath */
	for( p = cmdpath; p != NULL; p = p->next )
	    lprintf( stdout, "%s ", p->name );
	lprintf( stdout, "\n" );
      }
    else
      {
	if( strcmp( targv[1], "+" ) == 0 )
	    shift_args( TRUE );
	else
	  {
	    while( (p = cmdpath) != NULL )
	      {
		cmdpath = cmdpath->next;
		Vfree( p );
	      }
	  }
	last = &cmdpath;
	for( i = 1; i < targc; i++ )
	  {
	    p = (Path *) Valloc( SIZEOF( Path ) + strlen( targv[i] ), 1 );
	    (void) strcpy( p->name, targv[i] );
	    p->next = NULL;
	    *last = p;
	    last = &p->next;
	  }
      }
    return( 0 );
  }


/*
 * Initialize the path to be the current working directory.
 */
public void InitCmdPath()
  {
    targc = 2;
    targv[1] = ".";
    (void) docmdpath();
  }

#endif /* TCL_IRSIM */

/*
 * Set value of a node/vector to the requested value (hlux).
 */

private int setvalue()
{
    apply(setin, NULL, targv[0]);
    return 0;
}

/*
 * Toggle bit value(s) of a node/vector.
 */

private int togglevalue()
{
    apply(setin, NULL, "!");
    return 0;
}

#define	UnAlias( N )	while( (N)->nflags & ALIAS ) (N) = (N)->nlink

/*
 * add/delete node to/from display list.
 */
private int xwatch( n, flag )
  nptr  n;
  char  *flag;
  {
    UnAlias( n );

    if( not (n->nflags & MERGED) )
      {
	if( *flag == '+' )
	    iinsert_once( n, &wlist );
	else
	    idelete( n, &wlist );
      }
    return( 1 );
  }


/*
 * add/delete vector to/from display list
 */
private int vwatch( b, flag )
  bptr  b;
  char  *flag;
  {
    if( *flag == '+' )
	iinsert_once( (nptr) b, &wvlist );
    else
	idelete( (nptr) b, &wvlist );

    return( 1 );
  }


/* manipulate display list */
private int display()
  {
    apply( xwatch, vwatch, plus_minus );
    return( 0 );
  }

/*
 * Display a bit vector.
 */

private int dvec(b)
  register bptr  b;
{
    register int  i;
    char bits[250];

#ifdef TCL_IRSIM
    if (tcldproc == NULL)
    {
#endif
	i = strlen(b->name) + 2 + b->nbits;
	if (column + i >= MAXCOL)
	{
	    lprintf(stdout, "\n");
	    column = 0;
	}
	column += i;
#ifdef TCL_IRSIM
    }
#endif

    for (i = 0; i < b->nbits; i++)
	bits[i] = vchars[b->nodes[i]->npot];
    bits[i] = '\0';

#ifdef TCL_IRSIM
    if (tcldproc != NULL)
    {
	char tclcmd[250];
	int status;

	snprintf(tclcmd, 249, "%s %s %s %f\n", tcldproc, b->name,
		bits, d2ns(cur_delta));
	status = Tcl_EvalEx(irsiminterp, tclcmd, -1, 0);
	if (status == TCL_ERROR) {
	    lprintf(stderr, "Tcl callback error:  disabling callback\n");
	    free(tcldproc);
	    tcldproc = (char *)NULL;
	}
    }
    else
#endif
	lprintf(stdout, "%s=%s ", b->name, bits);

    return(1);
}


/* 
 * Print the value of a specific node.
 */

private void dnode(n)
  register nptr n;
{
    register char *name;
    register int i;

    name = pnode(n);
    UnAlias(n);

#ifdef TCL_IRSIM
    if (tcldproc == NULL)
    {
#endif
	i = strlen(name) + ((n->nflags & MERGED) ? 23 : 3);
	if (column + i >= MAXCOL)
	{
	    lprintf(stdout, "\n");
	    column = 0;
	}
	column += i;

	if (n->nflags & MERGED)
	    lprintf(stdout, "%s=<in transistor stack> ", name);
	else
	    lprintf(stdout, "%s=%c ", name, vchars[n->npot]);

#ifdef TCL_IRSIM
    }
    else
    {
	if (!(n->nflags & MERGED))
	{
	    char tclcmd[250];
	    int status;

	    snprintf(tclcmd, 249, "%s %s %c %f\n", tcldproc, name,
			vchars[n->npot], d2ns(cur_delta));
	    status = Tcl_EvalEx(irsiminterp, tclcmd, -1, 0);
	    if (status == TCL_ERROR)
	    {
		lprintf(stderr, "Tcl callback error:  disabling callback\n");
		free(tcldproc);
		tcldproc = (char *)NULL;
	    }
	}
    }
#endif
}


/*
 * print current simulated time and the state of the event list.
 */
private void prtime(col)
int  col;
{
#ifdef TCL_IRSIM
    if (tcldproc != NULL)
    {
	char tclcmd[250];
	int status;

	snprintf(tclcmd, 249, "%s time t %f\n", tcldproc, d2ns(cur_delta));
	status = Tcl_EvalEx(irsiminterp, tclcmd, -1, 0);
	if (status == TCL_ERROR)
	{
	    lprintf(stderr, "Tcl callback error:  disabling callback\n");
	    free(tcldproc);
	    tcldproc = (char *)NULL;
	}
	return;
    }
#endif /* TCL_IRSIM */

    if (col != 0)
	lprintf(stdout, "\n");
    lprintf(stdout, "time = %.3fns", d2ns(cur_delta));
    if ((npending - spending) > 0)
	lprintf(stdout, "; there are %d pending events",npending);
    lprintf(stdout, "\n");
}


/*
 * display node/vector values in display list
 */

private void pnwatchlist()
{
    register iptr  w;

    column = 0;

    /* print value of each watched bit vector. */

    for (w = wvlist; w != NULL; w = w->next)
	dvec((bptr)w->inode);

    /* now print value of each watched node. */

    for (w = wlist; w != NULL; w = w->next)
	dnode((nptr)w->inode);

    prtime(column);
}


/*
 * Just append node to the list whose tail is pointed to by 'ptail'.
 */

private int get_nd_list(n, ptail)
  nptr n, **ptail;
{
    if (!(n->nflags & VISITED))
    {
	n->nflags |= VISITED;
	n->n.next = NULL;
	**ptail = n;
	*ptail = &n->n.next;
    }
    return(1);
}


/*
 * display node values, either those specified or from display list
 */
private int pnlist()
{
    if (targc == 1)
	pnwatchlist();
    else
    {
	nptr n = NULL, *ntail = &n;

	column = 0;

	/* first print any bit vectors the user has specified */
	apply(get_nd_list, dvec, (char *)&ntail);

	/* then print individual nodes collected in the list */
	for (; n != NULL; n->nflags &= ~VISITED, n = n->n.next)
	    dnode(n);

	prtime(column);
    }
    return(0);
  }


/*
 * set/clear trace bit in node
 */

private int xtrace( n, flag )
  nptr  n;
  char  *flag;
  {
    UnAlias( n );

    if( n->nflags & MERGED )
      {
	lprintf( stdout, "can't trace %s\n", pnode( n ) );
	return( 1 );
      }

    if( *flag == '+' )
	n->nflags |= WATCHED;
    else if( n->nflags & WATCHED )
      {
	lprintf( stdout, "%s was watched; not any more\n", pnode( n ) );
	n->nflags &= ~WATCHED;
      }

    return( 1 );
  }


#ifdef POWER_EST
/*
 * set/clear powtrace bit in node
 */
private int xpowtrace( n, flag )
  nptr  n;
  char  *flag;
  {
    if( n->nflags & MERGED )
      {
	lprintf( stdout, "can't trace %s\n", pnode( n ) );
	return( 1 );
      }

    if( *flag == '+' )
	n->nflags |= POWWATCHED;
    else if( n->nflags & POWWATCHED )
      {
	lprintf( stdout, "%s was capwatched; not any more\n", pnode( n ) );
	n->nflags &= ~POWWATCHED;
      }

    return( 1 );
  }
#endif /* POWER_EST */

/*
 * set/clear trace bit in vector
 */
private int vtrace( b, flag )
  register bptr  b;
  char           *flag;
  {
    register int   i;

    if( *flag == '+' )
	b->traced |= WATCHVECTOR;
    else
      {
	for( i = 0; i < b->nbits; i += 1 )
	    b->nodes[i]->nflags &= ~WATCHVECTOR;
	b->traced &= ~WATCHVECTOR;
      }
    return( 1 );
  }

#ifdef POWER_EST
/*
 * set/clear powtrace bit in vector
 */
private int vpowtrace( b, flag )
  register bptr  b;
  char           *flag;
  {
    register int   i;

    if( *flag == '+' )
	b->traced |= POWWATCHVECTOR;
    else
      {
	for( i = 0; i < b->nbits; i += 1 )
	    b->nodes[i]->nflags &= ~POWWATCHVECTOR;
	b->traced &= ~POWWATCHVECTOR;
      }
    return( 1 );
  }
#endif /* POWER_EST */

/*
 * just in case node appears in more than one bit vector, run through all
 * the vectors being traced and make sure the flag is set for each node.
 */
private void set_vec_nodes( flag )
  int  flag;
  {
    register bptr  b;
    register int   i;

    for( b = blist; b != NULL; b = b->next )
	if( b->traced & flag )
	    for( i = 0; i < b->nbits; i += 1 )
		b->nodes[i]->nflags |= flag;
  }


/*
 * set/clear stop bit in node
 */
private int nstop( n, flag )
  nptr  n;
  char  *flag;
  {
    UnAlias( n );

    if( n->nflags & MERGED )
	return( 1 );

    if( *flag == '-' )
	n->nflags &= ~STOPONCHANGE;
    else
	n->nflags |= STOPONCHANGE;

    return( 1 );
  }


/*
 * set/clear stop bit in vector
 */
private int vstop( b, flag )
  register bptr  b;
  char           *flag;
  {
    register int   i;

    if( *flag == '+' )
	b->traced |= STOPVECCHANGE;
    else
      {
	for( i = 0; i < b->nbits; i += 1 )
	    b->nodes[i]->nflags &= ~STOPVECCHANGE;
	b->traced &= ~STOPVECCHANGE;
      }
    return( 1 );
  }


/*
 * mark nodes and vectors for tracing
 */
private int settrace()
  {
    apply(xtrace, vtrace, plus_minus);
    set_vec_nodes( WATCHVECTOR );
    return( 0 );
  }

#ifdef POWER_EST
/*
 * mark nodes and vectors for cap tracing
 */
private int setpowtrace()
  {
    apply(xpowtrace, vpowtrace, plus_minus);
    set_vec_nodes( POWWATCHVECTOR );
    return( 0 );
  }


/*
 * Helper routine for summing capacitance
 */
private int sumcapdoit( n, capsum )
  register nptr  n;
  float *capsum;
  {
    UnAlias( n );

    if( not (n->nflags & (MERGED | ALIAS)) and !isnan(n->ncap))
      *capsum += n->ncap;

    return( 0 );
  }


/*
 * Print sum of capacitance of nodes
 */
private int sumcap()
  {
    float capsum = 0;

    lprintf( stdout, "Sum of nodal capacitances: " );
    walk_net( sumcapdoit, (char *) &capsum );
    lprintf( stdout, "%f pF \n", capsum );
    return( 0 );
  }

/*
 * Helper function for clearing the VISITED flag of the node 
 */
private int clear_visited( nptr n )
{
    n->nflags &= ~VISITED;
    return( 0 );
}

/*
 * Helper function for walking through the current path of the power supply
 */
private int walk_path( nptr n, float vsupply )
{
    if ( (n->nflags & VISITED) or (n == GND_node) ) 
        return( 0 );
    n->vsupply = vsupply;
    n->vsupply2 = vsupply * vsupply;
    n->nflags |= VISITED;
    lptr l = n->nterm;
    while (l != NULL)
    {
	tptr t = l->xtor;
	if ( t->source == n )
	    walk_path( t->drain, vsupply );
        else if ( t->drain == n )
	    walk_path( t->source, vsupply );
        l = l->next; 
    }
    return( 0 );
}


/*
 * Set the supply voltage to a known value -- used in calculating power
 */
private int setvsupply()
  {
    float vsupply;
    nptr v_node;
    if( targc == 2 )
      {
        vsupply = atof( targv[1] );
        lprintf( stdout, "Supply Voltage = %4.2f Volts\n", vsupply );
      }
    else if( targc == 3 )
      { 
	v_node = RsimGetNode( targv[1] );
	vsupply = atof(targv[2]);
	walk_path( v_node, vsupply );
        walk_net( clear_visited, (char*)0 );
	lprintf( stdout, "Supply Voltage = %4.2f Volts at %s\n", vsupply, targv[1] );
      }
    return(0);
  }


/*
 * Toggle display of power estimation at end of each step
 */
private int togglepstep()
  {
    pstep = !pstep;
    if (pstep)
	lprintf(stdout,"Power display enabled\n");
    else
	lprintf(stdout,"Power display disabled\n");
    return(0);
  }
#endif /* POWER_EST */



/*
 * mark nodes and vectors for stopping
 */
private int setstop()
  {
    if( isatty( (int) fileno( stdin ) ) )
      {
	apply( nstop, vstop, plus_minus );
	set_vec_nodes( STOPVECCHANGE );
      }
    return( 0 );
  }

/*
 *-------------------------------------------------------------------------
 * Read a vector value from a string.  If this string appears to be a
 * binary number, it checks the number of bits against "nbits" and
 * flags an error.
 *
 * The return value can be:
 *   NULL on error
 *   a pointer to vstring if vstring is already a binary bit vector
 *   an allocated string representing the conversion of vstring into a binary vector
 *
 * It is the responsibility of the calling routing to free() the return value
 * if it is neither NULL nor equal to vstring.
 *-------------------------------------------------------------------------
 */

private char *readVector( vstring, nbits )
   char *vstring;
   int nbits;
{
    char *newvecs, *locstring, *result;
    int i, c, b, r = 0;
    Ulong nval;

    /* Check for negative; record this, and do 2's complement later */
    locstring = vstring;
    if (locstring[0] == '-') locstring++;

    /* Check for format 0b 0d 0o 0h 0x or %b %d %o %h %x */

    if (locstring[0] == '0' || locstring[0] == '%')
    {
	c = tolower(locstring[1]);
	switch (c) {
	    case 'x':
		/* "0x" is a binary number *if* the bit count == */
		/* nbits and no digits appear other than 0 and 1 */

		if ((b = strlen(locstring)) == nbits)
		{
		    for (i = 0; i < b; i++)
		    {
			char t = tolower(locstring[i]);
			if (t != '1' && t != '0' && t != 'u' && t != 'x')
			    break;
		    }
		    if (i != b) break;
		}
		/* drop through */

	    case 'd': case 'b':
	    case 'o': case 'h':
		r = 1;
		newvecs = malloc(nbits + 1);
		break;
	    default:
		newvecs = locstring;	/* In case '0' starts a binary number */
		break;
	}
	if (r == 1)
	{

	   r = 0;
	   switch (c) {
	       case 'b':
		   for (b = 0 ; b < nbits; b++) newvecs[b] = '0';
		   strcpy(newvecs + (nbits - strlen(locstring + 2)), locstring + 2);
		   r = 1;
		   break;
	       case 'd':
		   nval = strtoUlong(locstring + 2, &result, 10);
		   if (*result == '\0') r = 1;
		   break;
	       case 'o':
		   nval = strtoUlong(locstring + 2, &result, 8);
		   if (*result == '\0') r = 1;
		   break;
	       case 'h':
	       case 'x':
		   nval = strtoUlong(locstring + 2, &result, 16);
		   if (*result == '\0') r = 1;
		   break;
	   }

	   if (r != 1) {
	       rsimerror(filename, lineno, "error: bad vector value '%s'\n", locstring);
	       free(newvecs);
	       return NULL;
	    }

	   if (locstring != vstring) nval--;	/* For 2's complement calculation */

	   switch (c) {
	      case 'd': case 'o':
	      case 'h': case 'x':
		  for (b = 0; b < nbits; b++)
		      newvecs[b] = '0';
		  newvecs[b] = '\0';
		  for (b = 0; b < nbits; b++)
		      if (nval & ((Ulong)1 << b))
		          newvecs[nbits - (b + 1)] = '1';
		  if ((nval & ~(((Ulong)1 << b) - 1)) != 0)
		  {
		      rsimerror(filename, lineno, "warning: vector value '%s' too "
				"large for vector.  Value truncated\n", vstring);
		  }
		  break;
	    }

	    if (locstring != vstring) 	/* Complement part of 2's complement */ 
	    {
		for (b = 0; b < nbits; b++)
		{
		    if (newvecs[b] == '0')
		        newvecs[b] = '1';
		    else if (newvecs[b] == '1')
		        newvecs[b] = '0';
		}
	    }
	}
    }
    else
	newvecs = locstring;

    if (strlen(newvecs) != nbits)
    {
	rsimerror(filename, lineno, "wrong number of bits for this vector\n");
	return NULL;
    }

    for (i = 0; i < nbits; i++)
    {
	if ((newvecs[i] = potchars[ch2pot(newvecs[i])]) == '.')
	{
	    if (newvecs != locstring) free(newvecs);
	    return NULL;
	}
    }
    return newvecs;
}

/*
 * Parse a string for vector notation.
 * Return the vector component of the "idx"th index of vector "vstring".
 * If the bus has no "idx"th index, then return NULL.
 */

nptr parse_bus(vstring, idx)
    char *vstring;
    int idx;
{
    static char *rstring = NULL;
    char *nsplit, *i;
    int idx1, idx2, idxc;

    /* Check for "a:b" notation */
    nsplit = strrchr(vstring, ':');
    if (nsplit == NULL) return NULL;

    if (sscanf(nsplit + 1, "%d", &idx2) != 1) return NULL;
    i = nsplit - 1;
    while (isdigit(*i) && (i > vstring)) i--;
    if (sscanf(++i, "%d", &idx1) != 1) return NULL;

    /* Check if we're outside the requested range */
    idxc = idx2 - idx1;
    if (idxc < 0) idxc = -idxc;
    idxc++;

    if (idx < 0 || idx >= idxc) return NULL;

    nsplit++;
    while (isdigit(*nsplit)) nsplit++;

    if (rstring != NULL) free(rstring);
    rstring = strdup(vstring);

    idxc = idx1 + ((idx2 > idx1) ? idx : -idx);
	
    sprintf(rstring + (int)(i - vstring), "%d", idxc);
    strcat(rstring, nsplit);

    return(find(rstring));
}

/*
 * define bit vector
 */

private int dovector()
  {
    nptr  n;
    bptr  b, last;
    int   i, vecextra, idx, locargc, errcond;

    if( find( targv[1] ) != NULL )
      {
	rsimerror( filename, lineno, "'%s' is a node, can't be a vector\n",
	  targv[1] );
	return( 0 );
      }

	/* get rid of any vector with the same name */
    for( b = blist, last = NULL; b != NULL; last = b, b = b->next )
	if( strcmp( b->name, targv[1] ) == 0 )
	  {
	    if( undefseq( (nptr) b, &slist, &maxsequence ) or
	      undefseq( (nptr) b, &xclock, &maxclock ) )
	      {
		rsimerror( filename, lineno,
		  "%s is a clock/sequence; can't change it while stopped\n",
		  b->name );
		return( 0 );
	      }
	    idelete( (nptr) b, &wvlist );	/* untrace its nodes */
	    if( last == NULL )
		blist = b->next;
	    else
		last->next = b->next;		/* remove from display list */
	    (void) vtrace( b, "-" );
	    if( analyzerON )
		RemoveVector( b );
	    Vfree( b->name );
	    Vfree( b );
	    break;
	  }

    vecextra = 0;
    errcond = 0;
    for (i = 2; i < targc; i++)
    {
	if ((n = find(targv[i])) == NULL)
	{
	    idx = 0;
	    while (parse_bus(targv[i], idx) != NULL)
	    {
		idx++;
		vecextra++;
	    }
	    if (idx > 0)
		vecextra--;
	    else
	    {
		rsimerror(filename, lineno, "No such node %s\n", targv[i]);
		errcond = 1;
	    }
	}
    }
    locargc = targc + vecextra;
    if (errcond > 0) return 0;

    b = (bptr) Valloc( SIZEOF( Bits ) + (locargc - 3) * SIZEOF( nptr ), 0 );
    if( b == NULL or (b->name = Valloc( strlen( targv[1] ) + 1, 0 )) == NULL )
      {
	if( b ) Vfree( b );
	rsimerror( filename, lineno, "Not enough memory for vector\n" );
	return( 0 );
      }
    b->traced = 0;
    b->nbits = 0;
    (void) strcpy( b->name, targv[1] );

    idx = 0;
    for (i = 2; i < targc; i++)
    {
	if ((n = find(targv[i])) == NULL)
	{
	    /* Something in "a:b" notation */
	    if ((n = parse_bus(targv[i], idx)) != NULL)
	    {
		idx++;
		i--;	/* Repeat cycle i until we have found all components */
	    }
	    else if (idx > 0)
		idx = 0;
	}

	if (n != NULL)
	{
	    UnAlias( n );
	    if( n->nflags & MERGED )
		rsimerror( filename, lineno, "%s can not be part of a vector\n",
		  pnode( n ) );
	    else
		b->nodes[b->nbits++] = n;
	}
    }

    if( b->nbits == locargc - 2 )
      {
	b->next = blist;
	blist = b;
      }
    else
      {
	Vfree( b->name );
	Vfree( b );
      }

    return( 0 );
  }


/* set bit vector */

private int setvector()
{
    register bptr  b;
    register int   i;
    char           *val;

    /* find vector */
    for (b = blist; b != NULL; b = b->next)
    {
	if (str_eql(b->name, targv[1]) == 0)
	    goto got_it;
    }

    rsimerror(filename, lineno, "%s: No such vector\n", targv[1]);
    return 0;

got_it:

    /* set nodes */

    val = readVector(targv[2], b->nbits);
    if (val == NULL) return 0;

    for (i = 0; i < b->nbits; i++)
	(void) setin(b->nodes[i], &val[i]);

    if (val != targv[2]) free(val);
    return 0;
}


private int CompareVector( np, name, nbits, mask, value )
  nptr  *np;
  char  *name;
  int   nbits;
  char  *mask;
  char  *value;
  {
    int   i, val;
    nptr  n;

    if( strlen( value ) != nbits )
      {
	rsimerror( filename, lineno, "wrong number of bits for value\n" );
	return 0;
      }
    if( mask != NULL and strlen( mask ) != nbits )
      {
	rsimerror( filename, lineno, "wrong number of bits for mask\n" );
	return 0;
      }

    for( i = 0; i < nbits; i++ )
      {
	if( mask != NULL and mask[i] != '0' )
	    continue;
	n = np[i];
	if( (val = ch2pot( value[i] )) >= N_POTS )
	    return 0;
	if( val == HIGH_Z ) val = X;
	if( n->npot != val )
	    goto fail;
      }
    return 0;

  fail :
#ifndef OLD_ASSERT
    return 1;
#else
    lprintf( stderr, "(%s, %d): assertion failed on '%s' ",
     filename, lineno, name );
    for( i = 0; i < nbits; i++ )
      {
	if( mask != NULL and mask[i] != '0' )
	  {
	    lprintf( stdout, "-" );
	    value[i] = '-';
	  }
	else
	    lprintf( stdout, "%c", vchars[ np[i]->npot ] );
      }
    lprintf( stdout, " (%s)\n", value );
#endif
  }


typedef struct
  {
    nptr  node;
    bptr  vec;
    int   num;
  } Find1Arg;


private int SetNode( nd, find_one )
  nptr      nd;
  Find1Arg  *find_one;
  {
    find_one->node = nd;
    find_one->num++;
    return( 1 );
  }

private int SetVector( bp, find_one )
  bptr      bp;
  Find1Arg  *find_one;
  {
    find_one->vec = bp;
    find_one->num++;
    return( 1 );
  }


/*----------------------------------------------*/
/* find vector or node (1st argument) 		*/
/*----------------------------------------------*/

private void FindOne( f )
  Find1Arg  *f;
  {
    targc = 2;
    f->num = 0;
    f->vec = NULL;
    f->node = NULL;
    apply( SetNode, SetVector, (char *) f );
  }
  
/*---------------------------------*/
/* convert a bit vector to decimal */
/*---------------------------------*/

Ulong convertVector(nodes, nbits)
    nptr *nodes;
    int nbits;
{
    int i;
    Ulong decvalue;

    decvalue = (Ulong)0;
    for (i = 0; i < nbits; i++)
    {
	decvalue <<= 1;
	if (nodes[i]->npot == X) return -1;
	else if (nodes[i]->npot == HIGH)
	    decvalue |= 1;
    }
    return decvalue;
}

/*--------------------------------*/
/* assert (or query) a bit vector */
/*--------------------------------*/

int doAssert()
{
    char *mask, *value, *name;
    Find1Arg  f;
    int i, nbits, comp = 0, shortform = 0, saveargs;
    nptr *nodes;

    if (targc == 4) {
	mask = targv[2];
	value = targv[3];
    }
    else if (targc == 3) {
	mask = NULL;
	value = targv[2];
	if (value[0] == '%' && (strlen(value) == 2))
	    shortform = 1;
    }
    else if (targc == 2)
	shortform = 1;

    saveargs = targc;
    FindOne(&f);

    if (f.num == 0) return 0;
    else if (f.num > 1)
    {
	rsimerror(filename, lineno, "%s matches more than one node or vector\n",
			targv[1]);
	return 0;
    }
    else if (f.node != NULL)
    {
	name = pnode(f.node);
	UnAlias(f.node);
	if (!shortform)
	    comp = CompareVector(&f.node, name, 1, mask, value);
        nodes = &f.node;
        nbits = 1;
    }
    else if (f.vec != NULL)
    { 
	if (!shortform)
	    comp = CompareVector(f.vec->nodes, f.vec->name, f.vec->nbits, mask, value);
        name = f.vec->name;
        nbits = f.vec->nbits;
        nodes = f.vec->nodes; 
    }

    if (shortform)
    {
	lprintf(stdout, "%s = ", name);
	if (saveargs == 3 && value[1] != 'b')
	{
	    Ulong decvalue = convertVector(nodes, nbits);
	    if (decvalue < 0)
		lprintf(stdout, "???");
	    else
	    {
		switch(value[1])
		{ 
		    case 'o':	/* print value in octal */
			lprintf(stdout, PRINTF_LLONG "o", decvalue);
			break;
		    case 'x':	/* print value in hex */
		    case 'h':
			lprintf(stdout, PRINTF_LLONG "x", decvalue);
			break;
		    default:	/* print value in decimal */
			lprintf(stdout, PRINTF_LLONG "u", decvalue);
			break;
		}
	    }
	}
	else
	{
	    for (i = 0; i < nbits; i++)
		lprintf(stdout, "%c", vchars[nodes[i]->npot]);
	}
	lprintf(stdout, "\n");
    }
    else if (comp != 0)
    {
        lprintf(stderr, "(%s, %d): assertion failed on '%s' ",
			filename, lineno, name );
        for (i = 0; i < nbits; i++)
        {
     	    if ((mask != NULL) && (mask[i] != '0'))
	    {
	       lprintf(stdout, "-");
	       value[i] = '-';
	    }
	    else
	       lprintf(stdout, "%c", vchars[nodes[i]->npot]);
        }
        lprintf(stdout, " (%s)\n", value);
    }
    return 0;
}


/*----------------------------------------*/
/* query a bit vector (Tcl version)	  */
/* Returns the value in decimal		  */
/* Equivalent to assert, short form only. */
/*--------------------------------------*/

int doQuery()
{
    char *name;
    Find1Arg  f;
    int nbits;
    nptr *nodes;
    Ulong decvalue;

    FindOne(&f);

    if (f.num == 0) return 0;
    else if (f.num > 1)
    {
	rsimerror(filename, lineno, "%s matches more than one node or vector\n",
			targv[1]);
	return 0;
    }
    else if (f.node != NULL)
    {
	name = pnode(f.node);
	UnAlias(f.node);
        nodes = &f.node;
        nbits = 1;
    }
    else if (f.vec != NULL)
    { 
        name = f.vec->name;
        nbits = f.vec->nbits;
        nodes = f.vec->nodes; 
    }
    decvalue = convertVector(nodes, nbits);
#ifdef TCL_IRSIM 
    if (nbits < 32)
       Tcl_SetObjResult(irsiminterp, Tcl_NewIntObj((unsigned int)decvalue));
    else {
       char *bres;
       bres = malloc(nbits + 1);
       sprintf(bres, PRINTF_LLONG "u", decvalue);
       Tcl_SetResult(irsiminterp, bres, TCL_VOLATILE);
       free(bres);
    }
#else
    lprintf(stdout, PRINTF_LLONG "u\n", decvalue);
#endif
    return 0;
}

private int doUntil( )
  {
    char      *mask, *value, *name;
    Find1Arg  f;
    int i, nbits, ccount, cnt = 0, comp = 0;
    nptr *nodes;

    if( targc == 5 )
      {
	mask = targv[2];
	value = targv[3];
        ccount = atoi(targv[4]);
      }
    else
      {
	mask = NULL;
	value = targv[2];
        ccount = atoi(targv[3]);
      }

    FindOne( &f );

    if( f.num > 1 )
	rsimerror( filename, lineno, "%s matches more than one node or vector\n",
	  targv[1] );
    else if( f.node != NULL )
      {
	name = pnode( f.node );
	UnAlias( f.node );
        targc = 1;
	while((cnt <= ccount) && (comp = CompareVector( &f.node, name, 1, mask, value ) != 0)) {
            cnt++;
            clockit(1); }
        nodes = &f.node;
        nbits = 1;
      }
    else if( f.vec != NULL ) {
        targc = 1;
	while((cnt <= ccount) && (comp = CompareVector( f.vec->nodes, f.vec->name, f.vec->nbits, mask,
            value ) != 0)) {
            cnt++;
            clockit(1); }
        name = f.vec->name;
        nbits = f.vec->nbits;
        nodes = f.vec->nodes; }
    if(comp != 0) {
        lprintf( stderr, "(%s, %d): assertion failed on '%s' ",
         filename, lineno, name );
        for( i = 0; i < nbits; i++ )
          {
     	    if( mask != NULL and mask[i] != '0' )
	      {
	       lprintf( stdout, "-" );
	       value[i] = '-';
	      }
	   else
	       lprintf( stdout, "%c", vchars[ nodes[i]->npot ] );
          }
        lprintf( stdout, " (%s)\n", value );
        }
    return( 0 );
  }


private	nptr aw_trig ; /* keeps current assertWhen trigger */
private awptr aw_p ;   /* track pointer on the current assertWhen list */

/*----------------------------------------------------------*/
/*----------------------------------------------------------*/

private int setupAssertWhen(n, val)
 nptr n;
 char *val;
  {
	register awptr p;

	p = ( awptr ) Falloc(sizeof(assertWhen),1);
	p->node = n ;
	if (val) p->val = *val ;
	p->nxt = NULL ;
	p->proc = NULL ;
	p->tag = -1 ;

	p->nxt = aw_trig->awpending;
	aw_trig->awpending = p ;
	aw_p = p ;	/* For use by caller */
	return 1;
  }

/*---------------------------------------------------------------*/
/* Schedule an event to occur (once) at a specific signal edge.  */
/*---------------------------------------------------------------*/

private int doWhen()
{
    Find1Arg  trig;
    char *apot;

    FindOne( &trig );

    if( trig.num > 1 )
	rsimerror( filename, lineno, "%s matches more than one node or vector\n",
	  targv[1] );
    else if( trig.node != NULL )
      {
	UnAlias( trig.node );
	aw_trig = trig.node ;
	aw_trig->awmask = (char)0;
	for (apot = targv[2]; *apot != '\0'; apot++)
	    aw_trig->awmask |= POT2MASK(ch2pot(*apot));

	setupAssertWhen(NULL, NULL);
	aw_p->proc = strdup(targv[3]);
	aw_p->tag = scheduletag;
#ifdef TCL_IRSIM
	Tcl_SetObjResult(irsiminterp, Tcl_NewIntObj((int)scheduletag++));
#endif
      }
    else if( trig.vec != NULL ) 
	rsimerror( filename, lineno, "trigger to when %s can't be a vector\n",
		targv[1]);
    return( 0 );
}

/*-----------------------------------------*/
/* Cancel a scheduled edge-triggered event */
/*-----------------------------------------*/

int cancelWhen(n, tag)
  register nptr  n;
  int *tag;
{
    awptr p, plast;

    if (n->awpending) {
       plast = NULL;
       for (p = n->awpending; p != NULL; p = p->nxt) {
	  if (p->tag == *tag) {
	     free(p->proc);
	     if (plast == NULL)
		n->awpending = p->nxt;
	     else	
		plast->nxt = p->nxt;
	     Ffree(p); 
	     return -1;
	  }
	  plast = p;
       }
    }
    return 0;
}

/*-----------------------------------------------------------------*/
/* Print the procedure defined by a scheduled edge-triggered event */
/*-----------------------------------------------------------------*/

int getWhen(n, tag)
  register nptr  n;
  int *tag;
{
    awptr p;

    if (n->awpending) {
       for (p = n->awpending; p != NULL; p = p->nxt) {
	  if (p->tag == *tag) {
	     lprintf( stdout, "%s\n", p->proc );
	     return -1;
	  }
       }
    }
    return 0;
}

/*--------------------------------------------------------------*/
/* Schedule an event to occur at a specific signal timing.  	*/
/*--------------------------------------------------------------*/

private int doWhenever()
{
    Find1Arg  trig;
    char *apot;

    if (targc == 3 ) {
       int tag = (int) atoi(targv[2]);
       if (!strcmp(targv[1], "cancel")) {
	   walk_net( cancelWhen, (char *)&tag );
       }
       else if (!strcmp(targv[1], "get")) {
	   walk_net( getWhen, (char *)&tag );
       }
       else {
	   rsimerror( filename, lineno, "usage: whenever cancel|get tag\n" );
       }
       return 0;
    }

    FindOne( &trig );

    if( trig.num > 1 )
	rsimerror( filename, lineno, "%s matches more than one node or vector\n",
	  targv[1] );
    else if( trig.node != NULL )
      {
	UnAlias( trig.node );
	aw_trig = trig.node ;
	aw_trig->awmask = (char)0;
	for (apot = targv[2]; *apot != '\0'; apot++)
	    aw_trig->awmask |= POT2MASK(ch2pot(*apot));
	setupAssertWhen((nptr)1, NULL);
	aw_p->proc = strdup(targv[3]);
	aw_p->tag = scheduletag;
#ifdef TCL_IRSIM
	Tcl_SetObjResult(irsiminterp, Tcl_NewIntObj((int)scheduletag++));
#endif
      }
    else if( trig.vec != NULL ) 
	rsimerror( filename, lineno, "trigger to when %s can't be a vector\n",
		targv[1]);
    return( 0 );
}

/*--------------------------------------------------------------------*/
/* Assert that a signal has a specific value at a specific transition */
/*--------------------------------------------------------------------*/

private int doAssertWhen()
{
    Find1Arg  trig;
    char *apot;

    FindOne( &trig );

    if( trig.num > 1 )
	rsimerror( filename, lineno, "%s matches more than one node or vector\n",
	  targv[1] );
    else if( trig.node != NULL )
      {
	applyStart = 3; targc  = 4;

	UnAlias( trig.node );
	aw_trig = trig.node ;
	aw_trig->awmask = (char)0;
	for (apot = targv[2]; *apot != '\0'; apot++)
	    aw_trig->awmask |= POT2MASK(ch2pot(*apot));
	apply(setupAssertWhen, NULL, targv[4]);

	applyStart = 1; targc = 4;
      }
    else if( trig.vec != NULL ) 
	rsimerror( filename, lineno, "trigger to assertWhen %s can't be a vector\n",
		targv[1]);
    return( 0 );
}

/*--------------------------------------------------------------*/
/* Evaluate an edge-triggered assertion or scheduled event	*/
/*--------------------------------------------------------------*/

public	void	evalAssertWhen(n)
  nptr n;
{
	register awptr p, p2, awstart;
	char cval[2] ; 
	char *name, comp;

	awstart = n->awpending;
	cval[0] = 0 ;
	cval[1] = 0 ;
	for ( p = n->awpending ; p ;  ) {
	    if (p->tag >= 0) {
		evptr new;

		/* A procedure to enqueue.  Create a new event */
		new = EnqueueOther(TIMED_EV, cur_delta + 1);
		new->enode = (nptr)p->proc;
		new->delay = 0;
		new->rtime = p->tag;

		if (p->node != NULL) {
		    /* Recurrent action */
		    aw_trig = n;
		    setupAssertWhen((nptr)1, NULL);
		    aw_p->proc = strdup(p->proc);
		    aw_p->tag = p->tag;
		    n->awpending->nxt = NULL;
		}
   	    }
	    else {
		/* A normal assertWhen assertion */
		cval[0] = p->val ; 
		name = pnode( p->node );
		comp = CompareVector(&(p->node), name, 1, NULL, cval);
		if ( comp != 0 ) 
		   lprintf( stderr, "(%s, %d): assertion failed on '%s' ",
		   filename, lineno, name );
	    }
	    p2 = p ; p = p->nxt ;
	    Ffree(p2, sizeof( assertWhen ) );
	}
	if (awstart == n->awpending) n->awpending = NULL ;
}




private int collect_inputs( n, inps )
  register nptr  n;
  register nptr  inps[];
  {
    if( (n->nflags & (INPUT|ALIAS|POWER_RAIL|VISITED|INPUT_MASK) ) == INPUT )
      {
	n->n.next = inps[ n->npot ];
	inps[ n->npot ] = n;
	n->nflags |= VISITED;
      }
    return( 0 );
  }


/* display current inputs */
private int inputs()
  {
    register iptr  list;
    register nptr  n;
    nptr           inptbl[ N_POTS ];

    inptbl[ HIGH ] = inptbl[ LOW ] = inptbl[ X ] = NULL;
    walk_net( collect_inputs, (char *) inptbl );

    lprintf( stdout, "h inputs: " );
    for( list = hinputs; list != NULL; list = list->next )
	lprintf( stdout, "%s ", pnode( list->inode ) );
    for( n = inptbl[ HIGH ]; n != NULL; n->nflags &= ~VISITED, n = n->n.next )
	lprintf( stdout, "%s ", pnode( n ) );
    lprintf( stdout, "\nl inputs: " );
    for( list = linputs; list != NULL; list = list->next )
	lprintf( stdout, "%s ", pnode( list->inode ) );
    for( n = inptbl[ LOW ]; n != NULL; n->nflags &= ~VISITED, n = n->n.next )
	lprintf( stdout, "%s ", pnode( n ) );
    lprintf( stdout, "\nu inputs: " );
    for( list = uinputs; list != NULL; list = list->next )
	lprintf( stdout, "%s ", pnode( list->inode ) );
    for( n = inptbl[ X ]; n != NULL; n->nflags &= ~VISITED, n = n->n.next )
	lprintf( stdout, "%s ", pnode( n ) );
    lprintf( stdout, "\n" );
    return( 0 );
  }


/* set stepsize */
private int setstep()
  {
    if( targc == 1 )
	lprintf( stdout, "stepsize = %f\n", d2ns( stepsize ) );
    else if( targc == 2 )
      {
	Ulong  newsize = (Ulong)ns2d(atof(targv[1]));

	if (newsize <= 0)
	    rsimerror( filename, lineno, "bad step size: %s\n", targv[1] );
	else
	    stepsize = newsize;
      }
    return( 0 );
  }

#ifdef TCL_IRSIM

/*
 * Return the list of all known vectors as a Tcl list
 */

public Tcl_Obj *list_all_vectors()
{
    bptr b;
    Tcl_Obj *lobj, *robj;

    lobj = Tcl_NewListObj(0, NULL);

    for (b = blist; b != NULL; b = b->next) {
	robj = Tcl_NewStringObj(b->name, -1);
	Tcl_ListObjAppendElement(irsiminterp, lobj, robj);
    }
    return lobj;
}

#endif /* TCL_IRSIM */

/*
 * Display traced vectors that just changed.  There should be at least one.
 */
public void disp_watch_vec( which )
  long  which;
  {
    register bptr  b;
    register int   i;
    char           temp[20];

    which &= (WATCHVECTOR | STOPVECCHANGE);
    (void) sprintf( temp, " @ %.3fns ", d2ns( cur_delta ) );
    lprintf( stdout, "%s", temp );
    column = strlen( temp );
    for( b = blist; b != NULL; b = b->next )
      {
	if( (b->traced & which) == 0 )
	    continue;
	for( i = b->nbits - 1; i >= 0; i-- )
	    if( b->nodes[i]->c.time == cur_delta )
		break;
	if( i >= 0 )
	    (void) dvec( b );
      }
    lprintf( stdout, "\n" );
  }


private void EnterStopState()
  {
    char  *ofname = filename;
    int   olineno = lineno;

    lprintf( stdout, "--> STOP  " );
    prtime( 0 );
    filename = "STOP";
    lineno = 0;
    stopped_state = TRUE;
#ifdef TCL_IRSIM
    while( fgetc(stdin) <= 0 );		/* this is TEMPORARY! */
#else
    while( input( stdin ) <= 0 );	/* ignore quit */
#endif
    stopped_state = FALSE;
    lineno = olineno;
    filename = ofname;
  }


/* 
 * Settle network until the specified stop time is reached.
 * Premature returns (before stop time) indicate that a node/vector whose
 * stop-bit set has just changed value, so popup a stdin command interpreter.
 *
 * Return value is 0 if the stop time was reached, 1 for premature returns.
 */

private int relax( stoptime )
  Ulong  stoptime;
  {
    while( step( stoptime ) )
      {
	EnterStopState();
      }
    return(((cur_delta - stoptime) > 0) ? 1 : 0);
  }


#define	CHECK_STOP() \
  if( stopped_state ) \
    { \
	rsimerror( filename, lineno, not_in_stop ); \
	return( 0 ); \
   }

/*
 * store data for histogramming
 */
struct _buckets
{
    float range;
    int bins;
};

typedef struct _buckets *Buckets;
Buckets hist = NULL;

/*
 * relax network, optionally set stepsize
 */
private int dostep()
  {
    Ulong newsize;
#ifdef POWER_EST
    Ulong pstepstart;
#endif /* POWER_EST */
    CHECK_STOP();

    if( targc == 2 )
      {
	newsize = (Ulong) ns2d( atof( targv[1] ) );
	if( newsize <= 0 )
	  {
	    rsimerror( filename, lineno, "bad step size: %s\n", targv[1] );
	    return( 0 );
	  }
      }
    else
	newsize = stepsize;

#ifdef POWER_EST
    if ( hist == NULL ) 
    {
        pstepstart = cur_delta;
        step_cap_x_trans = 0;
	step_pow_x_trans = 0;
    }
#endif /* POWER_EST */

    (void) relax( cur_delta + newsize );
#ifdef TCL_IREIM
    check_interrupt();	/* Allows the console window to update */
#endif
    if( ddisplay )
	pnwatchlist();
    power = step_pow_x_trans/(2*(d2ns(cur_delta-pstepstart)));	
#ifdef POWER_EST
    if ( hist == NULL )
    {
	 if( pstep ) 
         {
	    lprintf(stdout,
	        "Dynamic power estimate for powtrace'd nodes on last step = %f mW\n",
	        step_pow_x_trans/(2*(d2ns(cur_delta-pstepstart)))); 
         }
    }
#endif /* POWER_EST */

    return( 0 );
  }

/* 
 * set the power estimation previously calculated by the powtrace command
 */
private int getpow() 
  {
#ifdef TCL_IRSIM 
    Tcl_SetObjResult(irsiminterp, Tcl_NewDoubleObj((double)power));
#else
    lprintf(stdout, "Dynamic power estimate retrieved was = %f mW\n", power);
#endif 
    return( 0 );
  }


/*
 * display info about a node
 */
private int quest()
  {
    apply( info, NULL, targv[0] );
    return( 0 );
  }


/*
 * exit this level of command processing
 */
private int quit()
  {
    return( -1 );
  }


/*
 * return to command level
 */
private int doexit()
  {
    TerminateAnalyzer();

#ifdef TCL_IRSIM
    Tcl_Eval(irsiminterp, "catch {tkcon eval exit}\n");
    /* exit() call is not reached unless console is not present */
#endif

    exit( (targc == 2) ? atoi( targv[1] ) : 0 );
  }


/*
 * destroy sequence for given node/vector: update sequence list and length.
 * return -1 if we can't destroy the sequence (in stopped state).
 */
int undefseq( p, list, lmax )
  nptr  p;
  sptr  *list;
  int   *lmax;
  {
    register sptr  u, t;
    register int   i;

    for( u = NULL, t = *list; t != NULL; u = t, t = t->next )
	if( t->ptr.n == p )
	    break;
    if( t )
      {
	if( stopped_state )	/* disallow changing sequences if stopped */
	    return( -1 );
	if( u == NULL )
	    *list = t->next;
	else
	    u->next = t->next;
	Vfree( t );
	for( i = 0, t = *list; t != NULL; t = t->next )
	    if( t->nvalues > i ) i = t->nvalues;
	*lmax = i;
      }
    return( 0 );
  }


/*
 * process command line to yield a sequence structure.  first arg is the
 * name of the node/vector for which the sequence is to be defined, second
 * and following args are the values.
 */
private void defsequence( list, lmax )
  sptr  *list;
  int   *lmax;
  {
    register sptr  s;
    register nptr  n = NULL;
    register bptr  b;
    register int   i;
    register char  *q;
    int            which, size;

	/* if no arguments, get rid of all the sequences we have defined */
    if( targc == 1 )
      {
	while( *list != NULL )
	    (void) undefseq( (*list)->ptr.n, list, lmax );
	return;
      }

	/* see if we can determine if name is for node or vector */
    for( b = blist; b != NULL; b = b->next )
	if( str_eql( b->name, targv[1] ) == 0 )
	  {
	    which = 1;    size = b->nbits;    goto okay;
	  }

    n = find( targv[1] );
    if( n == NULL )
      {
	rsimerror( filename, lineno, "%s: No such node or vector\n", targv[1] );
	return;
      }
    UnAlias( n );
    if( n->nflags & MERGED )
      {
	rsimerror(filename, lineno, "%s can't be part of a sequence\n", pnode(n));
	return;
      }
    which = 0; size = 1;

  okay :
    if( targc == 2 )	/* just destroy the given sequence */
      {
	(void) undefseq( which ? (nptr) b : n, list, lmax );
	return;
      }

    s = (sptr) Valloc( SIZEOF( sequence ) + size * (targc - 2) - 1, 0 );
    if( s == NULL )
      {
	rsimerror( filename, lineno, "Insufficient memory for sequence\n" );
	return;
      }
    s->which = which;
    s->vsize = size;
    s->nvalues = targc - 2;
    if( which )	s->ptr.b = b;
    else	s->ptr.n = n;

	  /* process each value specification saving results in sequence */
    for (i = 2, q = s->values; i < targc; i++, q += size)
    {
	char *val;

	val = readVector(targv[i], size);
	if (val == NULL)
	{
	    Vfree(s);
	    return;
	}
	strcpy(q, val);
	if (val != targv[i]) free(val);
    }

	/* all done!  remove any old sequences for this node or vector. */
    (void) undefseq( s->ptr.n, list, lmax );

	/* insert result onto list */
    s->next = *list;
    *list = s;
    if( s->nvalues > *lmax )
	*lmax = s->nvalues;
  }


/*
 * mark any vector that contains a deleted node (used by netupdate).
 */
private int mark_deleted_vectors()
  {
    register bptr  b;
    register int   i, total = 0;

    for( b = blist; b != NULL; b = b->next )
      {
	for( i = b->nbits - 1; i >= 0; i-- )
	  {
	    if( b->nodes[i]->nflags & DELETED )
	      {
		b->traced = DELETED;
		total ++;
		break;
	      }
	    UnAlias( b->nodes[i] );
	  }
      }
    return( total );
  }


/*
 * Remove all deleted nodes/vectors from the sequence list
 */
private int rm_from_seq( list )
  register sptr  *list;
  {
    register sptr  s;
    register int   max;

    max = 0;
    while( (s = *list) != NULL )
      {
	if( ((s->which) ? s->ptr.b->traced : s->ptr.n->nflags) & DELETED )
	  {
	    *list = s->next;
	    Vfree( s );
	  }
	else
	  {
	    if( s->which == 0 )
		UnAlias( s->ptr.n );

	    if( s->nvalues > max )
		max = s->nvalues;
	    list = &(s->next);
	  }
      }
    return( max );
  }


/*
 * Remove any deleted node/vector from any lists in which it may appear.
 */
public void rm_del_from_lists()
  {
    register iptr  w, *list;
    int            vec_del;

    vec_del = mark_deleted_vectors();

    maxsequence = rm_from_seq( &slist );
    maxclock = rm_from_seq( &xclock );

    if( analyzerON ) RemoveAllDeleted();

    for( list = &wvlist; (w = *list) != NULL; )
      {
	if( ((bptr) (w->inode))->traced & DELETED )
	  {
	    *list = w->next;
	    FreeInput( w );
	  }
	else
	    list = &w->next;
      }

    for( list = &wlist; (w = *list) != NULL; )
      {
	if( w->inode->nflags & DELETED )
	  {
	    *list = w->next;
	    FreeInput( w );
	  }
	else
	  {
	    UnAlias( w->inode );
	    list = &w->next;
	  }
      }

    if( vec_del )
      {
	register bptr  b, *lst;

	for( lst = &blist; (b = *lst) != NULL; )
	  {
	    if( b->traced & DELETED )
	      {
		*lst = b->next;
		Vfree( b->name );
		Vfree( b );
	      }
	    else
		lst = &b->next;
	  }
      }
  }


/*
 * set each node/vector in sequence list to its next value
 */
private void vecvalue( list, index )
  register sptr  list;
  int            index;
  {
    register int   offset, i;
    register nptr  *n;

    for( ; list != NULL; list = list->next )
      {
	offset = list->vsize * (index % list->nvalues);
	n = (list->which == 0) ? &list->ptr.n : list->ptr.b->nodes;
	for( i = 0; i < list->vsize; i++ )
	    (void) setin( *n++, &list->values[ offset++ ] );
      }
  }


/*
 * setup sequence of values for a node
 */
private int setseq()
  {
    CHECK_STOP();
	/* process sequence and add to list */
    defsequence( &slist, &maxsequence );
    return( 0 );
  }


/*
 * define clock sequences(s)
 */
private int setclock()
  {
    CHECK_STOP();
	/* process sequence and add to clock list */
    defsequence( &xclock, &maxclock );
    return( 0 );
  }


/*
 * Step each clock node through one simulation step
 */
private int step_phase()
{
    static int  which_phase = 0;

    vecvalue(xclock, which_phase);

    if (relax(cur_delta + stepsize))
	return(1);

    if (maxclock)
	which_phase = (which_phase + 1) % maxclock;

    return(0);
}


/* Do one simulation step */
private int dophase()
  {
    CHECK_STOP();

    if (xclock == NULL)
	rsimerror(filename, lineno, "no clock nodes defined!\n");
    else
    {
        (void) step_phase();
        if( ddisplay )
	    pnwatchlist();
    }

    return( 0 );
  }


/*
 * clock circuit specified number of times
 */

int clockit(n)
  register int  n;
{
    register int i = 0, j = 0;

    if (xclock == NULL)
	rsimerror(filename, lineno, "no clock nodes defined!\n");
    else
    {
	/*
	 * run by setting each clock node to successive values of its
	 * associated sequence until all phases have been run.
	 */

	while (n-- > 0) {
#ifdef TCL_IRSIM
	    if (++j == 50) {		/* check for interrupt every 50 cycles */
		j = 0;
		if (check_interrupt())
		    goto done;
	    }
#endif
	    for (i = 0; i < maxclock; i++)
		if (step_phase())
		    goto done;
	}

	/* finally display results if requested to do so */
done:
	if (ddisplay)
	    pnwatchlist();
    }
    return (maxclock - i);
}


/*
 * clock circuit through all the input vectors previously set up
 */

private int runseq()
{
    register int  i, n = 1;

    CHECK_STOP();

    /* calculate how many clock cycles to run */
    if (targc == 2)
    {
	n = atoi(targv[1]);
	if (n <= 0)
	    n = 1;
    }

    /*
     * run by setting each input node to successive values of its
     * associated sequence.
     */

    if (slist == NULL)
	rsimerror(filename, lineno, "no input vectors defined!\n");
    else
	while (n-- > 0)
	    for (i = 0; i < maxsequence; i++)
	    {
		vecvalue(slist, i);
		if (clockit(1))
		    return 0;
		if (ddisplay)
		    pnwatchlist();
#ifdef TCL_IRSIM
		if (check_interrupt())
		    return 0;
#endif
	    }

    return 0;
}


/*
 * process "c" command line
 */

private int doclock()
  {
    register int  n = 1;

    if( stopped_state )		/* continue after stop */
	return( 1 );

	/* calculate how many clock cycles to run */
    if( targc == 2 )
      {
	n = atoi( targv[1] );
	if( n <= 0 )
	    n = 1;
      }

    (void) clockit( n );		/* do the hard work */
    return( 0 );
  }


/*
 * output message to console/log file
 */
private int domsg()
  {
    int  n;

    for( n = 1; n < targc; n += 1 )
	lprintf( stdout, "%s ", targv[n] );
    lprintf( stdout, "\n" );
    return( 0 );
  }


/*
 * Return a number whose bits corresponding to the index of 'words' match
 * the argument.
 * If 'offwrd' is given then that argument turns all bits off.
 * If 'offwrd' is given and the argument is '*' turns all bits on.
 * if the argument is '?' the display the avaliable options (words).
 * With no arguments just prints the word whose corresponding bit is set.
 */
private int do_flags( bits, name, offwrd, words )
  int   bits;
  char  *name, *offwrd, *words[];
  {
    int  i, t, tmp;

    if( targc == 1 )
      {
	lprintf( stdout, "%s: ", name );
	if( bits == 0 and offwrd != NULL )
	    lprintf( stdout, offwrd );
	else
	  {
	    for( i = 0; words[i] != NULL; i++ )
		if( bits & (1 << i) )
		    lprintf( stdout, " %s", words[i] );
	  }
	lprintf( stdout, "\n" );
      }
    else if( targc == 2 and strcmp( targv[1], "?" ) == 0 )
      {
	lprintf( stdout, "%s options are:", name );
	if( offwrd )
	    lprintf( stdout, "[*][%s]", offwrd );
	for( t = '[', i = 0; words[i] != NULL; i++, t = ' ' )
	    lprintf( stdout, "%c%s", t, words[i] );
	lprintf( stdout, "]\n" );
      }
    else if( targc == 2 and offwrd != 0 and strcmp( targv[1], offwrd ) == 0 )
      {
	bits = 0;
      }
    else if( targc == 2 and offwrd != 0 and str_eql( targv[1], "*" ) == 0 )
      {
	for( i = 0; words[i] != NULL; i++ );
	bits = (1 << i) - 1;
      }
    else
      {
	for( t = 1, tmp = bits, bits = 0; t < targc; t++ )
	  {
	    for( i = 0; words[i] != NULL; i++ )
		if( str_eql( words[i], targv[t] ) == 0 )
		  {
		    bits |= (1 << i);
		    break;
		  }
	    if( words[i] == NULL )
	      {
		rsimerror( filename, lineno, "%s: Invalid %s option\n",
		 targv[t], name );
		bits = tmp;
		break;
	      }
	  }
      }
    return( bits );
  }


	/* various debug flags */
public
#define	DEBUG_EV		0x01		/* event scheduling */
public
#define	DEBUG_DC		0x02		/* final value computation */
public
#define	DEBUG_TAU		0x04		/* tau/delay computation */
public
#define	DEBUG_TAUP		0x08		/* taup computation */
public
#define	DEBUG_SPK		0x10		/* spike analysis */
public
#define	DEBUG_TW		0x20		/* tree walk */


/* set debugging level */
private int setdbg()
  {
    static char  *dbg[] = { "ev", "dc", "tau", "taup", "spk", "tw", NULL };
    int          new;

    new = do_flags( debug, "Debug", "off", dbg );
    if( new != debug )
      {
	debug = new;
	targc = 1;
	(void) do_flags( debug, "Debug is now", "OFF", dbg );
      }
    return( 0 );
  }


public
#define	REPORT_DECAY	0x1
public
#define	REPORT_DELAY	0x2
public
#define	REPORT_TAU	0x4
public
#define	REPORT_TCOORD	0x8
#ifdef POWER_EST
public
#define REPORT_CAP      0x10
#endif /* POWER_EST */



/*
 * set treport parameter
 */
private int setreport()
  {
    static char  *rep[] = { "decay", "delay", "tau", "tcoord", NULL };

    treport = do_flags( treport, "report", "none", rep );
    return( 0 );
  }


/*
 * set which evaluation model to use
 */
private int setmodel()
  {
    static char  *m_name[] = { "linear", "switch", NULL };
    int          new;

	/* note: the following will only work for 2 models! */
    new = do_flags( model_num + 1, "model", NULL, m_name ) - 1;

    if( new != model_num )
      {
	model_num = new;
	model = model_table[ model_num ];
	NewModel( new );
      }
    return( 0 );
  }


/*
 * set up or finish off logfile
 */
private int setlog()
  {
    if( logfile != NULL )
      {
	(void) fclose( logfile );
	logfile = NULL;
      }

    if( targc == 2 )
      {
	char  *mode = "w";
	char  *s = targv[1];

	if( *s == '+' )
	  {
	    s++;
	    mode = "a";
	  }
	if( (logfile = fopen( s, mode )) == NULL )
	rsimerror( filename, lineno, "cannot open log file %s for output\n", s );
      }
    return( 0 );
  }


#ifdef POWER_EST
/*
 * Helper routine for printing capacitance and power totals.
 */
private int capsummer( n )
  register nptr  n;
  {
    UnAlias( n );

    if( (not (n->nflags & (MERGED | ALIAS))) && (n->nflags & POWWATCHED) )
    {
	lprintf( stdout, " %-35s\t%.3f\t%5d\t%f\t%f\n",
			  pnode( n ), n->ncap, n->toggles,
			  n->toggles*n->ncap*powermult,
			  n->toggles*n->ncap / toggled_cap );
    }
    return( 0 );
  }

/*
 * set up or finish off logfile
 */
private int setcaplog()
  {
    if( caplogfile != NULL )
      {
	(void) fclose( caplogfile );
	caplogfile = NULL;
	capstoptime = d2ns(cur_delta);
	captime = capstoptime - capstarttime;
	powermult = vsupply*vsupply/(2*captime);
	walk_net( capsummer, (char *) 0 );
	power = toggled_cap * powermult * 1e-3;
	lprintf(stdout,
	 "Dynamic power estimate for powtrace'd nodes = %f Watts (%f)\n",
	   toggled_cap * powermult * 1e-3, toggled_cap);
      }

    if( targc == 2 )
      {
	char  *mode = "w";
	char  *s = targv[1];

	if( *s == '+' )
	  {
	    s++;
	    mode = "a";
	  }
	if( (caplogfile = fopen( s, mode )) == NULL )
	rsimerror( filename, lineno, "cannot open log file %s for output\n", s );
	capstarttime = d2ns(cur_delta);
      }
    return( 0 );
  }
#endif /* POWER_EST */

/*
 * restore state of network
 */
private int do_rdstate()
  {
    char  *err;

    CHECK_STOP();

    if( err = rd_state( targv[1], (targv[0][1] == '<') ? TRUE : FALSE ) )
	rsimerror( filename, lineno, err );
    return( 0 );
  }


/*
 * restore state of network
 */
private int do_wrstate()
  {
    if( wr_state( targv[1] ) )
	rsimerror( filename, lineno, "can not write state file: %s\n", targv[1] );
    return( 0 );
  }


/*
 * declare a power net name
 */

private int dopower()
  {
    if( targc == 2 )
      {
	if (*(targv[1]) == '\0')
	    *power_net_name = NULL;
	else
	  {
            power_net_name_size++;
	    VDD_node_size++;
	    if( power_net_name == NULL )
	        power_net_name = (char**)malloc(sizeof(char*));
	    else
	      {
	        power_net_name = (char**)realloc(power_net_name, power_net_name_size *
		  sizeof(char*));
              }
	    *(power_net_name + power_net_name_size - 1) = strdup(targv[1]);
	    if( VDD_node == NULL )
	        VDD_node = (nptr*)malloc(sizeof(nptr));
	    else
	      {
	        VDD_node = (nptr*)realloc(VDD_node, VDD_node_size *
		  sizeof(nptr));
              }
	    *(VDD_node + VDD_node_size - 1) = RsimGetNode( targv[1] );
 	  }
      }
    else
      {
	if (power_net_name != NULL)
	    lprintf( stdout, "Power net = \"%s\"\n", power_net_name );
	else
	    lprintf( stdout, "Power net name is not set, default is Vdd\n" );
      }

    return( 0 );
  }

/*
 * declare a ground net name
 */

private int doground()
  {
    if( targc == 2 )
      {
	if (*(targv[1]) == '\0')
	    ground_net_name = NULL;
	else
	    ground_net_name = strdup(targv[1]);
      }
    else
      {
	if (ground_net_name != NULL)
	    lprintf( stdout, "Ground net = \"%s\"\n", ground_net_name );
	else
	    lprintf( stdout, "Ground net name is not set, default is Gnd\n" );
      }

    return( 0 );
  }

/*
 * set decay parameter
 */
private int setdecay()
  {
    if( targc == 1 )
      {
	if( tdecay == 0 )
	    lprintf( stdout, "decay = No decay\n" );
	else
	    lprintf( stdout, "decay = %.3fns\n", d2ns( tdecay ) );
      }
    else
      {
	if( (tdecay = ns2d( atof( targv[1] ) )) < 0 )
	    tdecay = 0;
      }
    return( 0 );
  }

/*
 * set settling parameter (time for conflict resolution on multiply-driven nodes)
 */
private int setsettle()
  {
    if( targc == 1 )
      {
	if( settle == 0 )
	    lprintf( stdout, "secondary decay = No decay\n" );
	else
	    lprintf( stdout, "secondary decay = %.3fns\n", d2ns( settle ) );
      }
    else
      {
	if( (settle = ns2d( atof( targv[1] ) )) < 0 )
	    settle = 0;
      }
    return( 0 );
  }


/*
 * set unitdelay parameter
 */
private int setunit()
  {
    if( targc == 1 )
      {
	if( tunitdelay == 0 )
	    lprintf( stdout, "unitdelay = OFF\n" );
	else
	    lprintf( stdout, "unitdelay = %.2f\n", d2ns( tunitdelay ) );
      }
    else
      {
	if( (tunitdelay = (int) ns2d( atof( targv[1] ) )) < 0 )
	    tunitdelay = 0;
      }
    return( 0 );
  }

/*
 * print traceback of node's activity and that of its ancestors
 */
private void cpath( n, level )
  register nptr  n;
  int            level;
  {
    static long  ptime;		/* previous time, used during trace back */

	/* no last transition! */
    if( (n->nflags & MERGED) or n->t.cause == NULL )
      {
	lprintf( stdout, "  there is no previous transition!\n" );
      }
    else if( n->t.cause == inc_cause )
      {
	if( level == 0 )
	    lprintf( stdout,
	      "  previous transition due to incremental update\n" );
	else
	    lprintf( stdout,
	      "  transition of %s due to incremental update\n", pnode( n ) );
      }
	/* here if we come across a node which has changed more recently than
	 * the time reached during the backtrace.  We can't continue the
	 * backtrace in any reasonable fashion, so we stop here.
	 */
    else if( level != 0 and n->c.time > ptime )
      {
	lprintf( stdout,
	  "  transition of %s, which has since changed again\n", pnode( n ) );
      }
	/* here if there seems to be a cause for this node's transition.
	 * If the node appears to have 'caused' its own transition (n->t.cause
	 * == n), that means it was input.  Otherwise continue backtrace...
	 */
    else if( n->t.cause == n )
      {
	lprintf( stdout, "  %s -> %c @ %.3fns , node was an input\n",
	  pnode( n ), vchars[ n->npot ], d2ns( n->c.time ) );
      }
    else if( n->t.cause->nflags & VISITED )
      {
	lprintf( stdout, "  ... loop in traceback\n" );
      }
    else
      {
	long  delta_t = n->c.time - n->t.cause->c.time;

	n->nflags |= VISITED;
	ptime = n->c.time;
	cpath( n->t.cause, level + 1 );
	n->nflags &= ~VISITED;
	if( delta_t < 0 )
	    lprintf( stdout, "  %s -> %c @ %.3fns   (?)\n", pnode( n ),
		vchars[ n->npot ], d2ns( n->c.time ) );
	else
	    lprintf( stdout, "  %s -> %c @ %.3fns   (%.3fns)\n", pnode( n ),
		vchars[ n->npot ], d2ns( n->c.time ), d2ns( delta_t ) );
      }
  }

private int do_cpath( n )
  nptr  n;
  {
    lprintf( stdout, "critical path for last transition of %s:\n", pnode(n) );
    UnAlias( n );
    cpath( n, 0 );
    return( 1 );
  }


/*
 * discover and print critical path for node's last transistion
 */
private int dopath()
  {
    apply( do_cpath, NULL, (char *) 0 );
    return( 0 );
  }


#define	NBUCKETS		20	/* number of buckets in histogram */

typedef struct
  {
    long  begin, end, size;
    long  table[ NBUCKETS ];
  } Accounts;


/*
 * helper routine for activity command
 */
private int adoit( n, ac )
  register nptr      n;
  register Accounts  *ac;
  {
    if( not (n->nflags & (ALIAS | MERGED | POWER_RAIL)) )
      {
	if( n->c.time >= ac->begin and n->c.time <= ac->end )
	    ac->table[ (n->c.time - ac->begin) / ac->size ] += 1;
      }
    return( 0 );
  }


/*
 * print histogram of circuit activity in specified time interval
 */
private int doactivity()
  {
    register int  i;
    static char   st[] = "**************************************************";
    Accounts      ac;
    long          total;

#   define	SIZE_ST		( sizeof( st ) - 1 )

    if( targc == 2 )
      {
	ac.begin = ns2d( atof( targv[1] ) );
	ac.end = cur_delta;
      }
    else
      {
	ac.begin = ns2d( atof( targv[1] ) );
	ac.end = ns2d( atof( targv[2] ) );
      }

    if( ac.end < ac.begin )
	SWAP( long, ac.end, ac.begin );

	/* collect histogram info by walking the network */
    for( i = 0; i < NBUCKETS; ac.table[ i++ ] = 0 );

    ac.size = (ac.end - ac.begin + 1) / NBUCKETS;
    if( ac.size <= 0 )
	ac.size = 1;

    walk_net( adoit, (char *) &ac );

	/* print out what we found */
    for( total = i = 0; i < NBUCKETS; i++ ) total += ac.table[i];

    lprintf( stdout,
      "Histogram of circuit activity: %.2f -> %.3fns (bucket size = %.2f)\n",
      d2ns( ac.begin ), d2ns( ac.end ), d2ns( ac.size ) );

    for( i = 0; i < NBUCKETS; i += 1 )
	lprintf( stdout, " %10.2f -%10.2f%6d  %s\n",
	  d2ns(ac.begin + (i * ac.size)), d2ns(ac.begin + (i + 1) * ac.size),
	  ac.table[i], &st[ SIZE_ST - (SIZE_ST * ac.table[i]) / total ] );

    return( 0 );
  }

/*
 * obtain histogram data for various power ranges over simulation time 
 */
private int dopowhist()
  {
    register   int i;
    float      hist_power;
    Ulong      time = 0;	/* store the current time from histogramming */

    if( targc == 2 )
      {
	if( str_eql( "capture", targv[1] ) == 0 ) 
	  {
            if( hist == NULL )	 
	      {	    
                lprintf( stdout, "Histogram has not been initialized" );
	        return( -1 );
	      }
            else
	      {
		if ( histpstepstart == -1 )
		  {
		    histpstepstart = cur_delta;
		    return 0;
		  }
		hist_power = step_pow_x_trans/(2*(d2ns(cur_delta-histpstepstart)));	    	        
		i = (int)( (hist_power-hist[0].range) / (hist[1].range - hist[0].range));
		if (i >= 0 and i < size) 
	            hist[i].bins++;
		histpstepstart = cur_delta;
		step_cap_x_trans = 0;
		step_pow_x_trans = 0;
	      }
	  }
	else if ( str_eql( "print", targv[1] ) == 0 )
	  {
            if ( hist == NULL )
 		return -1;			
	    lprintf( stdout,
      	      "Histogram of power: %.2f -> %.3fns (bucket size = %.2f)\n",
      	      d2ns( time ), d2ns( cur_delta ), (max - min) / size );

    	    for( i = 0; i < size; i++ ) 
	      {
		lprintf( stdout, " %10.2f -%10.2f%6d\n",
	  	  hist[i].range, hist[i].range + ((max - min) / size),
	  	  hist[i].bins );
	      }
	  }
        else if ( str_eql( "reset", targv[1] ) == 0 ) 
	  {
            if ( hist == NULL )
            	return( -1 );
	    min = -1;
	    max = -1;
	    size = -1;
	    free(hist);
	    time = cur_delta;
	    hist = NULL;
	    lprintf( stdout, "Histogram has been reset" );
	  }
	else
	    rsimerror( filename, lineno, "don't know what '%s' means\n", targv[1] );
      }    
    else if( str_eql( "init", targv[1] ) == 0 ) 
      {
	if( targc == 4 ) 
            size = NBUCKETS;
	else if( targc == 5 )
	    size = atoi( targv[4] );
	
	min = (float)atof(targv[2]);
	max = (float)atof(targv[3]);
        if( min >= max ) 
	  {
	    lprintf( stdout, "min greater than max" );
	    return( -1 );	/* this is an error */	
	  }
	hist = malloc( size * sizeof(struct _buckets) );	
	for( i = 0; i < size; i++ )
	  {
	    hist[i].range = min + ((max - min) / size) * i;
	    hist[i].bins = 0;
	  } 
	time = cur_delta;
	phist = 1;	
      }        
    return( 0 );
  }

/*
 * Helper routine for "changes" command.
 */
private int cdoit( n, ac )
  register nptr  n;
  Accounts  *ac;
  {
    int   i;

    UnAlias( n );

    if( n->nflags & (MERGED | ALIAS) )
	return( 0 );

    if( n->c.time >= ac->begin and n->c.time <= ac->end )
      {
	i = strlen( pnode( n ) ) + 2;
	if( column + i >= MAXCOL )
	  {
	    lprintf( stdout, "\n" );
	    column = 0;
	  }
	column += i;
	lprintf( stdout, "  %s", pnode( n ) );
      }
    return( 0 );
  }


/*
 * Print list of nodes which last changed value in specified time interval
 */
private int dochanges()
  {
    Accounts  ac;

    if( targc == 2 )
      {
	ac.begin = ns2d( atof( targv[1] ) );
	ac.end = cur_delta;
      }
    else
      {
	ac.begin = ns2d( atof( targv[1] ) );
	ac.end = ns2d( atof( targv[2] ) );
      }

    column = 0;
    lprintf(stdout,"Nodes with last transition in interval %.2f -> %.3fns:\n",
      d2ns( ac.begin ), d2ns( ac.end ) );

    walk_net( cdoit, (char *) &ac );
    if( column != 0 )
	lprintf( stdout, "\n" );

    return( 0 );
  }

/*
 * Helper function for doXRelax()
 */
private int xrelax(n, type)
  register nptr  n;
  char *type;
{
   if (n->npot == X) {
      char set_type = *type;
      if (set_type == X) {
	 if (rand() % 2 == 1)
	    set_type = LOW;
	 else
	    set_type = HIGH;
      }
      enqueue_input(n, set_type);
   }
   return 0;
}

/*
 * Attempt to resolve all undefined values.  This is likely to be an
 * evolving algorithm.  First cut is to set all undriven values to
 * zero.  This step may be removed later.  The ultimate goal is to
 * resolve nodes driven by conflicting driven values.
 */
private int doXRelax()
{
   char set_type = LOW;

   /* Optional argument indicates type to set to:  l for LOW,	*/
   /* h for HIGH, and r for RANDOM.  For random, we pass the	*/
   /* type X to the callback routine.				*/

   if (targc == 2) {
      if (!strncmp(targv[1], "h", 1))
	 set_type = HIGH;
      else if (strncmp(targv[1], "r", 1))
	 set_type = X;
   }
	 
   walk_net(xrelax, &set_type);
   /* Step with zero time to set values */
   step(cur_delta);
   return (0);   
}

/*
 * Helper routine for "printx" command.
 */
private int xdoit( n, ac )
  register nptr  n;
  Accounts  *ac;
  {
    int   i;

    UnAlias( n );

    if( not (n->nflags & (MERGED | ALIAS)) and n->npot == X )
      {
	i = strlen( pnode( n ) ) + 2;
	if( column + i >= MAXCOL )
	  {
	    lprintf( stdout, "\n" );
	    column = 0;
	  }
	column += i;
	lprintf( stdout, "  %s", pnode( n ) );
      }
    return( 0 );
  }

/*
 * Print list of nodes with undefined (X) value
 */
private int doprintX()
  {
    lprintf( stdout, "Nodes with undefined potential:\n" );
    column = 0;
    walk_net( xdoit, (char *) 0 );
    if( column != 0 )
	lprintf( stdout, "\n" );
    return( 0 );
  }

/*
 * Helper routine for printing aliases
 */

private int aldoit(n, canonical)
  nptr  n;
  char *canonical; 
{
    char  *nname = pnode(n);
    char  *is_merge;

    if (n->nflags & ALIAS)
    {
	UnAlias(n);
	is_merge = (n->nflags & MERGED) ? " (part of a stack)" : "";
	if ((canonical == NULL) || (!strcmp(nname, canonical)))
	    lprintf(stdout, "  %s -> %s%s\n", nname, pnode(n), is_merge);
    }
    return (0);
}


/*
 * Print (or create) nodes that are aliases
 */

private int doprintAlias()
{
    char *aliasroot = NULL;

    if (targc >= 3) {
       alias(targc, targv);
    }
    else {
       if (targc >= 2) aliasroot = targv[1];
       if (naliases == 0)
	  lprintf( stdout, "there are no aliases\n" );
       else {
	  if (targc == 1)
	      lprintf(stdout, "there are %d aliases:\n", naliases);
	  walk_net(aldoit, aliasroot);
       }
    }
    return (0);
}


/*
 * Helper routine to print pending events
 */
private int print_list( n, l, eolist, doX )
  evptr  l, eolist;
  int    n;
  int	 doX;
  {
    if( l == NULL )
	return( n );
    for( eolist = eolist->flink; l != eolist and n != 0; l = l->flink, n-- )
      {
	if ((doX == FALSE) && (vchars[l->eval] == 'X')) continue;
	lprintf( stdout, "Node %s -> %c @ %.3fns (%.3fns)\n",
	  pnode( l->enode ), vchars[ l->eval ], d2ns( l->ntime ),
	  d2ns( l->ntime - cur_delta ) );
      }
    return( n );
  }


/*
 * Print list of pending events, including pending unknowns.
 */
private int printPendingX()
  {
    int    n;
    Ulong   delta = 0;
    evptr  list, eolst;

    n = (targc == 2) ? atoi( targv[1] ) : -1;

    while( (delta = pending_events( delta, &list, &eolst )) and n != 0 )
        n = print_list( n, list, eolst, TRUE );
    n = print_list( n, list, eolst, TRUE );
    return( 0 );
  }

/*
 * Print list of pending events, excluding pending unknowns.
 */
private int printPending()
  {
    int    n;
    Ulong   delta = 0;
    evptr  list, eolst;

    n = (targc == 2) ? atoi( targv[1] ) : -1;

    while( (delta = pending_events( delta, &list, &eolst )) and n != 0 )
        n = print_list( n, list, eolst, FALSE );
    n = print_list( n, list, eolst, FALSE );
    return( 0 );
  }



/*
 * set/reset various display parameters
 */
private int dodisplay()
{
    static char    cmdfile_str[] = "cmdfile", automatic_str[] = "automatic";
#ifdef TCL_IRSIM
    static char	   tclproc_str[] = "tclproc";
#endif
    register int   i, value;
    register char  *p;

    if (targc == 1)
    {
	lprintf(stdout, "display = %s%s %s%s",
		dcmdfile ? "" : "-", cmdfile_str,
		ddisplay ? "" : "-", automatic_str);

#ifdef TCL_IRSIM
	if (tcldproc)
	    lprintf(stdout, " %s=%s", tclproc_str, tcldproc);
	else
	    lprintf(stdout, "-%s", tclproc_str);
#endif

	lprintf(stdout, "\n");
	return(0);
    }

    for (i = 1; i < targc; i++)
    {
	p = targv[i];
	if (*p == '-')
	{
	    value = 0;
	    p += 1;
	}
	else
	    value = 1;

	if (str_eql(p, cmdfile_str) == 0)
	    dcmdfile = value;
	else if (str_eql(p, automatic_str) == 0)
	    ddisplay = value;
#ifdef TCL_IRSIM
	else if(str_eql(p, tclproc_str) == 0)
	{
	    if (tcldproc != NULL)
	    {
		free(tcldproc);
		tcldproc = (char *)NULL;
	    }
	    if ((value == 1) && (i == targc - 1))
	    {
		rsimerror(filename, lineno, "Usage: display tclproc <name>");
	    }
	    else if (value == 1)
	    {
		i++;
		p = targv[i];
		if (strcmp(p, "")) tcldproc = strdup(p);
	    }
	}
#endif
	else
	    rsimerror(filename, lineno, "unrecognized display parameter: %s\n",
	      targv[i]);
    }
    return(0);
}


/*
 * Print list of transistors with src/drn shorted (or between power supplies).
 */
private int print_tcap()
  {
    tptr  t;

    if( tcap->scache.t == tcap )
	lprintf( stdout, "there are no shorted transistors\n" );
    else
	lprintf( stdout, "shorted transistors:\n" );
    for( t = tcap->scache.t; t != tcap; t = t->scache.t )
      {
	lprintf( stdout, " %s g=%s s=%s d=%s (%gx%g)\n",
	  device_names[t->ttype]->devname, 
	  pnode( t->gate ), pnode( t->source ), pnode( t->drain ),
	  t->r->length / (double) LAMBDACM, t->r->width / (double) LAMBDACM );
      }
    return( 0 );
  }


private int set_incres()
  {
    if( targc == 1 )
	lprintf( stdout, "incremental resolution = %.2f\n", d2ns(INC_RES) );
    else
      {
	long  new_res = (long) ns2d( atof( targv[1] ) );

	if( new_res < 0 )
	    rsimerror( filename, lineno, "resolution must be positive\n" );
	else
	    INC_RES = new_res;
      }
    return( 0 );
  }


private	char	*changelog = NULL;


/*
 * set the changes-log filename. 
 */
private int setlogchanges()
  {
    Fstat  *stat;

    if( targc == 1 )
      {
	lprintf( stdout, "changes-logfile is %s\n",
	  (changelog == NULL) ? "turned OFF" : changelog );
      }
    else
      {
	if( str_eql( "off", targv[1] ) == 0 )
	  {
	    if( changelog != NULL )
	      {
		Vfree( changelog );
		changelog = NULL;
	      }
	  }
	else
	  {
	    stat = FileStatus( targv[1] );
	    if( stat->write == 0 )
		lprintf( stdout, "can't write to file '%s'\n", targv[1] );
	    else
	      {
		if( stat->exist == 0 )
		    lprintf( stdout, "OK, starting a new log file\n" );
		else
		    lprintf( stdout,"%s already exists, will append to it\n",
		      targv[1] );
		if( changelog != NULL )
		    Vfree( changelog );
		changelog = Valloc( strlen( targv[1] ) + 1, 0 );
		if( changelog != NULL )
		    (void) strcpy( changelog, targv[1] );
		else
		    lprintf( stderr, "out of memory, logfile is OFF\n" );
	      }
	  }
      }
    return( 0 );
  }

/*
 * Schedule a procedure to occur at a specific time or cyclically
 */
private int schedule()
{
    Ulong stime;
    long interval = 0;
    char *proc;
    evptr new;
    int procidx = 2;

    if (targc != 3)
    {
	if (targc != 4 || targv[0][0] != 'e')
	{
	    rsimerror(filename, lineno, "Missing time and/or procedure\n" );
	    return(0);
	}
    }
    else if (targc == 3)
    {
	/* Note that "cancel" and "get" don't differentiate between	*/
	/* "every" and "at" commands.					*/

	if (!strcmp(targv[1], "cancel"))
	{
	    short schedidx = atoi(targv[2]);
	    /* Remove the scheduled event */
	    DequeueScheduled(schedidx);
	    return(0);
	}
	else if (!strcmp(targv[1], "get"))
	{
	    evptr ev;
	    short schedidx = atoi(targv[2]);
	    ev = FindScheduled(schedidx);
	    if (ev)
		lprintf(stdout, "%s\n", (char *)ev->enode);
	    return(0);
	}
    }

    stime = ns2d(atof(targv[1]));
    if (targv[0][0] == 'e')		/* "every" command */
    {
	interval = (long)stime;
	if (targc == 4)
	{
	    procidx++;
	    stime = ns2d(atof(targv[2]));	/* Start at stated time */
	    if (targv[2][0] == '+')
		stime += cur_delta;		/* Relative to "now" */
	}
	else
	    stime += cur_delta;		/* Schedule <interval> from now */
    }
    else if (targv[1][0] == '+')
	stime += cur_delta;		/* Time relative to "now" */

    if (stime < cur_delta)
    {
	rsimerror(filename, lineno, "%s: invalid time\n", targv[1]);
	return(0);
    }

    proc = strdup(targv[procidx]);

    /* Create a new event */
    new = EnqueueOther(TIMED_EV, stime);

    new->enode = (nptr)proc;
    new->delay = interval;
    new->rtime = scheduletag;

#ifdef TCL_IRSIM
    Tcl_SetObjResult(irsiminterp, Tcl_NewIntObj((int)scheduletag++));
#endif

    /* Due to limited handling of return values, the non-Tcl version	*/
    /* doesn't record scheduled event numbers.				*/
    return (0);
}

/*
 * read changes and do incremental simulation.
 */
private int do_incsim()
  {
    iptr  ch_list = NULL;

    CHECK_STOP();

    if( sim_time0 != 0 )
      {
	lprintf( stderr, "Warning: part of the history was flushed:\n" );
	lprintf( stderr, "         incremental results may be wrong\n" );
      }

    ch_list = rd_changes( targv[1], changelog );

    if( ch_list == NULL )
	lprintf( stdout, "no affected nodes: done\n" );
    else
	incsim( ch_list );
    if( ddisplay )
	pnwatchlist();
    else
	prtime( 0 );
    return( 0 );
  }


private char    x_display[40];

private int xDisplay()
  {
    char  *s, *getenv();

    if( targc == 1 )
      {
	s = x_display;
	if( *s == '\0' )
	    s = getenv( "DISPLAY" );
	lprintf( stdout, "DISPLAY = %s\n", (s == NULL) ? "unknown" : s );
      }
    else if( analyzerON )
	lprintf( stdout, "analyzer running, can't change display\n" );
    else
	(void) strcpy( x_display, targv[1] );

    return( 0 );
  }


/*
 * Startup analyzer window and/or add more signals to analyzer display list.
 */

private int analyzer()
{
    extern int AddNode(), AddVector(), OffsetNode(), OffsetVector();

    if (!analyzerON)
    {
#ifndef TCL_IRSIM
	if (!InitDisplay(first_file, (*x_display) ? x_display : NULL))
	    return(0);
	InitTimes(sim_time0, stepsize, cur_delta, 1);
#endif
    }

    if (targc > 1)
    {
	int  voffset = 0, ndigits = 0;

	if (strlen(targv[1]) > 1)
	{
	    if ((targv[1][0] == '-') && (targv[1][2] == '\0'))
	    {
		switch (targv[1][1])
		{
		    case 'b' :	ndigits = 1;	shift_args(TRUE);	break;
		    case 'o' :	ndigits = 3;	shift_args(TRUE);	break;
		    case 'h' :	ndigits = 4;	shift_args(TRUE);	break;
		    default : ;
		}
	    }
	    else if ((targv[1][0] == '-') && (!strncmp(&targv[1][1], "off", 3)))
	    {
		shift_args(TRUE);
		if (targc > 1) {
		    voffset = atoi(targv[1]);
		    shift_args(TRUE);
		}
	    }
	}

	if (targc > 1)
	    apply(AddNode, AddVector, (char *) &ndigits);

	if (voffset > 0)
	    apply(OffsetNode, OffsetVector, (char *) &voffset);
    }

    DisplayTraces(analyzerON);		/* pass 0 first time */

    analyzerON = TRUE;
    return(0);
}

#ifdef TCL_IRSIM

/* 
 * This is the same as the above called with no arguments,
 * and made public so that it can be called from the Tk
 * window generator routine in tkAnalyzer.c.
 */

public int start_analyzer(tkwind)
    Tk_Window tkwind;
{
    if (!analyzerON)
    {
	if (!InitDisplay(first_file, tkwind))
	    return(0);
	InitTimes(sim_time0, stepsize, cur_delta, 1);
    }

    DisplayTraces(analyzerON);		/* pass 0 first time */

    analyzerON = TRUE;
    return(0);
}

#endif

/*
 * Clear the analyzer display list.
 */
private int clear_analyzer()
  {
    if( analyzerON )
	ClearTraces();
    else
	lprintf( stdout, "analyzer is off\n" );
    return( 0 );
  }


/*
 * Write out entire history to a file.
 */
private int dump_hist()
  {
    char  fname[ 256 ];

    if( first_file == NULL or cur_delta == 0 )
      {
	rsimerror( filename, lineno, "Nothing to dump\n" );
	return( 0 );
      }

    if( targc == 1 )
	(void) sprintf( fname, "%s.hist", first_file );
    else
	(void) strcpy( fname, targv[1] );

    DumpHist( fname );

    return( 0 );
  }


/*
 * Read network history from a file.
 */
private int do_readh()
  {
    if( analyzerON )	StopAnalyzer();
    ReadHist( targv[1] );
    if( analyzerON )	RestartAnalyzer( sim_time0, cur_delta, TRUE );
    return( 0 );
  }


/*
 * Move back simulation time to specified time.
 */
private int back_time()
  {
    long  newt;

    CHECK_STOP();

    newt = ns2d( atof( targv[1] ) );
    if( newt < sim_time0 or newt >= cur_delta )
      {
	rsimerror( filename, lineno, "%s: invalid time\n", targv[1] );
	return( 0 );
      }
    if( analyzerON )	StopAnalyzer();

    cur_delta = newt;
    ClearInputs();
    (void) back_sim_time( cur_delta, FALSE );
    cur_node = NULL;			/* fudge */
    walk_net( backToTime, (char *) 0 );
    if( cur_delta == 0 )
	ReInit();

    if( analyzerON )	RestartAnalyzer( sim_time0, cur_delta, TRUE );

    pnwatchlist();
    return( 0 );
  }


/*
 * Write network to file.
 */
private int wr_net()
  {
    char  fname[ 256 ];

    if( first_file == NULL )
      {
	rsimerror( filename, lineno, "No network?\n" );
	return( 0 );
      }

    if( targc == 1 )
	(void) sprintf( fname, "%s.inet", first_file );
    else
	(void) strcpy( fname, targv[1] );

    wr_netfile( fname );

    return( 0 );
  }


#ifdef STATS

int  ev_hgm = 0;
private	struct
  {
    hptr  head;
    hptr  tail;
  } ev_hgm_table[5];

void IncHistEvCnt( tp )
  int   tp;
  {
    hptr  h;
    long  tm;
    int   indx;

    if( ev_hgm == 0 ) return;

    switch( tp )
      {
	case -1:					indx = 0;	break;
	case PUNTED:	case REVAL:	case DECAY_EV:	indx = 1;	break;
	case STIM_INP:	case STIM_XINP:	case STIMULI:	indx = 2;	break;
	case CHECK_PNT:					indx = 3;	break;
	case XINP_EV:	case INP_EV:			indx = 4;	break;
	default :					return;
      }
    
    tm = cur_delta / 10;
    h = ev_hgm_table[ indx ].tail;
    if( h->time == tm )
	h->t.xx += 1;
    else
      {
	if( (h = freeHist) == NULL )
	    h = (hptr) MallocList( sizeof( HistEnt ), 1 );
	freeHist = h->next;
	if( ev_hgm_table[ indx ].tail == last_hist )
	    ev_hgm_table[ indx ].head = h;
	else
	    ev_hgm_table[ indx ].tail->next = h;
	ev_hgm_table[ indx ].tail = h;
	h->next = last_hist;
	h->time = tm;
	h->t.xx = 1;
      }
  }


private int do_ev_stats()
  {
    int   i;

    if( targc == 1 )
      {
	lprintf( stdout, "event recording is %s\n", (ev_hgm) ? "ON" : "OFF" );
	return( 0 );
      }

    if( str_eql( "on", targv[1] ) == 0 )
      {
	static int last = 5;

	ev_hgm = 1;
	for( i = 0; i < last; i++ )
	    ev_hgm_table[i].head = ev_hgm_table[i].tail = last_hist;
	last = 0;
      }
    else if( str_eql( "clear", targv[1] ) == 0 )
      {
	for( i = 0; i < 5; i++ )
	    ev_hgm_table[i].head = ev_hgm_table[i].tail = last_hist;
      }
    else if( str_eql( "off", targv[1] ) == 0 )
	ev_hgm = 0;
    else
	rsimerror( filename, lineno, "don't know what '%s' means\n", targv[1] );

    return( 0 );
  }


private int do_pr_ev_stats()
  {
    static char *ev_name[] =
      { "evaluation", "I-evaluation", "stimulus", "check-point", "input" };
    FILE       *fp;
    int        i, lim, j;
    hptr       h;

    if( targc == 2 )
      {
	fp = fopen( targv[1], "w" );
	if( fp == NULL )
	  {
	    rsimerror( filename, lineno, "cannot open file '%s'\n", targv[1] );
	    return( 0 );
	  }
      }
    else if( logfile )
	fp = logfile;
    else
	fp = stdout;

    fprintf( fp, "Event Activity" );

    lim = (i_nevals != 0) ? 5 : 1;

    for( i = j = 0; i < lim; i++ )
      {
	h = ev_hgm_table[i].head;
	if( h == last_hist ) continue;
	j++;
	fprintf( fp, "\n** %s:\n", ev_name[i] );
	while( h != last_hist )
	  {
	    fprintf( fp, "%d\t%d\n", h->time, h->t.xx );
	    h = h->next;
	  }
	fprintf( fp, "\n" );
      }
    if( j == 0 )
      {
	fprintf( fp, ": Nothing Recorded\n" );
	if( targc == 2 ) lprintf( fp, ": Nothing Recorded\n" );
      }
    if( targc == 2 )
	fclose( fp );
    return( 0 );
  }

#endif /* STATS */

#ifdef CL_STATS

private	int CLcount[ 1001 ];

void RecordConnList( ntx )
  int             ntx;
  {
    if( ntx > 1000 ) ntx = 1000;
    CLcount[ ntx ] += 1;
  }

private int cl_compar( a, b )
  short  *a, *b;
  {
    return( CLcount[ *b ] - CLcount[ *a ] );
  }

int do_cl_stats()
  {
    short   cnt_indx[ 1001 ];
    FILE    *fp;
    int     i, ch, tot, n;
    double  avg, dev;
    extern double sqrt();

    if( targc == 2 )
      {
	fp = fopen( targv[1], "w" );
	if( fp == NULL )
	  {
	    rsimerror( filename, lineno, "cannot open file '%s'\n", targv[1] );
	    return( 0 );
	  }
      }
    else if( logfile )
	fp = logfile;
    else
	fp = stdout;

    for( avg = 0, tot = i = 0; i <= 1000; i++ )
      {
	cnt_indx[i] = i;
	if( CLcount[i] > 0 )
	  {
	    avg += i * CLcount[i];
	    tot += CLcount[i];
	  }
      }
    avg = avg / tot;

    for( dev = 0, i = 0; i <= 1000; i++ )
      {
	if( CLcount[i] > 0 )
	    dev += CLcount[i] * (i - avg) * (i - avg);
      }
    dev = sqrt( dev / tot );

    qsort( cnt_indx, 1001, sizeof( short ), cl_compar );

    fprintf( fp, "Connection-list statistics\n" );
    fprintf( fp, "\tavg-num-trans = %.2f  std-deviation = %.2f\n", avg, dev );

    fprintf( fp, "num-trans  num-times      %%  %%accum\n" );
    fprintf( fp, "---------  ---------  -----  ------\n" );
    avg = tot;
    dev = 0;
    for( i = 0; i <= 1000; i++ )
      {
	double  p;

	n = cnt_indx[ i ];
	if( (tot = CLcount[ n ]) == 0 )
	    continue;
	ch = (n == 1000) ? '>' : ' ';
	p = 100.0 * tot / avg;
	dev += p;
	fprintf( fp, "%c%8d  %9d  %5.2f  %6.2f\n", ch, n, tot, p, dev );
      }

    if( targc == 2 )
	fclose( fp );
    return( 0 );
  }

#endif /* CL_STATS */


typedef struct
  {
    int  nsd, ng;
  } TranCnt;

private int count_trans( n, tt )
  nptr     n;
  TranCnt  *tt;
  {
    register lptr  l;
    register int   i;

    if( (n->nflags & (ALIAS | POWER_RAIL)) == 0 )
      {
	for( i = 0, l = n->ngate; l != NULL; l = l->next, i++ );
	tt->ng += i;
	for( i = 0, l = n->nterm; l != NULL; l = l->next, i++ );
	tt->nsd += i;
      }
    return( 0 );
  }


/*
 * Print event statistics.
 */
private int do_stats()
  {
    char  n1[10], n2[10];

    if( targc == 2 )
      {
	static TranCnt  tt = { 0, 0 };

	if( tt.ng == 0 and tt.nsd == 0 )
	  {
	    walk_net( count_trans, &tt );
	    lprintf( stdout, "avg: # gates/node = %g,  # src-drn/node = %g\n",
	      (double) tt.ng / nnodes, (double) tt.nsd / nnodes );
	  }
      }
    lprintf( stdout, "changes = %d\n", num_edges );
    lprintf( stdout, "punts (cns) = %d (%d)\n",
      num_punted, num_cons_punted );
    if( num_punted == 0 )
      {
	(void) strcpy( n1, "0.0" );
	(void) strcpy( n2, n1 );
      }
    else
      {
	(void) sprintf( n1, "%2.2f",
	  100.0 / ((float) num_edges / num_punted + 1.0) );
	(void) sprintf( n2, "%2.2f",
	  (float) (num_cons_punted * 100.0 /num_punted) );
      }
    lprintf( stdout, "punts = %s%%, cons_punted = %s%%\n", n1, n2 );

    lprintf( stdout, "nevents = %ld; evaluations = %ld\n", nevent, nevals );
    if( i_nevals != 0 )
      {
	lprintf( stdout, "inc. evaluations = %ld; events:\n", i_nevals );
	lprintf( stdout, "reval:      %ld\n", nreval_ev );
	lprintf( stdout, "punted:     %ld\n", npunted_ev );
	lprintf( stdout, "stimuli:    %ld\n", nstimuli_ev );
	lprintf( stdout, "check pnt:  %ld\n", ncheckpt_ev );
	lprintf( stdout, "delay chk:  %ld\n", ndelaychk_ev );
	lprintf( stdout, "delay ev:   %ld\n", ndelay_ev );
      }

    return( 0 );
  }


/*
 * Shift the command left/right by 1.  HOWEVER, never overwrite the
 * command (argument 0), as otherwise the Tcl tag callback feature
 * gets messed up, since it does a lookup on a table indexed by the
 * command name.  NOTE:  This is a hack solution, as a tag callback
 * function could try to look at command arguments.  A proper
 * solution would be to always pass the index of the argument to
 * shift over, rather than shifting everything.
 */
void shift_args( left )
  int  left;
  {
    register int   ac;
    register char  **ap;
    register char  *wp;

    if( left )
      {
	targc--;
	for( ac = 1, ap = targv, wp = wildCard; ac < targc; ac++ )
	    ap[ac] = ap[ac + 1],    wp[ac] = wp[ac + 1];
      }
    else
      {
	for( ac = targc++, ap = targv, wp = wildCard; ac >= 0; ac-- )
	    ap[ac + 1] = ap[ac],    wp[ac + 1] = wp[ac];
      }
  }


/*
 * time a command and print resource utilization.
 */
private int do_time()
  {
    char  usage_str[40];
    int   narg, i = 0;

    shift_args( TRUE );
    narg = targc;
    if( narg )
      {
	set_usage();
#ifndef TCL_IRSIM			/* TEMPORARY! */
	i = exec_cmd();
#endif
      }
    print_usage( narg, usage_str );	/* targc == 0 -> total usage */
    lprintf( stdout, "%s", usage_str );
    return( i );
  }

private int doHist()
  {
     if(targc == 1) {
        lprintf( stdout, "History is ");
        if(sm_stat && OUT_OF_MEM) lprintf( stdout, "off.\n");
        else lprintf( stdout, "on.\n"); }
     else {
        if(strcmp(targv[1], "on") != 0) {
           sm_stat |= OUT_OF_MEM; }
        else { sm_stat &= ~OUT_OF_MEM; }
     }
  return (0);
  }

/*
 * Flush out the recorded history up to the (optional) specified time.
 */
private int flush_hist()
  {
    Ulong  ftime;

    if( targc == 1 )
	ftime = cur_delta;
    else
      {
	ftime = ns2d( atof( targv[1] ) );
	if( ftime < 0 or ftime > cur_delta )
	  {
	    rsimerror( filename, lineno, "%s: Invalid flush time\n", targv[1] );
	    return( 0 );
	  }
      }

    if( ftime == 0 )
	return( 0 );

    if( analyzerON )	StopAnalyzer();

    FlushHist(ftime);
    sim_time0 = ftime;

    if( analyzerON )	RestartAnalyzer( sim_time0, cur_delta, TRUE );

    return( 0 );
  }


/*
 * Just print the string YES if transistor coordinates are avalible.
 * This is used by rsim magic interface to use transistor positions to
 * identify nodes.
 */

private int HasCoords()
{
    lprintf(stdout, "%s\n", (txt_coords) ? "YES" : "NO");
    return 0;
}


#ifdef FAULT_SIM

extern	int	add_trigger();
extern	int	add_sampler();
extern	int	add_prim_output();
extern	void	cleanup_fsim();
extern	void	exec_fsim();


private int parse_trigger()
  {
    int       edge;
    long      delay;
    Find1Arg  f;

    if( (targc < 3 or targc > 4 ) )
	goto bad_trigger;

    delay = (targc > 3) ? ns2d( atof( targv[3] ) ) : 0;

    edge = ch2pot( targv[2][0] );
    if( edge >= N_POTS )
	return( 1 );
    if( edge != LOW and edge != HIGH )
	goto bad_trigger;

    FindOne( &f );
    if( f.num > 1 or f.vec != NULL )
      {
	rsimerror( filename, lineno, "%s: not a single node\n", targv[1] );
	return( 1 );
      }
    if( add_trigger( f.node, edge, delay ) )
      {
	rsimerror( filename, lineno, "trigger: %s has no %s transitions\n",
	  pnode( f.node ), (edge == LOW) ? "1 -> 0" : "1 -> 0 " );
      }
    return( 0 );

  bad_trigger:
    rsimerror( filename, lineno, "expected: \"trigger\" node 0|1 [delay]\n");
    return( 1 );
  }


private int parse_sampler()
  {
    long  period, offset = 0;

    if( targc < 2 or targc > 3 )
	goto bad_sample;

    period = ns2d( atof( targv[1] ) );
    if( period <= 0 )
      {
	rsimerror( filename, lineno, "%s: Illegal period\n", targv[1] );
	return( 1 );
      }
    if( targc == 3 )
      {
	offset = ns2d( atof( targv[2] ) );
	if( offset < 0 )
	    goto bad_sample;
      }

    if( cur_delta <= offset )
      {
	rsimerror( filename, lineno, "can't sample, simulation time too small\n");
	return( 1 );
      }

    return( add_sampler( period, offset ) );

  bad_sample :
    rsimerror( filename, lineno, "expected: \"sample\" period [offset]\n" );
    return( 1 );
  }


private int setup_fsim( file, p_seed )
  char  *file;
  int   *p_seed;
  {
    FILE      *fp;
    char      line[ 256 ];
    int       olineno = lineno;
    char      *ofname = filename;
    int       doing_outputs, err, percent, look_p;

    if( (fp = fopen( file, "r" )) == NULL )
      {
	rsimerror( filename, lineno, "cannot open '%s'\n", file );
	return( 1 );
      }

    filename = file;
    lineno = 0;
    err = percent = 0;
    doing_outputs = FALSE;
    look_p = TRUE;

    while( not err and fgetline( line, 256, fp ) != NULL )
      {
	lineno += 1;
	parse_line( line, 256 );
	if( targc == 0 )
	    continue;

	if( look_p )	/* seed must be 1st non-empty line in file */
	  {
	    look_p = FALSE;
	    if( str_eql( "seed", targv[0] ) == 0 )
	      {
		if( targc < 2 )
		  {
		    rsimerror( file, lineno, "syntax: \"seed\" <percentage>\n" );
		    err = 1;
		  }
		else
		  {
		    percent = atoi( targv[1] );
		    if( percent <= 0 or percent > 100 )
		      {
			rsimerror( file, lineno,
			  "percentage must be in the range [1-100]\n" );
			err = 1;
		      }
		  }
		continue;
	      }
	  }

	if( not doing_outputs )
	  {
	    if( str_eql( "sample", targv[0] ) == 0 )
		err = parse_sampler();
	    else if( str_eql( "trigger", targv[0] ) == 0 )
		err = parse_trigger();
	    else
	      {
		rsimerror( file, lineno, "expected: \"trigger\" or \"sample\"\n");
		err = 1;
	      }
	    doing_outputs = TRUE;
	  }
	else if( targc == 1 and strcmp( "***", targv[0] ) == 0 )
	  {
	    doing_outputs = FALSE;
	  }
	else
	  {
	    int  any = 0;
	    shift_args( FALSE );
	    apply( add_prim_output, NULL, (char *) &any );
	    if( any != 1 )
		err = 1;
	  }
      }

    (void) fclose( fp );
    filename = ofname;
    lineno = olineno;
    *p_seed = percent;
    return( err );
  }


private int do_fsim()
  {
    int   p_seed;
    char  *outname;

    CHECK_STOP();

    if( cur_delta == 0 )
      {
	lprintf( stderr, "Circuit needs to be simulated before faultsim\n" );
	return( 0 );
      }
    if( sim_time0 != 0 )
      {
	lprintf( stderr, "Can't faultsim: Incomplete history\n" );
	return( 0 );
      }

    outname = (targc == 3) ? targv[ 2 ] : NULL;

    if( setup_fsim( targv[1], &p_seed ) == 0 )
	exec_fsim( outname, p_seed );

    cleanup_fsim();

    return( 0 );
  }
#endif


/* Remove path and extension from a filename */
public char *BaseName( fname )
  register char  *fname;
  {
    register char  *s = fname;

    while( *s )
	s++;
    while( s > fname and *s != '/' )
	s--;
    fname = ( *s == '/' ) ? s + 1 : s;

    for( s = fname; *s != '\0' and *s != '.'; s++ );
    *s = '\0';
    return( fname );
  }


private	int do_help();	/* forward reference */


/* list of commands and their handlers */
public Command  cmds[] = 
  {

#ifdef TCL_IRSIM
    /* Some of the IRSIM commands are renamed for the Tcl version so as */
    /* not to conflict with Tcl syntax.					*/
    /* Note that "@" is missing, as it is covered by the Tcl "source"	*/
    /* command.								*/
    { "restorestate",	do_rdstate,	2,	2,
      "file -> restore network state from file"				},
    { "restoreall",	do_rdstate,	2,	2,
      "file -> restore network and inputs state from file"		},
    { "savestate",	do_wrstate,	2,	2,
      "file -> write current network state to file"			},
    { "querygate",	quest,		2,	MAXARGS,
      "node/vector... -> info regarding node(s) gate connections"	},
    { "querysource",	quest,		2,	MAXARGS,
      "node/vector... -> info regarding node(s) src/drn connections"	},
#else
    { "!",		quest,		2,	MAXARGS,
      "node/vector... -> info regarding node(s) gate connections"	},
    { "<",		do_rdstate,	2,	2,
      "file -> restore network state from file"				},
    { "<<",		do_rdstate,	2,	2,
      "file -> restore network and inputs state from file"		},
    { ">",		do_wrstate,	2,	2,
      "file -> write current network state to file"			},
    { "?",		quest,		2,	MAXARGS,
      "node/vector... -> info regarding node(s) src/drn connections"	},
    { "@",		cmdfile,	2,	2,
      "file -> read commands from command file"				},
#endif
    { "activity",	doactivity,	2,	3,
      "from [to] -> circuit activity in time interval"			},
    { "alias",		doprintAlias,	1,	MAXARGS,
      " ->  print node aliases"						},
    { "ana",		analyzer,	1,	MAXARGS,
      "shorthand for \"analyzer\""					},
    { "analyzer",	analyzer,	1,	MAXARGS,
      "[-b|-o|-h] node/vector... -> display node/vector(s) in analyzer"	},
    { "assert",		doAssert,	2,	4,
      "node/vector [[mask] val] -> assert node/vector = val [& mask = 0]"},
    { "assertWhen",		doAssertWhen,	5,	5,
      "nodeT valT node val -> assert node = val when nodeT switches to valT"},
    { "at",		schedule,	3,	3,
      "time {procedure} -> schedule procedure to occur at <time> ns"},
    { "back",		back_time,	2,	2,
      "time -> move simulation time back to specified to time"		},
    { "c",		doclock,	1,	2,
      "[n] -> simulate for n clock cycles (default 1)\n\
    -> or continue last simulation command prior to stopping"		},
    { "changes",	dochanges,	2,	3,
      "from [to] -> print nodes that changed in time interval"		},
    { "clear",		clear_analyzer,	1,	1,
      " -> remove all signals from analyzer window"			},
    { "clock",		setclock,	1,	MAXARGS,
      "[node/vector [val]] -> define clock sequence for node/vector"	},
    { "d",		pnlist,		1,	MAXARGS,
      "[node/vector]... -> print node/vector(s) or display-list"	},
    { "debug",		setdbg,		1,	MAXARGS,
      "[options] -> print/set debug state (? for help)"			},
    { "decay",		setdecay,	1,	2,
      "[n] -> set  charge  decay  time (0 => no decay)"			},
    { "display",	dodisplay,	1,	4,
      "[[-]option] -> print/set cmdfile and auto-watch-list display"	},
    { "dumph",		dump_hist,	1,	2,
      "[file] -> write network state history to file"			},
    { "every",		schedule,	3,	4,
      "interval [start] {proc} -> schedule proc to occur every interval ns"},
    { "exit",		doexit,		1,	2,
      "[status] -> exit program with given status (default: 0)"		},
#ifdef TCL_IRSIM
    { "histflush",	flush_hist,	1,	2,
      "[time] -> flush out history up to time (default:current-time)"	},
#else
    { "flush",		flush_hist,	1,	2,
      "[time] -> flush out history up to time (default:current-time)"	},
#endif
    { "h",		setvalue,	2,	MAXARGS,
      "node/vector... -> drive node/vector(s) to 1 (high)"		},
    { "has_coords",	HasCoords,	1,	1,
      " -> print if transistor coordinates are available"		},
#ifdef STATS
    { "histev",		do_ev_stats,	1,	2,
      "[clear | off | on] -> enable/disable/clear event activity record"},
    { "evstats",	do_pr_ev_stats,	1,	2,
      "[file] -> print event activity recorded"				},
#endif /* STATS */
#ifdef CL_STATS
    { "clstats",	do_cl_stats,	1,	2,
      "[file] -> print connection-list statistics"			},
#endif /* CL_STATS */
    { "ground",		doground,	1,	2,
      "[name] -> add name for ground net"				},
    { "help",		do_help,	1,	MAXARGS,
      "[command]... -> print info on command(s) or available commands"	},
    { "hist",           doHist,         1,      2,
       "[on/off] -> display or set history collection mode"             },
    { "inputs",		inputs,		1,	1,
      " -> print currently driven (input) nodes"			},
    { "ires",		set_incres,	1,	2,
      "[time] -> print/set incremental resolution to time"		},
    { "isim",		do_incsim,	2,	2,
      "[file] -> read changes from file and incrementally resimulate"	},
    { "l",		setvalue,	2,	MAXARGS,
      "node/vector... -> drive node/vector(s) to 0 (low)"		},
    { "logfile",	setlog,		1,	2,
      "[[+]file] -> start/stop log file (+file appends to file)"	},
    { "model",		setmodel,	1,	2,
      "[linear|switch] -> print/change simulation model"		},
    { "p",		dophase,	1,	1,
      "step clock one simulation step (phase)"				},
    { "path",		dopath,		2,	MAXARGS,
      "node/vector... -> critical path for last transition of node(s)"	},
    { "power",		dopower,	1,	2,
      "[name] -> add name for power net"				},
    { "print",		domsg,		1,	MAXARGS,
      "[text...] -> print specified text"				},
    { "printp",		printPending,	1,	2,
      "[n] -> print up to 'n' pending events (default: all)"		},
    { "printpx",	printPendingX,	1,	2,
      "[n] -> print up to 'n' pending events (default: all)"		},
    { "printx",		doprintX,	1,	1,
      " -> print all undefined (X) nodes"				},
    { "q",		quit,		1,	1,
      " -> terminate input from current stream"				},
    { "query",		doQuery,	2,	2,
      "node/vector -> query node/vector value"				},
    { "R",		runseq,		1,	2,
      "[n] -> simulate for 'n' cycles (default: longest sequence)"	},
    { "readh",		do_readh,	2,	2,
      "file -> read network state history from file"			},
    { "relax",		doXRelax,	1,	2,
      "instantaneous network relaxation (resolve unknown states)"	},
    { "report",		setreport,	1,	10,
      "[args] -> print/set trace-info or decay report (? for help)"	},
    { "s",		dostep,		1,	2,
      "[time] -> simulate for specified time (default: stepsize)"	},
#ifdef TCL_IRSIM
    { "setvector",	setvector,	3,	3,
      "vector value -> assign value to vector"				},
#else
    { "set",		setvector,	3,	3,
      "vector value -> assign value to vector"				},
#endif
    { "setlog",		setlogchanges,	1,	2,
      "[file|off] -> print/change net-changes log-filename"		},
    { "setpath",	docmdpath,	1,	MAXARGS,
      "[[+] path]... -> print/set/add-to cmd files search path"		},
    { "settle",		setsettle,	1,	2,
      "[n] -> set  settlement time (0 => no conflict settling)"		},
    { "stats",		do_stats,	1,	2,
      " -> print event statistics"					},
    { "stepsize",	setstep,	1,	2,
      "[time] -> print/set simulation step size"			},
    { "stop",		setstop,	2,	MAXARGS,
      "[-]node/vector... -> pause simulation when node/vector(s) change"},
    { "t",		settrace,	2,	MAXARGS,
      "[-]node/vector... -> start/stop tracing specified node/vector(s)"},
    { "tcap",		print_tcap,	1,	1,
      " -> print all shorted transistors"				},
    { "time",		do_time,	1,	MAXARGS,
      "[command...] -> time given command or program"			},
    { "toggle",		togglevalue,	2,	MAXARGS,
      "[node/vector]... -> toggle bit values of a node or vector"	},
    { "u",		setvalue,	2,	MAXARGS,
      "[node/vector]... -> drive a node/vector(s) to X (undefined)"	},
    { "unitdelay",	setunit,	1,	2,
      "[time] -> force transitions to specified time (0 to disable)"	},
    { "until",          doUntil,        4,      5,
      "node/vec [mask] val count -> sim until = val [& mask = 0] or count runout"	},
    { "V",		setseq,		1,	MAXARGS,
      "[node/vector [val...]] -> define input sequence for node/vector"	},
    { "vector",		dovector,	3,	MAXARGS,
      "nodeT valT proc -> schedule procedure whenever nodeT switches to valT"},
    { "wnet",		wr_net,		1,	MAXARGS,
      "[file] -> write network to file (smaller/faster than sim file)"	},
    { "x",		setvalue,	2,	MAXARGS,
      "node/vector... -> make node/vector(s) undriven (non-input)"	},
    { "Xdisplay",	xDisplay,	1,	2,
      "[Xserver] -> print/set X display (for analyzer)"			},
#ifdef FAULT_SIM
    { "faultsim",	do_fsim,	2,	3,
      "infile [outfile] -> do stuck-at fault simulation"		},
#endif
#ifdef POWER_EST
    { "powhist",	dopowhist,	2,	5,
      "[init <min> <max> <buckets>|print|capture|reset] -> power histogram data"},
    { "powlogfile",	setcaplog,	1,	2,
      "[[+]file] -> start/stop power logfile (+file appends to file)"	},
    { "powquery", 	getpow, 	1, 	2,
      "node/vector -> query node/vector power value"			},
    { "powtrace",	setpowtrace,	2,	MAXARGS,
      "[-]node/vector... -> start/stop power tracing specified node/vector(s)"},
    { "sumcap",		sumcap,		1,	2,
      " -> print out sum of capacitances of all nodes"			},
    { "vsupply",	setvsupply,	1,	3,
      "[v] Set supply voltage = v Volts (no arguments displays value)"	},
    { "powstep",	togglepstep,	1,	1,			
       " -> Toggle display of power estimate for each step"		},
#endif /* POWER_EST */
    { NULL,		NULL,		0,	0,	NULL		}
  };

#ifndef TCL_IRSIM
private int cmd_cmp( a, b )
  Command *a, *b;
  {
    return( str_eql( a->name, b->name ) );
  }

public void init_commands()
  {
    register int      n;
    register Command  *c;

    n = sizeof( cmds ) / sizeof( Command ) - 1;
    qsort( cmds, n, sizeof( Command ), cmd_cmp );

    for( n = 0; n < CMDTBLSIZE; n++ )
        cmdtbl[n] = NULL;
    for( c = cmds; c->name != NULL; c += 1 )
      {
        n = HashCmd( c->name );
        c->next = cmdtbl[ n ];
        cmdtbl[ n ] = c;
      }
  }

#endif /* TCL_IRSIM */

private int do_help()
  {
    Command  *c;
    int      i, col = 0;

    if( targc == 1 )
      {
	lprintf( stdout, "available commands:\n" );
	for( c = cmds; c->name != NULL; c++ )
	  {
	    i = strlen( c->name ) + 1;
	    if( col + i >= MAXCOL )
	      {
		lprintf( stdout, "\n" );
		col = 0;
	      }
	    col += i;
	    lprintf( stdout, " %s", c->name );
	  }
	lprintf( stdout, "\n" );
      }
    else
      {
	for( i = 1; i < targc; i++ )
	  {
#ifdef TCL_IRSIM
	    for( c = cmds; c->name != NULL; c++ )
		if (!strcmp(targv[i], c->name))
		    break;
#else
	    for( c = cmdtbl[ HashCmd( targv[i] ) ]; c != NULL; c = c->next )
	      {
		if( strcmp( targv[i], c->name ) == 0 )
		    break;
	      }
#endif
	    if( c )
		lprintf( stdout, "%s %s\n", c->name, c->help );
	    else
		lprintf( stdout, "%s -> UNKNOWN\n", targv[i] );
	  }
      }
    return( 0 );
  }
