#include <stdio.h>
#include "defs.h"
#include "net.h"
#include "globals.h"


public  hptr   last_hist;		/* pointer to dummy hist-entry that
					 * serves as tail for all nodes */
public	Ulong	max_time;


public void init_hist()
  {
    static HistEnt  dummy;
    static HistEnt  dummy_model;

    max_time = MAX_TIME;

    last_hist = &dummy;
    dummy.next = last_hist;
    dummy.time = max_time;
    dummy.val = X;
    dummy.inp = 1;
    dummy.punt = 0;
    dummy.t.r.delay = dummy.t.r.rtime = 0;

#ifdef notdef
    dummy_model.time = 0;
    dummy_model.val = model;
    dummy_model.inp = 0;
    dummy_model.punt = 0;
    dummy_model.next = NULL;
    first_model = curr_model = &dummy_model;
#endif
  }
