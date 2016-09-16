MODULE	  = tclsubckt
IRSIMDIR  = ..
SRCS      = diglib.c

include ${IRSIMDIR}/defs.mak

EXTRA_LIBS = ${MAIN_EXTRA_LIBS}

LIBS += -lm ${MAC_EXTRA_LIBS}
CFLAGS +=  -I${IRSIMDIR}/base
CLEANS +=  irsim.user diglib${SHDLIB_EXT} diglib.o

main: 

install:

tcl-main: diglib${SHDLIB_EXT}

diglib.o: diglib.c
	${CC} ${CFLAGS} ${CPPFLAGS} ${DFLAGS} diglib.c -c -o diglib.o

diglib${SHDLIB_EXT}: diglib.o
	@echo --- making diglib subcircuit shared library
	${RM} diglib${SHDLIB_EXT}
	${CC} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} -o $@ ${LDDL_FLAGS} diglib.o \
		${MAC_SHARED} -lc ${LIBS} ${LD_EXTRA_LIBS}

install-tcl: $(DESTDIR)${TCLDIR}/diglib${SHDLIB_EXT}

$(DESTDIR)${TCLDIR}/diglib${SHDLIB_EXT}: diglib${SHDLIB_EXT}
	${RM} $(DESTDIR)${TCLDIR}/diglib${SHDLIB_EXT}
	${CP} diglib${SHDLIB_EXT} $(DESTDIR)${TCLDIR}/diglib${SHDLIB_EXT}

include ${IRSIMDIR}/rules.mak
