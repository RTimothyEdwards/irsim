/* thread.c --
 *
 * This file contains functions to run the X11 event loop as a separate
 * thread.  Used by X11.  Compiled only if option "threads" is set in
 * "make config".
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <signal.h>

#include <X11/keysym.h>
#include <pthread.h>

#include "ana.h"
#include "ana_glob.h"
#include "graphics.h"

extern Display *display;
extern Window  window;
extern Func FGetEvent;

pthread_t xloop_thread = 0;
Func EventHandlerPtr = NULL;

/*---------------------------------------------------------
 * Display locking and unlocking
 *
 * Locking the display flushes the X11.  Normally, this is
 * done by XNextEvent(), but with the threaded event watcher,
 * XNextEvent() is only run by one of the two threads.  So
 * X11 calls made by the other thread will not get updated
 * unless XFlush() is called when the thread releases control
 * of the thread lock.
 *---------------------------------------------------------
 */

public void
DisableInput()
{
    if (display)
	XLockDisplay(display);
}

public void
EnableInput()
{
    if (display)
    {
        XFlush(display);
	XUnlockDisplay(display);
    }
}

/*---------------------------------------------------------
 * xloop_begin:  Threaded version of XHelper7 (X11Helper.c)
 *
 *---------------------------------------------------------
 */

int
xloop_begin()
{
    XEvent xevent;
    int parentID;

    /* Set this thread to cancel immediately when requested */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (1) {
	XNextEvent(display, &xevent);
	DisableInput();
        (*EventHandlerPtr)(&xevent);
	EnableInput();
    }
}  

/*----------------------------------------------------------------------*/
/* xloop_create function creates a thread for the X event loop.         */
/* Returns 0 on success of pthread_create, non-zero on failure.         */
/*----------------------------------------------------------------------*/

int
xloop_create()
{
    int status = 0;

    if (xloop_thread == (pthread_t)0)
        status = pthread_create(&xloop_thread, NULL, (void *)xloop_begin,
		NULL);

    return status;
}

/*----------------------------------------------------------------------*/
/* Kill the X event loop thread.					*/
/*----------------------------------------------------------------------*/

void
xloop_end()
{
    pthread_cancel(xloop_thread);
    pthread_join(xloop_thread, NULL);
}

/*------------------------------------------------------*/
/* EventHandler is modified from original in event.c	*/
/*------------------------------------------------------*/

/* Active event handler procedure */

private void EventHandler(event)
    XEvent *event;
{
    switch (event->type)
    {
	case ButtonPress:
	    if (windowState.tooSmall);
	    else if (FGetEvent != NULL)
		(*FGetEvent)(event);
	    else
		HandleButton(&event->xbutton);
	    break;

	case KeyPress:
	    if (windowState.tooSmall);
	    else if (FGetEvent != NULL)
		(*FGetEvent)(&event->xkey);
	    else
		HandleKey(&event->xkey);
	    break;

	case EnterNotify:
	    if (event->xcrossing.window == window)
		WindowCrossed(TRUE);
	    break;

	case LeaveNotify:
	    if (event->xcrossing.window == window)
		WindowCrossed(FALSE);
	    break;

	case Expose:
	    if (event->xexpose.window == iconW)
		RedrawIcon(&event->xexpose);
	    else if (windowState.tooSmall)
		RedrawSmallW();
	    else if (event->xexpose.window == window)
		WindowExposed(&event->xexpose);
	    break;

	case ConfigureNotify:
	    WindowResize(&event->xconfigure);
	    break;

	case UnmapNotify:
	    windowState.iconified = TRUE;
	    break;

	case MapNotify:
	    windowState.iconified = FALSE;
	    windowState.selected = FALSE;
	    (void) WindowChanges();
	    break;

	default:
	    break;
    }
}

/* Inactive event handler procedure */

private void DisabledEventHandler(event)
    XEvent *event;
{
    switch(event->type)
    {
	case EnterNotify:
	    if (event->xcrossing.window == window)
		WindowCrossed(TRUE);
	    break;

	case LeaveNotify:
	    if(event->xcrossing.window == window)
		WindowCrossed(FALSE);
	    break;

	case Expose:
	    if (event->xexpose.window == iconW)
		RedrawIcon(event);
	    else if (windowState.tooSmall)
		RedrawSmallW();
	    break;

	case ConfigureNotify:
	    WindowResize(&event->xconfigure);
	    break;

	case UnmapNotify:
	    windowState.iconified = TRUE;
	    break;

	case MapNotify:
	    windowState.iconified = FALSE;
	    windowState.selected = FALSE;
	    break;
    }
}

/* Initialize handler by installing the active event handler */

public int InitHandler(fd)
    int fd;
{
    int   flags;
    char  *senv;

    EventHandlerPtr = EventHandler;
    return (TRUE);
}

/* Install inactive event handler */

public void DisableAnalyzer()
{
    EventHandlerPtr = DisabledEventHandler;
}

/* Install active event handler */

public void EnableAnalyzer()
{
    int change;

    updatePending = FALSE;

    if (FGetEvent != NULL)
    {
	(*FGetEvent)((XEvent *)NULL);
	FGetEvent = NULL;
    }

    if (windowState.iconified == 0 && windowState.tooSmall == 0)
    {
	change = WindowChanges();
	if (!(change & RESIZED))
	{
	    BBox  box;

	    box.left = box.top = 0;
	    box.right = XWINDOWSIZE - 1;
	    box.bot = YWINDOWSIZE - 1;
	    RedrawWindow( box );
	}
    }
    EventHandlerPtr = EventHandler;
}

/* Do nothing; thread is configured to terminate automatically */

public void TerminateAnalyzer()
{
}
