#include <stdio.h>
#include "defs.h"
#include "net.h"
#include "globals.h"


	/* stuff needed by irsim modules (not used by this program) */
public	FILE	*logfile = NULL;
public	double	StackCap( t )	tptr  t;		{ return 0; }
public	void	init_listTbl()				{}


public	int	do_sim_cap = TRUE;
public	int	two_term_caps = FALSE;
public	int	do_thresholds = TRUE;
public	int	do_delays = TRUE;
public	int	use_GND = FALSE;
private	int	short_file = FALSE;


private	char	*sim_file = NULL;
private	char	*inet_file = NULL;
private	void	sort_node_names();


main( argc, argv )
  int   argc;
  char  *argv[];
  {
    FILE  *fin, *fout;
    int   i;

    for( i = 1; i < argc; i++ )
      {
	if( argv[i][0] == '-' )
	  {
	    switch( argv[i][1] )
	      {
		case 'C' :	do_sim_cap = FALSE;	break;
		case '2' :	two_term_caps = TRUE;	break;
		case 'T' :	do_thresholds = FALSE;	break;
		case 'D' :	do_delays = FALSE;	break;
		case 'G' :	use_GND = TRUE;		break;
		case 'c' :	short_file = TRUE;	break;
		case 'o' :
		    if( ++i < argc )
			sim_file = argv[i];
		    else
			Usage();
		    break;
		default :
		    (void) fprintf( stderr, "Unknown switch %s\n", argv[i] );
		    Usage();
	      }
	  }
	else
	  {
	    inet_file = argv[i];
	  }
      }

    if( inet_file == NULL )
	Usage();

    CTGA = CTDE = CTDW = 0.0;
    LAMBDACM = 0;

    init_hist();
    if( (fin = fopen( inet_file, "r" )) == NULL )
      {
	lprintf( stderr, "cannot open '%s' for inet input\n", inet_file );
	exit( 1 );
      }
    if( sim_file == NULL )
	fout = stdout;
    else if( (fout = fopen( sim_file, "w" )) == NULL )
      {
	lprintf( stderr, "cannot open '%s' for sim output\n", sim_file );
	exit( 1 );
      }

    rd_network( fin );

    (void) fprintf( fout, "| inet2sim: %s\n", inet_file );

    if( short_file ) sort_node_names( fout );

    wr_sim( fout );

    exit( 0 );
    /* NOTREACHED */
  }


private void init_tab( n, ptab )
  nptr  n;
  nptr  *ptab;
  {
    static int  i = 0;

    ptab[ i++ ] = n;
  }

private int cmp_nnames( n1, n2 )
  nptr  *n1, *n2;
  {
    return( strcmp( (*n1)->nname, (*n2)->nname ) );
  }


private void sort_node_names( fp )
  FILE  *fp;
  {
    nptr  *ntable;
    int   i;

    ntable = (nptr *) Valloc( (nnodes + naliases) * sizeof( nptr ), 1 );

    walk_net( init_tab, (char *) ntable );

    qsort( ntable, nnodes + naliases, sizeof( nptr ), cmp_nnames );

    for( i = 0; i < nnodes + naliases; i++ )
      {
	char           buff[15];
	register nptr  np = ntable[ i ];

	(void) sprintf( buff, "$%d", i + 1 );
	(void) fprintf( fp, "= %s %s\n", np->nname, buff );
	Vfree( np->nname );
	np->nname = Valloc( strlen( buff ) + 1, 1 );
	(void) strcpy( np->nname, buff );
      }
    Vfree( ntable );
  }


#define	PRINT	(void) fprintf

private Usage()
  {
    char  *prog = "inet2sim";

    PRINT( stderr, "usage: %s [-C][-2][-t][-D][-G][-c][-o file] inetfile\n",
      prog );
    PRINT( stderr, "  -C\tdo NOT generate node capacitance statements\n" );
    PRINT( stderr, "  -2\tUse 2 terminal capacitance format (ala magic)\n" );
    PRINT( stderr, "  -T\tdo NOT generate node thresholds statements\n" );
    PRINT( stderr, "  -D\tdo NOT generate node User-Delays statements\n" );
    PRINT( stderr, "  -G\tuse GND instead of Gnd\n" );
    PRINT( stderr, "  -c\tgenerate a compressed file\n" );
    exit( 1 );
  }
