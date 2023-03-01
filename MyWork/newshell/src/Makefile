##
##
DBG=n

ifeq ($(DBG),y)
OPTS=
DEBUG=-DDEBUG
LDFLAGS=-g
else
OPTS=-O2
DEBUG=
LDFLAGS=
endif

OBJS=shell.o commands.o console.o
SRCLIST=shell.c shell.h commands.c console.c console.h
FILELIST=Makefile ${SRCLIST}

LIBS=-lreadline
LIBS+= -ltermcap

CFLAGS= ${OPTS} ${DEBUG} ${LDFLAGS} -DEMUL

all: shell

shell: ${OBJS}
	${CC} ${LDFLAGS} -o shell ${OBJS} ${LIBS}

clean:
	/bin/rm -f *.o shell *~


release:
	-rm -f shell.tgz
	tar -czvf shell.tgz ${FILELIST}
	scp shell.tgz  ron@klariodt01.rocketemslabs.com:



cscope: ${SRCLIST}
	/bin/rm -f cscope.*
	$(foreach f,$(SRCLIST),echo $(f) >> cscope.files;)
	cscope -b

