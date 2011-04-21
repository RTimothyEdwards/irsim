#ifndef _GLOBALS_H
#define _GLOBALS_H

#ifndef _NET_H
#include "net.h"
#endif

#ifdef USER_SUBCKT
#ifndef _SUBCKT_H
#include "usersubckt/subckt.h"
#endif
#endif

	/* EXPORTS FROM access.c */

typedef	struct
  {
    char  exist;
    char  read;
    char  write;
  } Fstat;
extern Fstat *FileStatus( /*  name */ );

	/* EXPORTS FROM binsim.c */

extern void wr_netfile( /*  fname */ );
extern int rd_netfile( /*  f, line */ );
extern nptr bin_connect_txtors( /* */ );

	/* EXPORTS FROM cad_dir.c */

extern char    *cad_lib;
extern char    *cad_bin;
extern void InitCAD( /* */ );

	/* EXPORTS FROM config.c */

extern double  CM2A ;
extern double  CM2P ;
extern double  CMA ;
extern double  CMP ;
extern double  CPA ;
extern double  CPP ;
extern double  CDA ;
extern double  CDP ;
extern double  CPDA ;
extern double  CPDP ;
extern double  CGA ;
extern double	CTDW;
extern double	CPTDW;
extern double	CTDE;
extern double	CPTDE;
extern double	CTGA;
extern double  LAMBDA ;
extern double  LAMBDA2 ;
extern long	LAMBDACM ;
extern double  LOWTHRESH ;
extern double  HIGHTHRESH ;
extern double  DIFFEXT ;

#define CM_M	100.0		/* centimicrons per micron */
#define CM_M2	(CM_M * CM_M)	/* centimicrons per micron ^ 2 */

extern int	config_flags ;
#define	TDIFFCAP	0x01	/* set if DIFFPERIM or DIFFEXTF are true     */
#define CONFIG_LOADED	0x20	/* set if configuration file has been loaded */

extern int config( /*  cname */ );
extern Resists *requiv( /*  type, width, length */ );

	/* EXPORTS FROM conn_list.c */

#define	MAX_PARALLEL	30	/* this is probably sufficient per stage */
extern tptr  parallel_xtors[ /* MAX_PARALLEL */ ];
#define	par_list( T )		( parallel_xtors[ (T)->n_par ] )
extern void BuildConnList( /*  n */ );
extern void WarnTooManyParallel( /*  s1, s2 */ );

#define hash_terms(T)   ((pointertype)((T)->source) ^ (pointertype)((T)->drain))

	/* EXPORTS FROM eval.c */

#define	LIN_MODEL	0
#define	SWT_MODEL	1
#define	NMODEL		2		/* number of models supported */
extern void (*model_table[ /*NMODEL*/ ])( /* */ );
extern int	model_num ;
extern void	(*model)( /* */ );
extern int	sm_stat ;
extern int	treport ;
extern void NoInit( /* */ );
extern void ReInit( /* */ );
extern int step( /*  stop_time */ );
extern char  switch_state[ /*NTTYPES*/ ][ 4 ] ;
#define	 compute_trans_state( TRANS )					\
    ( ((TRANS)->ttype & GATELIST) ?					\
	ComputeTransState( TRANS ):					\
	switch_state[ BASETYPE( (TRANS)->ttype ) ][ (TRANS)->gate->npot ] )
extern int ComputeTransState( /*  t */ );

	/* EXPORTS FROM fio.c */

extern char *fgetline( /*  bp, len, fp */ );
extern int Fread( /*  ptr, size, fp */ );
extern int Fwrite( /*  ptr, size, fp */ );

	/* EXPORTS FROM hist.c */

extern hptr   freeHist ;
extern  hptr   last_hist;
extern int    num_edges ;
extern int    num_punted ;
extern int    num_cons_punted ;
extern hptr   first_model;
extern void init_hist( /* */ );
extern void SetFirstHist( /*  node, value, inp, time */ );
extern void AddHist( /*  node, value, inp, time, delay, rtime */ );
extern void AddPunted( /*  node, ev, tim */ );
extern void FreeHistList( /*  node */ );
extern void NoMoreIncSim( /* */ );
extern void NewModel( /*  nmodel */ );
extern void NewEdge( /*  nd, ev */ );
extern void DeleteNextEdge( /*  nd */ );
extern void FlushHist( /*  ftime */ );
extern int backToTime( /*  nd */ );

	/* EXPORTS FROM hist_io.c */

extern void DumpHist( /*  fname */ );
extern void ReadHist( /*  fname */ );

	/* EXPORTS FROM intr.c */

extern int    int_received ;
extern void InitSignals( /* */ );

	/* EXPORTS FROM incsim.c */

