---
title: RESOLVCONF-ADMIN
section: 1
author: Daniel Kahn Gillmor <dkg@fifthhorseman.net>
date: 2017 September
---

NAME
====

resolvconf-admin - a setuid program for setting up DNS resolution

SYNOPSIS
========
 
resolvconf-admin add NETIF [-s SEARCH] [-d DOMAIN] NAMESERVER [...]

resolvconf-admin del NETIF

DESCRIPTION
===========

This setuid program allows specific non-privileged users to invoke
`/sbin/resolvconf` (if it is present) with a constrained argument to add
or remove DNS resolvers; or, if `/sbin/resolvconf` is not executable, it
can replace `/etc/resolv.conf`.

This is useful, for example, for running a DHCP client as a
non-privileged user.

When the non-privileged user wants to set up the DNS resolvers due to
information it learned from interface NETIF, it should invoke:

    resolvconf-admin add NETIF [-s SEARCH] [-d DOMAIN] NAMESERVER [...]

Note that DNS search path and domain name are optional.  However, at
least one nameserver is required.

When the non-privileged user wants to tear down the DNS resolver
information that it had previously set for interface NETIF, it should
invoke:

    resolvconf-admin del NETIF

WARNING
=======

A better (non-suid) approach for setting up the DNS in a
non-privileged way is to make an authenticated IPC call to some
running daemon that already manages the local DNS resolution
configuration (e.g., `systemd-resolved(8)`).  However, some systems do
not run such a daemon, so we offer this setuid approach instead, for
those limited systems only.

This setuid program *should not* be installed on systems that already run
such a daemon, because every setuid program increases the attack surface of
the operating system.

*DO NOT INSTALL THIS TOOL IF YOU HAVE BETTER OPTIONS AVAILABLE TO YOU!*

INTERLEAVED OPERATION WITHOUT RESOLVCONF(8)
===========================================

On a system where `resolvconf(8)` is not installed, the behavior is
not very sophisticated.  On these systems:

 * The first time `resolvconf-admin add` is invoked, the old
   `/etc/resolv.conf` is backed up to
   `/etc/resolv.conf.bak.resolvconf-admin`.

 * The first time `resolvconf-admin del` is invoked, the backed up
   file is restored.

If multiple daemons (or a single daemon monitoring multiple sources of
DNS resolver information) invokes `resolvconf-admin` in an interleaved
fashion (e.g. two `add`s before a `del`), this will almost certainly
not be the behavior that you want.  If your system is likely to have
this kind of interleaved operation, it should also have
`resolvconf(8)` installed.

SEE ALSO
========

resolvconf(8), resolv.conf(5), systemd-resolved(8) 
