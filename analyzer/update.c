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
#include "ana_glob.h"


public	int         autoScroll = TRUE;
public	int         updatePending = FALSE;
public	TimeType    updPendTime;
public	int         freezeWindow = FALSE;


public void ScrollUpdate( m )
  MenuItem *m;
  {
    autoScroll ^= TRUE;			/* toggle flag */

    if( autoScroll )
	m->mark = MENU_MARK;
    else
	m->mark = MENU_UNMARK;
  }


public void DisableScroll()
  {
    freezeWindow = TRUE;
  }


public void RestoreScroll()
  {
    freezeWindow = FALSE;
    if( updatePending )
	UpdateWindow( updPendTime );

    updatePending = FALSE;
  }
