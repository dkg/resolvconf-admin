#!/usr/bin/make -f

OBJECTS = resolvconf-admin resolvconf-admin.1

all: $(OBJECTS)

resolvconf-admin: resolvconf-admin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -Wall -Werror -pedantic -g -o $@ $<

resolvconf-admin.1: resolvconf-admin.1.md
	pandoc -s -f markdown -t man -o $@ $<

resolvconf-admin-test: resolvconf-admin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -Wall -Werror -DSBINRESOLVCONF=\"$(CURDIR)/tests/dummy-resolvconf2\" -DETCRESOLVCONF=\"$(CURDIR)/tests/resolv.conf\" -pedantic -g -o $@ $<

check: resolvconf-admin-test tests/run tests/dummy-resolvconf
	tests/run

clean:
	rm -f $(OBJECTS) resolvconf-admin-test tests/resolv.conf tests/dummy-resolvconf2

.PHONY: all clean check