extern long	INC_RES ;
extern nptr	inc_cause ;
extern long    nevals ;
extern long    i_nevals ;
extern long	nreval_ev ;
extern long	npunted_ev ;
extern long	nstimuli_ev ;
extern long	ncheckpt_ev ;
extern long	ndelaychk_ev ;
extern long	ndelay_ev ;
extern int	fault_mode ;
extern void incsim( /*  ch_list */ );
extern void init_faultsim( /* */ );
extern void end_faultsim( /* */ );
extern void faultsim( /*  n */ );

	/* EXPORTS FROM mem.c */

typedef union MElem
  {
    union MElem  *next;		/* points to next element in linked list */
    int          align[1];	/* dummy used to force word alignment */
  } *MList;
extern char *Falloc( /*  nbytes, no_mem_exit */ );
extern void Ffree( /*  p, nbytes */ );
extern MList MallocList( /*  nbytes, no_mem_exit */ );
extern void Vfree( /*  ptr */ );
extern char *Valloc( /*  nbytes, no_mem_exit */ );

	/* EXPORTS FROM netupdate.c */

extern iptr rd_changes( /*  fname, logname */ );

	/* EXPORTS FROM network.c */

extern iptr  hinputs ;
extern iptr  linputs ;
extern iptr  uinputs ;
extern iptr  xinputs ;
extern iptr  infree ;
extern iptr  *listTbl[ /*8*/ ];
#define	FreeInput( X )	(X)->next = infree, infree = (X)
extern void init_listTbl( /* */ );
extern void idelete( /*  n, list */ );
extern void iinsert( /*  n, list */ );
extern void iinsert_once( /*  n, list */ );
extern void ClearInputs( /* */ );
extern int setin( /*  n, which */ );
extern int wr_state( /*  fname */ );
extern char *rd_state( /*  fname, restore */ );
extern int info( /*  n, which */ );

	/* EXPORTS FROM newrstep.c */

extern int       tunitdelay ;
extern int       tdecay ;
extern int       settle ;
extern char      withdriven;
extern void linear_model( /*  n */ );
extern void QueueFVal( /*  nd, fval, tau, delay */ );
extern void InitThevs( /* */ );

	/* EXPORTS FROM nsubrs.c */

extern char   vchars[ /**/ ] ;
extern char  *ttype[ /* NTTYPES */ ] ;
extern int GetHashSize( /* */ );
extern int str_eql( /*  s1, s2 */ );
extern int str_match( /*  p, s */ );
extern nptr find( /*  name */ );
extern nptr RsimGetNode( /*  name */ );
extern nptr GetNewNode( /*  name */ );
extern void n_insert( /*  nd */ );
extern void n_delete( /*  nd */ );
extern void walk_net( /*  fun, arg */ );
extern void walk_net_index( /*  fun, arg */ );
extern nptr GetNodeList( /* */ );
extern nptr Index2node( /*  index */ );
extern pointertype Node2index( /*  nd */ );
extern int match_net( /*  pattern, fun, arg */ );
#define	pnode( NODE )	( (NODE)->nname )
extern void init_hash( /* */ );

	/* EXPORTS FROM parallel.c */

extern void make_parallel( /*  nlist */ );
extern void UnParallelTrans( /*  t */ );
extern void pParallelTxtors( /* */ );

	/* EXPORTS FROM prints.c */

#ifdef TCL_IRSIM
extern void lprintf(FILE *, const char *, ...);
extern void rsimerror(char *, ...); 
#else
extern void lprintf( /*  va_alist */ );
extern void lprintf( /* FILE *max, ... */ );
extern void rsimerror( /*  va_alist */ );
extern void rsimerror( /* int max, ...*/ );
#endif

	/* EXPORTS FROM rsim.c */

extern  int     contline ;
extern int	analyzerON ;
extern Ulong	sim_time0 ;
extern Ulong	stepsize ;
extern FILE	*logfile ;
extern FILE	*caplogfile ;
extern double	toggled_cap ;
extern  float   vsupply ;
extern  float   capstarttime ;
extern  float   capstoptime ;
extern  float	captime ;
extern  float	powermult ;
extern  int	pstep ;
extern  float   step_cap_x_trans ;
extern int doAssert( /* */ );
extern void	evalAssertWhen( /* n*/ );
extern void disp_watch_vec( /*  which */ );
extern void rm_del_from_lists( /* */ );
extern int ch2pot( /* ch */ );
#ifdef STATS
extern void IncHistEvCnt( /* tp */ );
#endif

#define	DEBUG_EV		0x01		/* event scheduling */
#define	DEBUG_DC		0x02		/* final value computation */
#define	DEBUG_TAU		0x04		/* tau/delay computation */
#define	DEBUG_TAUP		0x08		/* taup computation */
#define	DEBUG_SPK		0x10		/* spike analysis */
#define	DEBUG_TW		0x20		/* tree walk */
#define	REPORT_DECAY	0x1
#define	REPORT_DELAY	0x2
#define	REPORT_TAU	0x4
#define	REPORT_TCOORD	0x8
#define REPORT_CAP      0x10

	/* EXPORTS FROM sched.c */

