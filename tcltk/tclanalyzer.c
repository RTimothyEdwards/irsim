/*----------------------------------------------------------------------*/
/* tclanalyzer.c --- The command-line commands to control the analyzer	*/
/*		window.							*/
/*									*/
/*    Written by Tim Edwards January 2005				*/
/*									*/
/*----------------------------------------------------------------------*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tcl.h>
#include <tk.h>

#include "globals.h"
#include "ana.h"
#include "ana_glob.h"
#include "graphics.h"
#include "units.h"

extern Tcl_Interp *irsiminterp;
extern char **targv;
extern int targc;

extern int lookup();
extern Trptr get_trace();
extern void WritePSfile();
extern void SetTimeLeft(), SetTimeRight();
extern void TraceValue(), TraceInput(), TraceBits(), RemoveTrace();
extern void UpdateWinRemove(), GetNameLen();
extern void MoveToTimeValue(), ChangeTraceBase(), SetCursor();
extern void MoveCursorToTime(), MoveDeltaToTime(), MoveToT();
extern void MoveTraces(), SelectTrace();
extern int ValidTime();

extern int psBanner, psLegend, psTimes, psOutline;
extern int max_name_len, autoScroll;

/*------------------------------------------------------*/
/* The original Analyzer was driven by Xt callbacks, so	*/
/* there is no command syntax for it.  We create one	*/
/* here.						*/
/*------------------------------------------------------*/

/*------------------------------------------------------*/
/* base [trace] [value]					*/
/*------------------------------------------------------*/

int tclirsim_base()
{
    int varg = 2, idx;
    Trptr trace = selectedTrace;
    char *bptr;

    static char *baseTypes[] = {
	"none", "binary", "quartile", "octal", "hexidecimal", "decimal",
	"signed"
    };

    static char *baseOptions[] = {
        "get", "set", NULL
    };
    enum baseopt {
	BASE_GET, BASE_SET
    };

    if (targc == 1) {
	lprintf(stderr, "Usage: base get [trace]\n");
	lprintf(stderr, "Usage: base set [trace] type\n");
        return 0;
    }

    idx = lookup(targv[1], baseOptions, FALSE);
    if (idx < 0) return -1;

    /* If there are 3 arguments, the second one should	*/
    /* be a vector or node.  Otherwise, trace is set to	*/
    /* NULL, and ChangeTraceBase will use the		*/
    /* currently selected trace, if there is one.	*/

    if (((idx == BASE_GET) && (targc == 3))
		|| ((idx == BASE_SET) && (targc == 4))) {
       varg = 3;
       trace = get_trace(targv[2]);
       if (trace == NULL) {
	  lprintf(stderr, "No trace named \"%s\"!\n", targv[2]);
	  return -1;
       }
    }

    if (idx == BASE_GET) {
	Tcl_SetResult(irsiminterp, baseTypes[trace->bdigit], 0);
    }
    else if (targc <= varg) {
	lprintf(stderr, "Trace types are:  binary, decimal, octal, or hexidecimal.\n");
	lprintf(stderr, "Trace type may begin with \"u\" to make it unsigned.\n");
	return -1;
    }
    else {
	bptr = targv[varg];
	switch (*bptr) {
	    case 'b': case 'd': case 'o':
	    case 'h': case 'x': case 's':
		ChangeTraceBase(trace, bptr);
		break;
	    default:
	        lprintf(stderr, "Unknown/unhandled numeric base.\n");
	        return -1;
	}
    }
    return 0;
}

/*------------------------------------------------------*/
/* marker [set|move|delta]				*/
/*------------------------------------------------------*/

