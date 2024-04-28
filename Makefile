#
# Makefile redesigned for irsim-9.7
#

IRSIMDIR = .
PROGRAMS = irsim
LIBRARIES =
MODULES  = base analyzer

MAKEFLAGS  =
INSTALL_CAD_DIRS = lib doc

-include defs.mak

all:	$(ALL_TARGET)

standard:
	@echo --- errors and warnings logged in file make.log
	@${MAKE} mains 2>&1 | tee -a make.log

tcl:
	@echo --- errors and warnings logged in file make.log
	@${MAKE} tcllibrary 2>&1 | tee -a make.log

force:	clean all

defs.mak:
	@echo No \"defs.mak\" file found.  Run "configure" to make one.

config:
	${IRSIMDIR}/configure

tcllibrary: modules
	@echo --- making Tcl shared-object libraries
	for dir in ${PROGRAMS}; do \
		${MAKE} -C $$dir tcl-main; done

mains: modules
	@echo --- making main programs
	for dir in ${PROGRAMS}; do \
		${MAKE} -C $$dir main; done

modules:
	@echo --- making modules
	for dir in ${MODULES}; do \
		${MAKE} -C $$dir module; done

libs:
	@echo --- making libraries
	for dir in ${LIBRARIES}; do \
		${MAKE} -C $$dir lib; done

depend:
	for dir in ${MODULES} ${PROGRAMS}; do \
		${MAKE} -C $$dir depend; done

install: $(INSTALL_TARGET)

install-irsim:
	@echo --- installing executables to $(DESTDIR)${INSTALL_BINDIR}
	@echo --- installing run-time files to $(DESTDIR)${INSTALL_LIBDIR}
	@${MAKE} install-real >> install.log

install-real: install-dirs
	for dir in ${INSTALL_CAD_DIRS}; do \
		${MAKE} -C $$dir install; done
	for dir in ${PROGRAMS}; do \
		${MAKE} -C $$dir install; done

install-tcl-dirs:
	${IRSIMDIR}/scripts/mkdirs $(DESTDIR)${INSTALL_BINDIR} $(DESTDIR)${INSTALL_MANDIR} \
		$(DESTDIR)${INSTALL_TCLDIR} $(DESTDIR)${INSTALL_PRMDIR} $(DESTDIR)${INSTALL_XBMDIR}

install-dirs:
	${IRSIMDIR}/scripts/mkdirs $(DESTDIR)${INSTALL_BINDIR} $(DESTDIR)${INSTALL_MANDIR} \
		$(DESTDIR)${INSTALL_PRMDIR}

install-tcl:
	@echo --- installing executables to $(DESTDIR)${INSTALL_BINDIR}
	@echo --- installing run-time files to $(DESTDIR)${INSTALL_LIBDIR}
	@${MAKE} install-tcl-real 2>&1 >> install.log

install-tcl-real: install-tcl-dirs
	for dir in ${INSTALL_CAD_DIRS} ${PROGRAMS}; do \
		${MAKE} -C $$dir install-tcl; done

clean:
	for dir in ${MODULES} ${PROGRAMS} ${UNUSED_MODULES}; do \
		${MAKE} -C $$dir clean; done
	${RM} *.log

distclean:
	touch defs.mak
	@${MAKE} clean
	${RM} defs.mak old.defs.mak ${IRSIMDIR}/scripts/defs.mak
	${RM} ${IRSIMDIR}/scripts/default.conf
	${RM} ${IRSIMDIR}/scripts/config.log ${IRSIMDIR}/scripts/config.status
	${RM} scripts/irsim.spec irsim-`cat VERSION` irsim-`cat VERSION`.tgz

dist:
	${RM} scripts/irsim.spec irsim-`cat VERSION` irsim-`cat VERSION`.tgz
	sed -e /@VERSION@/s%@VERSION@%`cat VERSION`% \
	    scripts/irsim.spec.in > scripts/irsim.spec
	ln -nsf . irsim-`cat VERSION`
	tar zchvf irsim-`cat VERSION`.tgz --exclude CVS \
	    --exclude irsim-`cat VERSION`/irsim-`cat VERSION` \
	    --exclude irsim-`cat VERSION`/irsim-`cat VERSION`.tgz \
	    irsim-`cat VERSION`

clean-mains:
	for dir in ${PROGRAMS}; do \
		(cd $$dir && ${RM} $$dir); done

tags:
	${RM} tags
