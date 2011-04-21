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
 * Logic analyzer event manager.
 */

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>   /* **mdg** */

#ifdef SYS_V
#    include <termio.h>
#    ifdef hpux
#	define	SigBlock( sig )	sigsetmask( sigmask( sig ) )
#	define	SigRelease( sig )	\
		    sigsetmask( sigsetmask( 0 ) & ~sigmask( sig ) )
#    else
#	include <sys/filio.h>
#	include <stropts.h>
#	define	signal( A, B )		sigset( A, B )
#	define	SigBlock( sig )		sighold( sig )
#	define	SigRelease( sig )	sigrelse( sig )
#    endif
#    define	vfork()		fork()
#else
#    include <sys/ioctl.h>
#    ifndef	sigmask
#	define sigmask( m )		( 1 << (( m ) - 1 ) )
#    endif
#    define	SigBlock( sig )	sigsetmask( sigmask( sig ) )
#    define	SigRelease( sig )	\
		    sigsetmask( sigsetmask( 0 ) & ~sigmask( sig ) )
#endif /* SYS_V */

#include "ana.h"
#include "X11/Xutil.h"
#include "ana_glob.h"
#include "graphics.h"

#ifndef HAVE_PTHREADS
#include "helper.h"
#endif

#define	WITHINY( Y, Box )	( (Y <= (Box).bot) and (Y >= (Box).top) )
#define	WITHINX( X, Box )	( (X >= (Box).left) and (X <= (Box).right) )

private	int     x_server = 0;
private	int     x_helper = 0;		/* process id of helper process */
public	Func    FGetEvent = NULL;	/* used also in thread.c */

public void WindowResize( ev )
  XConfigureEvent  *ev;
  {
    int  ret;

    if( ev->width != XWINDOWSIZE or ev->height != YWINDOWSIZE )
      {
	XWINDOWSIZE = ev->width;
	YWINDOWSIZE = ev->height;
	ret = WindowChanges();
      }
  }


public void WindowExposed( event )
  XExposeEvent  *event;
  {
    BBox  box;

    box.left = event->x;
    box.right = event->x + event->width - 1;
    box.top = event->y;
    box.bot = event->y + event->height - 1;
    RedrawWindow( box );
  }


public void HandleButton( ev )
  XButtonEvent  *ev;
  {
    if( WITHINY( ev->y, scrollBox ) )
	DoScrollBar( ev );
    else if( WITHINY( ev->y, traceBox) )
      {
	if( WITHINX( ev->x, namesBox ) )
	    MoveTrace( ev->y );
	else if( WITHINX( ev->x, traceBox ) )
	    DoCursor( ev );
	else if( WITHINX( ev->x, cursorBox ) )
	    SelectCursTrace( ev->y );
      }
    else if( WITHINY( ev->y, bannerBox ) )
      {
	if( WITHINX( ev->x, iconBox ) )
	    IconifyMe();
	else if( WITHINX( ev->x, sizeBox ) )
	    ResizeMe();
	else if( WITHINX( ev->x, menuBox ) )
	    DoMenu( ev->x, ev->y );
	else
	  {
	    switch( ev->button & (Button1 | Button2 | Button3) )
	      {
		case Button1 : XRaiseWindow( display, window ); break;
		case Button2 : MoveMe( ev->x, ev->y ); break;
		case Button3 : XLowerWindow( display, window ); break;
	      }
	  }
      }
  }


public void HandleKey( ev )
  XKeyEvent  *ev;
  {
    char  buff[ 40 ];
    int   nChars, i;

    nChars = XLookupString( ev, buff, sizeof( buff ), NULL, NULL );
    for( i = 0; i < nChars; i++ )
      {
	switch( buff[i] )
	  {
	    case 'i' :
	    case 'o' :
		Zoom( buff );
		break;

	    case 'd' :
		DeltaT( buff );
		break;

	    case 'm' :
		MoveToTime( buff );
		break;

	    case 'p' :
		printPS( buff );
		break;

	    case 'w' :
		SetWidth( buff );
		break;

	    default:
		XBell( display, 0 );
	  }
      }
  }


