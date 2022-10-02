#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>


typedef struct
  {
    char    *name;
    double  *vals;
    double  hl_t;
    double  lh_t;
    double  tphl;
    double  tplh;
  } Signal;


struct
  {
    int     nv;
    int     np;
    Signal  *sigs;
  } plot;

typedef struct
  {
    double  w, l;
  } TranSz;


double midpoint;

Signal *timev, *in1, *in2, *in3, *out1, *out2, *out3, *out4;


main( argc, argv )
  int   argc;
  char  *argv[];
  {
    FILE    *fi = stdin;
    TranSz  p, n;
    double  cap;
    int     i;

    cap = p.w = p.l = n.w = n.l = -1;
    for( i = 1; i < argc; i++ )
      {
	if( argv[i][0] == '-' )
	  {
	    switch( argv[i][1] )
	      {
		case 'c' :
		    i++;
		    if( i >= argc ) usage( "missing argument :" );
		    if( sscanf( argv[i], "%lf", &cap ) != 1 || cap <= 0 )
			usage( "bad argument for 'c': " );
		    cap *= 1E-15;	/* convert to ff */
		    break;
		case 'n' :
		    i++;
		    if( i >= argc ) usage( "missing argument :" );
		    if( sscanf( argv[i], "%lf,%lf", &n.w, &n.l ) != 2 ||
		      n.w <= 0 || n.l <= 0 )
			usage( "bad argument for 'n': " );
		    break;
		case 'p' :
		    i++;
		    if( i >= argc ) usage( "missing argument :" );
		    if( sscanf( argv[i], "%lf,%lf", &p.w, &p.l ) != 2 ||
		      p.w <= 0 || p.l <= 0 )
			usage( "bad argument for 'p': " );
		    break;
		default :
		    usage( "unknown switch :" );
	      }
	  }
	else
	  {
	    if( (fi = fopen( argv[i], "r" )) == NULL )
		usage( "can not open input file :" );
	  }
      }

    if( cap <= 0 || n.l <= 0 || n.w <= 0 || p.l <= 0 || p.w <= 0 )
	usage( "i am missing parameters:" );

    if( read_header( fi ) == 0 )
	exit( 1 );
    if( read_variables( fi ) == 0 )
	exit( 1 );
    if( read_values( fi ) == 0 )
	exit( 1 );
    if( find_delays() == 0 )
	exit( 1 );

    PrintResistEntries( cap, &n, &p );
    exit( 0 );
  }


usage( s )
  char *s;
  {
    fprintf( stderr, "%s\n", s );
    fprintf( stderr, "usage: findr -c <cap> -nfet <w,l> -pfet <w,l> [rawfile]\n" );
    fprintf( stderr, "\tcap     -> loading capacitance (in fF)\n" );
    fprintf( stderr, "\tw,l     -> width, length of fet (in um)\n" );
    fprintf( stderr, "\trawfile -> spice rawfile (ascii)\n" );
    exit( 1 );
  }


#define eqsubstr( VS, FS )	( strncmp( (VS), FS, sizeof( FS ) -1 ) == 0 )

int read_header( f )
  FILE  *f;
  {
    char  line[512];
    int   plotname = 0;
    char  *s;

    while( fgets( line, 512, f ) )
      {
	if( eqsubstr( line, "Title:" ) )
	    ;
	else if( eqsubstr( line, "Date:" ) )
	    ;
	else if( eqsubstr( line, "Plotname:" ) )
	    plotname = 1;
	else if( eqsubstr( line, "Flags:" ) )
	    ;
	else if( eqsubstr( line, "Command:" ) )
	    ;
	else if( eqsubstr( line, "No. Variables:" ) )
	  {
	    if( plotname == 0 )
	      {
		fprintf( stderr, "missing Plotname entry\n" );
		return( 0 );
	      }
	    for( s = line; *s != ':'; s++ );
	    s++;
	    if( sscanf( s, "%d", &plot.nv ) != 1 || plot.nv <= 0 )
	      {
		fprintf( stderr, "bad line: %s", line );
		return( 0 );
	      }
	  }
	else if( eqsubstr( line, "No. Points:" ) )
	  {
	    if( plotname == 0 )
	      {
		fprintf( stderr, "missing Plotname entry\n" );
		return( 0 );
	      }
	    for( s = line; *s != ':'; s++ );
	    s++;
	    if( sscanf( s, "%d", &plot.np ) != 1 || plot.np <= 0 )
	      {
		fprintf( stderr, "bad line: %s", line );
		return( 0 );
	      }
	  }
	else if( eqsubstr( line, "Variables:" ) )
	  {
	    if( plotname == 0 || plot.np == 0 | plot.nv == 0 )
	      {
		fprintf( stderr, "missing entries:%s%s%s\n",
		  (plotname) ? "" : " 'Plotname'",
		  (plot.nv) ? "" : " 'No. Variables'",
		  (plot.np) ? "" : " 'No. Points'" );
	      }
	    return( 1 );
	  }
	else
	  {
	    fprintf( stderr, "Unrecognized line: %s", line );
	    return( 0 );
	  }
      }
    fprintf( stderr, "premature EOF\n" );
    return( 0 );
  }



