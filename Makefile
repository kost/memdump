SHELL	= /bin/sh
CC	= gcc -Wformat -Wunused
OPT	= -O
DEBUG	= -g
PROGS	= memdump
CFLAGS	= $(OPT) $(DEBUG) -I. $(XFLAGS) $(DEFS)
OBJS	= memdump.o convert_size.o error.o mymalloc.o
PROGS	= memdump
MAN	= memdump.1

what:
	@sh makedefs

all:	$(PROGS)

manpages: $(MAN)

memdump:	$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(SYSLIBS)

memdump.1: memdump.c
	srctoman $? >$@

clean:
	rm -f $(PROGS) *.o core *.core

depend: $(MAKES)
	(sed '1,/^# do not edit/!d' Makefile; \
	set -e; for i in [a-z][a-z0-9]*.c; do \
	    $(CC) -E $(DEFS) $(INCL) $$i | sed -n -e '/^# *1 *"\([^"]*\)".*/{' \
	    -e 's//'`echo $$i|sed 's/c$$/o/'`': \1/' -e 'p' -e '}'; \
	done) | grep -v '[.][o][:][ ][/]' >$$$$ && mv $$$$ Makefile

# do not edit below this line - it is generated by 'make depend'
convert_size.o: convert_size.c
convert_size.o: convert_size.h
error.o: error.c
error.o: error.h
memdump.o: memdump.c
memdump.o: convert_size.h
memdump.o: error.h
memdump.o: mymalloc.h
mymalloc.o: mymalloc.c
mymalloc.o: error.h
mymalloc.o: mymalloc.h
