/*----------------------------------------------------------------------*/
/* tclirsim.c --- Creates the interpreter-wrapped version of irsim	*/
/*									*/
/*    Written by Tim Edwards November 2002				*/
/*									*/
/*----------------------------------------------------------------------*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>	/* Ctrl-C interrupt handling */

#include <tcl.h>
#include <tk.h>

#include "defs.h"
#include "net.h"
#include "globals.h"

#include "rsim.h"

/*
 * Handling of VA_COPY.  These variables are set by the configuration
 * script.  Some systems define va_copy, some define __va_copy, and
 * some don't define it at all.  It is assumed that systems which do
 * not define it at all allow arguments to be copied with "=".
 */

#ifndef HAVE_VA_COPY
  #ifdef HAVE___VA_COPY
    #define va_copy(a, b) __va_copy(a, b)
  #else
    #define va_copy(a, b) a = b
  #endif
#endif

Tcl_Interp *irsiminterp;
Tcl_Interp *consoleinterp;

private int UseTkConsole = TRUE;

extern char *filename;		/* current input file (see rsim.h) */
extern int  lineno;		/* current line number */
extern char *first_file;	/* basename of network file read-in */
extern int targc;
extern char **targv;
extern Command cmds[];
extern char wildCard[MAXARGS];

extern void Tcl_stdflush();
extern void InitTkAnalyzer();
extern void TagInit();
extern int IrsimTagCallback();
extern void enable_interrupt();
extern void disable_interrupt();
extern Tcl_Obj *list_all_vectors();

private int InterruptPending = FALSE;
private void (*oldinthandler)() = SIG_DFL;

int tclirsim_base();
int tclirsim_marker();
int tclirsim_print();
int tclirsim_simtime();
int tclirsim_trace();
int tclirsim_zoom();

Command anacmds[] = {
   {"base", tclirsim_base, 1, 4, "base get|set [trace] [bin|oct|hex]", 0},
   {"marker", tclirsim_marker, 1, 4, "marker [1|2] set|move|delta...", 0},
   {"print", tclirsim_print, 1, 3, "print [<file>|<option>...]", 0},
   {"simtime", tclirsim_simtime, 1, 4, "simtime <option>", 0},
   {"trace", tclirsim_trace, 1, 4, "trace <option>", 0},
   {"zoom", tclirsim_zoom, 1, 2, "zoom [in|out]", 0},
   {NULL, NULL}
};

/*-------------------------------------------------------*/
/* Procedure finput(name) --- reads commands from a file */
/*-------------------------------------------------------*/

static int finput(char *filename)
{
   char *cmdstring;
   int result;

   result = Tcl_EvalFile(irsiminterp, filename);
   return (result == TCL_OK) ? 1 : 0;
}

/*------------------------------------------------------*/
/* Procedures cmdfile() and docmdpath():  do nothing	*/
/* (maybe deal with this properly, later)		*/
/*------------------------------------------------------*/

public int cmdfile() {
   return 0;
}

public int docmdpath() {
   return 0;
}

/*-----------------------------------------------------*/
/* Dispatch an IRSIM command from the Tcl command line */
/*-----------------------------------------------------*/

