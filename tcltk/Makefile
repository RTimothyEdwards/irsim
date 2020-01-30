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
	$(DESTDIR)${INSTALL_TCLDIR}/tkcon.tcl \
	$(DESTDIR)${INSTALL_TCLDIR}/console.tcl \
	$(DESTDIR)${INSTALL_TCLDIR}/vcd.tcl \
	$(DESTDIR)${INSTALL_TCLDIR}/irsim.tcl

tcl-main: irsimexec irsim.tcl irsim.sh

install-tcl: irsimexec $(DESTDIR)${INSTALL_BINDIR}/irsim.sh ${TCL_FILES}
	${RM} $(DESTDIR)${INSTALL_TCLDIR}/irsimexec
	${CP} irsimexec $(DESTDIR)${INSTALL_TCLDIR}/irsimexec

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

$(DESTDIR)${INSTALL_TCLDIR}/%: %
	${RM} $(DESTDIR)${INSTALL_TCLDIR}/$*
	${CP} $* $(DESTDIR)${INSTALL_TCLDIR}/$*

$(DESTDIR)${INSTALL_BINDIR}/irsim.sh: irsim.sh
	${RM} $(DESTDIR)${INSTALL_BINDIR}/irsim.sh $(DESTDIR)${INSTALL_BINDIR}/irsim
	${CP} irsim.sh $(DESTDIR)${INSTALL_BINDIR}/irsim
	(cd $(DESTDIR)${INSTALL_BINDIR}; chmod 0755 irsim)


include ${IRSIMDIR}/rules.mak
