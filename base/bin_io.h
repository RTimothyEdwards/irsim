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
 * Macros to read/write machine-independent binary files.
 */


/*
 * Extract NBYTES from byte stream BUFF and place result in NUM.
 * Restriction is 1 <= NBYTES <= 4.  (Force Unrolling of loops).
 */
#define UnpackBytes( BUFF, NUM, NBYTES )			\
  {								\
    register unsigned char  *B_S = (unsigned char *) (BUFF);	\
    register unsigned long  RES = 0;				\
								\
    if( NBYTES == 1 )						\
	RES = *B_S;						\
    else if( NBYTES == 2 )					\
      {								\
	RES = B_S[0];						\
	RES += ( ((unsigned int) B_S[1]) << 8 ) & 0x0ff00;	\
      }								\
    else if( NBYTES == 3 )					\
      {								\
	RES = B_S[0];						\
	RES += ( ((unsigned int) B_S[1]) << 8 ) & 0x0ff00;	\
	RES += ( ((unsigned int) B_S[2]) << 16 ) & 0x0ff0000;	\
      }								\
    else if( NBYTES == 4 )					\
      {								\
	RES = B_S[0];						\
	RES += ( ((unsigned int) B_S[1]) << 8 ) & 0x0ff00;	\
	RES += ( ((unsigned int) B_S[2]) << 16 ) & 0x0ff0000;	\
	RES += ( ((unsigned int) B_S[3]) << 24 ) & 0xff000000;	\
      }								\
    NUM = RES;							\
  }								\


/*
 * Pack NBYTES from NUM into byte stream pointed to by BUFF.
 * Restriction is 1 <= NBYTES <= 4.  (Force Unrolling of loops).
 */
#define	PackBytes( BUFF, NUM, NBYTES )				\
  {								\
    register unsigned long  B_T = (NUM);			\
    register unsigned char  *B_S = (unsigned char *) (BUFF);	\
								\
    if( NBYTES > 3 )						\
      {								\
	*B_S++ = B_T & 0x0ff;					\
	B_T >>= 8;						\
      }								\
    if( NBYTES > 2 )						\
      {								\
	*B_S++ = B_T & 0x0ff;					\
	B_T >>= 8;						\
      }								\
    if( NBYTES > 1 )						\
      {								\
	*B_S++ = B_T & 0x0ff;					\
	B_T >>= 8;						\
      }								\
    *B_S = B_T & 0x0ff;						\
  }								\

