#!/usr/bin/make -f

OBJECTS = resolvconf-admin resolvconf-admin.1

CFLAGS += -Wall -Werror -pedantic -g

SBINRESOLVCONF ?= /sbin/resolvconf
ETCRESOLVCONF ?= /etc/resolv.conf

FILENAMES = -DSBINRESOLVCONF=\"$(SBINRESOLVCONF)\" -DETCRESOLVCONF=\"$(ETCRESOLVCONF)\"

PREFIX ?= /usr
MANPATH ?= $(PREFIX)/share/man

VERSION ?= $(shell head -n1 < CHANGES | cut -f2 -d\ )


all: $(OBJECTS)

%: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(FILENAMES) -o $@ $<

resolvconf-admin.1.md: resolvconf-admin.1.md.in
	sed -e 's!@SBINRESOLVCONF@!$(SBINRESOLVCONF)!g' \
	    -e 's!@ETCRESOLVCONF@!$(ETCRESOLVCONF)!g' \
	   < $< > $@

resolvconf-admin.1: resolvconf-admin.1.md
	pandoc -s -f markdown -t man -o $@ $<

resolvconf-admin-test: resolvconf-admin.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -DSBINRESOLVCONF=\"$(CURDIR)/tests/dummy-resolvconf2\" -DETCRESOLVCONF=\"$(CURDIR)/tests/resolv.conf\" -o $@ $<

check: resolvconf-admin-test tests/run tests/dummy-resolvconf tests/getifname
	tests/run

install: resolvconf-admin resolvconf-admin.1
	install -D -m 4754 resolvconf-admin $(DESTDIR)$(PREFIX)/bin/resolvconf-admin
	install -D -m 0644 resolvconf-admin.1 $(DESTDIR)$(MANPATH)/man1/resolvconf-admin.1

clean:
	rm -f $(OBJECTS) resolvconf-admin-test tests/resolv.conf tests/dummy-resolvconf2 resolvconf-admin.1.md tests/getifname

# for upstream maintainer working from git only:
release:
	git tag -d resolvconf-admin-$(VERSION) || true
	git tag -s resolvconf-admin-$(VERSION) -m 'tagging release of resolvconf-admin $(VERSION)' master
	git archive --format=tar --prefix=resolvconf-admin-$(VERSION)/ resolvconf-admin-$(VERSION) | gzip -9n > ../resolvconf-admin_$(VERSION).orig.tar.gz
	gpg --armor --detach-sign ../resolvconf-admin_$(VERSION).orig.tar.gz

.PHONY: all clean check install release
