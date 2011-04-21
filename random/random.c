/*--------------------------------------------------------------*/
/* random ---							*/
/*								*/
/*	Loadable package to support pseudorandom manipulations	*/
/* 	in Tcl.  Based on the C stdlib "rand48" routines (48-	*/
/*	bit integer arithmetic).				*/
/*								*/
/* No known copyright --- this source code is apparently	*/
/* freeware.							*/
/*								*/
/* Integrated into the Tcl-based IRSIM simulator March 2003,	*/
/* R. Timothy Edwards, Open Circuit Design, Inc., for MultiGiG	*/
/* Ltd.								*/
/*--------------------------------------------------------------*/

/*
 *---------------------------------------------------------------
 * Return a pseudorandom number.			
 *
 *   Usage:   random [option]
 *
 *   By default (no option), the result is a floating-point value
 *   taken uniformly from the range [0..1)
 *
 *   Options:
 *
 *   -reset
 *	Reseed the generator using a combination of current pid
 *	and current time.
 *
 *   -seed n
 *	Reseed the generator with the integer value n.
 *
 *   -integer ...
 *	Round the value returned down to the largest integer less
 *	than or equal to the number which would otherwise be
 *	returned.
 *
 *   -bitstream n
 *	Return a uniformly-distributed random bitstream length n.
 *
 *   -normal m s
 *	Return a number taken from a gaussian distrubution with
 *	mean m and standard deviation s.
 *
 *   -exponential m
 *	Return a number taken from an exponential distribution
 *	with mean m.
 *
 *   -uniform low high
 *	Return a number taken from uniform distribution on
 *	[low,high).
 *
 *   -chi2 n
 *	Return a number taken from chi2 distribution with n
 *	degrees of freedom.
 *
 *   -select n list
 *	Select n elements at random from the list "list", with
 *	replacement.
 *
 *   -choose n list
 *	Select n elements at random from the list "list", without
 *	replacement.
 *
 *   -permutation n
 *	If n is a number, return a permutation of 0..n - 1.
 *	If n is a list, return a permutation of its elements.
 *
 *---------------------------------------------------------------
 */
   
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/time.h>

#include "../base/loctypes.h"

#ifdef TCL_IRSIM

#include <tcl.h>

/*--------------------------------------------------------------*/
/* return a sample from a unit normal distribution 		*/
/*--------------------------------------------------------------*/

double rand_gauss_dev()
{
    static int iset=0;
    static double gset;
    double fac, r, v1, v2;
    double drand48();

    if  (iset == 0) {
	/* sample from a circular disk by using the rejection method */
	do {
	    v1=2.0*drand48()-1.0;
	    v2=2.0*drand48()-1.0;
	    r=v1*v1+v2*v2;
	} while (r >= 1.0);

	/* now generate a radial variation */
	fac=sqrt(-2.0*log(r)/r);

	/* store the next return value */
	gset=v1*fac;
	iset=1;

	/* and return the current value */
	return v2*fac;
    } else {
	/* now return the old stored value */
	iset=0;
	return gset;
    }
}

/*--------------------------------------------------------------*/
/* Return a sample from a Chi^2 distribution			*/
/*--------------------------------------------------------------*/

double rand_chi2_dev(int dof)
{
    int i;
    double r;
    double drand48();

    /* summing dof/2 exponentially distributed deviates costs less than
       summing dof squared normal deviates both in terms of number of
       random bits needed and in terms of number of log calls needed */

    r = 0;
    for (i=1;i<=dof/2;i++) {
	r += log(1-drand48());
    }
    r = -2 * r;

    /* now if there was an extra, we have to add in a squared normal */

    if (dof & 1) {
	double x;
	x = rand_gauss_dev();
	r += x*x;
    }
    return r;
}

/*--------------------------------------------------------------*/
/* Main Tcl command callback function for Tcl command "random"	*/
/*--------------------------------------------------------------*/

