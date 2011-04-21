/* a simple DLL simulation in irsim */
#include <stdio.h>
#include "subckt.h"

uptr flop_init();
void flop_eval();
uptr dll_init();
void dll_eval();

#define	DEBUG 0
#define	dprintf if (DEBUG) lprintf

static float flop_res[1] = {5000.0, 5000.0};

userSubCircuit subs[] =
{
    { "flop",	flop_eval, flop_init, 2, 1, flop_res},
    { "dll",	dll_eval,  dll_init,  3, 0, NULL},
    { NULL,	NULL,	   NULL,      0, 0, NULL}
};

/*--------------------------------------------------------------*/
/* Flip-flop internal state structure				*/
/*--------------------------------------------------------------*/

typedef struct _flopState {
    int prvClk;
    int Q;
} flopState;

/*--------------------------------------------------------------*/
/* Flip-flop initialization function				*/
/* Return pointer to internal structure for this instance	*/
/*--------------------------------------------------------------*/

uptr flop_init()
{
    flopState *thisFlop = (flopState *)malloc(sizeof(flopState));

    thisFlop->prvClk = HIGH;
    thisFlop->Q = X;
    return (uptr)thisFlop;
}

/*--------------------------------------------------------------*/
/* Flip-flop evaluation function				*/
/*--------------------------------------------------------------*/

void flop_eval(ins, outs, delay, data)
    char *ins, *outs;
    double *delay;
    uptr  *data;
{
    flopState *ff = (flopState *)data;

    if (ff->prvClk == LOW && ins[1] == HIGH)
    {
	dprintf("flop edge @ %.2fns\n", d2ns(cur_delta));
	ff->Q = ins[0];
    }
    delay[0] = 100.0;
    ff->prvClk = ins[1];
    outs[0] = ff->Q;
}

/*--------------------------------------------------------------*/
/* DLL internal state structure					*/
/*--------------------------------------------------------------*/

typedef struct _dll_state {
    double chpmpV;	/* the charge pump voltage */
    int    prvClk;
} dll_state ;

#define RAIL 	3.3	/* rail of the charge pump 0 - 3.3 */
#define UDPULSE	2e-9	/* 2ns pulse */
#define CHPMP_I	100e-6	/* 100 uA chpmp current */
#define PMP_CAP	20e-12	/* 10 pf filter cap */
#define CHGPCKT	(CHPMP_I * UDPULSE / PMP_CAP) /* charge packet */

/*--------------------------------------------------------------*/
/* DLL initialization function					*/
/* Return pointer to internal structure for this instance	*/
/*--------------------------------------------------------------*/

uptr dll_init()
{
    dll_state *t = (dll_state *)malloc(sizeof(dll_state));

    t->chpmpV = 0.0;
    t->prvClk = LOW;
    return (uptr)t;
}


#define	DLY0		2800.0	/* 1.8ns start delay */
#define	DLYSLOPE	1000.0	/* 1.0ns/V dline gain */
#define	DLINE_TF(V)	(DLY0 + DLYSLOPE * (V))
#define	clkOut		(out[0])

/*--------------------------------------------------------------*/
/* DLL evaluation function					*/
/*	in[0] = up/!down					*/
/*	in[1] = clock						*/
/*	in[2] = clock output feedback				*/
/*	out   = no outputs defined.				*/
/*--------------------------------------------------------------*/

void dll_eval(in, out, delay, data)
    char *in, *out;
    double *delay;
    uptr  *data;
{
    dll_state *d = (dll_state *)data;
    char updn = in[0], clkIn = in[1], clkOutFb = in[2];

    dprintf("dll called @ %.2fns voltage %lf\n", d2ns(cur_delta), d->chpmpV);
    dprintf("\t updn=%c clkin=%c clkOutFb=%c prevClk=%c\n", 
		updn, clkIn, clkOutFb, d->prvClk);
    delay[0] = DLINE_TF(d->chpmpV);
    clkOut = clkIn;

    if (d->prvClk == LOW && clkOutFb == HIGH)
    {
	d->chpmpV += (updn == HIGH) ? CHGPCKT : -CHGPCKT ;
	dprintf("pumping %s @ %.2fns voltage %le\n", 
			(updn == LOW) ? "up" : "dn",
			d2ns(cur_delta), d->chpmpV);
    }
    d->prvClk = clkOutFb;
}

/*--------------------------------------------------------------*/
/* End of Flip-flop and DLL example compiled-in library		*/
/*--------------------------------------------------------------*/
