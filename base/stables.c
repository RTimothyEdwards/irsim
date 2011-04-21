/* DO NOT EDIT: THIS FILE IS GENERATED USING gentbl */
/* names for each value interval */
char *node_values[46] = {
	"EMPTY",
	"DH",
	"DHWH",
	"DHCH",
	"DHcH",
	"DHZ",
	"DHcL",
	"DHCL",
	"DHWL",
	"DHDL",
	"WH",
	"WHCH",
	"WHcH",
	"WHZ",
	"WHcL",
	"WHCL",
	"WHWL",
	"WHDL",
	"CH",
	"CHcH",
	"CHZ",
	"CHcL",
	"CHCL",
	"CHWL",
	"CHDL",
	"cH",
	"cHZ",
	"cHcL",
	"cHCL",
	"cHWL",
	"cHDL",
	"Z",
	"ZcL",
	"ZCL",
	"ZWL",
	"ZDL",
	"cL",
	"cLCL",
	"cLWL",
	"cLDL",
	"CL",
	"CLWL",
	"CLDL",
	"WL",
	"WLDL",
	"DL",
};

/* index for each value interval */
#define EMPTY	0

#define DH	1
#define DHWH	2
#define DHCH	3
#define DHcH	4
#define DHZ	5
#define DHcL	6
#define DHCL	7
#define DHWL	8
#define DHDL	9
#define WH	10
#define WHCH	11
#define WHcH	12
#define WHZ	13
#define WHcL	14
#define WHCL	15
#define WHWL	16
#define WHDL	17
#define CH	18
#define CHcH	19
#define CHZ	20
#define CHcL	21
#define CHCL	22
#define CHWL	23
#define CHDL	24
#define cH	25
#define cHZ	26
#define cHcL	27
#define cHCL	28
#define cHWL	29
#define cHDL	30
#define Z	31
#define ZcL	32
#define ZCL	33
#define ZWL	34
#define ZDL	35
#define cL	36
#define cLCL	37
#define cLWL	38
#define cLDL	39
#define CL	40
#define CLWL	41
#define CLDL	42
#define WL	43
#define WLDL	44
#define DL	45

/* conversion between interval and logic value */
char logic_state[46] = {
  0,	/* EMPTY */
  HIGH,	/* DH */
  HIGH,	/* DHWH */
  HIGH,	/* DHCH */
  HIGH,	/* DHcH */
  X,	/* DHZ */
  X,	/* DHcL */
  X,	/* DHCL */
  X,	/* DHWL */
  X,	/* DHDL */
  HIGH,	/* WH */
  HIGH,	/* WHCH */
  HIGH,	/* WHcH */
  X,	/* WHZ */
  X,	/* WHcL */
  X,	/* WHCL */
  X,	/* WHWL */
  X,	/* WHDL */
  HIGH,	/* CH */
  HIGH,	/* CHcH */
  X,	/* CHZ */
  X,	/* CHcL */
  X,	/* CHCL */
  X,	/* CHWL */
  X,	/* CHDL */
  HIGH,	/* cH */
  X,	/* cHZ */
  X,	/* cHcL */
  X,	/* cHCL */
  X,	/* cHWL */
  X,	/* cHDL */
  X,	/* Z */
  X,	/* ZcL */
  X,	/* ZCL */
  X,	/* ZWL */
  X,	/* ZDL */
  LOW,	/* cL */
  LOW,	/* cLCL */
  LOW,	/* cLWL */
  LOW,	/* cLDL */
  LOW,	/* CL */
  LOW,	/* CLWL */
  LOW,	/* CLDL */
  LOW,	/* WL */
  LOW,	/* WLDL */
  LOW,	/* DL */
};

