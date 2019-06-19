
# ==============================================================================
#                          Unix Makefile for libmpdec
# ==============================================================================


PACKAGE_TARNAME = mpdecimal
LIBSTATIC = libmpdec.a
LIBSONAME = libmpdec.so.2
LIBSHARED = libmpdec.so.2.4.2
INSTALL = /Users/localadmin/homebrew/bin/ginstall -c

prefix = /usr/local
exec_prefix = ${prefix}
includedir = ${prefix}/include
libdir = ${exec_prefix}/lib
datarootdir = ${prefix}/share
docdir = ${datarootdir}/doc/${PACKAGE_TARNAME}


default: FORCE
	cd libmpdec && $(MAKE)

check: default
	cd tests && $(MAKE) && ./runshort.sh

check_alloc: default
	cd tests && $(MAKE) && ./runshort_alloc.sh

profile: FORCE
	cd libmpdec && $(MAKE) profile


install: FORCE
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



