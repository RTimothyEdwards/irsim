MODULE = random
IRSIMDIR = ..
SRCS = random.c

include ${IRSIMDIR}/defs.mak

EXTRA_LIBS = ${MAIN_EXTRA_LIBS}

LIBS += -lm ${MAC_EXTRA_LIBS}
CLEANS += random${SHDLIB_EXT} randwrap.o

main:

install:

tcl-main: random${SHDLIB_EXT}

randwrap.o: random.c
	${CC} ${CFLAGS} ${CPPFLAGS} ${DFLAGS} random.c -c -o randwrap.o
	
random${SHDLIB_EXT}: randwrap.o
	@echo --- making random Tcl library
	${RM} random${SHDLIB_EXT}
	${CC} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} -o $@ ${LDDL_FLAGS} randwrap.o \
		${MAC_SHARED} -lc ${LIBS} ${LD_EXTRA_LIBS}

install-tcl: $(DESTDIR)${TCLDIR}/random${SHDLIB_EXT}

$(DESTDIR)${TCLDIR}/random${SHDLIB_EXT}: random${SHDLIB_EXT}
	${RM} $(DESTDIR)${TCLDIR}/random${SHDLIB_EXT}
	${CP} random${SHDLIB_EXT} $(DESTDIR)${TCLDIR}/random${SHDLIB_EXT}

include ${IRSIMDIR}/rules.mak
