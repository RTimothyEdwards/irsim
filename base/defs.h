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

#ifndef _DEFS_H
#define _DEFS_H

	/* Global definitions used by all modules */

#define	or	||
#define	and	&&
#define	not	!

#define	private	static
#define	public

#define SWAP( TYPE, A, B )		\
  {					\
    register TYPE TMP;			\
					\
    TMP = (A);				\
    (A) = (B);				\
    (B) = TMP;				\
  }					\

#ifdef SYS_V
 #ifndef bcopy
  #define	bcopy( A, B, C )	memcpy( B, A, C )
 #endif
 #ifndef bcmp
  #define	bcmp( A, B, C )		memcmp( B, A, C )
 #endif
#endif

#endif /* _DEFS_H */
