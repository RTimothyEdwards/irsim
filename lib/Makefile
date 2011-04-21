IRSIMDIR = ..

include ${IRSIMDIR}/defs.mak

FILES = \
amiabn.80.n99z.5.prm \
amic5.30.n98s.3.3.prm \
amicwl.50.n97l.5.prm \
hpcmos10.20.n98v.3.3.prm \
hpcmos14.30.n9ae.3.3.prm \
hpcmos26.40.n88x.5.prm \
orbit.60.n96e.5.prm \
scmos100.prm \
scmos1_6um.prm \
scmos2um.prm \
scmos30.prm \
scmos35.prm \
scmos50_3v.prm \
scmos50_5v.prm \
scmos60.prm \
tsmc18.10.nxxx.1.8.prm \
tsmc25.15.n99y.2.5.prm \
tsmc35.20.n98t.3.3.prm \
tsmc35p.20.n98tp.3.3.prm

BITMAPS = left.xbm right.xbm lleft.xbm rright.xbm center.xbm

INST_FILES= $(FILES:%=$(DESTDIR)${PRMDIR}/%)

INST_BITMAPS= $(BITMAPS:%=$(DESTDIR)${XBMDIR}/%)

install-tcl: ${INST_FILES} ${INST_BITMAPS}

install: ${INST_FILES}

$(DESTDIR)${PRMDIR}/%: prm/%
	(cd prm; ${CP} $* $(DESTDIR)${PRMDIR}/$*)

$(DESTDIR)${XBMDIR}/%: bitmaps/%
	(cd bitmaps; ${CP} $* $(DESTDIR)${XBMDIR}/$*)