public void SendEventTo( f )
  Func  f;
  {
    FGetEvent = f;
  }

#ifndef HAVE_PTHREADS

private void EventHandler()
  {
    XEvent  event;

    if( XEventsQueued( display, QueuedAfterReading ) > 0 )
      {
	do
	  {
	    XNextEvent( display, &event );
 	    /* printf("XNextEvent type %d\n", event.type); */
	    switch( event.type )
	      {
		case ButtonPress :
		    if( windowState.tooSmall )
			;
		    else if( FGetEvent != NULL )
			(*FGetEvent)( &event );
		    else
			HandleButton( &event.xbutton );
		    break;

		case KeyPress :
		    if( windowState.tooSmall )
			;
		    else if( FGetEvent != NULL )
			(*FGetEvent)( &event.xkey );
		    else
			HandleKey( &event.xkey );
		     break;

		case EnterNotify :
		    if( event.xcrossing.window == window )
			WindowCrossed( TRUE );
		    break;

		case LeaveNotify :
		    if( event.xcrossing.window == window )
			WindowCrossed( FALSE );
		break;

		case Expose :
		    if( event.xexpose.window == iconW )
			RedrawIcon( &event.xexpose );
		    else if( windowState.tooSmall )
			RedrawSmallW();
		    else if( event.xexpose.window == window )
			WindowExposed( &event.xexpose );
		    break;

		case ConfigureNotify :
		    WindowResize( &event.xconfigure );
		    break;

		case UnmapNotify :
		    windowState.iconified = TRUE;
		    break;

		case MapNotify :
		    windowState.iconified = FALSE;
		    windowState.selected = FALSE;
		    (void) WindowChanges();
		    break;

		default : ;
	      }
	  }
	while( XPending( display ) );
      }

#ifdef NEED_HELPER
    if( x_helper != 0 )
	(void) kill( x_helper, SIGINT );
#endif

#if defined( SYS_V ) || defined( LINUX )
    /* printf("signal( SIGIO, EventHandler ) at exit\n");   */
    (void) signal( SIGIO, EventHandler );
#endif

   }

private void DisabledEventHandler()
  {
    XEvent  event;

    if( XEventsQueued( display, QueuedAfterReading ) > 0 )
      {
	do
	  {
	    XNextEvent( display, &event );
	    switch( event.type )
	      {
		case EnterNotify :
		    if( event.xcrossing.window == window )
			WindowCrossed( TRUE );
		    break;

		case LeaveNotify :
		    if( event.xcrossing.window == window )
			WindowCrossed( FALSE );
		    break;

		case Expose :
		    if( event.xexpose.window == iconW )
			RedrawIcon( &event );
		    else if( windowState.tooSmall )
			RedrawSmallW();
		    break;

		case ConfigureNotify :
		    WindowResize( &event.xconfigure );
		    break;

		case UnmapNotify :
		    windowState.iconified = TRUE;
		    break;

		case MapNotify :
		    windowState.iconified = FALSE;
		    windowState.selected = FALSE;
		    break;

		default : ;
	      }
	  }
	while( XPending( display ) );
      }

#ifdef NEED_HELPER
    if( x_helper != 0 )
	(void) kill( x_helper, SIGINT );
#endif

#if defined( SYS_V ) || defined( LINUX )
    (void) signal( SIGIO, DisabledEventHandler );
#endif

   }


#ifdef NEED_HELPER