int tclirsim_marker()
{
    float rtime;
    TimeType time;	/* TimeType is "Ulong" in ana.h */
    Trptr t;
    int idx, which, argst;
    double dt;

    static char *markerOptions[] = {
        "get", "move", "set", "off", NULL
    };
    enum markeropt {
	MARKER_GET, MARKER_MOVE, MARKER_SET, MARKER_OFF
    };

    if (targc == 1) {
	lprintf(stderr, "Usage: marker [1|2] <option>...\n");
        return -1;
    }

    /* Assume primary cursor (1) unless indicated otherwise */
    if (sscanf(targv[1], "%d", &which) == 1)
	argst = 2;
    else {
	argst = 1;
	which = 1;
    }

    if (which <= 0 || which > 2) {
       lprintf(stderr, "Optional marker number must be 1 or 2\n");
       return -1;
    }

    idx = lookup(targv[argst], markerOptions, FALSE);
    if (idx < 0) return -1;

    switch (idx) {

	/* This duplicates "simtime marker" and "simtime delta" */
	case MARKER_GET:
           dt = (which == 1) ? analyzer_time_marker() : analyzer_time_delta();
           if (dt >= 0.0) {
                Tcl_SetObjResult(irsiminterp, Tcl_NewDoubleObj(dt));
	   }
	   break;

	case MARKER_SET:
           if ((targc - argst + 1) != 4) {
	      lprintf(stderr, "Usage: marker set <trace> <time>.\n");
	      return -1;
           }
	   else if (which == 2) {
	      lprintf(stderr, "Option not available for the delta marker\n");
	      return -1;
	   }
           t = get_trace(targv[argst + 1]);
           if (sscanf(targv[argst + 2], "%f", &rtime) != 1) {
	      lprintf(stderr, "Invalid time value.\n");
	      return -1;
           }
           time = (TimeType)(ns2d(rtime));
           if (t != NULL) SetCursor(t, time);
           MoveCursorToTime(time);
	   break;

 	case MARKER_MOVE:
           if ((targc - argst + 1) == 2) {
	      lprintf(stderr, "Usage: marker move <time>.\n");
	      return -1;
           }
           else if (sscanf(targv[argst + 1], "%f", &rtime) != 1) {
	      lprintf(stderr, "Invalid time value.\n");
	      return -1;
           }
           time = (TimeType)(ns2d(rtime));
	   if (which == 2)
	      MoveDeltaToTime(time);
	   else
              MoveCursorToTime(time);
	   break;

	case MARKER_OFF:
	   if (which == 2)
	      MoveDeltaToTime((TimeType) -1);
	   else
	      MoveCursorToTime((TimeType) -1);
	   break;
    }
    return 0;
}

/*------------------------------------------------------*/
/* print <name>						*/
/*------------------------------------------------------*/

int tclirsim_print()
{
    int idx, bidx, bval;
    Tcl_Obj *robj;

    static char *booleanOptions[] = {
        "false", "no", "off", "0", "true", "yes", "on", "1"
    };
    static char *timeOptions[] = {
        "banner", "legend", "times", "title", "outline", "file", NULL
    };
    enum timeopt {
	PRINT_BANNER, PRINT_LEGEND, PRINT_TIMES, PRINT_TITLE,
	PRINT_OUTLINE, PRINT_FILE
    };

    if (targc == 1) {
	lprintf(stderr, "Usage: print <option>...\n");
        return -1;
    }

    idx = lookup(targv[1], timeOptions, FALSE);
    if (idx < 0) return -1;

    if (targc == 2) {
	switch (idx) {
	    case PRINT_BANNER:
		robj = Tcl_NewBooleanObj(psBanner);
		Tcl_SetObjResult(irsiminterp, robj);
		break;
	    case PRINT_LEGEND:
		robj = Tcl_NewBooleanObj(psLegend);
		Tcl_SetObjResult(irsiminterp, robj);
		break;
	    case PRINT_TIMES:
		robj = Tcl_NewBooleanObj(psTimes);
		Tcl_SetObjResult(irsiminterp, robj);
		break;
	    case PRINT_TITLE:
		if (banner) {
		    robj = Tcl_NewStringObj(banner, -1);
		    Tcl_SetObjResult(irsiminterp, robj);
		}
		break;
	    case PRINT_OUTLINE:
		robj = Tcl_NewBooleanObj(psOutline);
		Tcl_SetObjResult(irsiminterp, robj);
		break;
	    case PRINT_FILE:
		printPS("");
		lprintf(stderr, "Filename required\n");
		return -1;
	}
	
    }
    else if (targc == 3) {
	if ((idx != PRINT_FILE) && (idx != PRINT_TITLE)) {
	    bidx = lookup(targv[2], booleanOptions, FALSE);
	    if (bidx < 0) return -1;
	    bval = (bidx  <= 3) ? 0 : 1;
	}
	switch (idx) {
	    case PRINT_BANNER:
		psBanner = bval;
		break;
	    case PRINT_LEGEND:
		psLegend = bval;
		break;
	    case PRINT_TIMES:
		psTimes = bval;
		break;
	    case PRINT_OUTLINE:
		psOutline = bval;
		break;
	    case PRINT_FILE:
		printPS(targv[2]);
		break;
	    case PRINT_TITLE:
		if (banner) free(banner);
		banner = strdup(targv[2]);
		bannerLen = strlen(banner);
		break;
	}
    }
    return 0;
}

