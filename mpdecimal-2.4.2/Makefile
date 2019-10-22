
# ==============================================================================
#                          Unix Makefile for libmpdec
# ==============================================================================


PACKAGE_TARNAME = mpdecimal
LIBSTATIC = libmpdec.a
LIBSONAME = libmpdec.so.2
LIBSHARED = libmpdec.so.2.4.2
INSTALL = /usr/bin/install -c

prefix = /usr/local
exec_prefix = ${prefix}
includedir = ${prefix}/include
libdir = ${exec_prefix}/lib
datarootdir = ${prefix}/share
docdir = ${datarootdir}/doc/${PACKAGE_TARNAME}

all: default

default: FORCE
	cd libmpdec && $(MAKE)
	rm -f ../${LIBSTATIC}
	cp libmpdec/${LIBSTATIC} ../

check: default
	cd tests && $(MAKE) && ./runshort.sh

check_alloc: default
	cd tests && $(MAKE) && ./runshort_alloc.sh

profile: FORCE
	cd libmpdec && $(MAKE) profile

# MDH@21OCT2019: in order to prevent installation with M install has been altered to installmpdec
install: FORCE
	@echo
	@echo The 'install' option has been renamed to 'installmpdec' in order to prevent installation of mpdecimal 2.4.2 when installing M, as this is not required by M.
	@echo Therefore, use 'make installmpdec' to truely install mpdecimal 2.4.2 on your computer!
	@echo

installmpdec: FORCE
	$(INSTALL) -d -m 755 $(DESTDIR)$(includedir)
	$(INSTALL) -m 644 libmpdec/mpdecimal.h $(DESTDIR)$(includedir)
	$(INSTALL) -d -m 755 $(DESTDIR)$(libdir)
	$(INSTALL) -m 644 libmpdec/$(LIBSTATIC) $(DESTDIR)$(libdir)
	$(INSTALL) -m 755 libmpdec/$(LIBSHARED) $(DESTDIR)$(libdir)
	cd $(DESTDIR)$(libdir) && ln -sf $(LIBSHARED) $(LIBSONAME) && ln -sf $(LIBSHARED) libmpdec.so
	$(INSTALL) -d -m 755 $(DESTDIR)$(docdir)
	cp -R doc/* $(DESTDIR)$(docdir)

clean: FORCE
	cd libmpdec && if [ -f Makefile ]; then $(MAKE) clean; else exit 0; fi
	cd tests && if [ -f Makefile ]; then $(MAKE) clean; else exit 0; fi

distclean: FORCE
	cd libmpdec && if [ -f Makefile ]; then $(MAKE) distclean; else exit 0; fi
	cd tests && if [ -f Makefile ]; then $(MAKE) distclean; else exit 0; fi
	rm -f config.h config.log config.status Makefile

FORCE:



