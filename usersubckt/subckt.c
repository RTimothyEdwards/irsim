#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef TCL_IRSIM
#include <tk.h>
#endif

#ifndef _DEFS_H
#include "defs.h"
#endif
#ifndef _GLOBALS_H
#include "globals.h"
#endif
#ifndef _NET_MACROS_H
#include "net_macros.h"
#endif
#ifndef _SUBCKT_H
#include "subckt.h"
#endif

#ifdef USER_SUBCKT 

typedef struct _Sub
{
    userSubCircuit *subckt;	/* subcircuit (possibly static) */
    int		   inst;	/* Number of instantiations */
#ifndef TCL_IRSIM
    struct _Sub	   *next;	/* list of commands in bucket */
#endif
} SubCircuit;

extern userSubCircuit subs[];
extern tptr rd_tlist;

#ifdef TCL_IRSIM

extern Tcl_Interp *irsiminterp;

/*----------------------------------------------*/
/* Tcl procedure characters representing states	*/
/*----------------------------------------------*/

static pot2ch[] = {'0', 'x', 'z', '1'};

/*------------------------------*/
/* Method 1:			*/
/* Make use of Tcl Hash tables	*/
/*------------------------------*/

Tcl_HashTable substbl;

/* First add any C-compiled user subcircuits */

public void init_subs(userSubCircuit *subckts)
{
    userSubCircuit *s;
    SubCircuit *sl;
    Tcl_HashEntry *he;
    int newptr;

    Tcl_InitHashTable(&substbl, TCL_STRING_KEYS);

    for (s = subckts; s->name != NULL; s++)
    {
	sl = (SubCircuit *)malloc(sizeof(SubCircuit));
	sl->subckt = s;
	sl->inst = 0 ;

	he = Tcl_CreateHashEntry(&substbl, s->name, &newptr);
	Tcl_SetHashValue(he, (ClientData)sl);
    }
}

/* Instantiate a user subcircuit */

public userSubCircuit *subckt_instantiate(char *sname, int *inst, uptr *udatap)
{
    int newptr, result;
    SubCircuit *sl;
    Tcl_HashEntry *he;
    Tcl_Obj *objv[1];

    *udatap = NULL;	/* placeholder, used only for procedures */

    he = Tcl_CreateHashEntry(&substbl, sname, &newptr);
    sl = (SubCircuit *)Tcl_GetHashValue(he);

    if (sl != NULL) {
	sl->inst++;
	*inst = sl->inst;
	return sl->subckt;
    }

    /* We allow subcircuits to be defined as Tcl procedures.		*/
    /* Two procedures need to be defined, given the name of the		*/
    /* subcircuit plus "_eval" and "_init", respectively.  The "_init"	*/
    /* procedure must return a list containing:  number of inputs,	*/
    /* number of outputs, list of output pullup and pulldown		*/
    /* resistances, and an object (Tcl_Obj *) containing any		*/
    /* information required by the instance.				*/

    objv[0] = Tcl_NewStringObj(sname, -1);
    Tcl_AppendToObj(objv[0], "_init", 5);
    result = Tcl_EvalObjv(irsiminterp, 1, objv, 0);
    if (result == TCL_OK)
    {
	int i, listlen, noutputs, ninputs;
	double dval;
	Tcl_Obj *elem, *rlist, *olist, *uobj;
	userSubCircuit *s;

	rlist = Tcl_GetObjResult(irsiminterp);
	result = Tcl_ListObjLength(irsiminterp, rlist, &listlen);
	if (result == TCL_OK && (listlen == 3 || listlen == 4))
	{
	    if (listlen == 4)
	        result = Tcl_ListObjIndex(irsiminterp, rlist, 3, &uobj);
	    else
		uobj = Tcl_NewListObj(0, NULL);
	    *udatap = (uptr)uobj;
	    Tcl_IncrRefCount(uobj);
	
	    result = Tcl_ListObjIndex(irsiminterp, rlist, 0, &elem);
	    result = Tcl_GetIntFromObj(irsiminterp, elem, &ninputs);
	    if (result != TCL_OK) return NULL;
	    result = Tcl_ListObjIndex(irsiminterp, rlist, 1, &elem);
	    result = Tcl_GetIntFromObj(irsiminterp, elem, &noutputs);
	    if (result != TCL_OK) return NULL;
	    result = Tcl_ListObjIndex(irsiminterp, rlist, 2, &olist);
	    result = Tcl_ListObjLength(irsiminterp, olist, &listlen);
	    if (listlen != (noutputs * 2)) return NULL;

	    /* Set up a new hash table record for this subcircuit */

	    sl = (SubCircuit *)malloc(sizeof(SubCircuit));
	    s = (userSubCircuit *)malloc(sizeof(userSubCircuit));
	    sl->inst = 1;
	    sl->subckt = s;
	    Tcl_SetHashValue(he, (ClientData)sl);
	    s->name = strdup(sname);
	    s->model = NULL;
	    s->init = NULL;
	    s->ninputs = ninputs;
	    s->noutputs = noutputs;
	    s->res = (float *)malloc(2 * noutputs * sizeof(float));

	    for (i = 0; i < (noutputs * 2); i++)
	    {
		double dres;
		result = Tcl_ListObjIndex(irsiminterp, olist, i, &elem);
		result = Tcl_GetDoubleFromObj(irsiminterp, elem, &dres);
		if (dres <= 0.001) {
		   dres = 500.0;
		   lprintf(stderr, "\tError: bad resistance %g\n", dres);
		}
		s->res[i] = (result == TCL_OK) ? dres : 500.0;
	    }
	    Tcl_ResetResult(irsiminterp);
	    *inst = sl->inst;
	    return s;
	}
    }
    return NULL;
}

