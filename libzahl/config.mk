# Please read INSTALL, section 'Configure libzahl'.

VERSION = 1.1

PREFIX = /usr/local
EXECPREFIX = $(PREFIX)
MANPREFIX = $(PREFIX)/share/man
DOCPREFIX = $(PREFIX)/share/doc

# MDH@29MAY2019: force using gcc instead (on my iMac)
CC = cc
AR = ar
RANLIB = ranlib

CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -DGOOD_RAND
CFLAGS   = -std=c99 -O3 -flto -Wall -pedantic
LDFLAGS  = -s
