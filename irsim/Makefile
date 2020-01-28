MODULE = irsim
IRSIMDIR = ..
SRCS =	irsim.c

include ${IRSIMDIR}/defs.mak

EXTRA_LIBS = ${IRSIMDIR}/analyzer/libanalyzer.o \
	     ${IRSIMDIR}/base/libbase.o \
	     ${MAIN_EXTRA_LIBS}

DFLAGS += -DIRSIM_DATE="\"`date`\""

CFLAGS += -I${IRSIMDIR}/base
LIBS += ${GR_LIBS} -lm
CLEANS += genspktbl gentbl tclirsim${SHDLIB_EXT}
SRCS += ${OTHER_SRCS}

main: irsim gentbl genspktbl

tcl-main: tclirsim${SHDLIB_EXT}

tclirsim${SHDLIB_EXT}: ${EXTRA_LIBS}
	@echo --- making irsim Tcl library \(tclirsim${SHDLIB_EXT}\)
	${RM} tclirsim${SHDLIB_EXT}
	${CC} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} -o $@ ${LDDL_FLAGS} ${LD_RUN_PATH} \
		${EXTRA_LIBS} -lc ${LIBS} ${LD_EXTRA_LIBS}

$(DESTDIR)${INSTALL_BINDIR}/irsim:
	${RM} $(DESTDIR)${INSTALL_BINDIR}/irsim
	${CP} irsim $(DESTDIR)${INSTALL_BINDIR}/irsim

$(DESTDIR)${INSTALL_BINDIR}/gentbl:
	${RM} $(DESTDIR)${INSTALL_BINDIR}/gentbl
	${CP} gentbl $(DESTDIR)${INSTALL_BINDIR}/gentbl

$(DESTDIR)${INSTALL_BINDIR}/genspktbl:
	${RM} $(DESTDIR)${INSTALL_BINDIR}/genspktbl
	${CP} genspktbl $(DESTDIR)${INSTALL_BINDIR}/genspktbl

gentbl:
	${CC} ${CFLAGS} ${CPPFLAGS} ${DFLAGS} gentbl.c -o $@ ${LIBS}

genspktbl:
	${CC} ${CFLAGS} ${CPPFLAGS} ${DFLAGS} genspktbl.c -o $@ ${LIBS}

install: $(DESTDIR)${INSTALL_BINDIR}/irsim $(DESTDIR)${INSTALL_BINDIR}/gentbl \
		$(DESTDIR)${INSTALL_BINDIR}/genspktbl

install-tcl: $(DESTDIR)${INSTALL_TCLDIR}/tclirsim${SHDLIB_EXT}

$(DESTDIR)${INSTALL_TCLDIR}/tclirsim${SHDLIB_EXT}: tclirsim${SHDLIB_EXT}
	${RM} $(DESTDIR)${INSTALL_TCLDIR}/tclirsim${SHDLIB_EXT}
	${CP} tclirsim${SHDLIB_EXT} $(DESTDIR)${INSTALL_TCLDIR}/tclirsim${SHDLIB_EXT}

include ${IRSIMDIR}/rules.mak
