OSRCS =	access.c cad_dir.c config.c fio.c nsubrs.c mem.c prints.c tpos.c

LSRCS = Binsim.c Hist.c Sim.c inet2sim.c

SRCS =	$(LSRCS) $(OSRCS)

OBJS =	access.o Binsim.o cad_dir.o config.o fio.o Hist.o inet2sim.o \
	nsubrs.o mem.o prints.o Sim.o tpos.o

CC	= cc
MAKEDEP	= ../../utils/makedep
EXPORT	= ../../utils/export

# sources included in the above SRCS
ISRCS =	binsim.c

CDIRS = -I../../include
LIBS =

OPTIM = -O
DBX = -g

CSW =	$(OPTIM)
#CSW = $(DBX)


EXPORTS = globals.h

inet2sim : MAKEFILE $(EXPORTS) $(SRCS) $(ISRCS)
	make -f MAKEFILE CSW="$(CSW)" inet2sim

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
	@echo CFLAGS = '$$(CDIRS) $$(CSW)' >> MAKEFILE
	@echo 'inet2sim : $$(OBJS)' >> MAKEFILE
	@echo '	cc $$(CFLAGS) -o inet2sim $$(OBJS) $(LIBS)' >> MAKEFILE
	@echo '$(OSRCS) $(ISRCS): ' >> MAKEFILE
	@echo '	ln -s ../../irsim/$$@ .' >> MAKEFILE
	@echo '$(LSRCS) : ' >> MAKEFILE
	@echo '	co $$@' >> MAKEFILE
	@/bin/csh ../../utils/CO $(LSRCS)
	@$(MAKEDEP) -I ../../include -N -o - $(SRCS) >> MAKEFILE

lint :
	lint -abhx $(CDIRS) $(SRCS) > inet2sim.lint

clean :
	rm -f MAKEFILE $(EXPORTS) $(OSRCS) $(ISRCS) $(OBJS)
