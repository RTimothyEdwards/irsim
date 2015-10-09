/*--------------------------------------------------------------*/
/* diglib ---							*/
/*								*/
/*	Demonstration of a digital library of subcircuits	*/
/*	that can be compiled as a shared library and loaded	*/
/*	on the fly by IRSIM at runtime.  This library defines	*/
/*	most subcircuits that one would find in a standard cell	*/
/*	library.						*/
/*								*/
/*    Important:  Any routines from the IRSIM code base must	*/
/*    be delared as exported symbols in tclirsim.so.  Add the	*/
/*    names of any routines used to the irsim/symbol.map file.	*/
/*								*/
/*--------------------------------------------------------------*/
/* Digital Library copyright 2007, GPL license			*/
/* R. Timothy Edwards						*/
/* MultiGiG, Inc.						*/
/* Scotts Valley, CA						*/
/*--------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef _GLOBALS_H
#include "globals.h"
#endif

#ifdef USER_SUBCKT
#ifdef TCLSUBCKT

#define DEBUG 0
#define dprintf if (DEBUG) lprintf

/* Simple macro for logic inversion */
#define INVERT(a) (((a) == HIGH) ? LOW : (((a) == LOW) ? HIGH : X))

#ifdef TCL_IRSIM

#include <tcl.h>

/* Declare the function prototyptes of the initialization and	*/
/* evaluation routines for each subcircuit.			*/

uptr dff_init();

void dff_eval(), dffr_eval(), dffs_eval(), dffsr_eval();
void lat_eval();
void invert_eval(), buffer_eval(), triinv_eval();
void nand2_eval(), nor2_eval(), mux2_eval();
void and2_eval(), or2_eval(), xor2_eval(), xnor2_eval();
void nand3_eval(), nand4_eval(), nor3_eval(), nor4_eval();
void and3_eval(), and4_eval(), or3_eval(), or4_eval();

/*----------------------------------------------------------------------*/
/* Define two resistors per output, one pullup, one pulldown.		*/
/*----------------------------------------------------------------------*/

static float std_res[2] = {500.0, 500.0};

/*----------------------------------------------------------------------*/
/* Define the structure of each subcircuit.  This is of the form: 	*/
/*									*/
/*  <name> <eval_func> <init_func> <num_inputs> <num_outputs> <res>	*/
/*									*/
/* Where:								*/
/*  <name> is the name of the subcircuit (used in calls in a .sim file) */
/*  <eval_func> is the function that evaluates outputs			*/
/*  <init_func> is the function that creates any instance structures	*/
/*  <num_inputs> is the number of nodes that are inputs			*/
/*  <num_outputs> is the number of nodes that are outputs		*/
/*  <res> is the array of pairs of output pull-up and pull-down		*/
/*	resistances, in ohms, declared above.				*/
/*----------------------------------------------------------------------*/

static userSubCircuit subckts[] =
{
    { "invert",	invert_eval,    NULL, 1, 1, std_res},
    { "buffer",	buffer_eval,    NULL, 1, 1, std_res},
    { "triinv",	triinv_eval,    NULL, 2, 1, std_res},
    { "nand2",	nand2_eval,     NULL, 2, 1, std_res},
    { "and2",	and2_eval,      NULL, 2, 1, std_res},
    { "nor2",	nor2_eval,      NULL, 2, 1, std_res},
    { "or2",	or2_eval,       NULL, 2, 1, std_res},
    { "xor2",	xor2_eval,      NULL, 2, 1, std_res},
    { "xnor2",	xnor2_eval,     NULL, 2, 1, std_res},
    { "nand3",	nand3_eval,     NULL, 3, 1, std_res},
    { "and3",	and3_eval,      NULL, 3, 1, std_res},
    { "nor3",	nor3_eval,      NULL, 3, 1, std_res},
    { "or3",	or3_eval,       NULL, 3, 1, std_res},
    { "nand4",	nand4_eval,     NULL, 4, 1, std_res},
    { "and4",	and4_eval,      NULL, 4, 1, std_res},
    { "nor4",	nor4_eval,      NULL, 4, 1, std_res},
    { "or4",	or4_eval,       NULL, 4, 1, std_res},
    { "lat",	lat_eval,   dff_init, 2, 2, std_res},
    { "dff",	dff_eval,   dff_init, 2, 2, std_res},
    { "dffr",	dffr_eval,  dff_init, 3, 2, std_res},
    { "dffs",	dffs_eval,  dff_init, 3, 2, std_res},
    { "dffsr",	dffsr_eval, dff_init, 4, 2, std_res},
    { NULL,	NULL,	        NULL, 0, 0, NULL}      /* Sentinel req'd */
};

/*--------------------------------------------------------------*/
/* Internal structure used for latch and flip-flop instances	*/
/*--------------------------------------------------------------*/

typedef struct _dffState {
    int prvClk;
    int Q;
} dffState;

/*--------------------------------------------------------------*/
/* Initialization function for subcircuit dff 			*/
/*--------------------------------------------------------------*/