/*------------------------------------------------------*/
/* simtime <option>					*/
/*------------------------------------------------------*/

int tclirsim_simtime()
{
    int idx, x;
    double dt;

    static char *booleanOptions[] = {
        "false", "no", "off", "0", "true", "yes", "on", "1"
    };
    static char *timeOptions[] = {
        "begin", "end", "left", "right", "delta", "marker", "cursor",
	"move", "scroll", NULL
    };
    enum timeopt {
        TIME_BEGIN, TIME_END, TIME_LEFT, TIME_RIGHT, TIME_DELTA,
	TIME_MARKER, TIME_CURSOR, TIME_MOVE, TIME_SCROLL
    };

    if (targc == 1) {
	lprintf(stderr, "Usage: simtime <option>");
        return -1;
    }

    idx = lookup(targv[1], timeOptions, FALSE);
    if (idx < 0) return -1;

    switch (idx) {
	case TIME_BEGIN:
            if (targc == 2)
                Tcl_SetObjResult(irsiminterp, Tcl_NewDoubleObj(analyzer_time_start()));
            break;

        case TIME_END:
            if (targc == 2) {
		dt = analyzer_time_end();
		if (dt >= 0.0) {
		    Tcl_SetObjResult(irsiminterp, Tcl_NewDoubleObj(dt));
		}
	    }
            break;

        case TIME_LEFT:
            if (targc == 3) {
		if (sscanf(targv[2], "%lg", &dt) != 1)
		     return -1;
		else {
		    /* Set the zoom such that the time at the left edge	*/
		    /* is set to the specified time while the right	*/
		    /* edge remains constant.				*/
		    if (dt < 0.0) dt = 0.0;
		    SetTimeLeft((TimeType)(ns2d(dt)));
		}
            }
            else if (targc == 2) {
                dt = analyzer_time_left();
		if (dt >= 0.0) {
		    Tcl_SetObjResult(irsiminterp, Tcl_NewDoubleObj(dt));
		}
	    }
            break;

        case TIME_RIGHT:
            if (targc == 3) {
		if (sscanf(targv[2], "%lg", &dt) != 1)
		     return -1;
		else {
		    /* Set the zoom such that the time at the right	*/
		    /* edge is set to the specified time while the	*/
		    /* left edge remains constant.			*/
		    if (dt < 0.0) dt = 0.0;
		    SetTimeRight((TimeType)(ns2d(dt)));
		}
            }
            else if (targc == 2) {
                dt = analyzer_time_right();
		if (dt >= 0.0) {
		    Tcl_SetObjResult(irsiminterp, Tcl_NewDoubleObj(dt));
		}
	    }
            break;

        case TIME_DELTA:
            if (targc == 2) {
                dt = analyzer_time_delta();
                if (dt >= 0.0) {
                    Tcl_SetObjResult(irsiminterp, Tcl_NewDoubleObj(dt));
                }
            }
            break;

        case TIME_MARKER:
            if (targc == 2) {
                dt = analyzer_time_marker();
                if (dt >= 0.0) {
                    Tcl_SetObjResult(irsiminterp, Tcl_NewDoubleObj(dt));
                }
            }
            break;

        case TIME_CURSOR:
            x = -1;
            if (targc == 3) {
		if (sscanf(targv[2], "%d", &x) != 1)
		     return -1;
            }
            if (x >= 0) {
                dt = (double)analyzer_time_cursor(x);
                Tcl_SetObjResult(irsiminterp, Tcl_NewDoubleObj(dt));
            }
            else {
                Tcl_SetResult(irsiminterp, "Bad position value", 0);
                return TCL_ERROR;
            }
            break;

	case TIME_MOVE:
            if (targc == 3) {
		if (sscanf(targv[2], "%lg", &dt) != 1)
		     return -1;
		if (targv[2][0] == '+' || targv[2][0] == '-') {
		    /* Move relative, by the indicated value */
		    TimeType ltime;
		    double rt = analyzer_time_left();
		    if ((rt + dt) < 0.0) rt = dt = 0.0;
		    ltime = (TimeType)(ns2d(rt + dt));
		    MoveToTimeValue(ltime);
		}
		else {
		    MoveToT(targv[2]);
		}
            }
	    break;

	case TIME_SCROLL:
	    if (targc == 2) {
	        if (autoScroll)
		    Tcl_SetResult(irsiminterp, "1", 0);
		else
		    Tcl_SetResult(irsiminterp, "0", 0);
	    }
	    else if (targc == 3) {
	       idx = lookup(targv[2], booleanOptions, FALSE);
	       if (idx < 0) return -1;
	       autoScroll = (idx  <= 3) ? 0 : 1;
	    }
	    break;
    }
    return 0;
}

