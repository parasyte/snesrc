all:	snesrc

CFLAGS  = -O3
LDRIVER	=
OBJECTS	= snesrc.o snes.o dasm.o emulate.o


snesrc:		${OBJECTS}
		${CC} ${CFLAGS} -o snesrc ${OBJECTS} ${LDRIVER}

clean:
		${RM} snesrc snesrc.exe ${OBJECTS}


snesrc.o:	snesrc.c snesrc.h _types.h snes.h dasm.h emulate.h

snes.o:		snes.c snes.h _types.h

dasm.o:		dasm.c dasm.h _types.h

emulate.o:	emulate.c emulate.h _types.h snesrc.h snes.h