int do_random(ClientData cl, Tcl_Interp *interp, int argc, char *argv[])
{
    int i, j;

    char r[TCL_DOUBLE_SPACE];
    pid_t getpid();
    double drand48();
    long lrand48();
    void srand48();

    if (argc == 1) {
	Tcl_PrintDouble(interp, drand48(), r);
	Tcl_SetResult(interp, r, TCL_VOLATILE);
    }
    else if (strcmp(argv[1], "-integer") == 0) {
	int a, b;
	a = 0x80000000;	/* Minimum (maximum negative) */
	b = 0x7fffffff;	/* Maximum */
	if (argc == 3) {
	    if (Tcl_GetInt(interp, argv[2], &b) != TCL_OK)
		return TCL_ERROR;
	    a = -b;
	}
	else if (argc == 4) {
	    if (Tcl_GetInt(interp, argv[2], &a) != TCL_OK)
		return TCL_ERROR;
	    if (Tcl_GetInt(interp, argv[3], &b) != TCL_OK)
		return TCL_ERROR;
	}
	sprintf(r, "%.0f", a + (b-a)*drand48());
	Tcl_SetResult(interp, r, TCL_VOLATILE);
    }

    else if (strncmp(argv[1], "-bit", 4) == 0) {
	int a;
	long x;
	char *rstr;
	a = 32;
	if (argc == 3) {
	    if (Tcl_GetInt(interp, argv[2], &a) != TCL_OK)
		return TCL_ERROR;
	}
	rstr = malloc(a + 1);
	for (i = 0; i < a; i += 32) {
	   x = lrand48(); 
	   for (j = 0; j < 32 && (i + j < a); j++) {
	      rstr[i + j] = (x & 0x1) ? '1' : '0';
	      x >>= 1;
	   }
	}
	rstr[a] = '\0';
	Tcl_SetResult(interp, rstr, TCL_VOLATILE);
	free(rstr);
    }

    else if (strcmp(argv[1], "-reset") == 0) {
	srand48(time(0) + getpid());
	r[0] = 0;
	Tcl_SetResult(interp, r, TCL_VOLATILE);
    }
    else if (strcmp(argv[1], "-seed") == 0) {
	int seed = 12345;;
	if (argc == 3) {
	    if (Tcl_GetInt(interp, argv[2], &seed) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	srand48((long int)seed);
	r[0] = 0;
	Tcl_SetResult(interp, r, TCL_VOLATILE);
    }
    else if (strcmp(argv[1], "-normal") == 0) {
	double m, s;
	m = s = 1;
	if (argc == 3 || argc == 4) {
	    if (Tcl_GetDouble(interp, argv[2], &m) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	if (argc == 4) {
	    if (Tcl_GetDouble(interp, argv[3], &s) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	Tcl_PrintDouble(interp, m + s*rand_gauss_dev(m, s), r);
	Tcl_SetResult(interp, r, TCL_VOLATILE);
    }
    else if (strcmp(argv[1], "-exponential") == 0) {
	double m;
	m = 1;
	if (argc > 2) {
	    if (Tcl_GetDouble(interp, argv[2], &m) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	Tcl_PrintDouble(interp, -m*log(1-drand48()), r);
	Tcl_SetResult(interp, r, TCL_VOLATILE);
    }
    else if (strcmp(argv[1], "-uniform") == 0) {
	double a, b;

	a = 0;
	b = 1;
	if (argc > 2) {
	    if (Tcl_GetDouble(interp, argv[2], &a) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	if (argc > 3) {
	    if (Tcl_GetDouble(interp, argv[3], &b) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	Tcl_PrintDouble(interp, a + (b-a)*drand48(), r);
	Tcl_SetResult(interp, r, TCL_VOLATILE);
    }
    else if (strcmp(argv[1], "-chi2") == 0) {
	double dof;

	dof = 1;
	if (argc > 2) {
	    if (Tcl_GetDouble(interp, argv[2], &dof) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	Tcl_PrintDouble(interp, rand_chi2_dev(dof), r);
	Tcl_SetResult(interp, r, TCL_VOLATILE);
    }
    else if (strcmp(argv[1], "-select") == 0) {
	int n;
	if (argc != 4) {
	    Tcl_SetResult(interp, "must have a count and a list for random -select",
			  TCL_STATIC);
	    return TCL_ERROR;
	}
	if (Tcl_GetInt(interp, argv[2], &n) != TCL_OK) {
	    return TCL_ERROR;
	}
	else {
	    int list_count;
	    char **list;

	    if (Tcl_SplitList(interp, argv[3], &list_count,
			(CONST char ***)&list) != TCL_OK) {
		return TCL_ERROR;
	    }
	    for (i=0;i<n;i++) {
		Tcl_AppendElement(interp, list[(int) (list_count*drand48())]);
	    }
	    free(list);
	}
    }
    else if (strcmp(argv[1], "-permute") == 0) {
	int list_count;
	char *t;
	char **list;

	if (argc != 3) {
	    Tcl_SetResult(interp, "must have a list for random -permute",
			  TCL_STATIC);
	    return TCL_ERROR;
	}

	if (Tcl_SplitList(interp, argv[2], &list_count,
			(CONST char ***)&list) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (i=1;i<list_count;i++) {
	    j = (i+1) * drand48();
	    if (i != j) {
		t = list[j];
		list[j] = list[i];
		list[i] = t;
	    }
	}
	for (i=0;i<list_count;i++) {
	    Tcl_AppendElement(interp, list[i]);
	}
	free(list);
    }
    else if (strcmp(argv[1], "-permutation") == 0) {
	int n;

	if (argc != 3) {
	    Tcl_SetResult(interp, "must have a count for random -permutation",
			  TCL_STATIC);
	    return TCL_ERROR;
	}

	if (Tcl_GetInt(interp, argv[2], &n) != TCL_OK) {
	    return TCL_ERROR;
	}
	else {
	    int j, t, *p;
	    p = (int *)ckalloc(n * sizeof(p[0]));
	    for (i=0;i<n;i++) {
		p[i] = i;
	    }
	    for (i=1;i<n;i++) {
		j = (i+1) * drand48();
		if (i != j) {
		    t = p[j];
		    p[j] = p[i];
		    p[i] = t;
		}
	    }
	    for (i=0;i<n;i++) {
		sprintf(r, "%d", p[i]);
		Tcl_AppendElement(interp, r);
	    }
	    free(p);
	}
    }
    else if (strcmp(argv[1], "-choose") == 0) {
	int n;
	if (argc != 4) {
	    Tcl_SetResult(interp, "must have a count and a list for random -choose",
			  TCL_STATIC);
	    return TCL_ERROR;
	}
	if (Tcl_GetInt(interp, argv[2], &n) != TCL_OK) {
	    return TCL_ERROR;
	}
	else {
	    int list_count;
	    char *t;
	    char **list;

	    if (Tcl_SplitList(interp, argv[3], &list_count,
			(CONST char ***)&list) != TCL_OK) {
		return TCL_ERROR;
	    }
	    for (i=0;i<n;i++) {
		j = (n-i) * drand48() + i;
		if (i != j) {
		    t = list[j];
		    list[j] = list[i];
		    list[i] = t;
		}
	    }
	    for (i=0;i<n;i++) {
		Tcl_AppendElement(interp, list[i]);
	    }
	    free(list);
	}
    }
    else {
	Tcl_SetResult(interp, "bad arguments for random", TCL_STATIC);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*--------------------------------------------------------------*/
/* Tcl Package initialization function for "random"		*/
/*--------------------------------------------------------------*/

int Random_Init(Tcl_Interp *interp)
{
    Tcl_CreateCommand(interp, "random", (Tcl_CmdProc *)do_random, NULL, NULL);

    /* Seed the generator with something, or else the first value	*/
    /* produced will always be zero.					*/
    srand48((long int)1023);

    return TCL_OK;
}

#endif /* TCL_IRSIM */
