IRSIMDIR = ..

include ${IRSIMDIR}/defs.mak

MANDIRS = $(DESTDIR)${MANDIR} \
	  $(DESTDIR)${MANDIR}/man1 \
	  $(DESTDIR)${MANDIR}/man3 \
	  $(DESTDIR)${MANDIR}/man5
DOCDIRS = $(DESTDIR)${DOCDIR}

MANFILES = $(DESTDIR)${MANDIR}/man3/irsim-analyzer.3 \
	   $(DESTDIR)${MANDIR}/man1/irsim.1 \
	   $(DESTDIR)${MANDIR}/man5/netchange.5 

DOCFILES = $(DESTDIR)${DOCDIR}/irsim-analyzer.doc \
	   $(DESTDIR)${DOCDIR}/irsim.doc \
	   $(DESTDIR)${DOCDIR}/netchange.doc \
	   $(DESTDIR)${DOCDIR}/powerEst.doc

install: ${MANDIRS} ${MANFILES} ${DOCDIRS} ${DOCFILES}

install-tcl: ${MANDIRS} ${MANFILES} ${DOCDIRS} ${DOCFILES}

${MANDIRS}: make-man-dirs

make-man-dirs:
	${SCRIPTS}/mkdirs ${MANDIRS}

${DOCDIRS}: make-doc-dirs

make-doc-dirs:
	${SCRIPTS}/mkdirs ${DOCDIRS}

$(DESTDIR)${MANDIR}/man1/%: % ${MANDIRS}
	${CP} $* $(DESTDIR)${MANDIR}/man1/$*

$(DESTDIR)${MANDIR}/man3/%: % ${MANDIRS}
	${CP} $* $(DESTDIR)${MANDIR}/man3/$*

$(DESTDIR)${MANDIR}/man5/%: % ${MANDIRS}
	${CP} $* $(DESTDIR)${MANDIR}/man5/$*

$(DESTDIR)${DOCDIR}/%: % ${DOCDIRS}
	${CP} $* $(DESTDIR)${DOCDIR}/$*