#else

/*------------------------------*/
/* Method 2:			*/
/* Simple fixed-size hash table */
/*------------------------------*/

#define SUBSTBLSIZE	16

private SubCircuit *substbl[SUBSTBLSIZE];

private int HashSub();

public void init_subs(userSubCircuit *subckts)
{
    register int n;
    register userSubCircuit *s;
    register SubCircuit *sl;

    for (n=0; n < SUBSTBLSIZE; n++)
	substbl[n] = NULL;
    for (s = subckts; s->name != NULL; s++) {
	sl = (SubCircuit *)malloc(sizeof(SubCircuit));
	sl->subckt = s;
	sl->inst = 0;
	n = HashSub(s->name);
	sl->next = substbl[n];
	substbl[n] = sl;
    }
}

private int HashSub(char *name)
{
    register int  hashcode = 0;

    do
	hashcode = (hashcode << 1) ^ (*name | 0x20);
    while (*(++name));
    return (((hashcode >= 0) ? hashcode : ~hashcode) % SUBSTBLSIZE);
}

public userSubCircuit *subckt_instantiate(char *sname, int *inst, uptr *udatap)
{
    register int n;
    register SubCircuit *s;

    *udatap = NULL;		/* placeholder, used by Tcl version only */
    n = HashSub(sname);
    
    for (s = substbl[n]; s != NULL; s = s->next) {
	if (str_eql(s->subckt->name, sname) == 0) {
	    s->inst++;
	    *inst = s->inst;
	    return s->subckt;
	}
    }
    return NULL;
}

#endif	/* !TCL_IRSIM */
	
/*
 *------------------------------------------------------------------
 * Evalutate a subcircuit model
 *------------------------------------------------------------------
 */

