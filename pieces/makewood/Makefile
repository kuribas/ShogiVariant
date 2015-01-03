CFLAGS=-Wall -O2
LIBS=-lnetppm -lnetpbm -lnetpgm -lnetpnm -rdynamic -lpgm -lpnm
LDFLAGS=${LIBS}


all: pnmshogi

pnmshogi: pnmshogi.c
	gcc pnmshogi.c ${CFLAGS} ${LDFLAGS} -o pnmshogi
