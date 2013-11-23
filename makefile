####### Output directory
LIBSDIR   = ./libs
BINDIR    = ./bin

CLOG=logfile.a
CLOG_OBJ=logfile.o

SLOG=simplifylog.a
SLOG_OBJ=simplifylog.o

TESTBIN=test

all: ${LIBSDIR} ${BINDIR} $(CLOG) $(SLOG) $(TESTBIN)

${LIBSDIR}:
	mkdir ${LIBSDIR}

${BINDIR}:
	mkdir ${BINDIR}

clear:
	rm -f *.o
	rm -f *.a
	@if [ -d ${LIBSDIR} ];  \
          then \
            rm -rf ${LIBSDIR}/*; \
          fi
	@if [ -d ${BINDIR} ];  \
          then \
            rm -rf ${BINDIR}/*; \
          fi


INCS   = -I./
DFLAGS  =
LFLAGS = 
DEP_INCS = 

$(CLOG_OBJ):	logfile.cxx  $(DEP_INCS)
	g++ -c -g -O2 -lpthread -D__FLUSHLOG__ -D_STREAM_COMPAT $(INCS) $<

$(CLOG):	$(CLOG_OBJ)
	ar -r $(CLOG) $(CLOG_OBJ) 
	mv -f *.o ${LIBSDIR}
	mv -f *.a ${LIBSDIR}

$(SLOG_OBJ):	simplifylog.cxx  $(DEP_INCS)
	g++ -c -g -lpthread -D__FLUSHLOG__ -D_STREAM_COMPAT $(INCS) $<

$(SLOG):	$(SLOG_OBJ)
	ar -r $(SLOG) $(SLOG_OBJ)
	mv -f *.o ${LIBSDIR}
	mv -f *.a ${LIBSDIR}

$(TESTBIN):	test.cxx  $(DEP_INCS)
	g++ -I$(INCS) -lpthread -Wall test.cxx -g -o $(BINDIR)/$(TESTBIN) $(LIBSDIR)/*.o 

r: clear all
