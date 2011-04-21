/* 
 *-----------------------------------------------------------------------
 * tkAnalyzer.c --
 *
 *   Tk implementation of the logic analyzer widget for IRSIM
 *
 *-----------------------------------------------------------------------
 */

#ifdef TCL_IRSIM

#include <stdio.h>
#include <stdlib.h>

#ifdef LINUX
#include <string.h>  /* for strncmp() */
#endif

#include <tk.h>

#include "ana.h"
#include "ana_glob.h"
#include "rsim.h"

/* Backwards compatibility to tk8.3 and earlier */
#if TK_MAJOR_VERSION == 8
  #if TK_MINOR_VERSION <= 3
    #define Tk_SetClassProcs(a,b,c) TkSetClassProcs(a,b,c)
  #endif
#endif

#ifndef CONST84
#define CONST84
#endif

/* Internal routine used---need to find an alternative! */
extern int TkpUseWindow();

/*
 * A data structure of the following type is kept for each
 * analyzer window that currently exists for this process:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the analyzer.  NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up. */
    Display *display;		/* Display containing widget.  Used, among
				 * other things, so that resources can be
				 * freed even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with widget.  Used
				 * to delete widget command. */
    Tcl_Command widgetCmd;	/* Token for analyzer's widget command. */
    char *className;		/* Class name for widget (from configuration
				 * option).  Malloc-ed. */
    int width;			/* Width to request for window.  <= 0 means
				 * don't request any size. */
    int height;			/* Height to request for window.  <= 0 means
				 * don't request any size. */
    XColor *background;		/* background pixel used by XClearArea */
    char *useThis;		/* If the window is embedded, this points to
				 * the name of the window in which it is
				 * embedded (malloc'ed).  For non-embedded
				 * windows this is NULL. */
    char *exitProc;		/* Callback procedure upon window deletion. */
    char *mydata;		/* This space for hire. */
    int flags;			/* Various flags;  see below for
				 * definitions. */
} TkAnalyzer;

/*
 * Flag bits for analyzers:
 *
 * GOT_FOCUS:	non-zero means this widget currently has the input focus.
 */

#define GOT_FOCUS		1

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_COLOR, "-background", "background", "Background",
	"Black", Tk_Offset(TkAnalyzer, background), 0},
    {TK_CONFIG_SYNONYM, "-bg", "background", (char *)NULL,
	(char *)NULL, 0, 0},
    {TK_CONFIG_PIXELS, "-height", "height", "Height",
	"0", Tk_Offset(TkAnalyzer, height), 0},
    {TK_CONFIG_PIXELS, "-width", "width", "Width",
	"0", Tk_Offset(TkAnalyzer, width), 0},
    {TK_CONFIG_STRING, "-use", "use", "Use",
	"", Tk_Offset(TkAnalyzer, useThis), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-exitproc", "exitproc", "ExitProc",
	"", Tk_Offset(TkAnalyzer, exitProc), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-data", "data", "Data",
	"", Tk_Offset(TkAnalyzer, mydata), TK_CONFIG_NULL_OK},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		ConfigureTkAnalyzer _ANSI_ARGS_((Tcl_Interp *interp,
			    TkAnalyzer *analyzerPtr, int objc, Tcl_Obj *CONST objv[],
			    int flags));
static void		DestroyTkAnalyzer _ANSI_ARGS_((char *memPtr));
static void		TkAnalyzerCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		TkAnalyzerEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		AnalyzerWidgetObjCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));


/*
 *--------------------------------------------------------------
 *
 * TkAnalyzerObjCmd --
 *
 *	This procedure is invoked to process the "tkanalyzer"
 *	Tcl command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.  These procedures are just wrappers;
 *	they call ButtonCreate to do all of the real work.
 *
 *--------------------------------------------------------------
 */