/* transmit interval through switch */
char transmit[46][4] = {
  0,	0,	0,	0,	/* EMPTY */
  Z,	DH,	DHZ,	WH,	/* DH */
  Z,	DHWH,	DHZ,	WH,	/* DHWH */
  Z,	DHCH,	DHZ,	WHCH,	/* DHCH */
  Z,	DHcH,	DHZ,	WHcH,	/* DHcH */
  Z,	DHZ,	DHZ,	WHZ,	/* DHZ */
  Z,	DHcL,	DHcL,	WHcL,	/* DHcL */
  Z,	DHCL,	DHCL,	WHCL,	/* DHCL */
  Z,	DHWL,	DHWL,	WHWL,	/* DHWL */
  Z,	DHDL,	DHDL,	WHWL,	/* DHDL */
  Z,	WH,	WHZ,	WH,	/* WH */
  Z,	WHCH,	WHZ,	WHCH,	/* WHCH */
  Z,	WHcH,	WHZ,	WHcH,	/* WHcH */
  Z,	WHZ,	WHZ,	WHZ,	/* WHZ */
  Z,	WHcL,	WHcL,	WHcL,	/* WHcL */
  Z,	WHCL,	WHCL,	WHCL,	/* WHCL */
  Z,	WHWL,	WHWL,	WHWL,	/* WHWL */
  Z,	WHDL,	WHDL,	WHWL,	/* WHDL */
  Z,	CH,	CHZ,	CH,	/* CH */
  Z,	CHcH,	CHZ,	CHcH,	/* CHcH */
  Z,	CHZ,	CHZ,	CHZ,	/* CHZ */
  Z,	CHcL,	CHcL,	CHcL,	/* CHcL */
  Z,	CHCL,	CHCL,	CHCL,	/* CHCL */
  Z,	CHWL,	CHWL,	CHWL,	/* CHWL */
  Z,	CHDL,	CHDL,	CHWL,	/* CHDL */
  Z,	cH,	cHZ,	cH,	/* cH */
  Z,	cHZ,	cHZ,	cHZ,	/* cHZ */
  Z,	cHcL,	cHcL,	cHcL,	/* cHcL */
  Z,	cHCL,	cHCL,	cHCL,	/* cHCL */
  Z,	cHWL,	cHWL,	cHWL,	/* cHWL */
  Z,	cHDL,	cHDL,	cHWL,	/* cHDL */
  Z,	Z,	Z,	Z,	/* Z */
  Z,	ZcL,	ZcL,	ZcL,	/* ZcL */
  Z,	ZCL,	ZCL,	ZCL,	/* ZCL */
  Z,	ZWL,	ZWL,	ZWL,	/* ZWL */
  Z,	ZDL,	ZDL,	ZWL,	/* ZDL */
  Z,	cL,	ZcL,	cL,	/* cL */
  Z,	cLCL,	ZCL,	cLCL,	/* cLCL */
  Z,	cLWL,	ZWL,	cLWL,	/* cLWL */
  Z,	cLDL,	ZDL,	cLWL,	/* cLDL */
  Z,	CL,	ZCL,	CL,	/* CL */
  Z,	CLWL,	ZWL,	CLWL,	/* CLWL */
  Z,	CLDL,	ZDL,	CLWL,	/* CLDL */
  Z,	WL,	ZWL,	WL,	/* WL */
  Z,	WLDL,	ZDL,	WL,	/* WLDL */
  Z,	DL,	ZDL,	WL,	/* DL */
};

