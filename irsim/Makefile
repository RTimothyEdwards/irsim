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
	${CC} ${CFLAGS} ${CPPFLAGS} -o $@ ${LDDL_FLAGS} ${LD_RUN_PATH} \
		${EXTRA_LIBS} -lc ${LIBS} ${LD_EXTRA_LIBS}

$(DESTDIR)${BINDIR}/irsim:
	${RM} $(DESTDIR)${BINDIR}/irsim
	${CP} irsim $(DESTDIR)${BINDIR}/irsim

$(DESTDIR)${BINDIR}/gentbl:
	${RM} $(DESTDIR)${BINDIR}/gentbl
	${CP} gentbl $(DESTDIR)${BINDIR}/gentbl

$(DESTDIR)${BINDIR}/genspktbl:
	${RM} $(DESTDIR)${BINDIR}/genspktbl
	${CP} genspktbl $(DESTDIR)${BINDIR}/genspktbl

gentbl:
	${CC} ${CFLAGS} ${CPPFLAGS} ${DFLAGS} gentbl.c -o $@ ${LIBS}

genspktbl:
	${CC} ${CFLAGS} ${CPPFLAGS} ${DFLAGS} genspktbl.c -o $@ ${LIBS}

install: $(DESTDIR)${BINDIR}/irsim $(DESTDIR)${BINDIR}/gentbl \
		$(DESTDIR)${BINDIR}/genspktbl

install-tcl: $(DESTDIR)${TCLDIR}/tclirsim${SHDLIB_EXT}

$(DESTDIR)${TCLDIR}/tclirsim${SHDLIB_EXT}: tclirsim${SHDLIB_EXT}
	${RM} $(DESTDIR)${TCLDIR}/tclirsim${SHDLIB_EXT}
	${CP} tclirsim${SHDLIB_EXT} $(DESTDIR)${TCLDIR}/tclirsim${SHDLIB_EXT}

include ${IRSIMDIR}/rules.mak