static int _irsim_dispatch(Command *command,
	Tcl_Interp *interp, int argc, char *argv[])
{
    static char *conflicts[] =
    {
	"clock", "trace", NULL
    };

    static char *resolutions[] =
    {
	"orig_clock", "orig_trace", NULL
    };

    typedef enum
    {
	IDX_CLOCK
    } conflictCommand;

    Tcl_Obj *objv0;
    char *argv0;
    int result, idx, i;
    int (*handler)();

    /* Check command (argv[0]) against known conflicting Tcl/Tk command */
    /* names.  If the command is potentially a Tcl/Tk call, then try it	*/
    /* as such, first.  If Tcl returns an error, try it again as an 	*/
    /* IRSIM command.							*/

    /* Note: of the command conflicts, 1) alias is a tkcon command and	*/
    /* is just renamed by the startup script 2) "clear" has the same	*/
    /* 1-argument syntax for both Tcl and IRSIM. 3) "time" has the same	*/
    /* 2-argument syntax for both Tcl and IRSIM.  So only "clock" can	*/
    /* be handled appropriately.					*/

    argv0 = argv[0];
    if (!strncmp(argv0, "::", 2)) argv0 += 2;
    objv0 = Tcl_NewStringObj(argv0, strlen(argv0));
    if (Tcl_GetIndexFromObj(interp, objv0, (CONST char **)conflicts,
		"overloaded command", 0, &idx) == TCL_OK)
    {
	Tcl_Obj **objv = (Tcl_Obj **)Tcl_Alloc(argc * sizeof(Tcl_Obj *));

	objv[0] = Tcl_NewStringObj(resolutions[idx], strlen(resolutions[idx]));
	Tcl_IncrRefCount(objv[0]);
	
	for (i = 1; i < argc; i++)
	{
	    objv[i] = Tcl_NewStringObj(argv[i], strlen(argv[i]));
	    Tcl_IncrRefCount(objv[i]);
	}
	result = Tcl_EvalObjv(interp, argc, objv, 0);

	for (i = 0; i < argc; i++)
	    Tcl_DecrRefCount(objv[i]);
	Tcl_Free((char *)objv);

	if (result == TCL_OK)
	    return result;
    }
    Tcl_ResetResult(interp);

    if ((argc < command->nmin) || (argc > command->nmax))
    {
        lprintf(stderr, "Usage: %s %s\n", command->name, command->help);
	return TCL_ERROR;
    }
    else
    {
	handler = command->handler;
	targc = argc;
	targv = argv;

	/* Check for wildcard character '*' */
	for (i = 1; i < argc; i++)
	    wildCard[i] = (strchr(argv[i], '*') != NULL) ? TRUE : FALSE;

	enable_interrupt();
	result = (*handler)();
	disable_interrupt();

	/* There should be a consensus on the return value. . . */
	if (result == -1)
	   return TCL_ERROR;
	else
	   return IrsimTagCallback(interp, argc, argv);
    }
}

/*----------------------------------------*/
/* Redefine fprintf for stdout and stderr */
/* For use with the TkCon console.	  */
/*----------------------------------------*/

void vlprintf(FILE *f, const char *fmt, va_list args_in)
{
    va_list args;
    static char outstr[128] = "puts -nonewline std";
    char *outptr, *bigstr = NULL, *finalstr = NULL;
    int i, nchars, result, escapes = 0;
    Tcl_Interp *printinterp = (UseTkConsole) ? consoleinterp : irsiminterp;

    strcpy (outstr + 19, (f == stderr) ? "err \"" : "out \"");
    outptr = outstr;

    va_copy(args, args_in);
    nchars = vsnprintf(outptr + 24, 102, fmt, args);
    va_end(args);

    if (nchars >= 102)
    {
	va_copy(args, args_in);
	bigstr = Tcl_Alloc(nchars + 26);
	strncpy(bigstr, outptr, 24);
	outptr = bigstr;
	vsnprintf(outptr + 24, nchars + 2, fmt, args);
	va_end(args);
    }
    else if (nchars == -1) nchars = 126;

    if (logfile != NULL) logprint(outptr + 24);

    for (i = 24; *(outptr + i) != '\0'; i++)
	if (*(outptr + i) == '\"' || *(outptr + i) == '[' ||
	    	*(outptr + i) == ']' || *(outptr + i) == '\\')
	    escapes++;

    if (escapes > 0)
    {
	finalstr = Tcl_Alloc(nchars + escapes + 26);
	strncpy(finalstr, outptr, 24);
	escapes = 0;
	for (i = 24; *(outptr + i) != '\0'; i++)
	{
	    if (*(outptr + i) == '\"' || *(outptr + i) == '[' ||
	    		*(outptr + i) == ']' || *(outptr + i) == '\\')
	    {
	        *(finalstr + i + escapes) = '\\';
		escapes++;
	    }
	    *(finalstr + i + escapes) = *(outptr + i);
	}
        outptr = finalstr;
    }

    *(outptr + 24 + nchars + escapes) = '\"';
    *(outptr + 25 + nchars + escapes) = '\0';

    result = Tcl_EvalEx(printinterp, outptr, -1, 0);

    if (bigstr != NULL) Tcl_Free(bigstr);
    if (finalstr != NULL) Tcl_Free(finalstr);

    /* return result; */ /* set result if necessary; don't return it, */
    /* or else redefine lprintf() in the header file. . . */
}
    
/*------------------------------------------------------*/
/* Standard multiple-argument version of the va_list	*/
/* defined routine vlprintf() above.			*/
/*------------------------------------------------------*/

void lprintf(FILE *f, const char *fmt, ...) 
{
    va_list args;

    va_start(args, fmt);
    vlprintf(f, fmt, args);
    va_end(args);
}

