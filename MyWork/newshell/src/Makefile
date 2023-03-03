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
SRCLIST=boardctl.c commands.c console.c console.h \
	gpio.c gpio.h i2c.c i2c.h irps5401.c irps5401.h led.c \
	lib.c lib.h pmbus.c pmbus_cmds.c pmbus_cmds.h pmbus.h \
	shell.c shell.h vrails.c vrails.h

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
	find . -name '*.[chSs]' > cscope.files
	cscope -b

