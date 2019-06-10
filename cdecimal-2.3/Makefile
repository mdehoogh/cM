
# ==============================================================================
#                          Unix Makefile for libmpdec
# ==============================================================================

include vars.mk

PACKAGE_TARNAME = mpdecimal
LIBSTATIC = libmpdec.a
LIBSONAME = libmpdec.so.2.3
LIBSHARED = libmpdec.so.2.3

CC = gcc
LD = gcc
AR = ar
GCOV ?= gcov
prefix = /usr/local
exec_prefix = ${prefix}
includedir = ${prefix}/include
libdir = ${exec_prefix}/lib
datarootdir = ${prefix}/share
docdir = ${datarootdir}/doc/${PACKAGE_TARNAME}

MPD_HEADER = 
MPD_WARN = -Wall -W -Wno-unknown-pragmas
MPD_CONFIG = -DCONFIG_64 -DASM
MPD_OPT = -O2 -fpic -s

MPD_CCOV = -O0 -g -fno-inline -fprofile-arcs -ftest-coverage -fpic
MPD_LDCOV = -fprofile-arcs
MPD_PGEN = -fprofile-generate -fprofile-values
MPD_PUSE = -fprofile-use -freorder-blocks
MPD_PREC = 19
MPD_DPREC = 38

ifneq (, $(findstring debug, $(MAKECMDGOALS)))
  CFLAGS = $(MPD_WARN) $(MPD_CONFIG) -O0 -g -fpic
endif
ifeq ($(MAKECMDGOALS), check)
  ifdef SAVED_CFLAGS
    CFLAGS = $(SAVED_CFLAGS)
  endif
  ifdef SAVED_LDFLAGS
    LDFLAGS = $(SAVED_LDFLAGS)
  endif
endif
ifeq ($(MAKECMDGOALS), libcoverage)
  CFLAGS = $(MPD_WARN) $(MPD_CONFIG) $(MPD_CCOV)
  LDFLAGS = $(MPD_LDCOV)
endif
ifeq ($(MAKECMDGOALS), pycoverage)
  CFLAGS = $(MPD_WARN) $(MPD_CONFIG) $(MPD_CCOV)
  LDFLAGS = $(MPD_LDCOV)
endif
ifeq ($(MAKECMDGOALS), coverage)
  CFLAGS = $(MPD_WARN) $(MPD_CONFIG) $(MPD_CCOV)
  LDFLAGS = $(MPD_LDCOV)
endif

CONFIGURE_CFLAGS = -Wall -W -Wno-unknown-pragmas -DCONFIG_64 -DASM -O2 -fpic -s
CFLAGS ?= $(CONFIGURE_CFLAGS)

CONFIGURE_LDFLAGS = 
LDFLAGS ?= $(CONFIGURE_LDFLAGS)

ifeq ($(MAKECMDGOALS), profile_gen)
  CFLAGS += $(MPD_PGEN)
  LDFLAGS += $(MPD_PGEN)
endif
ifeq ($(MAKECMDGOALS), profile_use)
  CFLAGS += $(MPD_PUSE)
  LDFLAGS += $(MPD_PUSE)
endif

CC := $(strip $(CC))
CFLAGS := $(strip $(CFLAGS))
LDFLAGS := $(strip $(LDFLAGS))

NEWVARS =
ifneq ($(SAVED_CC), $(CC))
  NEWVARS = NEWVARS
endif
ifneq ($(SAVED_CFLAGS), $(CFLAGS))
  NEWVARS = NEWVARS
endif
ifneq ($(SAVED_LDFLAGS), $(LDFLAGS))
  NEWVARS = NEWVARS
endif


default: $(LIBSTATIC) $(LIBSHARED)
debug: default


OBJS := basearith.o context.o constants.o convolute.o crt.o mpdecimal.o \
        mpsignal.o difradix2.o fnt.o fourstep.o io.o memory.o numbertheory.o \
        sixstep.o transpose.o

$(LIBSTATIC): Makefile vars.mk $(OBJS)
	$(AR) rc $(LIBSTATIC) $(OBJS)
	ranlib $(LIBSTATIC)

