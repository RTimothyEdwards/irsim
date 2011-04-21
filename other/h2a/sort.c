#include "history.h"


#define	BIT8		0xff
#define	BIT12		0xfff
#define	BIT24		0xffffff

#define	CNTSIZE		( 4096 + 4096 + 256 )
#define	COUNT1		&countBuff[ 0 ]
#define	COUNT2		&countBuff[ 4096 ]
#define	COUNT3		&countBuff[ 4096 + 4096 ]
#define	ENDCNT		&countBuff[ CNTSIZE ]


static	int	*countBuff;
static  phist	*data_;
static	phist	*dataBuff;
static	phist	*dataBuff1;


/*
 * Sort the edge array pointed to by 'lineBuff'.  Return a pointer to a
 * sorted array of pointers into the original data.
 */
phist *Sort( nlines, lineBuff, maxv )
  int    nlines;
  phist  *lineBuff;
  long   maxv;
  {
    phist  *tmp;

    data_ = lineBuff;
    dataBuff = tmp = (phist *) malloc( sizeof( phist ) * nlines );
    countBuff = (int *) malloc( sizeof( int ) * CNTSIZE );
    dataBuff1 = (phist *) malloc( sizeof( phist ) * nlines );
    BucketSort( nlines, maxv );
    if( dataBuff != tmp )			/* swapped by sort */
      {
	register phist	*src, *dst, *end;
	
	dataBuff1 = dataBuff;
	dataBuff = tmp;
	src = dataBuff1;
	dst = dataBuff;
	end = &dataBuff1[ nlines ];
	do { *dst++ = *src++; } while( src < end );
      }
    return( dataBuff );
  }


static BucketSort( nlines, maxv )
  int   nlines;
  long  maxv;
  {
   {
    register int	*cp, *end;

    cp = COUNT1;
    end = ENDCNT;
    do { *cp++ = 0; } while( cp < end );
   }

   {
    register phist	*dat, *end;
    register int	*count1, *count2, *count3;

    count1 = COUNT1;
    count2 = COUNT2;
    count3 = COUNT3;
    dat = data_;
    end = &data_[ nlines ];

    if( maxv <= BIT12 )
      {
	do
	  {
	    count1[ (*dat)->time & BIT12 ]++;
	    dat++;
	  }
	while( dat < end );
      }
    else if( maxv <= BIT24 )
      {
	do
	  {
	    count1[ (*dat)->time & BIT12 ]++;
	    count2[ ( (*dat)->time >> 12 ) & BIT12 ]++;
	    dat++;
	  }
	while( dat < end );
      }
    else
      {
	do
	  {
	    count1[ (*dat)->time & BIT12 ]++;
	    count2[ ( (*dat)->time >> 12 ) & BIT12 ]++;
	    count3[ ( (*dat)->time >> 24 ) & BIT8 ]++;
	    dat++;
	  }
	while( dat < end );
      }
   }

   {
    register int	*cp, *cp_1;
    register int	*cp2, *cp2_1;
    register int	*end;
    cp = COUNT1;
    cp_1 = &cp[1];
    cp2 = COUNT2;
    cp2_1 = &cp2[1];
    end = COUNT2;
    do
      {
	*cp_1++ += *cp++;
	*cp2_1++ += *cp2++;
      }
    while( cp_1 < end );
    
    cp = COUNT3;
    cp_1 = &cp[1];
    end = ENDCNT;
    do { *cp_1++ += *cp++; } while( cp_1 < end );
   }

   {
    register int	tmp;
    register int	*count;
    register phist	*dat;
    register phist	*datPtr, *end;
    
    dat = &data_[ nlines-1 ];
    count = COUNT1;
    datPtr = dataBuff;
    do
      {
	tmp = --count[ (*dat)->time & BIT12 ];
	datPtr[ tmp ] = (*dat);
	dat--;
      }
    while( dat >= data_ );
    
    if( maxv <= BIT12 )
	goto done;

    datPtr = &dataBuff[ nlines-1 ];
    end = dataBuff;
    count = COUNT2;
    do
      {
	tmp = -- count[ ((*datPtr)->time >> 12) & BIT12 ];
	dataBuff1[ tmp ] = *datPtr;
	datPtr--;
      }
    while( datPtr >= end );

    if( maxv <= BIT24 )
      {
	datPtr = dataBuff1;
	dataBuff1 = dataBuff;
	dataBuff = datPtr;
	goto done;
      }

    datPtr = &dataBuff1[ nlines-1 ];
    end = dataBuff1;
    count = COUNT3;
    do
      {
	tmp = -- count[ ((*datPtr)->time >> 24) & BIT8 ];
	dataBuff[ tmp ] = *datPtr;
	datPtr--;
      }
    while( datPtr >= end );

   }

  done : ;
  }