/*------------------------------------------------------*/
/* To go along with the redirected fprintf() routine,	*/
/* we have corresponding flush commands for stdout	*/
/* and stderr.						*/
/*------------------------------------------------------*/

void Tcl_stdflush(f)
    FILE *f;
{
    Tcl_SavedResult state;
    static char stdstr[] = "::tcl_flush stdxxx";
    char *stdptr = stdstr + 15;
    
    Tcl_SaveResult(irsiminterp, &state);
    strcpy(stdptr, (f == stderr) ? "err" : "out");
    Tcl_EvalEx(irsiminterp, stdstr, -1, 0);
    Tcl_RestoreResult(irsiminterp, &state);
}   

/*------------------------------------------------------*/
/* Interrupt handling routines				*/
/*------------------------------------------------------*/

/*------------------------------------------------------*/
/* Handler is only used when netgen is run from a       */
/* terminal, not the Tk console.                        */
/*------------------------------------------------------*/

void sighandler(int sig) 
{
   /* Don't do anything else here! */
   InterruptPending = 1;
}

/*------------------------------------------------------*/
/* Set up the interrupt flag (both methods) and signal  */
/* handler (terminal-based method only).                */
/*------------------------------------------------------*/

void enable_interrupt()
{
   InterruptPending = 0;
   oldinthandler = signal(SIGINT, sighandler);
}

void disable_interrupt()
{
   if (InterruptPending)
      InterruptPending = 0; 
   signal(SIGINT, oldinthandler);
}

/*------------------------------------------------------*/
/* Generate an interrupt condition       		*/
/* from a Control-C in the console window.              */
/* The console script binds this procedure to Ctrl-C.   */
/*------------------------------------------------------*/