$(LIBSHARED): Makefile vars.mk $(OBJS)
	$(LD) $(LDFLAGS) -shared -Wl,-install_name,$(LIBSONAME) -o $(LIBSHARED) $(OBJS) -lm


$(NEWVARS):

vars.mk: $(NEWVARS)
	@echo "SAVED_CC=$(CC)" > vars.mk
	@echo "SAVED_CFLAGS=$(CFLAGS)" >> vars.mk
	@echo "SAVED_LDFLAGS=$(LDFLAGS)" >> vars.mk


basearith.o:\
Makefile vars.mk basearith.c mpdecimal.h constants.h memory.h \
typearith.h basearith.h
	$(CC) $(CFLAGS) -c basearith.c

constants.o:\
Makefile vars.mk constants.c mpdecimal.h constants.h
	$(CC) $(CFLAGS) -c constants.c

context.o:\
Makefile vars.mk context.c mpdecimal.h
	$(CC) $(CFLAGS) -c context.c

convolute.o:\
Makefile vars.mk convolute.c mpdecimal.h bits.h constants.h fnt.h fourstep.h \
numbertheory.h sixstep.h umodarith.h typearith.h convolute.h vccompat.h
	$(CC) $(CFLAGS) -c convolute.c

crt.o:\
Makefile vars.mk crt.c mpdecimal.h numbertheory.h constants.h umodarith.h \
typearith.h crt.h
	$(CC) $(CFLAGS) -c crt.c

difradix2.o:\
Makefile vars.mk difradix2.c mpdecimal.h bits.h numbertheory.h constants.h \
umodarith.h typearith.h difradix2.h vccompat.h
	$(CC) $(CFLAGS) -c difradix2.c

fnt.o:\
Makefile vars.mk fnt.c bits.h mpdecimal.h difradix2.h numbertheory.h \
constants.h fnt.h vccompat.h
	$(CC) $(CFLAGS) -c fnt.c

fourstep.o:\
Makefile vars.mk fourstep.c mpdecimal.h numbertheory.h constants.h sixstep.h \
transpose.h umodarith.h typearith.h fourstep.h
	$(CC) $(CFLAGS) -c fourstep.c

io.o:\
Makefile vars.mk io.c mpdecimal.h bits.h constants.h memory.h typearith.h \
io.h vccompat.h
	$(CC) $(CFLAGS) -c io.c

memory.o:\
Makefile vars.mk memory.c mpdecimal.h typearith.h memory.h
	$(CC) $(CFLAGS) -c memory.c

mpdecimal.o:\
Makefile vars.mk mpdecimal.c basearith.h mpdecimal.h typearith.h bits.h \
convolute.h crt.h memory.h umodarith.h constants.h mptest.h mptypes.h \
vccompat.h
	$(CC) $(CFLAGS) -c mpdecimal.c

mpsignal.o:\
Makefile vars.mk mpdecimal.c mpdecimal.h vccompat.h
	$(CC) $(CFLAGS) -c mpsignal.c

numbertheory.o:\
Makefile vars.mk numbertheory.c mpdecimal.h bits.h umodarith.h constants.h \
typearith.h numbertheory.h vccompat.h
	$(CC) $(CFLAGS) -c numbertheory.c

sixstep.o:\
Makefile vars.mk sixstep.c mpdecimal.h bits.h difradix2.h numbertheory.h \
constants.h mptypes.h transpose.h umodarith.h typearith.h sixstep.h \
vccompat.h
	$(CC) $(CFLAGS) -c sixstep.c

transpose.o:\
Makefile vars.mk transpose.c mpdecimal.h bits.h constants.h typearith.h \
transpose.h vccompat.h
	$(CC) $(CFLAGS) -c transpose.c


# pycoverage
PYTHON2INC ?= -I/usr/local/include/python2.7
PYTHON2EXEC ?= /usr/local/bin/python2.7
PYTHON3INC ?= -I/usr/local/include/python3.2m
PYTHON3EXEC ?= /usr/local/bin/python3.2m

