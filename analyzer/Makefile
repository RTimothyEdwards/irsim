MODULE = analyzer
IRSIMDIR = ..

NULL_SRCS = noana.c
X11_SRCS = coords.c graphics.c movetrace.c setsteps.c wm.c \
   analyzer.c defaults.c icon.c namelen.c textwind.c zoom.c base.c \
   deltaT.c menu.c postscript.c update.c convert.c event.c movetot.c \
   scrollbar.c window.c
TK_SRCS = coords.c graphics.c movetrace.c setsteps.c \
   analyzer.c defaults.c namelen.c textwind.c zoom.c base.c \
   deltaT.c postscript.c update.c convert.c movetot.c tkana.c \
   window.c
TOGL_SRCS =

include ${IRSIMDIR}/defs.mak

X11HELPER_SRCS = anXhelper.c
X11HELP_PROG = anXhelper

SRCS = ${GR_SRCS}
OBJS = ${GR_SRCS:.c=.o}
CFLAGS += ${GR_CFLAGS} -I${IRSIMDIR}/base
DFLAGS += ${GR_DFLAGS}
CLEANS += ${GR_HELPER_PROG}
LIBS += ${GR_LIBS}

main: ${GR_HELPER_PROG}

${GR_HELPER_PROG}: ${GR_HELPER_SRCS}
	@echo --- building main ${GR_HELPER_PROG}
	${RM} ${GR_HELPER_PROG}
	${CC} ${CFLAGS} ${CPPFLAGS} ${DFLAGS} ${GR_HELPER_SRCS} -o ${GR_HELPER_PROG} ${LIBS}

install: $(DESTDIR)${BINDIR}/${GR_HELPER_PROG}

install-tcl:
	@echo --- Nothing to do for analyzer in Tcl version.

$(DESTDIR)${BINDIR}/${GR_HELPER_PROG}: ${GR_HELPER_PROG}
	${RM} $(DESTDIR)${BINDIR}/${GR_HELPER_PROG}
	${CP} ${GR_HELPER_PROG} $(DESTDIR)${BINDIR}

include ${IRSIMDIR}/rules.mak