public void subckt_model_C(tptr t)
{
    int i;
    SubcktT *subptr = (SubcktT *)(t->subptr);
    short nins = subptr->subckt->ninputs;
    short nouts = subptr->subckt->noutputs;
    nptr *nodes = subptr->nodes;
    uptr udata = subptr->udata;
#ifdef TCL_IRSIM
    int result;
#endif
    
    char *in, *out = NULL;
    double *delay;

    in  = (char *) malloc((nins + 1) * sizeof(char));
    out = (char *) malloc((nouts + 1) * sizeof(char));
    delay = (double *) malloc(nouts * sizeof(double));

    for (i = 0; i < nins; i++)
	in[i] = nodes[i]->npot;
    for (i = 0; i < nouts; i++)
	out[i] = nodes[i+nins]->npot;

    if (subptr->subckt->model != NULL)
	(*subptr->subckt->model)(in, out, delay, udata);

#ifdef TCL_IRSIM
    else
    {
	Tcl_Obj *objv[4];

	/* The Tcl version of IRSIM allows subcircuit to be defined as	*/
	/* Tcl procedures.  This procedure must return a list whose	*/
	/* first element is a string of all the output values, followed	*/
	/* by time delays to all of the outputs.			*/

	for (i = 0; i < nins; i++)
	    in[i] = pot2ch[in[i]];
	in[nins] = '\0';
	for (i = 0; i < nouts; i++)
	    out[i] = pot2ch[out[i]];
	out[nouts] = '\0';

	objv[0] = Tcl_NewStringObj(subptr->subckt->name, -1);
	Tcl_AppendToObj(objv[0], "_eval", 5);
	objv[1] = Tcl_NewStringObj(in, nins);
	objv[2] = Tcl_NewStringObj(out, nouts);
	objv[3] = (Tcl_Obj *)udata;
	result = Tcl_EvalObjv(irsiminterp, 4, objv, 0);

	if (result == TCL_OK)
	{
	    int listlen;
	    double dval;
	    char *newoutvals;
	    Tcl_Obj *elem, *rlist;

	    rlist = Tcl_GetObjResult(irsiminterp);
	    result = Tcl_ListObjLength(irsiminterp, rlist, &listlen);
	    if (result == TCL_OK && listlen == 1 + nouts)
	    {
		result = Tcl_ListObjIndex(irsiminterp, rlist, 0, &elem);
		newoutvals = Tcl_GetString(elem);
		if (strlen(newoutvals) == nouts)
		{
		    for (i = 0; i < nouts; i++)
		    {
			result = Tcl_ListObjIndex(irsiminterp, rlist, i + 1, &elem);
			result = Tcl_GetDoubleFromObj(irsiminterp, elem, &delay[i]);
			switch (tolower(newoutvals[i]))
			{
			    case '0': case 'l':
				out[i] = LOW;
				break;
			    case '1': case 'h':
				out[i] = HIGH;
				break;
			    case 'z':
				out[i] = HIGH_Z;
				break;
			    default:
				out[i] = X;
				break;
			}
		    }

		    /* Clear the interpreter result */
		    Tcl_ResetResult(irsiminterp);
		}
		else
		{
		    Tcl_SetResult(irsiminterp, "Subcircuit result does not match"
				" the number of defined outputs", 0);
		    nouts = 0;
		}
	    }
	    else
	    {
		if (result == TCL_OK)
		    Tcl_SetResult(irsiminterp, "Subcircuit evaluator did not"
				 " return the correct size list", 0);
		nouts = 0;
	    }
	}
	else nouts = 0;
    }
#endif

    for (i = 0; i < nouts; i++ ) {
	switch (out[i]) {
	    case LOW:
	    	QueueFVal(nodes[i+nins], HIGH, (double) 1.0, delay[i]);
	    	QueueFVal(nodes[i+nins+nouts], HIGH, (double) 1.0, delay[i]);
		break;
	    case X:
	    	QueueFVal(nodes[i+nins], X, (double) 1.0, delay[i]);
	    	QueueFVal(nodes[i+nins+nouts], X, (double) 1.0, delay[i]);
		break;
	    case HIGH_Z:
	    	QueueFVal(nodes[i+nins], HIGH, (double) 1.0, delay[i]);
	    	QueueFVal(nodes[i+nins+nouts], LOW, (double) 1.0, delay[i]);
		break;
	    case HIGH:
	    	QueueFVal(nodes[i+nins], LOW, (double) 1.0, delay[i]);
	    	QueueFVal(nodes[i+nins+nouts], LOW, (double) 1.0, delay[i]);
		break;
	}
    }

    free(in); 
    free(out); 
    free(delay);
}

/*
 *------------------------------------------------------------------
 * Create a new subcircuit from the .sim file line
 * "x <inputs> <outputs> [<parameters>] <subcircuit_name>"
 *
 * The subcircuit definition is assumed to be already known;
 * otherwise, this routine returns an error result (-1).
 * Return 0 on success.
 *------------------------------------------------------------------
 */

