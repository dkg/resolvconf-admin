#!/usr/bin/make -f

OBJECTS = resolvconf-admin resolvconf-admin.1

all: $(OBJECTS)

resolvconf-admin: resolvconf-admin.c
	gcc -Wall -Werror -pedantic -g -o $@ $<

resolvconf-admin.1: resolvconf-admin.1.md
	pandoc -s -f markdown -t man -o $@ $<

resolvconf-admin-test: resolvconf-admin.c
	gcc -Wall -Werror -DSBINRESOLVCONF=\"$(PWD)/tests/dummy-resolvconf2\" -DETCRESOLVCONF=\"$(PWD)/tests/resolv.conf\" -pedantic -g -o $@ $<

check: resolvconf-admin-test tests/run tests/dummy-resolvconf
	tests/run

clean:
	rm -f $(OBJECTS) resolvconf-admin-test

.PHONY: all clean check
