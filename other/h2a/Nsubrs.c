#define Index2node	INDEX2NODE

#include "nsubrs.c"

#undef Index2node

nptr Index2node( index )
  Ulong  index;
  {
    nptr  n = INDEX2NODE( index );
    if( n == NULL )
      {
	static nptr    node = NULL;
	static char    buff[30];
	register Uint  ma, mi;

	ma = index & ((1 << NBIT_HASH) - 1);
	mi = index >> NBIT_HASH;

	(void) sprintf( buff, "<%d,%d>", ma, mi );

	if( node == NULL )
	    node = GetNewNode( "?" );
	if( do_sort )
	  {
	    node->nname = Valloc( strlen( buff ) + 1, 1 );
	    (void) strcpy( node->nname, buff );
	  }
	else
	    node->nname = buff;
	n = node;
      }
    return( n );
  }