uptr dff_init(int targc, char *targv[])
{
    dffState *thisDff = (dffState *)malloc(sizeof(dffState));

    thisDff->prvClk = HIGH;
    thisDff->Q = X;

    return (uptr)thisDff;
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit lat			*/
/*	in[0] = D		out[0] = Q			*/
/*	in[1] = ENB		out[1] = Qbar			*/
/*--------------------------------------------------------------*/

void lat_eval(char *in, char *out, double *delay, uptr *data)
{
    dffState *ff = (dffState *)data;

    if (ff->prvClk == LOW && in[1] == HIGH) {
	ff->Q = in[0];
    }
    ff->prvClk = in[1];
    delay[0] = 100.0;
    if (in[1] == LOW)
	out[0] = in[0];
    else if (in[1] == HIGH)
	out[0] = ff->Q;
    else
	out[0] = X;
    out[1] = INVERT(ff->Q);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit dff			*/
/*	in[0] = D		out[0] = Q			*/
/*	in[1] = CLK		out[1] = Qbar			*/
/*--------------------------------------------------------------*/

void dff_eval(char *in, char *out, double *delay, uptr *data)
{
    dffState *ff = (dffState *)data;

    if (ff->prvClk == LOW && in[1] == HIGH) {
	dprintf(stdout, "dff edge @ %.2fns\n", d2ns(cur_delta));
	ff->Q = in[0];
    }
    ff->prvClk = in[1];
    delay[0] = 100.0;
    out[0] = ff->Q;
    out[1] = INVERT(ff->Q);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit dffr			*/
/*	in[0] = D		out[0] = Q			*/
/*	in[1] = CLK		out[1] = Qbar			*/
/*	in[2] = Rbar						*/
/*--------------------------------------------------------------*/

void dffr_eval(char *in, char *out, double *delay, uptr *data)
{
    dffState *ff = (dffState *)data;

    if (in[2] == LOW)
	ff->Q = LOW;
    else if (in[2] != HIGH)
	ff->Q = X;
    else if (ff->prvClk == LOW && in[1] == HIGH) {
	dprintf(stdout, "dffr edge @ %.2fns\n", d2ns(cur_delta));
	ff->Q = in[0];
    }
    ff->prvClk = in[1];
    delay[0] = 100.0;
    out[0] = ff->Q;
    out[1] = INVERT(ff->Q);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit dffs			*/
/*	in[0] = D		out[0] = Q			*/
/*	in[1] = CLK		out[1] = Qbar			*/
/*	in[2] = S						*/
/*--------------------------------------------------------------*/

void dffs_eval(char *in, char *out, double *delay, uptr *data)
{
    dffState *ff = (dffState *)data;

    if (in[2] == HIGH)
	ff->Q = HIGH;
    else if (in[2] != LOW)
	ff->Q = X;
    else if (ff->prvClk == LOW && in[1] == HIGH) {
	dprintf(stdout, "dffr edge @ %.2fns\n", d2ns(cur_delta));
	ff->Q = in[0];
    }
    ff->prvClk = in[1];
    delay[0] = 100.0;
    out[0] = ff->Q;
    out[1] = INVERT(ff->Q);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit dffsr			*/
/*	in[0] = D		out[0] = Q			*/
/*	in[1] = CLK		out[1] = Qbar			*/
/*	in[2] = S						*/
/*	in[3] = Rbar						*/
/*--------------------------------------------------------------*/

void dffsr_eval(char *in, char *out, double *delay, uptr *data)
{
    dffState *ff = (dffState *)data;

    if (in[2] == HIGH && in[3] == LOW)
	ff->Q = X;
    else if (in[2] == HIGH)
	ff->Q = HIGH;
    else if (in[3] == LOW)
	ff->Q = LOW;
    else if (in[2] != LOW || in[3] != HIGH)
	ff->Q = X;
    else if (ff->prvClk == LOW && in[1] == HIGH) {
	dprintf(stdout, "dffr edge @ %.2fns\n", d2ns(cur_delta));
	ff->Q = in[0];
    }
    ff->prvClk = in[1];
    delay[0] = 100.0;
    out[0] = ff->Q;
    out[1] = INVERT(ff->Q);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit invert			*/
/*--------------------------------------------------------------*/

void invert_eval(char *in, char *out, double *delay, uptr *data)
{
    out[0] = INVERT(in[0]);
    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit buffer			*/
/*--------------------------------------------------------------*/

void buffer_eval(char *in, char *out, double *delay, uptr *data)
{
    out[0] = in[0];
    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit triinv			*/
/*	in[0] = A		out[0] = Y			*/
/*	in[1] = EN						*/
/*--------------------------------------------------------------*/

void triinv_eval(char *in, char *out, double *delay, uptr *data)
{
    if (in[1] == HIGH)
	out[0] = INVERT(in[0]);
    else if (in[1] == LOW)
	out[0] = HIGH_Z;
    else
	out[0] = X;

    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit mux2			*/
/*	in[0] = A		out[0] = Y			*/
/*	in[1] = B						*/
/*	in[2] = S						*/
/*--------------------------------------------------------------*/

void mux2_eval(char *in, char *out, double *delay, uptr *data)
{
    if (in[2] == LOW)
	out[0] = in[0];
    else if (in[2] == HIGH)
	out[0] = in[1];
    else if (in[0] == in[1])	/* S unknown but result unambiguous */
	out[0] = in[1];
    else
	out[0] = X;

    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit nand2 (and2)		*/
/*--------------------------------------------------------------*/

void nand2_eval(char *in, char *out, double *delay, uptr *data)
{
    if (in[0] == LOW || in[1] == LOW)
	out[0] = HIGH;
    else if (in[0] == HIGH && in[1] == HIGH)
	out[0] = LOW;
    else
	out[0] = X;
	
    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/

void and2_eval(char *in, char *out, double *delay, uptr *data)
{
    nand2_eval(in, out, delay, data);
    out[0] = INVERT(out[0]);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit nor2 (or2)		*/
/*--------------------------------------------------------------*/

void nor2_eval(char *in, char *out, double *delay, uptr *data)
{
    if (in[0] == HIGH || in[1] == HIGH)
	out[0] = LOW;
    else if (in[0] == LOW && in[1] == LOW)
	out[0] = HIGH;
    else
	out[0] = X;
	
    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/

void or2_eval(char *in, char *out, double *delay, uptr *data)
{
    nor2_eval(in, out, delay, data);
    out[0] = INVERT(out[0]);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit nand3 (and3)		*/
/*--------------------------------------------------------------*/

void nand3_eval(char *in, char *out, double *delay, uptr *data)
{
    if (in[0] == LOW || in[1] == LOW || in[2] == LOW)
	out[0] = HIGH;
    else if (in[0] == HIGH && in[1] == HIGH && in[2] == HIGH)
	out[0] = LOW;
    else
	out[0] = X;
	
    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/

void and3_eval(char *in, char *out, double *delay, uptr *data)
{
    nand3_eval(in, out, delay, data);
    out[0] = INVERT(out[0]);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit nor3 (or3)		*/
/*--------------------------------------------------------------*/

void nor3_eval(char *in, char *out, double *delay, uptr *data)
{
    if (in[0] == HIGH || in[1] == HIGH || in[2] == HIGH)
	out[0] = LOW;
    else if (in[0] == LOW && in[1] == LOW && in[2] == LOW)
	out[0] = HIGH;
    else
	out[0] = X;
	
    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/

void or3_eval(char *in, char *out, double *delay, uptr *data)
{
    nor3_eval(in, out, delay, data);
    out[0] = INVERT(out[0]);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit nand4 (and4)		*/
/*--------------------------------------------------------------*/

void nand4_eval(char *in, char *out, double *delay, uptr *data)
{
    if (in[0] == LOW || in[1] == LOW || in[2] == LOW || in[3] == LOW)
	out[0] = HIGH;
    else if (in[0] == HIGH && in[1] == HIGH && in[2] == HIGH && in[3] == HIGH)
	out[0] = LOW;
    else
	out[0] = X;
	
    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/

void and4_eval(char *in, char *out, double *delay, uptr *data)
{
    nand4_eval(in, out, delay, data);
    out[0] = INVERT(out[0]);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit nor4 (or4)		*/
/*--------------------------------------------------------------*/

void nor4_eval(char *in, char *out, double *delay, uptr *data)
{
    if (in[0] == HIGH || in[1] == HIGH || in[2] == HIGH || in[3] == HIGH)
	out[0] = LOW;
    else if (in[0] == LOW && in[1] == LOW && in[2] == LOW && in[3] == LOW)
	out[0] = HIGH;
    else
	out[0] = X;
	
    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/

void or4_eval(char *in, char *out, double *delay, uptr *data)
{
    nor4_eval(in, out, delay, data);
    out[0] = INVERT(out[0]);
}

/*--------------------------------------------------------------*/
/* Evaluation function for subcircuit xnor2 (xor2)		*/
/*--------------------------------------------------------------*/

void xnor2_eval(char *in, char *out, double *delay, uptr *data)
{
    if (in[0] == HIGH && in[1] == HIGH)
	out[0] = HIGH;
    else if (in[0] == LOW && in[1] == LOW)
	out[0] = HIGH;
    else if ((in[0] == HIGH && in[1] == LOW) || (in[0] == LOW && in[1] == HIGH))
	out[0] = LOW;
    else
	out[0] = X;
	
    delay[0] = 20.0;
}

/*--------------------------------------------------------------*/

void xor2_eval(char *in, char *out, double *delay, uptr *data)
{
    xnor2_eval(in, out, delay, data);
    out[0] = INVERT(out[0]);
}

/*------------------------------------------------------*/
/* Tcl Package initialization function for "diglib"	*/
/*------------------------------------------------------*/

int Diglib_Init(Tcl_Interp *interp)
{
    userSubCircuit *s;
    int newptr;

    Tcl_PkgProvide(interp, "irsim_diglib", "1.0");

    /* Register all the functions defined by subckts[] */
    init_subs(subckts);

    return TCL_OK;
}

#endif /* TCL_IRSIM */
#endif /* TCLSUBCKT */
#endif /* USER_SUBCKT */
