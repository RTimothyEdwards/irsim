#include <stdio.h>
#include "subckt.h"

uptr mux2_init();
void mux2_eval();
uptr flop_init();
void flop_eval();

static float mux2_res[2] = {500.0, 500.0};
static float flop_res[2] = {1000.0, 1000.0};

static userSubCircuit subs[] =
{
    { "mux2",	mux2_eval,  mux2_init,	3, 1, mux2_res},
    { "flop",	flop_eval,  flop_init,	3, 1, flop_res},
    { NULL,	NULL,	    NULL, 	0, 0, NULL}
};

typedef struct _flopState {
    int prvClk;
    int Q;
} flopState;

uptr flop_init()
{
    flopState *thisFlop = (flopState *)malloc(sizeof(flopState));

    thisFlop->prvClk = HIGH;
    thisFlop->Q = X;
    return (uptr)thisFlop;
}

void flop_eval(ins, outs, delay, data)
    char *ins, *outs;
    double *delay;
    uptr  *data;
{
    flopState *ff = (flopState *)data ;

    if (ff->prvClk == LOW && ins[1] == HIGH) {
	lprintf("flop edge @ %.2fns\n", d2ns(cur_delta));
	ff->Q = ins[0];
    }
    delay[0] = 100.0;
    ff->prvClk = ins[1];
    outs[0] = ff->Q;
}

uptr mux2_init()
{
    return (uptr)NULL;
}

void mux2_eval(ins, outs, delay, udata)
    char *ins;
    char *outs;
    double *delay;
    uptr udata;
{
    delay[0] = 100.0;
    if (ins[2] == LOW)
	outs[0] = ins[1];
    else if (ins[2] == HIGH)
	outs[0] = ins[0];
    else
	outs[0] = X;
}