private int StartHelper( fd )
  int  fd;
  {
    extern char  *cad_bin, *getenv();
    static char  helper_name[] = "anXhelper";
    char         helper_proc[ 256 ];

    (void) sprintf( helper_proc, "%s/%s", cad_bin, helper_name );

    if( x_helper == 0 )			/* never start 2 processes */
	x_helper = vfork();

    if( x_helper == -1 )
      {
	fprintf( stderr, "can't fork helper process\n" );
	x_helper = 0;
	return( FALSE );
      }
    else if( x_helper == 0 )		/* child process */
      {
	if( dup2( fd, 0 ) == -1 )
	  {
	    fprintf( stderr, "can't dup descriptor\n" );
	    _exit( 1 );
	  }

	(void) execl( helper_proc, helper_name, NULL );
	fprintf( stderr, "can't exec helper process: %s/%s\n", helper_proc, helper_name );
	_exit( 1 );
      }
    (void) sleep( (unsigned) 1 );		/* make sure process started */

    if( kill( x_helper, SIGUSR1 ) != 0 )
    {
	x_helper = 0;
	fprintf( stderr, "helper process is dead\n" );
	return( FALSE );
      }
    return( TRUE );
  }

#endif /* NEED_HELPER */

public int InitHandler( fd )
  int fd;
  {
    int   flags;
    char  *senv;

    x_server = fd;
    (void) signal( SIGIO, EventHandler );

#ifdef NEED_HELPER
    return( StartHelper( fd ) );
#endif /* NEED_HELPER */

# ifdef SYS_V
    flags = 1;
    if( ioctl( fd, FIOASYNC, &flags ) == -1 )
      {
	fprintf( stderr, "ioctl: can not set FIOASYNC\n" );
	return( FALSE );
      }
# else
    if( (flags = fcntl( fd, F_GETFL, 0 )) == -1 )
      {
	fprintf( stderr, "fctl: can not do F_GETFL\n" );
	return( FALSE );
      }

    if( (flags & FASYNC) == 0 )
      {
	if( fcntl( fd, F_SETFL, flags | FASYNC ) == -1 )
	  {
	    fprintf( stderr, "fctl: can not do F_SETFL\n" );
	    return( FALSE );
	  }
      }
# endif /* SYS_V */

# ifdef I_SETSIG
	if( ioctl( fd, I_SETSIG, S_INPUT ) == -1 )
	{
	fprintf( stderr, "ioctl: can not do I_SETSIG\n" );
	return( FALSE );
	}
# else
# ifdef F_SETOWN
    if( fcntl( fd, F_SETOWN, getpid() ) == -1 )
      {
	fprintf( stderr, "fctl: can not do F_SETOWN\n" );
	return( FALSE );
      }
# endif
# endif

    return( TRUE );
  }


public void DisableInput()
  {
    (void) SigBlock( SIGIO );
  }


public void EnableInput()
  {
    if( XPending( display ) )
      {
	EventHandler();
      }
    else
      {
#ifdef NEED_HELPER
	if( x_helper != 0 )
	    (void) kill( x_helper, SIGINT );
#endif
	;
      }
    (void) SigRelease( SIGIO );
  }


public void DisableAnalyzer()
  {
    (void) SigBlock( SIGIO );
    (void) signal( SIGIO, DisabledEventHandler );
    (void) SigRelease( SIGIO );
  }


public void EnableAnalyzer()
  {
    int  change;

    DisableInput();

    updatePending = FALSE;

    if( FGetEvent != NULL )
      {
	(*FGetEvent)( (XEvent *) NULL );	/* let handler know */
	FGetEvent = NULL;
      }

    (void) signal( SIGIO, EventHandler );

    if( windowState.iconified == 0 and windowState.tooSmall == 0 )
      {
	change = WindowChanges();
	if( not (change & RESIZED) )
	  {
	    BBox  box;

	    box.left = box.top = 0;
	    box.right = XWINDOWSIZE - 1;
	    box.bot = YWINDOWSIZE - 1;
	    RedrawWindow( box );
	  }
      }
    EnableInput();
  }


public void TerminateAnalyzer()
  {
    if( x_helper != 0 )
	(void) kill( x_helper, SIGKILL );
  }

#endif  /* !HAVE_PTHREADS */
