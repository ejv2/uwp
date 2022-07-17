# uwp - micro wordpress
# See LICENSE file for copyright and license details
.POSIX:

include config.mk

MAIN  = uwp
PROGS = uwp-help uwp-test uwp-sites

COMMON  = copts.c wp.c conf.c
OCOMMON = $(COMMON:.c=.o)
HCOMMON = $(COMMON:.c=.h) json.h util.h

PREQ = $(OCOMMON) $(MAIN) config.h

all: $(PROGS)

clean:
	rm -f *.o
	rm -f $(PROGS)

uwp-help: help.c $(PREQ)
	$(CC) -o uwp-help $(FLAGS) $(OCOMMON) help.c

uwp-test: test.c $(PREQ)
	$(CC) -o uwp-test $(FLAGS) $(OCOMMON) test.c

uwp-sites: sites.c $(PREQ)
	$(CC) -o uwp-sites $(FLAGS) $(OCOMMON) sites.c

$(OCOMMON): $(HCOMMON)

.c.o:
	$(CC) -c $(UCFLAGS) $<

install: $(PROGS)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f uwp $(DESTDIR)$(PREFIX)/bin
	cp -f $(PROGS) $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/uwp
	chmod 755 $(DESTDIR)$(PREFIX)/bin/uwp-*

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/uwp
	rm -f $(DESTDIR)$(PREFIX)/bin/uwp-*

compdb:
	$(MAKE) clean
	compiledb $(MAKE) CC=cc

.PHONY: all clean install uninstall compdb
