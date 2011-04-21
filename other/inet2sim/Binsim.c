#include "binsim.c"

public void GetInetParms( tlist, lambda )
  tptr  *tlist;
  long  *lambda;
  {
    *tlist = rd_tlist;
    *lambda = inet_lambda;
  }