public int newsubckt(targc, targv)
   int  targc;
   char *targv[];
{
    userSubCircuit *subcircuit;
    SubcktT *subptr, *subptr2;
    int i, n, ninputs, noutputs;
    float rtf;
    ufun init;
    register tptr t = 0;
    tptr t2; lptr l;
    char *sname, *out_name_Ub, *out_name_D;
    int subckt_out_counter = 0;
    nptr *nodes;
    lptr diodes = (lptr)NULL, d;
    uptr usrData = NULL;
    int inst, result = 0;

    subcircuit = subckt_instantiate(targv[targc - 1], &inst, &usrData);
    if (subcircuit == NULL) {
	lprintf(stderr, "\tError: subcircuit \"%s\" is not defined!\n",
			targv[targc - 1]);
	return -1;
    }

    ninputs = subcircuit->ninputs;
    noutputs = subcircuit->noutputs;
    sname = subcircuit->name;

    if (targc < ninputs + noutputs + 2) {
	lprintf(stderr, "\tError: wrong # args %d\n", targc);
	lprintf(stderr, "\tsubcircuit %s has %d input nodes, %d output nodes\n",
		subcircuit->name, ninputs, noutputs);
	return -1;
    } 

    out_name_Ub = (char *)malloc(strlen(targv[targc-1]) + 20);
    out_name_D  = (char *)malloc(strlen(targv[targc-1]) + 20);

    targc--;
    targv++;
    nodes = (nptr *)malloc((ninputs + 2 * noutputs) * sizeof(nptr));
      
    lprintf(stdout, "defining new subcircuit \"%s\" instance %d #i:%d #o:%d\n",
		subcircuit->name, inst, ninputs, noutputs);

    /* If the functions are compiled from C code, the C code init	*/
    /* function is responsible for interpreting any parameters given to	*/
    /* the device instance in the .sim file.				*/

    if (subcircuit->init)
       	usrData = (uptr)(*subcircuit->init)(targc - ninputs - noutputs,
			targv + ninputs + noutputs);
#ifdef TCL_IRSIM
    else if (targc >= ninputs + noutputs + 2) {

	/* Extra user data list has been specified in the .sim	*/
	/* file.  Append this to the current user data object.	*/
	/* We assume that the evaluator procedure knows how to	*/
	/* deal with the extra data.				*/

	Tcl_Obj *sobj, *uobj = (Tcl_Obj *)usrData;

	if (uobj == NULL) {
	    uobj = Tcl_NewListObj(0, NULL);
	    Tcl_IncrRefCount(uobj);
	}
	for (i = ninputs + noutputs + 1; i < targc; i++)
	{
	    sobj = Tcl_NewStringObj(targv[i], -1);
	    Tcl_ListObjAppendElement(irsiminterp, uobj, sobj);
	}
    }
#endif
	    
    for (i = 0; i < ninputs; i++) {	/* create new "transistor" */
	NEW_TRANS(t);		
	NEW_SUBCKT(subptr);
	t->subptr = (char *)subptr;
	t->ttype = SUBCKT; 
	t->gate = RsimGetNode(targv[i]);
	subptr->nodes = nodes; 
	t->source = VDD_node; 
	t->drain = t->gate;

	/* link it to the list */
	t->scache.t = rd_tlist; rd_tlist = t; 
	t->r = requiv(RESIST, (int)2, (int)(1000000 * LAMBDACM));
	subptr->subckt = subcircuit;

	/* All this can be referenced by a pointer to a subckt structure */
	NEW_LINK(d); d->xtor = t; d->next = diodes;
	subptr->ndiode = d; diodes = d;
	nodes[i] = t->gate;
	subptr->udata = usrData;
    } 
    for (l = subptr->ndiode; l != NULL; l = l->next) {
	t2 = l->xtor;
	subptr2 = (SubcktT *)(t2->subptr);
	subptr2->ndiode = diodes;
    }

    n = 0;
    for (; i < ninputs + noutputs; i++) {

	/* define two transistors for each output : */

	sprintf(out_name_Ub, "%sUb_%d_%d", sname, inst, subckt_out_counter);
	sprintf(out_name_D , "%sD_%d_%d", sname, inst, subckt_out_counter++);
	if (find(out_name_Ub) || find(out_name_D)) {
	    lprintf(stderr,"Error: nodes named %s or %s already exist\n",
			out_name_Ub, out_name_D);
	    result = -1;
	    goto donesubckt;
	}

	NEW_TRANS(t);	/* Add pmos output driver */
	t->ttype    = PCHAN;
	t->gate     = RsimGetNode(out_name_Ub);
	nodes[i]    = t->gate;
	t->drain    = VDD_node;
	t->source   = RsimGetNode(targv[i]);
	t->scache.t = rd_tlist; /* link it to the list */
	rd_tlist    = t;
	rtf 	    = subcircuit->res[n++];
	t->r        = requiv(RESIST, (int)2, (int)(rtf * LAMBDACM));

	NEW_TRANS(t);	/* Add nmos output driver */
	t->ttype    = NCHAN;
	t->gate     = RsimGetNode(out_name_D);
	t->drain    = RsimGetNode(targv[i]);
	nodes[i + noutputs]    = t->gate;
	t->source   = GND_node;
	t->scache.t = rd_tlist; /* link it to the list */
	rd_tlist    = t;
	rtf 	    = subcircuit->res[n++];
	t->r        = requiv(RESIST, (int)2, (int)(rtf * LAMBDACM));
    } /* for loop over outputs */

donesubckt:
    free(out_name_D);
    free(out_name_Ub);
    return result;
}

#endif /* USER_SUBCKT */
