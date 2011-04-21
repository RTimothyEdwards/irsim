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
#include "ana.h"

private	char    result[ 200 ];
private	char    HexMap[] = "0123456789abcdefX";


/*
 * Convert a trace entry (vector) to an ascii string
 */
public char *HistToStr( hist, nbits, b_digit, offset )
  hptr  *hist;
  int   nbits, b_digit, offset;
  {
    register char  *p; 
    register int   i, j;
    Ulong digit;

    /* Handle decimal notation (special cases)		*/
    /* Since decimal does not map directly to bits,	*/
    /* we only print the result if no bits are unknown.	*/

    if (b_digit == 5) {
      hptr *htmp = hist;
      for (i = nbits; i > 0; i--) {
	 if ((*htmp)->val == X) {
	    sprintf(result, "???");
	    return (result);
	 }
	 htmp += offset;
      }
      digit = (Ulong)0;
      for (i = nbits; i > 0; i--) {
	 digit <<= 1;
	 if ((*hist)->val == HIGH) digit |= 1;
	 hist += offset;
      }
      if (digit < 0)
	 sprintf(result, "(overflow)");
      else
         sprintf(result, PRINTF_LLONG "u", digit);
      return (result);
    }

    /* Handle signed decimal notation */
    
    else if (b_digit == 6) {
      hptr *htmp = hist;
      char negative;

      negative = ((*htmp)->val == HIGH) ? 1 : 0;
      for (i = nbits; i > 0; i--) {
	 if ((*htmp)->val == X) {
	    sprintf(result, "???");
	    return (result);
	 }
	 htmp += offset;
      }
      digit = (Ulong)0;
      for (i = nbits; i > 0; i--) {
	 digit <<= 1;
	 if (((negative == 1) && ((*hist)->val == LOW)) ||
		((negative == 0) && ((*hist)->val == HIGH)))
	    digit |= 1;
	 hist += offset;
      }
      if (negative) digit = -(digit + 1);
      sprintf(result, PRINTF_LLONG "d", digit);
      return (result);
    }

    p = result;
    j = nbits % b_digit;
    if( j == 0 )
	j = b_digit;
    for( i = nbits; i > 0; i -= j )
      {
	digit = 0;
	do
	  {
	    switch( (*hist)->val )
	      {
		case LOW :
		    digit = (digit << 1);
		    break;
		case HIGH :
		    digit = (digit << 1) | 1;
		    break;
		case X :
		    digit = 16;
		    while( j != 1 )
		      {
			j--;
			hist += offset;
		      }
		    break;
	      }
	    j--;
	    hist += offset;
	  }
	while( j > 0 );
	*p++ = HexMap[ digit ];
	j = b_digit;
      }
    *p = '\0';
    return( result );
  }
