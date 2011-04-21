typedef struct History  *phist;

typedef struct History
  {
    phist  next;
    long    time;
    char    *name;
    short   delay;
    short   rtime;
    short   ptime;
    char    val;
    char    type;
  } History;


#define	H_FIRST		0
#define	H_FIRST_INP	1
#define	H_NORM		2
#define	H_NORM_INP	3
#define	H_PUNT		4
#define	H_PEND		5

