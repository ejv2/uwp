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

.PHONY: all clean
