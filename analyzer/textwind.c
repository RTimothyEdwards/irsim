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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "ana.h"
#include "ana_glob.h"
#include "graphics.h"

/* Note:  The functions in this file need to be redefined for Tcl/Tk */

#define	BUFFSIZE	256


private char    buffer[ BUFFSIZE ];	/* text widow buffer */
private int     txtPos = 0;		/* postion to print next character */
private	char    txt_cursor[] = " ";	/* cursor: a space in inverse video */
private	int     cursor_on = FALSE;	/* TRUE when cursor on */
private	int     maxPos;


#define	XPOS( Pos )		( ((Pos) * CHARWIDTH) + 2 )
#define	YPOS			( textBox.bot - 1 - descent )
#define	CWIDTH( X )		( (X) * CHARWIDTH )
#define	TXT_HEIGHT		( textBox.bot - textBox.top )


#define	PTEXT( Text, N, Len )			XDrawImageString \
    ( display, window, gcs.black, XPOS( N ), YPOS, Text, Len )

#define	PCURSOR()				XDrawImageString \
    ( display, window, gcs.white, XPOS( txtPos ), YPOS, txt_cursor, 1 )

#define	ERASE_CURSOR()							\
    PTEXT( txt_cursor, txtPos, 1 )


public void RedrawText()
  {
    if (window == 0) return;

    maxPos = textBox.right / CHARWIDTH;
    if( maxPos > BUFFSIZE )
	maxPos = BUFFSIZE;
    FillBox( window, textBox, gcs.white );
    HLine( window, textBox.left, textBox.right, textBox.top, gcs.black );
    if( txtPos )
	PTEXT( buffer, 0, txtPos );
    if( cursor_on )
	PCURSOR();
  }


public void PRINT( s )
  char  *s;
  {
    int  len;

    if (window == 0) return;

    if( *s == '\n' )
      {
	if( txtPos > 0 )
	    FillAREA( window, XPOS( 0 ), textBox.top + 1, CWIDTH( txtPos ),
	     TXT_HEIGHT, gcs.white );
	txtPos = 0;
	s++;
      }
    len = strlen( s );
    if( txtPos + len >= BUFFSIZE )
      {
	len = BUFFSIZE - 1 - txtPos;
      }
    if( len > 0 )
      {
	bcopy( s, buffer + txtPos, len );
	PTEXT( s, txtPos, len );
	txtPos += len;
      }
  }


/* VARARGS */

public void PRINTF(char *format, ...)
  {
    va_list  args;
    char     *s;
    int      len;

    if (window == 0) return;

    va_start(args, format);

    if( *format == '\n' )
      {
	if( txtPos > 0 )
	    FillAREA( window, XPOS( 0 ), textBox.top + 1, CWIDTH( txtPos ),
	     TXT_HEIGHT, gcs.white );
	txtPos = 0;
	format++;
	s = buffer;
      }
    else
	s = buffer + txtPos;

    (void) vsprintf(s, format, args);

    va_end(args);

    len = strlen(s);
    PTEXT(s, txtPos, len);
    txtPos += len;
  }


private	Func    FQuerying;
private	char    *inpStart;
private	void    StrInput();


public void Query( prompt, func )
  char   *prompt;
  Func   func;
  {
    if (window == 0) return;

    PRINT( prompt );
    inpStart = buffer + txtPos;
    cursor_on = TRUE;
    FQuerying = func;
    PCURSOR();
    SendEventTo( StrInput );
  }



	/* define some interesting control characters */

#define	BACKSPACE	'\b'
#define DELETE		'\177'
#define	TAB		'\t'
#define	RETURN		'\r'
#define	NEWLINE		'\n'
#define	CTRL_C		'\003'
#define	CTRL_U		'\025'
#define	CTRL_W		'\027'


private int Cancel()
  {
    int  left;

    if (window == 0) return -1;

    bcopy( "(canceled)", inpStart, 11 );
    left = XPOS( inpStart - buffer + 10 );
    FillAREA( window, left, textBox.top + 1, textBox.right - left + 1,
     TXT_HEIGHT, gcs.white );
    txtPos = inpStart - buffer;
    inpStart = NULL;
    return( txtPos + 10 );
  }


private void StrInput( ev )
  XKeyEvent  *ev;
  {
    char           buff[ 40 ];
    register char  *iPtr, *p;
    register int   newPos, len;
    int            nChars, finish = FALSE;

    if (window == 0) return;

    if( ev == NULL )
      {
	cursor_on = FALSE;
	ERASE_CURSOR();
	txtPos = 0;
	(*FQuerying)( NULL );
	return;
      }
	
    if( ev->type != KeyPress )
      {
	XBell( display, 0 );
	return;
      }

    newPos = txtPos;
    iPtr = buffer + newPos;
    nChars = XLookupString( ev, buff, sizeof( buff ), NULL, NULL );

    for( p = buff, len = nChars; len > 0 and not finish; p++, len-- )
      {
        switch( *p )
	  {
	    case BACKSPACE :
	    case DELETE :
		if( iPtr > inpStart )
		  {
		    newPos--;
		    iPtr--;
		  }
		break;
	    case RETURN :
	    case NEWLINE :
		*iPtr = '\0';
		finish = TRUE;
		break;

	    case TAB :
		*iPtr++ = ' ';
		newPos++;
		break;

	    case CTRL_C :
		newPos = Cancel();
		finish = TRUE;
		break;

	    case CTRL_U :
		if( iPtr > inpStart )
		  {
		    newPos = inpStart - buffer;
		    iPtr = inpStart;
		  }
		 break;

	    case CTRL_W :
		if( iPtr > inpStart )
		  {
		    iPtr--;
		    while( iPtr > inpStart and *iPtr == ' ' )
			iPtr--;
		    while( iPtr > inpStart and *iPtr != ' ' )
			iPtr--;
		    if( iPtr != inpStart )
			iPtr++;
		    newPos = iPtr - buffer;
		  }
		break;
	    default :
		if( *p >= ' ' )		/* displayable character */
		  {
		    if( newPos < maxPos )
		      {
			*iPtr++ = *p;
			newPos++;
		      }
		    else
			XBell( display, 0 );
		  }
	  }
      }

    if( newPos < txtPos )
	FillAREA( window, XPOS( newPos ), textBox.top + 1, 
	 CWIDTH( txtPos - newPos + 1 ), TXT_HEIGHT, gcs. white );
    else if( newPos > txtPos )
	PTEXT( buffer + txtPos, txtPos, newPos - txtPos );

    txtPos = newPos;

    if( finish )
      {
	SendEventTo( NULL );
	cursor_on = FALSE;
	ERASE_CURSOR();
	(*FQuerying)( inpStart );
      }
    else
	PCURSOR();
  }
