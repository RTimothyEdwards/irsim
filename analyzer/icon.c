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

#include "ana.h"
#include "graphics.h"
#include <X11/Xutil.h>
#include "ana_glob.h"

#include "Bitmaps/opus"


public Pixmap InitIconBitmap()
  {
    return( XCreateBitmapFromData( display, DefaultRootWindow( display ),
      opus_bits, opus_width, opus_height ) );
  }


public void RedrawIcon( exv )
  XEvent  *exv;
  {
    XCopyPlane( display, pix.icon, iconW, gcs.black, 0, 0, opus_width,
     opus_height, 0, 0, 1 );
  }


public Window CreateIconWindow( x, y )
  Coord  x, y;
  {
    Window      icw;

    icw = XCreateWindow( display, RootWindowOfScreen( screen ), x, y,
     opus_width, opus_height, 1, DefaultDepthOfScreen( screen ), InputOutput,
     (Visual *) CopyFromParent, 0L, (XSetWindowAttributes *) NULL );

    XSetWindowBackground( display, icw, colors.white );
    XSetWindowBorder( display, icw, colors.black );
    
    XSelectInput( display, icw, ExposureMask );

    return( icw );
  }

