OSRCS =	access.c binsim.c cad_dir.c config.c fio.c hist_io.c mem.c \
	prints.c sim.c tpos.c

LSRCS =	h2a.c Hist.c Nsubrs.c sort.c

SRCS =	$(LSRCS) $(OSRCS)

OBJS =	access.o binsim.o cad_dir.o config.o fio.o h2a.o Hist.o hist_io.o \
	Nsubrs.o mem.o prints.o sim.o sort.o tpos.o

CC	= cc
MAKEDEP	= ../../utils/makedep
EXPORT	= ../../utils/export

# sources included in the above SRCS
ISRCS =	nsubrs.c

CDIRS = -I../../include
LIBS =

OPTIM = -O
DBX = -g

CSW =	$(OPTIM)
#CSW = $(DBX)

EXPORTS = globals.h

h2a : MAKEFILE $(EXPORTS) $(SRCS)
	make -f MAKEFILE CSW="$(CSW)" h2a

$(EXPORTS) : $(SRCS) $(ISRCS)
	$(EXPORT) $(SRCS) $(ISRCS) globals.h

$(OSRCS) $(ISRCS) :
	ln -s ../../irsim/$@ .

$(LSRCS) :
	co $@

MAKEFILE : Makefile $(SRCS) $(ISRCS)
	@echo making MAKEFILE
	@echo SRCS = $(SRCS) > MAKEFILE
	@echo OBJS = $(OBJS) >> MAKEFILE
	@echo HDSR = $(HDRS) >> MAKEFILE
	@echo CSW = $(CSW) >> MAKEFILE
	@echo CDIRS = $(CDIRS) >> MAKEFILE
	@echo LIBS = $(LIBS) >> MAKEFILE
	@echo CFLAGS = '$$(CDIRS) $$(CSW) -DCAP_ADJUST_LIMIT=1e8' >> MAKEFILE
	@echo 'h2a : $$(OBJS)' >> MAKEFILE
	@echo '	cc $$(CFLAGS) -o h2a $$(OBJS) $(LIBS)' >> MAKEFILE
	@echo '$(OSRCS) $(ISRCS): ' >> MAKEFILE
	@echo '	ln -s ../../irsim/$$@ .' >> MAKEFILE
	@echo '$(LSRCS) : ' >> MAKEFILE
	@echo '	co $$@' >> MAKEFILE
	@/bin/csh ../../utils/CO $(LSRCS)
	@$(MAKEDEP) -I ../../include -N -o - $(SRCS) >> MAKEFILE

clean :
	rm -f MAKEFILE $(EXPORTS) $(OSRCS) $(ISRCS) $(OBJS)
