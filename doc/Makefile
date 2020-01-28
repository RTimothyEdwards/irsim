IRSIMDIR = ..

include ${IRSIMDIR}/defs.mak

MANDIRS = $(DESTDIR)${INSTALL_MANDIR} \
	  $(DESTDIR)${INSTALL_MANDIR}/man1 \
	  $(DESTDIR)${INSTALL_MANDIR}/man3 \
	  $(DESTDIR)${INSTALL_MANDIR}/man5
DOCDIRS = $(DESTDIR)${INSTALL_DOCDIR}

MANFILES = $(DESTDIR)${INSTALL_MANDIR}/man3/irsim-analyzer.3 \
	   $(DESTDIR)${INSTALL_MANDIR}/man1/irsim.1 \
	   $(DESTDIR)${INSTALL_MANDIR}/man5/netchange.5 

DOCFILES = $(DESTDIR)${INSTALL_DOCDIR}/irsim-analyzer.doc \
	   $(DESTDIR)${INSTALL_DOCDIR}/irsim.doc \
	   $(DESTDIR)${INSTALL_DOCDIR}/netchange.doc \
	   $(DESTDIR)${INSTALL_DOCDIR}/powerEst.doc

install: ${MANDIRS} ${MANFILES} ${DOCDIRS} ${DOCFILES}

install-tcl: ${MANDIRS} ${MANFILES} ${DOCDIRS} ${DOCFILES}

${MANDIRS}: make-man-dirs

make-man-dirs:
	${SCRIPTS}/mkdirs ${MANDIRS}

${DOCDIRS}: make-doc-dirs

make-doc-dirs:
	${SCRIPTS}/mkdirs ${DOCDIRS}

$(DESTDIR)${INSTALL_MANDIR}/man1/%: % ${MANDIRS}
	${CP} $* $(DESTDIR)${INSTALL_MANDIR}/man1/$*

$(DESTDIR)${INSTALL_MANDIR}/man3/%: % ${MANDIRS}
	${CP} $* $(DESTDIR)${INSTALL_MANDIR}/man3/$*

$(DESTDIR)${INSTALL_MANDIR}/man5/%: % ${MANDIRS}
	${CP} $* $(DESTDIR)${INSTALL_MANDIR}/man5/$*

$(DESTDIR)${INSTALL_DOCDIR}/%: % ${DOCDIRS}
	${CP} $* $(DESTDIR)${INSTALL_DOCDIR}/$*