cdecimal2.o:\
Makefile vars.mk cdecimal2.c mpdecimal.h docstrings.h memory.h mptypes.h
	$(CC) $(PYTHON2INC) -Wno-missing-field-initializers $(CFLAGS) -pthread -c cdecimal2.c

cdecimal3.o:\
Makefile vars.mk cdecimal3.c mpdecimal.h docstrings.h memory.h mptypes.h
	$(CC) $(PYTHON3INC) -Wno-missing-field-initializers $(CFLAGS) -pthread -c cdecimal3.c

cdecimal2.so:\
Makefile vars.mk cdecimal2.o $(LIBSTATIC)
	$(CC) $(PYTHON2INC) $(LDFLAGS) -pthread -shared -o cdecimal2.so cdecimal2.o $(LIBSTATIC)

cdecimal3.so:\
Makefile vars.mk cdecimal3.o $(LIBSTATIC)
	$(CC) $(PYTHON3INC) $(LDFLAGS) -pthread -shared -o cdecimal3.so cdecimal3.o $(LIBSTATIC)
# end pycoverage


check: $(LIBSTATIC) FORCE
	cd tests && $(MAKE) "CFLAGS=$(CFLAGS)" "LDFLAGS=$(LDFLAGS)" && ./runshort.sh

extended:\
Makefile $(LIBSTATIC)
	cd tests && $(MAKE) extended

build_libcoverage: clean $(LIBSTATIC)
	cd tests && $(MAKE) build_libcoverage

build_pycoverage: build_libcoverage cdecimal2.so cdecimal3.so

libcoverage: build_libcoverage
	@echo ""
	@cd tests && ./runshort.sh && ./cov && ./test_transpose && ./fntcov
	@for file in *.c; do $(GCOV) -l "$$file" > /dev/null 2>&1; done
	@echo ""
	@./tests/covreport.py

pycoverage: build_pycoverage
	@echo ""
	@cd python && cp ../cdecimal2.so cdecimal.so && ./gettests.sh && $(PYTHON2EXEC) test_cdecimal2.py
	@cd python && cp ../cdecimal3.so cdecimal.so && ./gettests.sh && $(PYTHON3EXEC) test_cdecimal3.py
	@for file in *.c; do $(GCOV) -l "$$file" > /dev/null 2>&1; done
	@echo ""
	@./tests/covreport.py

coverage: build_libcoverage build_pycoverage libcoverage pycoverage


FORCE:

bench: FORCE $(LIBSTATIC)
	$(CC) $(CFLAGS) -o bench bench.c $(LIBSTATIC) -lm

profile_gen: clean bench
	./bench $(MPD_PREC) 1000
	./bench $(MPD_DPREC) 1000
	rm -f *.o *.gch $(LIBSTATIC) $(LIBSHARED) bench runtest

profile_use: bench
	./bench $(MPD_PREC) 1000
	./bench $(MPD_DPREC) 1000

profile:
	$(MAKE) profile_gen
	$(MAKE) profile_use

install: FORCE
	mkdir -p $(DESTDIR)$(includedir) && cp mpdecimal.h $(DESTDIR)$(includedir)
	mkdir -p $(DESTDIR)$(libdir) && cp $(LIBSTATIC) $(LIBSHARED) $(DESTDIR)$(libdir)
	mkdir -p $(DESTDIR)$(docdir) && cp -R doc/* $(DESTDIR)$(docdir)

clean: FORCE
	rm -f *.o *.so *.gch *.gcda *.gcno *.gcov *.dyn *.dpi *.lock \
           $(LIBSTATIC) $(LIBSHARED) bench
	rm -rf build
	cd python && rm -f cdecimal*.so *.pyc
	cd tests && if [ -f Makefile ]; then $(MAKE) clean; else exit 0; fi

distclean: clean
	rm -f config.h config.log config.status Makefile mpdecimal.h
	echo "" > vars.mk
	cd tests && if [ -f Makefile ]; then $(MAKE) distclean; else exit 0; fi
	cd python && rm -rf decimaltestdata build __pycache__