int read_variables( f )
  FILE  *f;
  {
    char    line[200], name[100], type[100];
    int     i;
    int     ix;
    double  *v;
    char    *s, *p;

    plot.sigs = (Signal *) calloc( plot.nv, sizeof( Signal ) );
    if( plot.sigs == NULL )
      {
	fprintf( stderr, "no memory\n" );
	return( 0 );
      }

    for( i = 0; i < plot.nv; i++ )
      {
	if( fgets( line, 200, f ) == NULL )
	  {
	    fprintf( stderr, "premature EOF\n" );
	    return( 0 );
	  }
	if( sscanf( line, "%d %s %s", &ix, name, type ) != 3 )
	  {
	    fprintf( stderr, "bad 'variable' line: %s", line );
	    return( 0 );
	  }
	s = (char *) malloc( strlen( name ) + 1 );
	v = (double *) calloc( plot.np, sizeof( double ) );
	if( v == NULL || s == NULL )
	  {
	    fprintf( stderr, "no memory\n" );
	    return( 0 );
	  }
	plot.sigs[i].vals = v;
	plot.sigs[i].name = s;
	for( p = name; *p != '\0'; p++, s++ )
	    *s = ( isupper( *p ) ) ? tolower( *p ) : *p;
	*s = '\0';
      }

    if( fgets( line, 100, f ) == NULL )
      {
	fprintf( stderr, "premature EOF\n" );
	return( 0 );
      }
    if( eqsubstr( line, "Values:" ) )
	return( 1 );

    fprintf( stderr, "No 'Values' line\n" );
    return( 0 );
  }


int read_values( f )
  FILE  *f;
  {
    char    line[200];
    int     n, i, n_p, z;
    double  v;

    for( n = 0; n < plot.np; n++ )
      {
	if( fgets( line, 200, f ) == NULL )
	  {
	    fprintf( stderr, "premature EOF\n" );
	    return( 0 );
	  }

	if( sscanf( line, "%d%lf", &n_p, &v ) != 2 || n_p != n )
	  {
	    fprintf( stderr, "bad 'value' line: %s", line );
	    return( 0 );
	  }

	plot.sigs[0].vals[n] = v;
	for( i = 1; i < plot.nv; i++ )
	  {
	    if( fgets( line, 200, f ) == NULL )
	      {
		fprintf( stderr, "premature EOF\n" );
		return( 0 );
	      }
	    if( sscanf( line, "%lf", &v ) != 1 )
	      {
		fprintf( stderr, "bad 'value' line: %s", v, line );
		return( 0 );
	      }
	    plot.sigs[i].vals[n] = v;
	  }
      }
    return( 1 );
  }


Signal *find( name )
  char  *name;
  {
    int     i;
    Signal  *sig;

    for( i = 0, sig = plot.sigs; i < plot.nv; i++, sig++ )
      {
	if( strcmp( name, sig->name ) == 0 )
	    return( sig );
      }
    return( NULL );
  }


double interp( y, v1, v2, t1, t2 )
  double y, v1, v2, t1, t2;
  {
    return( t2 + (y - v2) * (t2 - t1) / (v2 - v1) );
  }


