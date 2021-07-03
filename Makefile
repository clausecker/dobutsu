CC=c99
CFLAGS=$(RLCFLAGS) $(INTLCFLAGS) -O3 -DNDEBUG -DLOCALEDIR=\"$(LOCALEDIR)\" -g

# for libedit support on FreeBSD
RLCFLAGS=-I/usr/include/edit
RLLDFLAGS=
RLLDLIBS=-ledit

# for readline support
#RLCFLAGS=
#RLLDFLAGS=
#RLLDLIBS=-lreadline -lhistory

# for libintl support
INTLCFLAGS=-I/usr/local/include
INTLLDFLAGS=-L/usr/local/lib
INTLLDLIBS=-lintl

# number of threads used during table base generation
NPROC=2

# customize this if your system uses a different path structure
PREFIX=/usr/local
TBDIR=$(PREFIX)/share/dobutsu
BINDIR=$(PREFIX)/bin
#BINDIR=$(PREFIX)/games
MANDIR=$(PREFIX)/share/man
LIBEXECDIR=$(PREFIX)/libexec
LOCALEDIR=$(PREFIX)/share/locale

# the msgfmt program to use
MSGFMT=msgfmt

#LIBEXECDIR=$(PREFIX)/lib

# prefix applied to installation directory during install step
#STAGING=

# replace with dobutsu.tb if you want to waste more space for a faster
# program start
# TBFILE=dobutsu.tb
TBFILE=dobutsu.tb.xz

# flags applied when compressing TBFILE.
# dictionary size must be harmonized with code in tbaccess.c
XZFLAGS=-4 -e -C crc32

GENTBOBJ=gentb.o tbgenerate.o poscode.o unmoves.o moves.o
XZOBJ=xz/xz_crc32.o xz/xz_dec_lzma2.o xz/xz_dec_stream.o
VALIDATETBOBJ=$(XZOBJ) validatetb.o tbvalidate.o tbaccess.o notation.o poscode.o validation.o moves.o
DOBUTSUOBJ=$(XZOBJ) dobutsu.o position.o ai.o notation.o tbaccess.o validation.o poscode.o moves.o
MOFILES=po/de.mo
MANPAGES=man6/dobutsu.6 de.UTF-8/man6/dobutsu.6

all: gentb validatetb dobutsu dobutsu-stub translate

.SUFFIXES: .po .mo

.po.mo:
	$(MSGFMT) -o $@ $<

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

gentb: $(GENTBOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o gentb $(GENTBOBJ) $(LDLIBS) -lpthread

validatetb: $(VALIDATETBOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o validatetb $(VALIDATETBOBJ) $(LDLIBS)

dobutsu: $(DOBUTSUOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(RLLDFLAGS) $(INTLLDFLAGS) -o dobutsu \
	    $(DOBUTSUOBJ) $(LDLIBS) $(RLLDLIBS) $(INTLLDLIBS) -lm

dobutsu-stub:
	echo '#!/bin/sh' >dobutsu-stub
	echo >>dobutsu-stub
	echo '[ -z "$$DOBUTSU_TABLEBASE" ] && DOBUTSU_TABLEBASE="$(TBDIR)/$(TBFILE)"' >>dobutsu-stub
	echo 'export DOBUTSU_TABLEBASE' >>dobutsu-stub
	echo 'exec "$(LIBEXECDIR)/dobutsu" "$$@"' >>dobutsu-stub
	chmod a+x dobutsu-stub

dobutsu.tb.xz: gentb dobutsu.tb
	rm -f dobutsu.tb.xz
	xz $(XZFLAGS) -k dobutsu.tb

dobutsu.tb: gentb
	./gentb -j $(NPROC) dobutsu.tb

translate: $(MOFILES)

clean:
	rm -f *.o xz/*.o gentb validatetb dobutsu dobutsu-stub po/*.mo

distclean: clean
	rm -f dobutsu.tb dobutsu.tb.xz dobutsu.6.gz

install: translate dobutsu dobutsu-stub $(TBFILE)
	mkdir -p $(STAGING)$(TBDIR)
	cp dobutsu.tb.xz $(STAGING)$(TBDIR)/$(TBFILE)
	mkdir -p $(STAGING)$(LIBEXECDIR)
	cp dobutsu $(STAGING)$(LIBEXECDIR)/dobutsu
	mkdir -p $(STAGING)$(BINDIR)
	cp dobutsu-stub $(STAGING)$(BINDIR)/dobutsu
	@for man in $(MANPAGES) ; \
	do \
		echo mkdir -p $(STAGING)$(MANDIR)/`dirname $$man` ; \
		mkdir -p $(STAGING)$(MANDIR)/`dirname $$man` ; \
		echo cp man/$$man $(STAGING)$(MANDIR)/$$man ; \
		cp man/$$man $(STAGING)$(MANDIR)/$$man ; \
	done
	@for mo in $(MOFILES:.mo=) ; \
	do \
		echo mkdir -p $(STAGING)$(LOCALEDIR)/$${mo#po/}/LC_MESSAGES ; \
		mkdir -p $(STAGING)$(LOCALEDIR)/$${mo#po/}/LC_MESSAGES ; \
		echo cp $$mo.mo $(STAGING)$(LOCALEDIR)/$${mo#po/}/LC_MESSAGES/dobutsu.mo ; \
		cp $$mo.mo $(STAGING)$(LOCALEDIR)/$${mo#po/}/LC_MESSAGES/dobutsu.mo ; \
	done

.PHONY: clean all distclean install translate

.POSIX:
