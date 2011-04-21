#include <stdio.h>
#include "defs.h"
#include "net.h"
#include "globals.h"


public	int	do_delay = FALSE;
public	int	do_rtime = FALSE;
public	int	do_punts = TRUE;
public	int	do_names = FALSE;	/* node name on every line */
public	int	do_sort = FALSE;
public	char	*only_name = NULL;

	/* stuff needed by irsim modules (not used by this program) */
public	long	sim_time0 = 0;
public	iptr	o_hinputs, o_linputs, o_uinputs;
public	nptr	cur_node = NULL;
public	long	cur_delta = 0;
public	FILE	*logfile = NULL;
public	int	stack_txtors = 0;
public
#define	compute_trans_state( X )	( 0 )

public	void	NoInit()				{}
public	double	StackCap( t )	tptr  t;		{ return 0; }
public	void	free_event( e ) evptr e;		{}
public	void	ClearInputs()				{}
public	void	iinsert( n, list ) nptr n; iptr list;	{}
public	void	init_listTbl()				{}
public	void	make_parallel( n, f ) nptr n; long f;	{}
public	void	pParallelTxtors()			{}
public	void	make_stacks( n ) nptr n;		{}
public	void	pStackedTxtors()			{}


private	char   *sim_file = NULL;
private	char   *hist_file = NULL;


main( argc, argv )
  int   argc;
  char  *argv[];
  {
    int   i;
    char  *s;

    for( i = 1; i < argc; i++ )
      {
	if( argv[i][0] != '-' )
	  {
	    hist_file = argv[i];
	    if( i + 1 != argc )
		Usage( "Arguments follow hist_file: %s...\n", argv[i+1] );
	    break;
	  }
	if( argv[i][1] == '\0' )
	    Usage( "No switch specified\n", 0 );

	for( s = &(argv[i][1]); *s != '\0'; s++ )
	  {
	    switch( *s )
	      {
		case 't' :	do_sort = TRUE;				break;
		case 'n' :	do_names = TRUE;			break;
		case 'P' :	do_punts = FALSE;			break;
		case 'r' :	do_rtime = TRUE;			break;
		case 'd' :	do_delay = TRUE;			break;
		case 'a' :	do_rtime = do_delay = do_punts = TRUE;	break;
		case 's' :
		    s = "\0";
		    sim_file = argv[++i];
		    if( i >= argc )
			Usage( "no filename following '-s'\n", 0 );
		    break;
		case 'N' :
		    s = "\0";
		    only_name = argv[++i];
		    if( i >= argc )
			Usage( "no nodename following '-N'\n", 0 );
		    break;
		default:
		    s[1] = '\0';
		    Usage( "Unknown switch: '%s'\n", s );
	      }
	  }
      }

    if( hist_file == NULL )
	Usage( "missing history file\n", 0 );

    init_hist();

    if( sim_file )
	rd_network( sim_file );

    printf( "      time  val" );
    if( do_delay )
	printf( "  delay" );
    if( do_rtime )
	printf( "    r/f" );
    printf( "\n" );

    ReadHist( hist_file );

    if( do_sort )
	sortAndPrint();

    exit( 0 );
  }


#define	PRINT	(void) fprintf

private Usage( s1, s2 )
  char  *s1;
  char  *s2;
  {
    PRINT( stderr, s1, s2 );
    PRINT( stderr, "h2a [-ntPrda] [-s sim_file] [-N node] hist_file\n" );
    PRINT( stderr, "\t-n\t\tprint node name on every line\n" );
    PRINT( stderr, "\t-t\t\tsort entire history by time\n" );
    PRINT( stderr, "\t-P\t\tdo NOT print punted events\n" );
    PRINT( stderr, "\t-r\t\tprint rise/fall times\n" );
    PRINT( stderr, "\t-d\t\tprint delay times\n" );
    PRINT( stderr, "\t-a\t\tprint everything (-r -d and NOT -P)\n" );
    PRINT( stderr, "\t-N node\t\tprint only history for node\n" );
    PRINT( stderr, "\t-s net\tread symbols from net (sim or inet) file\n" );
    exit( 1 );
  }