#define	L_H	1
#define	H_L	2
#define BOTH	( L_H | H_L )

Signal *FindTransitions( name, which )
  char  *name;
  int   which;
  {
    Signal  *sig;
    double  *v, *t;
    int     i;

    if( (sig = find( name )) == NULL )
      {
	fprintf( stderr, "signal '%s' missing\n" );
	return( NULL );
      }
    v = sig->vals;
    t = timev->vals;

    if( which & L_H )
      {
	for( i = 1; i < plot.np; i++ )
	  {
	    if( v[i-1] < midpoint && v[i] >= midpoint )
		goto foundlh;
	  }
	fprintf( stderr, "signal '%s' has no l->h transition\n", name );
	return( NULL );

      foundlh :
	sig->lh_t = interp( midpoint, v[i-1], v[i], t[i-1], t[i] );
      }

    if( which & H_L )
      {
	for( i = 1; i < plot.np; i++ )
	  {
	    if( v[i-1] > midpoint && v[i] <= midpoint )
		goto foundhl;
	  }
	fprintf( stderr, "signal '%s' has no h->l transition\n", name );
	return( NULL );

      foundhl :
	sig->hl_t = interp( midpoint, v[i-1], v[i], t[i-1], t[i] );
      }

    return( sig );
  }


int find_delays()
  {
    Signal  *vdd;

    if( (timev = find( "time" )) == NULL )
      {
	fprintf( stderr, "variable 'time' missing\n" );
	return( 0 );
      }
    if( (vdd = find( "v(vdd)" )) == NULL )
      {
	fprintf( stderr, "variable 'vdd' missing\n" );
	return( 0 );
      }
    midpoint = vdd->vals[0] / 2.0;

    if( (in1 = FindTransitions( "v(in1)", BOTH )) == NULL ) return( 0 );
    if( (in2 = FindTransitions( "v(in2)", L_H )) == NULL ) return( 0 );
    if( (in3 = FindTransitions( "v(in3)", H_L )) == NULL ) return( 0 );
    if( (out1 = FindTransitions( "v(out1)", BOTH )) == NULL ) return( 0 );
    if( (out2 = FindTransitions( "v(out2)", BOTH )) == NULL ) return( 0 );
    if( (out3 = FindTransitions( "v(out3)", L_H )) == NULL ) return( 0 );
    if( (out4 = FindTransitions( "v(out4)", H_L )) == NULL ) return( 0 );

    out1->tphl = out1->hl_t - in1->lh_t;
    out1->tplh = out1->lh_t - in1->hl_t;

    out2->tphl = out2->hl_t - out1->lh_t;
    out2->tplh = out2->lh_t - out1->hl_t;

    out3->tplh = out3->lh_t - in2->lh_t;
    out4->tphl = out4->hl_t - in3->hl_t;

    return( 1 );
  }


#define	SQ( A )		( (A) * (A) )

PrintResistEntries( cap, ns, ps )
  double  cap;
  TranSz  *ns, *ps;
  {
    double  dyn_h, dyn_l, stat;

    dyn_h = out3->tplh / cap;
    dyn_l = out1->tphl / cap;
    stat = ( SQ( out2->tphl ) - SQ( out1->tphl ) ) / (out1->tplh * cap );
    PrintR( "n-channel", ns->w, ns->l, dyn_h, dyn_l, stat ); 

    printf( "\n" );

    dyn_h = out1->tplh / cap;
    dyn_l = out4->tphl / cap;
    stat = ( SQ( out2->tplh ) - SQ( out1->tplh ) ) / (out1->tphl * cap );
    PrintR( "p-channel", ps->w, ps->l, dyn_h, dyn_l, stat ); 
  }


#define	round( A )	(int) ( (A) + 0.5 )


PrintR( ttype, w, l, rh, rl, rs )
  char    *ttype;
  double  w, l, rh, rl, rs;
  {
    static char *fmt = "resistance %s %s\t%.2f\t%.2f\t%d.0\n";

    printf( fmt, ttype, "dynamic-high", w, l, round( rh ) );
    printf( fmt, ttype, "dynamic-low\t", w, l, round( rl ) );
    printf( fmt, ttype, "static\t", w, l, round( rs ) );
  }
