AC_INIT([M],[0.1.3,[m.p.a.j.dehoogh@tudelft.nl],[https://github.com/mdehoogh/cM.git]])
AC_CONFIG_SRCDIR([src/M.c])

#PACKAGE=M
#VERSION=0.1.3

# MDH@23OCT2019: we do not want to overwrite src/Makefile.in (because I adjusted it)
#                NO we can change Makefile.in after autoconf creates it!!!
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz])

#INTI_REQUIRED_VERSION=1.0.7
#PKG_CHECK_MODULES(INTI, inti-1.0 >= $INTI_REQUIRED_VERSION)
#AC_SUBST(INTI_CFLAGS)
#AC_SUBST(INTI_LIBS)

# AC_PROG_CC replacing AC_PROG_CXX
AC_PROG_CC

# mpdecimal also has a configure script that we need to run before any make
# NOTE: libtommath does NOT have a configure script
AC_CONFIG_SUBDIRS([mpdecimal-2.4.2])

#AC_OUTPUT(Makefile src/Makefile)
# by not making src we will have to execute Mcompile instead (I think)
AC_OUTPUT([Makefile])