int _tkcon_interrupt(ClientData clientData,
    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{  
   InterruptPending = 1;
   return TCL_OK;
}

/*--------------------------------------------------------------*/
/* Allow Tcl to periodically do (Tk) window events.  This       */ 
/* will not cause problems because netgen is not inherently     */
/* window based and only the console defines window commands.   */
/* This also works with the terminal-based method although      */
/* in that case, Tcl_DoOneEvent() should always return 0.       */
/*--------------------------------------------------------------*/

int check_interrupt() {
   Tcl_DoOneEvent(TCL_WINDOW_EVENTS | TCL_DONT_WAIT);
   if (InterruptPending) {
      lprintf(stderr, "Interrupt!\n");
      return TRUE;
   }
   return FALSE;
}

/*--------------------------------------*/
/* Redefine the "error" function	*/
/*--------------------------------------*/

void rsimerror(char *filename, ... )
{
    va_list args;
    int lineno;
    char *fmt;

    va_start(args, filename);
    lineno = va_arg(args, int);
    fmt = va_arg(args, char *);
  
    if (filename != NULL)
	lprintf(stderr, "(%s,%d): ", filename, lineno);

    vlprintf(stderr, fmt, args);
    va_end(args);
}

/*------------------------------------------------------*/
/* Procedure Usage() --- usage of irsim in Tcl		*/
/*------------------------------------------------------*/

void Usage(char *fmt, ... )
{
    va_list args;

    va_start(args, fmt);

    vlprintf(stderr, fmt, args);
    va_end(args);
    lprintf(stderr, "usage:\n irsim ");
    lprintf(stderr, "[-s] prm_file [sim_file ..] "
		"[-tcl_file ..]|[-c tcl_file]|[-@ cmd_file]\n");
    lprintf(stderr, "\t-s\t\tstack series transistors\n");
    lprintf(stderr, "\tprm_file\telectrical parameters file\n");
    lprintf(stderr, "\tsim_file\tsim (network) file[s]\n");
    lprintf(stderr, "\ttcl_file\tTcl script command file[s]\n");
    lprintf(stderr, "\tcmd_file\tOriginal syntax IRSIM command file[s]\n");
}

/*------------------------------------------------------*/
/* Procedure to read in a .sim file from the command	*/
/* line.						*/
/*------------------------------------------------------*/

static int _irsim_readsim(ClientData clientData,
	Tcl_Interp *interp, int argc, char *argv[])
{
    char *filename, *pdptr, *prefix;
    int result = TCL_OK;

    if (argc != 2 && argc != 3)
    {
	lprintf(stderr, "Usage: readsim [<prefix>] <sim_filename>\n");
	return TCL_ERROR;
    } 
    if (argc == 3)
	prefix = argv[1];
    else
	prefix = NULL;

    filename = argv[argc - 1];
    if ((pdptr = strrchr(filename, '.')) == NULL) {
        filename = (char *)malloc(strlen(argv[argc - 1]) + 5);
	sprintf(filename, "%s.sim", argv[argc - 1]);
    } 
    
    if (rd_network(filename, prefix, (config_flags & CONFIG_LOADED) ? 0 : -1))
	result = TCL_ERROR;
    else
	ConnectNetwork();

    if (filename != argv[argc - 1]) free(filename);
    return result;
}

/*------------------------------------------------------*/
/* Procedure to add a "fake" node to the database.	*/
/* This is useful for generating reference signals or	*/
/* vectors in the analyzer window, or to use IRSIM as	*/
/* a backend to a program like "cver", replacing	*/
/* (for example) dinotrace.  Fake nodes don't connect	*/
/* to the database network, but they can be forced to	*/
/* specific values.					*/
/*------------------------------------------------------*/

static int _irsim_addnode(ClientData clientData,
	Tcl_Interp *interp, int argc, char *argv[])
{
    nptr  n;

    if (argc < 2)
    {
	lprintf(stderr, "Usage: addnode <nodename> [<capval>]\n");
	return TCL_ERROR;
    } 
    n = RsimGetNode(argv[1]);
    if (argc == 3)
	n->ncap += atof(argv[2]);

    return TCL_OK;
}

/*------------------------------------------------------*/
/* Procedure to list all of the known nodes, and	*/
/* return them as a Tcl list.				*/
/*------------------------------------------------------*/

static int _irsim_listnodes(ClientData clientData,
	Tcl_Interp *interp, int argc, char *argv[])
{
    nptr  n;

    for (n = GetNodeList(); n != NULL; n = n->n.next)
    {
        if (n->nflags & ALIAS)
            continue;
	Tcl_AppendElement(interp, n->nname);
    }
    return TCL_OK;
}

/*------------------------------------------------------*/
/* Procedure to list all of the known vectors, and	*/
/* return them as a Tcl list.				*/
/*------------------------------------------------------*/

static int _irsim_listvectors(ClientData clientData,
	Tcl_Interp *interp, int argc, char *argv[])
{
    Tcl_SetObjResult(interp, list_all_vectors());
    return TCL_OK;
}

/*------------------------------------------------------*/
/* Main startup procedure				*/
/* This function replaces function main() in rsim.c	*/
/*------------------------------------------------------*/

static int _irsim_start(ClientData clientData,
	Tcl_Interp *interp, int argc, char *argv[])
{
    int i, arg1, has_prm_file = -1;
    int result = TCL_OK;
    char versionstr[128];

    /* Did we start in the same interpreter as we initialized? */
    if (irsiminterp != interp)
    {
        lprintf(stderr, "Warning:  Switching interpreters.  "
		"Tcl-irsim is not set up to handle this.\n");
        irsiminterp = interp;
    }
    lprintf(stdout, "Starting irsim under Tcl interpreter\n");

    /* Initialization stuff from original main() function */

    InitSignals();
    InitUsage();
    InitThevs();
    InitCAD();
    init_hist();
#ifdef USER_SUBCKT
#ifdef TCLSUBCKT
    init_subs(subs);
#endif
#endif
    InitTimes(sim_time0, stepsize, cur_delta, 0);
    sprintf(versionstr, "IRSIM %s.%s compiled on %s\n", IRSIM_VERSION,
		IRSIM_REVISION, IRSIM_DATE);
    lprintf(stdout, versionstr);
    Tcl_stdflush(stdout);

    filename = "*initialization*";

    for (arg1 = 1; arg1 < argc; arg1++)
    {
	if (argv[arg1][0] == '-')
	{
	    switch(argv[arg1][1])
	    {
		case 's' :			/* stack series transistors */
		    stack_txtors = TRUE;
		    break;
		default :
		    Usage("Unknown switch: %s\n", argv[arg1]);
		    return TCL_ERROR;
	    }
	}
	else
	  break;
    }

    /* Read in the electrical configuration file, if specified */

    if (arg1 < argc)
    {
	if (strstr(argv[arg1], ".sim") == NULL) {
	   has_prm_file = config(argv[arg1]);
	   if (has_prm_file == 0) arg1++;
	}
    }

    /* Read network files (sim files) */

    for (i = arg1; i < argc; i++)
    {
	if (argv[i][0] != '-' and argv[i][0] != '+')
	{
	    if (rd_network(argv[i], NULL, has_prm_file))
		return TCL_ERROR;

	    if (first_file == NULL)
		first_file = BaseName(argv[i]);
	}
	else if ((!strcmp(argv[i], "-c") || !strcmp(argv[i], "-@")) && (i < argc - 1))
	    i++;
    }

    init_event();

    if (first_file == NULL)
    {
	Usage("No sim file specified.\n");
	return TCL_OK;
    }

    ConnectNetwork();	/* connect all txtors to corresponding nodes */

    /* Search for -filename for command files to process.		*/
    /* This may also be specified as "-c filename" to facilitate	*/
    /* the use of filename completion on the command line, which can't	*/
    /* be used when the "-" is attached to the filename.		*/
    /* The alternative form "-@ filename" forces backwards		*/
    /* compatibility for the original IRSIM syntax (e.g., "set" instead	*/
    /* of "setvector").							*/

    filename = "command line";
    lineno = 1;
    result = TCL_OK;
    for (i = arg1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    char *farg;

	    if (!strcmp(&argv[i][1], "c") && (i < (argc - 1)))
	    {
		farg = argv[++i];
		if (!finput(farg)) {
		    rsimerror(filename, lineno, "error reading script %s\n", farg);
		    result = TCL_ERROR;
		}
	    }
	    else if (!strcmp(&argv[i][1], "@") && (i < (argc - 1)))
	    {
		farg = argv[++i];
		Tcl_VarEval(irsiminterp, "@ ", farg, (char *)NULL);
	    }
	    else
	    {
		farg = &argv[i][1];
		if (!finput(farg)) {
		    rsimerror(filename, lineno, "error reading script  %s\n", farg);
		    result = TCL_ERROR;
		}
	    }
	}
    }
    return result;
}

