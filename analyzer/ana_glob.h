#ifndef _ANA_GLOB_H
#define _ANA_GLOB_H

#ifdef TCL_IRSIM
  #ifndef _TCL
    #include <tcl.h>
  #endif /* (_TCL) */
#endif

	/* EXPORTS FROM analyzer.c */

extern char *SetName( /*  n */ );
extern int AddNode( /*  nd, flag */ );
extern int AddVector( /*  vec, flag */ );
extern int OffsetNode( /*  nd, flag */ );
extern int OffsetVector( /*  vec, flag */ );
extern void DisplayTraces( /*  isMapped */ );
extern void StopAnalyzer( /* */ );
extern void RestartAnalyzer( /*  first_time, last_time, same_hist */ );
extern void ClearTraces( /* */ );
extern void RemoveVector( /*  b */ );
extern void RemoveNode( /*  n */ );
extern void RemoveAllDeleted( /* */ );

	/* EXPORTS FROM base.c */

extern void ChangeBase( /*  base */ );

	/* EXPORTS FROM convert.c */

extern char *HistToStr( /*  hist, nbits, b_digit, offset */ );

	/* EXPORTS FROM coords.c */

extern int     XWINDOWSIZE ;
extern int     YWINDOWSIZE ;
extern BBox    traceBox;
extern BBox    bannerBox;
extern BBox    scrollBox;
extern BBox    barPos;
extern BBox    namesBox;
extern BBox    cursorBox;
extern BBox    timesBox;
extern BBox    textBox;
extern BBox    iconBox;
extern BBox    sizeBox;
extern BBox    selectBox;
extern BBox    menuBox;
extern void SetSignalPos( /* */ );
extern void GetMinWsize( /*  w, h */ );
#define	NTRACE_CHANGE		0x01	/* # of visible traces changed */
#define	WIDTH_CHANGE		0x02	/* change in width of trace window */
#define	HEIGHT_CHANGE		0x04	/* change in height of trace window */
#define	RESIZED			0x10	/* window is too small,should resize */
extern int WindowChanges( /* */ );

	/* EXPORTS FROM deltaT.c */

extern void DeltaT( /*  s */ );

	/* EXPORTS FROM event.c */

extern void SendEventTo( /*  f */ );
extern int InitHandler( /*  fd */ );
extern void DisableInput( /* */ );
extern void EnableInput( /* */ );
extern void DisableAnalyzer( /* */ );
extern void EnableAnalyzer( /* */ );
extern void TerminateAnalyzer( /* */ );

	/* EXPORTS FROM defaults.c */

#define	DEFL_GEOM	0
#define	DEFL_BG		1
#define	DEFL_FG		2
#define	DEFL_RV		3
#define	DEFL_HIGL	4
#define	DEFL_TRCOLOR	5
#define	DEFL_BANN_BG	6
#define	DEFL_BANN_FG	7
#define	DEFL_BDRCOLOR	8
#define	DEFL_FONT	9
#define	DEFL_BDRWIDTH	10
extern char *GetXDefault( /*  key */ );
extern int IsDefault( /*  key, val */ );
extern char *ProgDefault( /*  key */ );

	/* EXPORTS FROM graphics.c */

extern PIX       pix;
extern GCS       gcs;
extern COL       colors;
extern CURS      cursors;
extern void InitGraphics( /*  font */ );

	/* EXPORTS FROM icon.c */

extern Pixmap InitIconBitmap( /* */ );
extern void RedrawIcon( /*  exv */ );
extern Window CreateIconWindow( /*  x, y */ );

	/* EXPORTS FROM menu.c */

extern Menu menu[ /**/ ] ;
extern void InitMenus( /* */ );
extern void DoMenu( /*  x, y */ );

	/* EXPORTS FROM movetot.c */

extern void MoveToTime( /*  s */ );

	/* EXPORTS FROM movetrace.c */

extern Trptr    selectedTrace ;
extern Trptr GetYTrace( /*  y */ );
extern void UnderlineTrace( /*  t, color */ );
extern void MoveTrace( /*  y */ );
extern void SelectCursTrace( /*  y */ );

	/* EXPORTS FROM namelen.c */

