# GNU Make only, please
PLATFORM=$(shell uname -s)

COMMON_LIBS=-lrt

ifeq "$(PLATFORM)" "FreeBSD"
# dlopen(), et al, are in libc on FreeBSD. -liconv is required, as well.
LIBS=-L/usr/local/lib $(COMMON_LIBS) -liconv
else
LIBS=$(COMMON_LIBS) -ldl
endif

ifeq "$(PLATFORM)" "Darwin"
# Need libiconv
LIBS+=-liconv
endif

HEADERS=*.h
COMMON=dscp.c

DEBUG:=-DDEBUG
PROGS=twd test_clock
BINDIR=/usr/local/bin
OBJECTS=twd.o
# Note gnu99 lets posix get_time work. c99 doesn't
CFLAGS += -Wall -Wextra -pedantic
STRIP=strip

ifeq "$(PLATFORM)" "Darwin"
CFLAGS += -fno-common
LDFLAGS  = -dynamiclib
else
CC=gcc -std=c99
CFLAGS += -rdynamic
endif

CFLAGS+=-g

all: $(PROGS) 

test_clock: test_clock.c $(HEADERS) $(COMMON) 
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) test_clock.c $(COMMON) $(LIBS) -o test_clock

twd: twd.c $(HEADERS) $(COMMON) 
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) twd.c $(COMMON) $(LIBS) -o twd

install: $(PROGS)
	@mkdir -p $(DESTDIR)$(BINDIR)
	cp $(PROGS) $(DESTDIR)$(BINDIR)

install-stripped: $(PROGS)
	$(STRIP) $(PROGS)
	cp $(PROGS) $(DESTDIR)$(BINDIR)

clean:
	rm -f $(OBJECTS) $(PROGS) *~