/*--------------------------------------*/
/* Tcl Package Initialization procedure	*/
/*--------------------------------------*/

int Tclirsim_Init(interp)
    Tcl_Interp *interp;
{
    int n;
    char keyword[100];
    char *cadroot;

    /* Sanity check! */
    if (interp == NULL) return TCL_ERROR;

    /* Remember the interpreter */
    irsiminterp = interp;

    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) return TCL_ERROR;

    /* Use namespace to avoid conflicts with existing commands */
    for (n = 0; cmds[n].name != NULL; n++)
    {
	sprintf(keyword, "irsim::%s", cmds[n].name);
	Tcl_CreateCommand(interp, keyword, (Tcl_CmdProc *)_irsim_dispatch,
			(ClientData)(&cmds[n]), (Tcl_CmdDeleteProc *) NULL);
    }

    /* Start command */
    Tcl_CreateCommand(interp, "irsim::start", (Tcl_CmdProc *)_irsim_start,
			(ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);

    /* Commands unique to the Tcl version */
    Tcl_CreateCommand(interp, "irsim::listnodes", (Tcl_CmdProc *)_irsim_listnodes,
			(ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "irsim::listvectors", (Tcl_CmdProc *)_irsim_listvectors,
			(ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "irsim::addnode", (Tcl_CmdProc *)_irsim_addnode,
			(ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "irsim::readsim", (Tcl_CmdProc *)_irsim_readsim,
			(ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);

    for (n = 0; anacmds[n].name != NULL; n++)
    {
	sprintf(keyword, "irsim::%s", anacmds[n].name);
	Tcl_CreateCommand(interp, keyword, (Tcl_CmdProc *)_irsim_dispatch,
			(ClientData)(&anacmds[n]), (Tcl_CmdDeleteProc *) NULL);
    }

    /* Set up tag callbacks */
    TagInit(interp);

    /* Set up the command callback for the Tk analyzer window */
    InitTkAnalyzer(interp);

    /* Export the namespace commands */

    Tcl_Eval(interp, "namespace eval irsim namespace export *");

    /* Set $CAD_ROOT as a Tcl variable */

    cadroot = getenv("CAD_ROOT");
    if (cadroot == NULL) cadroot = CAD_DIR;
    Tcl_SetVar(interp, "CAD_ROOT", cadroot, TCL_GLOBAL_ONLY);

    Tcl_PkgProvide(interp, "Tclirsim", IRSIM_VERSION);

   if ((consoleinterp = Tcl_GetMaster(interp)) == NULL)
      consoleinterp = interp;

   Tcl_CreateObjCommand(consoleinterp, "irsim::interrupt", _tkcon_interrupt,
                (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}