int
TkAnalyzerObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkAnalyzer *analyzerPtr;
    Tk_Window new;
    char *arg, *useOption;
    int i, c;
    size_t length;
    unsigned int mask;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?options?");
	return TCL_ERROR;
    }

    /*
     * Pre-process the argument list.  Scan through it to find any
     * "-use" option, or the "-main" option.  If the "-main" option
     * is selected, then the application will exit if this window
     * is deleted.
     */

    useOption = NULL;
    for (i = 2; i < objc; i += 2) {
	arg = Tcl_GetStringFromObj(objv[i], (int *) &length);
	if (length < 2) {
	    continue;
	}
	c = arg[1];
	if ((c == 'u') && (strncmp(arg, "-use", length) == 0)) {
	    useOption = Tcl_GetString(objv[i+1]);
	}
    }

    /*
     * Create the window, and deal with the special option -use.
     */

    if (tkwin != NULL) {
	new = Tk_CreateWindowFromPath(interp, tkwin, Tcl_GetString(objv[1]),
		NULL);
    }
    if (new == NULL) {
	goto error;
    }
    Tk_SetClass(new, "TkAnalyzer");
    if (useOption == NULL) {
	useOption = (char *)Tk_GetOption(new, "use", "Use");
    }
    if (useOption != NULL) {
	if (TkpUseWindow(interp, new, useOption) != TCL_OK) {
	    goto error;
	}
    }

    /*
     * Create the widget record, process configuration options, and
     * create event handlers.  Then fill in a few additional fields
     * in the widget record from the special options.
     */

    analyzerPtr = (TkAnalyzer *) ckalloc(sizeof(TkAnalyzer));
    analyzerPtr->tkwin = new;
    analyzerPtr->display = Tk_Display(new);
    analyzerPtr->interp = interp;
    analyzerPtr->widgetCmd = Tcl_CreateObjCommand(interp,
	    Tk_PathName(new), AnalyzerWidgetObjCmd,
	    (ClientData) analyzerPtr, TkAnalyzerCmdDeletedProc);
    analyzerPtr->className = NULL;
    analyzerPtr->width = 0;
    analyzerPtr->height = 0;
    analyzerPtr->background = NULL;
    analyzerPtr->useThis = NULL;
    analyzerPtr->exitProc = NULL;
    analyzerPtr->flags = 0;
    analyzerPtr->mydata = NULL;

    /*
     * Store backreference to analyzer widget in window structure.
     */
    Tk_SetClassProcs(new, NULL, (ClientData) analyzerPtr);

    /* We only handle focus and structure events, and even that might change. */
    mask = StructureNotifyMask|FocusChangeMask|NoEventMask|ExposureMask;
    Tk_CreateEventHandler(new, mask, TkAnalyzerEventProc, (ClientData) analyzerPtr);

    if (ConfigureTkAnalyzer(interp, analyzerPtr, objc-2, objv+2, 0) != TCL_OK) {
	goto error;
    }
    Tcl_SetResult(interp, Tk_PathName(new), TCL_STATIC);
    return TCL_OK;

    error:
    if (new != NULL) {
	Tk_DestroyWindow(new);
    }
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * AnalyzerWidgetObjCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a analyzer widget path name.  See
 *	the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
AnalyzerWidgetObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Information about analyzer widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    static char *tkanalyzerOptions[] = {
	"cget", "configure", "height", "width", "init", "help", (char *) NULL
    };
    enum options {
	ANALYZER_CGET, ANALYZER_CONFIGURE, ANALYZER_HEIGHT,
	ANALYZER_WIDTH, ANALYZER_INIT, ANALYZER_HELP
    };
    register TkAnalyzer *analyzerPtr = (TkAnalyzer *) clientData;
    int result = TCL_OK, idx;
    int c, i;
    size_t length;
    Tcl_Obj *robj;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1],
		(CONST84 char **)tkanalyzerOptions, "option", 0,
		&idx) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_Preserve((ClientData) analyzerPtr);

    switch ((enum options)idx) {
      case ANALYZER_HELP:
	Tcl_SetResult(interp, "Options are \"configure\", \"cget\", "
		"\"height\", \"width\", \"init\", or \"help\".\n", NULL);
	break;

      case ANALYZER_CGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    result = TCL_ERROR;
	    goto done;
	}
	result = Tk_ConfigureValue(interp, analyzerPtr->tkwin, configSpecs,
		(char *) analyzerPtr, Tcl_GetString(objv[2]), 0);
	break;

      case ANALYZER_CONFIGURE:
	if (objc == 2) {
	    result = Tk_ConfigureInfo(interp, analyzerPtr->tkwin, configSpecs,
		    (char *) analyzerPtr, (char *) NULL, 0);
	} else if (objc == 3) {
	    result = Tk_ConfigureInfo(interp, analyzerPtr->tkwin, configSpecs,
		    (char *) analyzerPtr, Tcl_GetString(objv[2]), 0);
	} else {
	    for (i = 2; i < objc; i++) {
		char *arg = Tcl_GetStringFromObj(objv[i], (int *) &length);
		if (length < 2) {
		    continue;
		}
		c = arg[1];
		if ((c == 'u') && (strncmp(arg, "-use", length) == 0)) {
		    Tcl_AppendResult(interp, "can't modify ", arg,
			    " option after widget is created", (char *) NULL);
		    result = TCL_ERROR;
		    goto done;
		}
	    }
	    result = ConfigureTkAnalyzer(interp, analyzerPtr, objc-2, objv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
	break;

      /* Report actual window width and height, for manipulating */
      /* the slider, and converting pixels to simulation time.	 */

      case ANALYZER_WIDTH:
	robj = Tcl_NewIntObj(Tk_Width(analyzerPtr->tkwin));
	Tcl_SetObjResult(interp, robj);
	break;

      case ANALYZER_HEIGHT:
	robj = Tcl_NewIntObj(Tk_Height(analyzerPtr->tkwin));
	Tcl_SetObjResult(interp, robj);
	break;

      case ANALYZER_INIT:
	/* Force mapping of the window */
	Tk_MakeWindowExist(analyzerPtr->tkwin);
        start_analyzer(analyzerPtr->tkwin);
	result = TCL_OK;
	break;
    }

    done:
    Tcl_Release((ClientData) analyzerPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyTkAnalyzer --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a analyzer at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the analyzer is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyTkAnalyzer(memPtr)
    char *memPtr;		/* Info about analyzer widget. */
{
    register TkAnalyzer *analyzerPtr = (TkAnalyzer *) memPtr;

    Tk_FreeOptions(configSpecs, (char *) analyzerPtr, analyzerPtr->display,
	    TK_CONFIG_USER_BIT);
    if (analyzerPtr->exitProc != NULL) {
	/* Call the exit procedure */
	Tcl_EvalEx(analyzerPtr->interp, analyzerPtr->exitProc, -1, 0);
    }
    ckfree((char *) analyzerPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureTkAnalyzer --
 *
 *	This procedure is called to process an objv/objc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a analyzer widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for analyzerPtr;  old resources get freed, if there
 *	were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureTkAnalyzer(interp, analyzerPtr, objc, objv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register TkAnalyzer *analyzerPtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int objc;			/* Number of valid entries in objv. */
    Tcl_Obj *CONST objv[];	/* Arguments. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    if (Tk_ConfigureWidget(interp, analyzerPtr->tkwin, configSpecs,
	    objc, (CONST84 char **) objv, (char *) analyzerPtr,
	    flags | TK_CONFIG_OBJS) != TCL_OK) {
	return TCL_ERROR;
    }

    if ((analyzerPtr->width > 0) || (analyzerPtr->height > 0)) {
	Tk_GeometryRequest(analyzerPtr->tkwin, analyzerPtr->width,
		analyzerPtr->height);
    }

    if (analyzerPtr->background != NULL) {
	Tk_SetWindowBackground(analyzerPtr->tkwin, analyzerPtr->background->pixel);
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkAnalyzerEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher on
 *	structure changes to a analyzer.  For analyzers with 3D
 *	borders, this procedure is also invoked for exposures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
TkAnalyzerEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    register XEvent *eventPtr;	/* Information about event. */
{
    register TkAnalyzer *analyzerPtr = (TkAnalyzer *) clientData;

    switch (eventPtr->type) {
    
        case DestroyNotify:
	    if (analyzerPtr->tkwin != NULL) {

		/*
		 * If this window is a container, then this event could be
		 * coming from the embedded application, in which case
		 * Tk_DestroyWindow hasn't been called yet.  When Tk_DestroyWindow
		 * is called later, then another destroy event will be generated.
		 * We need to be sure we ignore the second event, since the analyzer
		 * could be gone by then.  To do so, delete the event handler
		 * explicitly (normally it's done implicitly by Tk_DestroyWindow).
		 */
    
		Tk_DeleteEventHandler(analyzerPtr->tkwin,
			StructureNotifyMask | FocusChangeMask,
			TkAnalyzerEventProc, (ClientData) analyzerPtr);
		analyzerPtr->tkwin = NULL;
        	Tcl_DeleteCommandFromToken(analyzerPtr->interp, analyzerPtr->widgetCmd);
	    }
	    Tcl_EventuallyFree((ClientData) analyzerPtr, DestroyTkAnalyzer);
	    analyzerON = FALSE;
	    break;

	case FocusIn:
	    if (eventPtr->xfocus.detail != NotifyInferior) {
		analyzerPtr->flags |= GOT_FOCUS;
	    }
	    break;

	case FocusOut:
	    if (eventPtr->xfocus.detail != NotifyInferior) {
		analyzerPtr->flags &= ~GOT_FOCUS;
	    }
	    break;

	case Expose:
	    {
		XExposeEvent *exposeEvent = (XExposeEvent *)eventPtr;
		BBox box;

		box.left = exposeEvent->x;
		box.right = exposeEvent->x + exposeEvent->width - 1;
		box.bot = exposeEvent->y + exposeEvent->height - 1;
		box.top = exposeEvent->y;
		/* Note that we call RedrawTraces(), not RedrawWindow(),
		 * because RedrawTraces() is the only thing handled by
		 * the TkAnalyzer window, and it takes up the whole
		 * window, so it always must intersect traceBox.
		 */
		RedrawTraces(&box);
	    }
	    break;

	case ConfigureNotify:
	    {
		BBox box;

	        XWINDOWSIZE = Tk_Width(analyzerPtr->tkwin);
	        YWINDOWSIZE = Tk_Height(analyzerPtr->tkwin);
	        start_analyzer(analyzerPtr->tkwin);
	        WindowChanges();

		box.left = box.top = 0;
		box.right = XWINDOWSIZE;
		box.bot = YWINDOWSIZE;
		RedrawTraces(&box);
	    }
	    break;
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * TkAnalyzerCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
TkAnalyzerCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    TkAnalyzer *analyzerPtr = (TkAnalyzer *) clientData;
    Tk_Window tkwin = analyzerPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	analyzerPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/* Create the command callback for the Tk analyzer window */

void
InitTkAnalyzer(interp)
    Tcl_Interp *interp;
{
    Tk_Window tktop;

    tktop = Tk_MainWindow(interp);

    Tcl_CreateObjCommand(interp, "tkanalyzer",
                (Tcl_ObjCmdProc *)TkAnalyzerObjCmd,
                (ClientData)tktop, (Tcl_CmdDeleteProc *)NULL);
}

#endif /* TCL_IRSIM */