extern int  max_name_len ;
extern void SetNameLen( /*  s */ );

	/* EXPORTS FROM postscript.c */

extern void SetPSParms( /*  s */ );
extern void printPS( /*  s */ );

	/* EXPORTS FROM setsteps.c */

extern void SetWidth( /*  s */ );

	/* EXPORTS FROM scrollbar.c */

#ifdef TCL_IRSIM
#define	SCROLLBARHEIGHT		0
#define	ARROW_WIDTH		0
#else
#define	SCROLLBARHEIGHT		16
#define	ARROW_WIDTH		32
#endif

extern void DrawScrollBar( /*  isExpose */ );
extern void UpdateScrollBar( /* */ );
extern void DoScrollBar( /*  ev */ );

	/* EXPORTS FROM textwind.c */

extern void RedrawText( /* */ );
extern void PRINT( /*  s */ );
extern void PRINTF(char *, ...);	    /* VARARGS requires prototypes! */
extern void Query( /*  prompt, func */ );

	/* EXPORTS FROM update.c */

extern int         autoScroll ;
extern int         updatePending ;
extern TimeType    updPendTime;
extern int         freezeWindow ;
extern void ScrollUpdate( /*  s */ );
extern void DisableScroll( /* */ );
extern void RestoreScroll( /* */ );

	/* EXPORTS FROM window.c */

extern Display     *display ;
extern Screen      *screen;
extern Window      window ;
extern Window      iconW ;
extern int         CHARHEIGHT ;
extern int         CHARWIDTH ;
extern int	    descent;
extern Times       tims;
extern Traces      traces;
extern Wstate      windowState ;
extern char        *banner;
extern int         bannerLen;
extern Coord TimeToX( /* t */ );
extern TimeType XToTime( /*  x */ );
extern int InitDisplay( /*  fname, display_unit */ );
extern int InitWindow( /*  firstTime, state, x, y, w, h, ix, iy */ );
extern int SetFont();

#define	DEF_STEPS	4	/* default simulation steps per screen */
extern void InitTimes( /*  firstT, stepsize, lastT, reInit */ );
extern void RedrawWindow( /*  box */ );
extern void RedrawBanner( /* */ );
extern void WindowCrossed( /*  selected */ );
extern void RedrawSmallW( /* */ );
extern void RedrawTimes( /* */ );
extern void UpdateTimes( /*  start, end */ );
extern void RedrawNames( /*  rb */ );
extern void RedrawTraces( /*  box */ );
extern void UpdateTraceCache( /*  first_trace */ );
extern void FlushTraceCache( /* */ );
extern void DrawTraces( /*  t1, t2 */ );
extern hptr    tmpHBuff[ /* 400 */ ];
extern void UpdateWindow( /*  endT */ );
extern void DoCursor( /*  ev */ );
extern void DrawCursVal( /*  rb */ );
extern void ExpandCursVal( /*  t */ );

	/* EXPORTS FROM wm.c */

extern void GetWindowGeometry( /*  w, x, y, width, height, border */ );
extern void GrabMouse( /*  w, ev_mask, cursor */ );
extern void ResizeMe( /* */ );
extern void MoveMe( /*  x, y */ );
extern void IconifyMe( /* */ );

	/* EXPORTS FROM zoom.c */

extern void Zoom( /*  what */ );

	/* TCL Exports */

#ifdef TCL_IRSIM

extern float analyzer_time_start();
extern float analyzer_time_end();
extern float analyzer_time_left();
extern float analyzer_time_right();
extern float analyzer_time_step();
extern float analyzer_time_marker();
extern float analyzer_time_delta();
extern float analyzer_time_cursor();

extern int analyzer_trace_top();
extern int analyzer_trace_bottom();
extern int analyzer_trace_order();
extern int analyzer_trace_base();
extern char *analyzer_trace_class();
extern char *analyzer_trace_cursor();

extern Tcl_Obj *analyzer_list_vectors();
extern Tcl_Obj *analyzer_list_nodes();
extern Tcl_Obj *analyzer_list_all();

#endif /* (TCL_IRSIM) */

#endif /* _ANA_GLOB_H */