/*------------------------------------------------------*/
/* trace <option>					*/
/*------------------------------------------------------*/

int tclirsim_trace()
{
    Trptr t, s;
    int result = 0, idx, tidx, y;
    char *tracename;

    static char *traceOptions[] = {
        "top", "bottom", "order", "base", "class", "cursor", "input",
	"list", "select", "value", "bits", "remove", "characters",
	"move", (char *)NULL
    };
    enum traceopt {
        TRACE_TOP, TRACE_BOTTOM, TRACE_ORDER, TRACE_BASE, TRACE_CLASS,
        TRACE_CURSOR, TRACE_INPUT, TRACE_LIST, TRACE_SELECT, TRACE_VALUE,
	TRACE_BITS, TRACE_REMOVE, TRACE_CHARS, TRACE_MOVE
    };

    static char *listOptions[] = {
        "vectors", "nodes", "all", (char *)NULL
    };
    enum listopt {
        LIST_VECTORS, LIST_NODES, LIST_ALL
    };

    if (targc == 1) {
	lprintf(stderr, "Usage: trace <option>");
        return -1;
    }

    idx = lookup(targv[1], traceOptions, FALSE);
    if (idx < 0) return -1;

    tracename = (targc >= 3) ? targv[2] : NULL;

    switch (idx) {
	case TRACE_MOVE:
	    if (targc != 4) {
		lprintf(stderr, "Usage: trace move <trace1> <trace2>\n");
		return -1;
	    }
	    t = get_trace(targv[2]);
	    s = get_trace(targv[3]);
	    if (t == NULL || s == NULL) {
		lprintf(stderr, "invalid trace name.\n");
		return -1;
	    }
	    MoveTraces(t, s);
	    break;

	case TRACE_TOP:
	    if (targc == 3)
                 Tcl_SetObjResult(irsiminterp,
			Tcl_NewIntObj(analyzer_trace_top(tracename)));
	    break;

        case TRACE_BOTTOM:
            if (targc == 3)
                 Tcl_SetObjResult(irsiminterp,  
                        Tcl_NewIntObj(analyzer_trace_bottom(tracename)));
            break;

        case TRACE_ORDER:
            if (targc == 3)
                 Tcl_SetObjResult(irsiminterp,
                        Tcl_NewIntObj(analyzer_trace_order(tracename)));
            break;

        case TRACE_BASE:
            if (targc == 3)
                 Tcl_SetObjResult(irsiminterp,
                        Tcl_NewIntObj(analyzer_trace_base(tracename)));
            break;

        case TRACE_CLASS:
            if (targc == 3)
                 Tcl_SetResult(irsiminterp, analyzer_trace_class(tracename), NULL);
            break;

        case TRACE_CURSOR:
            y = -1;
            if (targc == 3) {
		if (sscanf(targv[2], "%d", &y) != 1)
		     return -1;
            }
	    tracename = analyzer_trace_cursor((TimeType)y);
            if (tracename != NULL) {
                Tcl_SetObjResult(irsiminterp, Tcl_NewStringObj(tracename, -1));
            }
	    /* If no trace was found, simply return nothing. */
            break;

	case TRACE_LIST:
	    tidx = -1;
	    if (targc >= 3) tidx = lookup(targv[2], listOptions, FALSE);
	    if (tidx < 0) {
	        lprintf(stderr, "Usage: trace list [all|nodes|vectors]\n");
	        return -1;
	    }
	    switch(tidx) {
		case LIST_VECTORS:
		    Tcl_SetObjResult(irsiminterp, analyzer_list_vectors(irsiminterp));  
		    break;
		case LIST_NODES:
		    Tcl_SetObjResult(irsiminterp, analyzer_list_nodes(irsiminterp));
		    break;
		case LIST_ALL:
		    Tcl_SetObjResult(irsiminterp, analyzer_list_all(irsiminterp));
		    break;
	    }
	    break;

	case TRACE_SELECT:
	    if (targc == 2) {
	        if (selectedTrace) {
		    SelectTrace(selectedTrace);	/* prints stuff. . . */
	        }
	        else {
		    lprintf(stderr, "must select or specify a trace.\n");
		    return -1;
	        }
	    }
	    else {
	        t = get_trace(targv[2]);
	        if (t == NULL) {
		    lprintf(stderr, "invalid trace name.\n");
		    return -1;
	        }
	        SelectTrace(t);
	    }
	    break;

	case TRACE_VALUE:
	    if (targc != 3) {
		lprintf(stderr, "Usage: trace value <trace>\n");
		return -1; 
	    }
	    t = get_trace(targv[2]);
	    if (t == NULL) {
		lprintf(stderr, "invalid trace name.\n");
		return -1;
	    }
	    TraceValue(t, 0);  /* Set 2nd arg. = 1 to force binary */
	    break;

	case TRACE_INPUT:
	    if (targc != 3) {
		lprintf(stderr, "Usage: trace input <trace>\n");
		return -1; 
	    }
	    t = get_trace(targv[2]);
	    if (t == NULL) {
		lprintf(stderr, "invalid trace name.\n");
		return -1;
	    }
	    TraceInput(t);
	    break;

	case TRACE_BITS:
	    if (targc != 3) {
		lprintf(stderr, "Usage: trace input <trace>\n");
		return -1; 
	    }
	    t = get_trace(targv[2]);
	    if (t == NULL) {
		lprintf(stderr, "invalid trace name.\n");
		return -1;
	    }
	    TraceBits(t);
	    break;

	case TRACE_REMOVE:
	    if (targc != 3) {
	        lprintf(stderr, "Usage: trace remove [all|<trace>]\n");
	        return -1;
	    }
	    t = get_trace(targv[2]);
	    if (t == NULL) {
	       if (!strcmp(targv[2], "all"))
		  ClearTraces();
	       else {
		  lprintf(stderr, "invalid trace name.\n");
		  return -1;
	       }
	    }
	    else
	       RemoveTrace(t);
	    UpdateWinRemove();
	    break;

	case TRACE_CHARS:
	    if (targc == 2) {
	       Tcl_SetObjResult(irsiminterp, Tcl_NewIntObj(max_name_len));
	    }
	    else if (targc == 3) {
	       GetNameLen(targv[2]);
	    }
	    break;
    }
    return result;
}

/*------------------------------------------------------*/
/* zoom [in|out]					*/
/*------------------------------------------------------*/

int tclirsim_zoom()
{
    int idx;

    static char *zoomOptions[] = {
	"in", "out", NULL
    };
    enum zoomopt {
	ZOOM_IN, ZOOM_OUT
    };

    if (targc == 1) {
        /* To-do: return zoom factor */
        return 0;
    }

    idx = lookup(targv[1], zoomOptions, FALSE);
    if (idx < 0) return -1;

    switch(idx) {
	case ZOOM_IN:
	    Zoom("in");
	    break;
	case ZOOM_OUT:
	    Zoom("out");
	    break;
    }
    return 0;
}

