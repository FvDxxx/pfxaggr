
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


all: clean pfxaggr

clean:
	rm -f ${OBJAG}

pfxaggr: ${OBJAG}
	${CC} ${CFLAGS} -o pfxagg ${OBJAG} $(LIBS_$(OSTYPE))
