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
#include "defs.h"
#include "net.h"

public	long    INC_RES = 5;
public	nptr    inc_cause = (nptr) 1;
public	long    nevals = 0;
public	long    i_nevals = 0;
public	long    nreval_ev = 0;
public	long    npunted_ev = 0;
public	long    nstimuli_ev = 0;
public	long    ncheckpt_ev = 0;
public	long    ndelaychk_ev = 0;
public	long    ndelay_ev = 0;

public void incsim( ch_list )
  nptr  ch_list;
  {
    lprintf( stderr, "Incremental simulation not supported in this version\n" );
  }

#ifdef FAULT_SIM

public	int	fault_mode = FALSE;

public void faultsim( n )
  nptr  n;
  {
    lprintf( stderr, "Fault simulation not supported in this version\n" );
  }
#endif /* FAULT_SIM */