/* result of shorting two intervals */
char smerge[46][46] = {

/* EMPTY */
  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,
  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,
  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,
  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,
  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,
  0   ,  0   ,  0   ,  0   ,  0   ,  0   ,
/* DH */
  0   ,  DH  ,  DH  ,  DH  ,  DH  ,  DH  ,  DH  ,  DH  ,
  DH  ,  DHDL,  DH  ,  DH  ,  DH  ,  DH  ,  DH  ,  DH  ,
  DH  ,  DHDL,  DH  ,  DH  ,  DH  ,  DH  ,  DH  ,  DH  ,
  DHDL,  DH  ,  DH  ,  DH  ,  DH  ,  DH  ,  DHDL,  DH  ,
  DH  ,  DH  ,  DH  ,  DHDL,  DH  ,  DH  ,  DH  ,  DHDL,
  DH  ,  DH  ,  DHDL,  DH  ,  DHDL,  DHDL,
/* DHWH */
  0   ,  DH  ,  DHWH,  DHWH,  DHWH,  DHWH,  DHWH,  DHWH,
  DHWL,  DHDL,  DHWH,  DHWH,  DHWH,  DHWH,  DHWH,  DHWH,
  DHWL,  DHDL,  DHWH,  DHWH,  DHWH,  DHWH,  DHWH,  DHWL,
  DHDL,  DHWH,  DHWH,  DHWH,  DHWH,  DHWL,  DHDL,  DHWH,
  DHWH,  DHWH,  DHWL,  DHDL,  DHWH,  DHWH,  DHWL,  DHDL,
  DHWH,  DHWL,  DHDL,  DHWL,  DHDL,  DHDL,
/* DHCH */
  0   ,  DH  ,  DHWH,  DHCH,  DHCH,  DHCH,  DHCH,  DHCL,
  DHWL,  DHDL,  DHWH,  DHCH,  DHCH,  DHCH,  DHCH,  DHCL,
  DHWL,  DHDL,  DHCH,  DHCH,  DHCH,  DHCH,  DHCL,  DHWL,
  DHDL,  DHCH,  DHCH,  DHCH,  DHCL,  DHWL,  DHDL,  DHCH,
  DHCH,  DHCL,  DHWL,  DHDL,  DHCH,  DHCL,  DHWL,  DHDL,
  DHCL,  DHWL,  DHDL,  DHWL,  DHDL,  DHDL,
/* DHcH */
  0   ,  DH  ,  DHWH,  DHCH,  DHcH,  DHcH,  DHcL,  DHCL,
  DHWL,  DHDL,  DHWH,  DHCH,  DHcH,  DHcH,  DHcL,  DHCL,
  DHWL,  DHDL,  DHCH,  DHcH,  DHcH,  DHcL,  DHCL,  DHWL,
  DHDL,  DHcH,  DHcH,  DHcL,  DHCL,  DHWL,  DHDL,  DHcH,
  DHcL,  DHCL,  DHWL,  DHDL,  DHcL,  DHCL,  DHWL,  DHDL,
  DHCL,  DHWL,  DHDL,  DHWL,  DHDL,  DHDL,
/* DHZ */
  0   ,  DH  ,  DHWH,  DHCH,  DHcH,  DHZ ,  DHcL,  DHCL,
  DHWL,  DHDL,  DHWH,  DHCH,  DHcH,  DHZ ,  DHcL,  DHCL,
  DHWL,  DHDL,  DHCH,  DHcH,  DHZ ,  DHcL,  DHCL,  DHWL,
  DHDL,  DHcH,  DHZ ,  DHcL,  DHCL,  DHWL,  DHDL,  DHZ ,
  DHcL,  DHCL,  DHWL,  DHDL,  DHcL,  DHCL,  DHWL,  DHDL,
  DHCL,  DHWL,  DHDL,  DHWL,  DHDL,  DHDL,
/* DHcL */
  0   ,  DH  ,  DHWH,  DHCH,  DHcL,  DHcL,  DHcL,  DHCL,
  DHWL,  DHDL,  DHWH,  DHCH,  DHcL,  DHcL,  DHcL,  DHCL,
  DHWL,  DHDL,  DHCH,  DHcL,  DHcL,  DHcL,  DHCL,  DHWL,
  DHDL,  DHcL,  DHcL,  DHcL,  DHCL,  DHWL,  DHDL,  DHcL,
  DHcL,  DHCL,  DHWL,  DHDL,  DHcL,  DHCL,  DHWL,  DHDL,
  DHCL,  DHWL,  DHDL,  DHWL,  DHDL,  DHDL,
/* DHCL */
  0   ,  DH  ,  DHWH,  DHCL,  DHCL,  DHCL,  DHCL,  DHCL,
  DHWL,  DHDL,  DHWH,  DHCL,  DHCL,  DHCL,  DHCL,  DHCL,
  DHWL,  DHDL,  DHCL,  DHCL,  DHCL,  DHCL,  DHCL,  DHWL,
  DHDL,  DHCL,  DHCL,  DHCL,  DHCL,  DHWL,  DHDL,  DHCL,
  DHCL,  DHCL,  DHWL,  DHDL,  DHCL,  DHCL,  DHWL,  DHDL,
  DHCL,  DHWL,  DHDL,  DHWL,  DHDL,  DHDL,
/* DHWL */
  0   ,  DH  ,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,
  DHWL,  DHDL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,
  DHWL,  DHDL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,
  DHDL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHDL,  DHWL,
  DHWL,  DHWL,  DHWL,  DHDL,  DHWL,  DHWL,  DHWL,  DHDL,
  DHWL,  DHWL,  DHDL,  DHWL,  DHDL,  DHDL,
/* DHDL */
  0   ,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
/* WH */
  0   ,  DH  ,  DHWH,  DHWH,  DHWH,  DHWH,  DHWH,  DHWH,
  DHWL,  DHDL,  WH  ,  WH  ,  WH  ,  WH  ,  WH  ,  WH  ,
  WHWL,  WHDL,  WH  ,  WH  ,  WH  ,  WH  ,  WH  ,  WHWL,
  WHDL,  WH  ,  WH  ,  WH  ,  WH  ,  WHWL,  WHDL,  WH  ,
  WH  ,  WH  ,  WHWL,  WHDL,  WH  ,  WH  ,  WHWL,  WHDL,
  WH  ,  WHWL,  WHDL,  WHWL,  WHDL,  DL  ,
/* WHCH */
  0   ,  DH  ,  DHWH,  DHCH,  DHCH,  DHCH,  DHCH,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHCH,  WHCH,  WHCH,  WHCL,
  WHWL,  WHDL,  WHCH,  WHCH,  WHCH,  WHCH,  WHCL,  WHWL,
  WHDL,  WHCH,  WHCH,  WHCH,  WHCL,  WHWL,  WHDL,  WHCH,
  WHCH,  WHCL,  WHWL,  WHDL,  WHCH,  WHCL,  WHWL,  WHDL,
  WHCL,  WHWL,  WHDL,  WHWL,  WHDL,  DL  ,
/* WHcH */
  0   ,  DH  ,  DHWH,  DHCH,  DHcH,  DHcH,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcH,  WHcH,  WHcL,  WHCL,
  WHWL,  WHDL,  WHCH,  WHcH,  WHcH,  WHcL,  WHCL,  WHWL,
  WHDL,  WHcH,  WHcH,  WHcL,  WHCL,  WHWL,  WHDL,  WHcH,
  WHcL,  WHCL,  WHWL,  WHDL,  WHcL,  WHCL,  WHWL,  WHDL,
  WHCL,  WHWL,  WHDL,  WHWL,  WHDL,  DL  ,
/* WHZ */
  0   ,  DH  ,  DHWH,  DHCH,  DHcH,  DHZ ,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcH,  WHZ ,  WHcL,  WHCL,
  WHWL,  WHDL,  WHCH,  WHcH,  WHZ ,  WHcL,  WHCL,  WHWL,
  WHDL,  WHcH,  WHZ ,  WHcL,  WHCL,  WHWL,  WHDL,  WHZ ,
  WHcL,  WHCL,  WHWL,  WHDL,  WHcL,  WHCL,  WHWL,  WHDL,
  WHCL,  WHWL,  WHDL,  WHWL,  WHDL,  DL  ,
/* WHcL */
  0   ,  DH  ,  DHWH,  DHCH,  DHcL,  DHcL,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcL,  WHcL,  WHcL,  WHCL,
  WHWL,  WHDL,  WHCH,  WHcL,  WHcL,  WHcL,  WHCL,  WHWL,
  WHDL,  WHcL,  WHcL,  WHcL,  WHCL,  WHWL,  WHDL,  WHcL,
  WHcL,  WHCL,  WHWL,  WHDL,  WHcL,  WHCL,  WHWL,  WHDL,
  WHCL,  WHWL,  WHDL,  WHWL,  WHDL,  DL  ,
/* WHCL */
  0   ,  DH  ,  DHWH,  DHCL,  DHCL,  DHCL,  DHCL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCL,  WHCL,  WHCL,  WHCL,  WHCL,
  WHWL,  WHDL,  WHCL,  WHCL,  WHCL,  WHCL,  WHCL,  WHWL,
  WHDL,  WHCL,  WHCL,  WHCL,  WHCL,  WHWL,  WHDL,  WHCL,
  WHCL,  WHCL,  WHWL,  WHDL,  WHCL,  WHCL,  WHWL,  WHDL,
  WHCL,  WHWL,  WHDL,  WHWL,  WHDL,  DL  ,
/* WHWL */
  0   ,  DH  ,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,
  DHWL,  DHDL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,
  WHWL,  WHDL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,
  WHDL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,  WHDL,  WHWL,
  WHWL,  WHWL,  WHWL,  WHDL,  WHWL,  WHWL,  WHWL,  WHDL,
  WHWL,  WHWL,  WHDL,  WHWL,  WHDL,  DL  ,
/* WHDL */
  0   ,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,
  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,
  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,
  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,
  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  DL  ,
/* CH */
  0   ,  DH  ,  DHWH,  DHCH,  DHCH,  DHCH,  DHCH,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHCH,  WHCH,  WHCH,  WHCL,
  WHWL,  WHDL,  CH  ,  CH  ,  CH  ,  CH  ,  CHCL,  CHWL,
  CHDL,  CH  ,  CH  ,  CH  ,  CHCL,  CHWL,  CHDL,  CH  ,
  CH  ,  CHCL,  CHWL,  CHDL,  CH  ,  CHCL,  CHWL,  CHDL,
  CHCL,  CHWL,  CHDL,  WL  ,  WLDL,  DL  ,
/* CHcH */
  0   ,  DH  ,  DHWH,  DHCH,  DHcH,  DHcH,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcH,  WHcH,  WHcL,  WHCL,
  WHWL,  WHDL,  CH  ,  CHcH,  CHcH,  CHcL,  CHCL,  CHWL,
  CHDL,  CHcH,  CHcH,  CHcL,  CHCL,  CHWL,  CHDL,  CHcH,
  CHcL,  CHCL,  CHWL,  CHDL,  CHcL,  CHCL,  CHWL,  CHDL,
  CHCL,  CHWL,  CHDL,  WL  ,  WLDL,  DL  ,
/* CHZ */
  0   ,  DH  ,  DHWH,  DHCH,  DHcH,  DHZ ,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcH,  WHZ ,  WHcL,  WHCL,
  WHWL,  WHDL,  CH  ,  CHcH,  CHZ ,  CHcL,  CHCL,  CHWL,
  CHDL,  CHcH,  CHZ ,  CHcL,  CHCL,  CHWL,  CHDL,  CHZ ,
  CHcL,  CHCL,  CHWL,  CHDL,  CHcL,  CHCL,  CHWL,  CHDL,
  CHCL,  CHWL,  CHDL,  WL  ,  WLDL,  DL  ,
/* CHcL */
  0   ,  DH  ,  DHWH,  DHCH,  DHcL,  DHcL,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcL,  WHcL,  WHcL,  WHCL,
  WHWL,  WHDL,  CH  ,  CHcL,  CHcL,  CHcL,  CHCL,  CHWL,
  CHDL,  CHcL,  CHcL,  CHcL,  CHCL,  CHWL,  CHDL,  CHcL,
  CHcL,  CHCL,  CHWL,  CHDL,  CHcL,  CHCL,  CHWL,  CHDL,
  CHCL,  CHWL,  CHDL,  WL  ,  WLDL,  DL  ,
/* CHCL */
  0   ,  DH  ,  DHWH,  DHCL,  DHCL,  DHCL,  DHCL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCL,  WHCL,  WHCL,  WHCL,  WHCL,
  WHWL,  WHDL,  CHCL,  CHCL,  CHCL,  CHCL,  CHCL,  CHWL,
  CHDL,  CHCL,  CHCL,  CHCL,  CHCL,  CHWL,  CHDL,  CHCL,
  CHCL,  CHCL,  CHWL,  CHDL,  CHCL,  CHCL,  CHWL,  CHDL,
  CHCL,  CHWL,  CHDL,  WL  ,  WLDL,  DL  ,
/* CHWL */
  0   ,  DH  ,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,
  DHWL,  DHDL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,
  WHWL,  WHDL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,
  CHDL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,  CHDL,  CHWL,
  CHWL,  CHWL,  CHWL,  CHDL,  CHWL,  CHWL,  CHWL,  CHDL,
  CHWL,  CHWL,  CHDL,  WL  ,  WLDL,  DL  ,
/* CHDL */
  0   ,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,
  WHDL,  WHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,
  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,
  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,
  CHDL,  CHDL,  CHDL,  WLDL,  WLDL,  DL  ,
/* cH */
  0   ,  DH  ,  DHWH,  DHCH,  DHcH,  DHcH,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcH,  WHcH,  WHcL,  WHCL,
  WHWL,  WHDL,  CH  ,  CHcH,  CHcH,  CHcL,  CHCL,  CHWL,
  CHDL,  cH  ,  cH  ,  cHcL,  cHCL,  cHWL,  cHDL,  cH  ,
  cHcL,  cHCL,  cHWL,  cHDL,  cHcL,  cHCL,  cHWL,  cHDL,
  CL  ,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* cHZ */
  0   ,  DH  ,  DHWH,  DHCH,  DHcH,  DHZ ,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcH,  WHZ ,  WHcL,  WHCL,
  WHWL,  WHDL,  CH  ,  CHcH,  CHZ ,  CHcL,  CHCL,  CHWL,
  CHDL,  cH  ,  cHZ ,  cHcL,  cHCL,  cHWL,  cHDL,  cHZ ,
  cHcL,  cHCL,  cHWL,  cHDL,  cHcL,  cHCL,  cHWL,  cHDL,
  CL  ,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* cHcL */
  0   ,  DH  ,  DHWH,  DHCH,  DHcL,  DHcL,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcL,  WHcL,  WHcL,  WHCL,
  WHWL,  WHDL,  CH  ,  CHcL,  CHcL,  CHcL,  CHCL,  CHWL,
  CHDL,  cHcL,  cHcL,  cHcL,  cHCL,  cHWL,  cHDL,  cHcL,
  cHcL,  cHCL,  cHWL,  cHDL,  cHcL,  cHCL,  cHWL,  cHDL,
  CL  ,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* cHCL */
  0   ,  DH  ,  DHWH,  DHCL,  DHCL,  DHCL,  DHCL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCL,  WHCL,  WHCL,  WHCL,  WHCL,
  WHWL,  WHDL,  CHCL,  CHCL,  CHCL,  CHCL,  CHCL,  CHWL,
  CHDL,  cHCL,  cHCL,  cHCL,  cHCL,  cHWL,  cHDL,  cHCL,
  cHCL,  cHCL,  cHWL,  cHDL,  cHCL,  cHCL,  cHWL,  cHDL,
  CL  ,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* cHWL */
  0   ,  DH  ,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,
  DHWL,  DHDL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,
  WHWL,  WHDL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,
  CHDL,  cHWL,  cHWL,  cHWL,  cHWL,  cHWL,  cHDL,  cHWL,
  cHWL,  cHWL,  cHWL,  cHDL,  cHWL,  cHWL,  cHWL,  cHDL,
  CLWL,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* cHDL */
  0   ,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,
  WHDL,  WHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,
  CHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,
  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,
  CLDL,  CLDL,  CLDL,  WLDL,  WLDL,  DL  ,
/* Z */
  0   ,  DH  ,  DHWH,  DHCH,  DHcH,  DHZ ,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcH,  WHZ ,  WHcL,  WHCL,
  WHWL,  WHDL,  CH  ,  CHcH,  CHZ ,  CHcL,  CHCL,  CHWL,
  CHDL,  cH  ,  cHZ ,  cHcL,  cHCL,  cHWL,  cHDL,  Z   ,
  ZcL ,  ZCL ,  ZWL ,  ZDL ,  cL  ,  cLCL,  cLWL,  cLDL,
  CL  ,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* ZcL */
  0   ,  DH  ,  DHWH,  DHCH,  DHcL,  DHcL,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcL,  WHcL,  WHcL,  WHCL,
  WHWL,  WHDL,  CH  ,  CHcL,  CHcL,  CHcL,  CHCL,  CHWL,
  CHDL,  cHcL,  cHcL,  cHcL,  cHCL,  cHWL,  cHDL,  ZcL ,
  ZcL ,  ZCL ,  ZWL ,  ZDL ,  cL  ,  cLCL,  cLWL,  cLDL,
  CL  ,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* ZCL */
  0   ,  DH  ,  DHWH,  DHCL,  DHCL,  DHCL,  DHCL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCL,  WHCL,  WHCL,  WHCL,  WHCL,
  WHWL,  WHDL,  CHCL,  CHCL,  CHCL,  CHCL,  CHCL,  CHWL,
  CHDL,  cHCL,  cHCL,  cHCL,  cHCL,  cHWL,  cHDL,  ZCL ,
  ZCL ,  ZCL ,  ZWL ,  ZDL ,  cLCL,  cLCL,  cLWL,  cLDL,
  CL  ,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* ZWL */
  0   ,  DH  ,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,
  DHWL,  DHDL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,
  WHWL,  WHDL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,
  CHDL,  cHWL,  cHWL,  cHWL,  cHWL,  cHWL,  cHDL,  ZWL ,
  ZWL ,  ZWL ,  ZWL ,  ZDL ,  cLWL,  cLWL,  cLWL,  cLDL,
  CLWL,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* ZDL */
  0   ,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,
  WHDL,  WHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,
  CHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,  ZDL ,
  ZDL ,  ZDL ,  ZDL ,  ZDL ,  cLDL,  cLDL,  cLDL,  cLDL,
  CLDL,  CLDL,  CLDL,  WLDL,  WLDL,  DL  ,
/* cL */
  0   ,  DH  ,  DHWH,  DHCH,  DHcL,  DHcL,  DHcL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCH,  WHcL,  WHcL,  WHcL,  WHCL,
  WHWL,  WHDL,  CH  ,  CHcL,  CHcL,  CHcL,  CHCL,  CHWL,
  CHDL,  cHcL,  cHcL,  cHcL,  cHCL,  cHWL,  cHDL,  cL  ,
  cL  ,  cLCL,  cLWL,  cLDL,  cL  ,  cLCL,  cLWL,  cLDL,
  CL  ,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* cLCL */
  0   ,  DH  ,  DHWH,  DHCL,  DHCL,  DHCL,  DHCL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCL,  WHCL,  WHCL,  WHCL,  WHCL,
  WHWL,  WHDL,  CHCL,  CHCL,  CHCL,  CHCL,  CHCL,  CHWL,
  CHDL,  cHCL,  cHCL,  cHCL,  cHCL,  cHWL,  cHDL,  cLCL,
  cLCL,  cLCL,  cLWL,  cLDL,  cLCL,  cLCL,  cLWL,  cLDL,
  CL  ,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* cLWL */
  0   ,  DH  ,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,
  DHWL,  DHDL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,
  WHWL,  WHDL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,
  CHDL,  cHWL,  cHWL,  cHWL,  cHWL,  cHWL,  cHDL,  cLWL,
  cLWL,  cLWL,  cLWL,  cLDL,  cLWL,  cLWL,  cLWL,  cLDL,
  CLWL,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* cLDL */
  0   ,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,
  WHDL,  WHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,
  CHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cHDL,  cLDL,
  cLDL,  cLDL,  cLDL,  cLDL,  cLDL,  cLDL,  cLDL,  cLDL,
  CLDL,  CLDL,  CLDL,  WLDL,  WLDL,  DL  ,
/* CL */
  0   ,  DH  ,  DHWH,  DHCL,  DHCL,  DHCL,  DHCL,  DHCL,
  DHWL,  DHDL,  WH  ,  WHCL,  WHCL,  WHCL,  WHCL,  WHCL,
  WHWL,  WHDL,  CHCL,  CHCL,  CHCL,  CHCL,  CHCL,  CHWL,
  CHDL,  CL  ,  CL  ,  CL  ,  CL  ,  CLWL,  CLDL,  CL  ,
  CL  ,  CL  ,  CLWL,  CLDL,  CL  ,  CL  ,  CLWL,  CLDL,
  CL  ,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* CLWL */
  0   ,  DH  ,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,
  DHWL,  DHDL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,
  WHWL,  WHDL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,  CHWL,
  CHDL,  CLWL,  CLWL,  CLWL,  CLWL,  CLWL,  CLDL,  CLWL,
  CLWL,  CLWL,  CLWL,  CLDL,  CLWL,  CLWL,  CLWL,  CLDL,
  CLWL,  CLWL,  CLDL,  WL  ,  WLDL,  DL  ,
/* CLDL */
  0   ,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,
  WHDL,  WHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,  CHDL,
  CHDL,  CLDL,  CLDL,  CLDL,  CLDL,  CLDL,  CLDL,  CLDL,
  CLDL,  CLDL,  CLDL,  CLDL,  CLDL,  CLDL,  CLDL,  CLDL,
  CLDL,  CLDL,  CLDL,  WLDL,  WLDL,  DL  ,
/* WL */
  0   ,  DH  ,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,  DHWL,
  DHWL,  DHDL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,  WHWL,
  WHWL,  WHDL,  WL  ,  WL  ,  WL  ,  WL  ,  WL  ,  WL  ,
  WLDL,  WL  ,  WL  ,  WL  ,  WL  ,  WL  ,  WLDL,  WL  ,
  WL  ,  WL  ,  WL  ,  WLDL,  WL  ,  WL  ,  WL  ,  WLDL,
  WL  ,  WL  ,  WLDL,  WL  ,  WLDL,  DL  ,
/* WLDL */
  0   ,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,  WHDL,
  WHDL,  WHDL,  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,
  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,
  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,
  WLDL,  WLDL,  WLDL,  WLDL,  WLDL,  DL  ,
/* DL */
  0   ,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,  DHDL,
  DHDL,  DHDL,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,
  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,
  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,
  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,
  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,  DL  ,
};
