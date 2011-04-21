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
 * Procedures needing to be defined separately for the Tk version
 * of the analyzer window.
 */

#include <stdio.h>
#ifdef LINUX
#include <string.h>  /* for strcmp() */
#endif
#include "ana.h"
#include <X11/Xutil.h>
#include "graphics.h"
#include "ana_glob.h"
#include "ana_tk.h"

#ifdef TCL_IRSIM

/*
 * Procedures querying the state of the analyzer display.
 */

float analyzer_time_start()
{
    return (float)d2ns(tims.first);
}

float analyzer_time_end()
{
    if (tims.last >= TIME_BOUND)
	return -1.0;
    else
	return (float)d2ns(tims.last);
}

float analyzer_time_left()
{
    if (tims.start >= TIME_BOUND)
	return -1.0;
    else
	return (float)d2ns(tims.start);
}

float analyzer_time_right()
{
    if (tims.end >= TIME_BOUND)
	return -1.0;
    else
        return (float)d2ns(tims.end);
}

float analyzer_time_step()
{
    return (float)d2ns(tims.steps);
}

float analyzer_time_delta()
{
    if (tims.delta >= TIME_BOUND)
	return -1.0;
    else
        return (float)d2ns(tims.delta);
}

float analyzer_time_marker()
{
    if (tims.cursor >= TIME_BOUND)
	return -1.0;
    else
        return (float)d2ns(tims.cursor);
}

float analyzer_time_cursor(int x)
{
    float ctime = d2ns(XToTime(x));
    if (ctime >= TIME_BOUND)
	return -1.0;
    else
	return (float)ctime;
}

Trptr get_trace(char *tracename)
{
    Trptr t;

    for (t = traces.first; t != NULL; t = t->next)
       if (!strcmp(t->name, tracename))
	   return t;

    return (Trptr)NULL;
}

int analyzer_trace_top(char *tracename)
{
    Trptr t = get_trace(tracename);
    return (t != NULL) ? t->top : -1;
}

int analyzer_trace_bottom(char *tracename)
{
    Trptr t = get_trace(tracename);
    return (t != NULL) ? t->bot : -1;
}

int analyzer_trace_order(char *tracename)
{
    Trptr t;
    int i = 0;

    for (t = traces.first; t != NULL; t = t->next) {
       if (!strcmp(t->name, tracename))
	   return i;
       i++;
    }
    return -1;
}

int analyzer_trace_base(char *tracename)
{
    Trptr t = get_trace(tracename);
    if (t == NULL) return -1;
    else if (!t->vector) return -2;
    else return t->bdigit;
}

char *analyzer_trace_class(char *tracename)
{
    Trptr t = get_trace(tracename);
    if (t == NULL) return NULL;
    else if (t->vector) return "vector";
    else return "node";
}

char *analyzer_trace_cursor(int y)
{
    Trptr t;

    for (t = traces.first; t != NULL; t = t->next) {
       if (t && (t->bot >= y) && (t->top <= y))
	  return t->name;
    }
    return NULL;
}

Tcl_Obj *analyzer_list_all(interp)
    Tcl_Interp *interp;
{
    Tcl_Obj *tlist;
    Trptr t;

    tlist = Tcl_NewListObj(0, NULL);
    for (t = traces.first; t != NULL; t = t->next) {
       Tcl_ListObjAppendElement(interp, tlist, Tcl_NewStringObj(t->name, t->len));
    }
    return tlist;
}

Tcl_Obj *analyzer_list_vectors(interp)
    Tcl_Interp *interp;
{
    Tcl_Obj *tlist;
    Trptr t;

    tlist = Tcl_NewListObj(0, NULL);
    for (t = traces.first; t != NULL; t = t->next) {
       if (t->vector)
	  Tcl_ListObjAppendElement(interp, tlist, Tcl_NewStringObj(t->name, t->len));
    }
    return tlist;
}

Tcl_Obj *analyzer_list_nodes(interp)
    Tcl_Interp *interp;
{
    Tcl_Obj *tlist;
    Trptr t;

    tlist = Tcl_NewListObj(0, NULL);
    for (t = traces.first; t != NULL; t = t->next) {
       if (!(t->vector))
	  Tcl_ListObjAppendElement(interp, tlist, Tcl_NewStringObj(t->name, t->len));
    }
    return tlist;
}

/*
 * Handle various procedures that are different for the Tk window
 */

public void SendEventTo(f)
    Func  f; 
{
    /* Do Nothing --- events are handled by the Tk Event Handler */
}

public void UpdateScrollBar()
{
    /* Do Nothing (for now)---need to communicate w/the Tk scrollbar */
}

public void DrawScrollBar(b)
    int b;
{
    /* Do Nothing (for now)---need to communicate w/the Tk scrollbar */
}

public void RedrawBanner()
{
    /* Do Nothing */
}

public void RedrawTimes()
{
    /* Do Nothing */
}

public void UpdateTimes()
{
    /* Do Nothing */
}

public void GrabMouse(w, ev_mask, cursor)
    Window         w;
    unsigned long  ev_mask;
    Cursor         cursor;
{
    /* Do Nothing (?) */
}

public void EnableAnalyzer()
{
    /* Do Nothing (?) */
}

public void DisableAnalyzer()
{
    /* Do Nothing (?) */
}

public void TerminateAnalyzer()
{
    /* Do Nothing (?) */
}

public void EnableInput()
{
    /* Do Nothing (?) */
}

public void DisableInput()
{
    /* Do Nothing (?) */
}

/*
 * Tk initializes the X11 display.  This routine is called when
 * Tk maps the analyzer window, and we can query the Tk_Window
 * structure about the display.
 */

public int InitDisplay(fname, tkwind)
    char  *fname;
    Tk_Window tkwind;
{
    XFontStruct  *font;
    Tk_Window tktop = Tk_MainWindow(irsiminterp);

    if (tkwind == NULL)
	return FALSE;

    if (!Tk_IsMapped(tkwind))
	Tk_MapWindow(tkwind);

    /* In the Tcl version, the display should have been initialized. */
    display = Tk_Display(tktop);
    screen = Tk_Screen(tktop);
    window = Tk_WindowId(tkwind);

    XWINDOWSIZE = Tk_Width(tkwind);
    YWINDOWSIZE = Tk_Height(tkwind);

    /* Initialize the fonts and graphics contexts */
    return SetFont();
}

#endif /* (TCL_IRSIM) */