extern int    debug ;
extern Ulong  cur_delta;
extern nptr   cur_node;
extern long   nevent;
extern evptr  evfree ;
extern int    npending;
extern int    spending;
extern Ulong pending_events( /*  delta, list, end_of_list */ );
extern evptr get_next_event( /*  stop_time */ );
#define free_from_node( ev, nd )					\
  {									\
    if( (nd)->events == (ev) )						\
	(nd)->events = (ev)->nlink;					\
    else								\
      {									\
	register evptr  evp;						\
	for( evp = (nd)->events; evp->nlink != (ev); evp = evp->nlink );\
	evp->nlink = (ev)->nlink;					\
      }									\
  }
#define	FreeEventList( L )	(L)->blink->flink = evfree, evfree = (L)
extern void free_event( /*  event */ );
extern void enqueue_event( /*  n, newvalue, delta, rtime */ );
extern void enqueue_input( /*  n, newvalue */ );
extern void init_event( /* */ );
extern void PuntEvent( /*  node, ev */ );
extern void requeue_events( /*  evlist, thread */ );
extern evptr back_sim_time( /*  btime, is_inc */ );
extern int EnqueueHist( /*  nd, hist, type */ );
extern void DequeueEvent( /*  nd */ );
extern void DelayEvent( /*  ev, delay */ );
extern evptr FindScheduled( /* idx */ );
extern void DequeueScheduled( /* idx */ );
extern evptr EnqueueOther( /*  type, time */ );
extern void rm_inc_events( /*  all */ );

	/* EXPORTS FROM sim.c */

extern nptr   VDD_node;
extern nptr   GND_node;
extern char  *simprefix;
extern lptr   on_trans;
extern int    nnodes;
extern int    naliases;
extern int    ntrans[ /* NTTYPES */ ];
extern lptr   freeLinks ;
extern tptr   freeTrans ;
extern nptr   freeNodes ;
extern tptr   tcap ;
#define	MIN_CAP		0.00001		/* minimum node capacitance (in pf) */
extern int rd_network( /*  simfile */ );
extern void pTotalNodes( /* */ );
extern void pTotalTxtors( /* */ );
extern void ConnectNetwork( /* */ );

	/* EXPORTS FROM sstep.c */

extern void switch_model( /*  n */ );

	/* EXPORTS FROM stack.c */

extern int     stack_txtors ;
extern void pStackedTxtors( /* */ );
extern double StackCap( /*  t */ );
extern void make_stacks( /*  nlist */ );
extern void DestroyStack( /*  stack */ );

	/* EXPORTS FROM tpos.c */

extern int	txt_coords ;
extern void EnterPos( /*  tran, is_pos */ );
extern tptr FindTxtorPos( /*  x, y */ );
extern void DeleteTxtorPos( /*  tran */ );
extern nptr FindNode_TxtorPos( /*  s */ );
extern void walk_trans( /*  func, arg */ );

	/* EXPORTS FROM usage.c */

extern void InitUsage( /* */ );
extern void set_usage( /* */ );
extern void uset_usage( /* */ );
extern void print_usage( /*  partial, dest */ );
extern void get_usage( /*  dest */ );
extern void InitUsage( /* */ );
extern void set_usage( /* */ );
extern void uset_usage( /* */ );
extern void print_usage( /*  partial, dest */ );
extern void get_usage( /*  dest */ );

	/* EXPORTS FROM subckt.c */

#ifdef USER_SUBCKT
extern int  newsubckt( /* targc, targv */ );
extern void init_subs( /* */ );
extern userSubCircuit *subckt_instantiate( /* sname, inst */ );
extern void subckt_model_C( /* t */ );
#endif

	/* EXPORTS FROM ana.c */

extern int AddNode( /*  nd, flag */ );
extern int AddVector( /*  vec, flag */ );
extern int OffsetNode( /*  nd, flag */ );
extern int OffsetVector( /*  vec, flag */ );
extern void DisplayTraces( /*  isMapped */ );
extern void StopAnalyzer( /* */ );
extern void RestartAnalyzer( /*  first_time, last_time, same_hist */ );
extern void ClearTraces( /* */ );
extern void RemoveVector( /*  b */ );
extern void RemoveNode( /*  n */ );
extern void RemoveAllDeleted( /* */ );
extern void UpdateWindow( /*  endT */ );
extern void TerminateAnalyzer( /* */ );
extern int InitDisplay( /*  fname, display_unit */ );
extern void InitTimes( /*  firstT, stepsize, lastT, reInit */ );
extern int SetFont();

	/* Other exports */

#ifdef CL_STATS
extern void RecordConnList( /* num_trans */ );
#endif /* CL_STATS */


#endif  /* _GLOBALS_H */
