MODULE = tcltk
IRSIMDIR = ..
SRCS =	tclirsim.c tclanalyzer.c lookup.c
TK_SRCS = tkAnalyzer.c tkTag.c

include ${IRSIMDIR}/defs.mak

EXTRA_LIBS = ${MAIN_EXTRA_LIBS}

DFLAGS += -DIRSIM_DATE="\"`date`\""
LIBS += ${GR_LIBS} -lm
CLEANS += irsim.sh irsim.tcl irsimexec
SRCS += ${GR_SRCS}
CFLAGS += -I${IRSIMDIR}/base -I${IRSIMDIR}/analyzer

TCL_FILES = \
	$(DESTDIR)${TCLDIR}/tkcon.tcl \
	$(DESTDIR)${TCLDIR}/console.tcl \
	$(DESTDIR)${TCLDIR}/vcd.tcl \
	$(DESTDIR)${TCLDIR}/irsim.tcl

tcl-main: irsimexec irsim.tcl irsim.sh

install-tcl: irsimexec $(DESTDIR)${BINDIR}/irsim.sh ${TCL_FILES}
	${RM} $(DESTDIR)${TCLDIR}/irsimexec
	${CP} irsimexec $(DESTDIR)${TCLDIR}/irsimexec

irsimexec: irsimexec.c tclirsim.o
	${CC} ${CFLAGS} ${CPPFLAGS} ${DFLAGS} ${LDFLAGS} irsimexec.c -o irsimexec \
		${LIBS} ${LIB_SPECS}

irsim.tcl: irsim.tcl.in
	sed -e /TCL_DIR/s%TCL_DIR%${TCLDIR}%g \
	    -e /SHDLIB_EXT/s%SHDLIB_EXT%${SHDLIB_EXT}%g \
	    irsim.tcl.in > irsim.tcl

irsim.sh: irsim.sh.in
	sed -e /TCL_DIR/s%TCL_DIR%${TCLDIR}%g \
	    -e /TCLLIB_DIR/s%TCLLIB_DIR%${TCL_LIB_DIR}%g \
	    -e /WISH_EXE/s%WISH_EXE%${WISH_EXE}%g \
	    irsim.sh.in > irsim.sh

$(DESTDIR)${TCLDIR}/%: %
	${RM} $(DESTDIR)${TCLDIR}/$*
	${CP} $* $(DESTDIR)${TCLDIR}/$*

$(DESTDIR)${BINDIR}/irsim.sh: irsim.sh
	${RM} $(DESTDIR)${BINDIR}/irsim.sh $(DESTDIR)${BINDIR}/irsim
	${CP} irsim.sh $(DESTDIR)${BINDIR}/irsim
	(cd $(DESTDIR)${BINDIR}; chmod 0755 irsim)


include ${IRSIMDIR}/rules.mak
