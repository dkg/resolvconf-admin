#!/usr/bin/make -f

OBJECTS = resolvconf-admin resolvconf-admin.1

CFLAGS += -Wall -Werror -pedantic -g

SBINRESOLVCONF ?= /sbin/resolvconf
ETCRESOLVCONF ?= /etc/resolv.conf

FILENAMES = -DSBINRESOLVCONF=\"$(SBINRESOLVCONF)\" -DETCRESOLVCONF=\"$(ETCRESOLVCONF)\"

all: $(OBJECTS)

resolvconf-admin: resolvconf-admin.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(FILENAMES) -o $@ $<

resolvconf-admin.1.md: resolvconf-admin.1.md.in
	sed -e 's!@SBINRESOLVCONF@!$(SBINRESOLVCONF)!g' \
	    -e 's!@ETCRESOLVCONF@!$(ETCRESOLVCONF)!g' \
	   < $< > $@

resolvconf-admin.1: resolvconf-admin.1.md
	pandoc -s -f markdown -t man -o $@ $<

resolvconf-admin-test: resolvconf-admin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -DSBINRESOLVCONF=\"$(CURDIR)/tests/dummy-resolvconf2\" -DETCRESOLVCONF=\"$(CURDIR)/tests/resolv.conf\" -o $@ $<

check: resolvconf-admin-test tests/run tests/dummy-resolvconf
	tests/run

clean:
	rm -f $(OBJECTS) resolvconf-admin-test tests/resolv.conf tests/dummy-resolvconf2 resolvconf-admin.1.md

.PHONY: all clean check
