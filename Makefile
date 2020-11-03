
CC=cc
#CC=/opt/eurorings/CC/SolarisStudio12.3-solaris-sparc-bin/solarisstudio12.3/bin/cc
CFLAGS=-O3 -Wall
#CFLAGS=-g -Wall
#CFLAGS=-g
OBJAG=pfx_aggr.o pfx_tree.o pfx_addr.o
DATESTR=`date +%Y%m%d`
LIBS_solaris=-lnsl -lsocket
LIBS_linux=
LIBS_FreeBSD=
PREFIX=/usr/local
BINPREFIX=${PREFIX}/bin
MANPREFIX=${PREFIX}/share/man/man1


all: clean pfxaggr

clean:
	rm -f ${OBJAG}

pfxaggr: ${OBJAG}
	${CC} ${CFLAGS} -o pfxagg ${OBJAG} $(LIBS_$(OSTYPE))

install: pfxaggr
	@echo "You better do this manually - or use install-all to copy pfxaggr to"
	@echo ${BINPREFIX}" (755) and the man page to "${MANPREFIX}" (644)"

install-all: pfxaggr pfxaggr.1
	cp pfxaggr ${BINPREFIX}
	chmod 755 ${BINPREFIX}/pfxaggr
	cp pfxaggr.1 ${MANPREFIX}
	chmod 644 ${MANPREFIX/pfxaggr}.1
