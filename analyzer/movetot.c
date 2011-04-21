/***** Copyright (C) 1989 Stanford University, Arturo Salz ****/

#include <stdio.h>
#include <stdlib.h>

#include "ana.h"
#include "ana_glob.h"

public void MoveToTimeValue(TimeType start)
{
    if (start == tims.start)
	return;
    else if (start < tims.first)
	start = tims.first;
    else if (start > tims.last)
	start = tims.last;

    tims.start = start;
    tims.end = start + tims.steps;
    RedrawTimes();
    UpdateScrollBar();
    DrawTraces(start, tims.end);
}

public void MoveToT(char *str)
{
    double   tmp;
    TimeType start;

    if (str == NULL) {
	XBell(display, 0);
	return;
    }
    tmp = atof(str);
    if (tmp < 0.0) tmp = 0.0;
    start = (TimeType) ns2d(tmp);
    MoveToTimeValue(start);
}

public void MoveToTime(char *s)
{
    /* Argument "s" is the menu string; ignore it. */
    Query( "\nEnter Time > ", MoveToT );
}

/*----------------------------------------------------------------------*/
/* Determine if the indicated time (in ps) is valid.  Return 0 if the	*/
/* time is within the simulation period and on-screen.  Return -1 if	*/
/* the time is within the simulation period but outside the analyzer	*/
/* window bounds.  Return -2 if the time is in the future of the	*/
/* simulation, and -3 if the time is invalid (< 0).			*/
/*----------------------------------------------------------------------*/

public int ValidTime(TimeType t)
{
   if (t < tims.start)
	return -3;
   else if (t > tims.end)
	return -2;
   else if ((t < tims.first) || (t > tims.last))
	return -1;
   else
	return 0;
}
