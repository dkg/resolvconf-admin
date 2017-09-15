---
title: RESOLVCONF-ADMIN
section: 1
author: Daniel Kahn Gillmor <dkg@fifthhorseman.net>
date: 2017 September
---

NAME
====

resolvconf-admin - a setuid helper program for setting up the DNS

SYNOPSIS
========
 
resolvconf-admin add NETIF [-s SEARCH] [-d DOMAIN] NAMESERVER [...]

resolvconf-admin del NETIF

DESCRIPTION
===========

When the non-privileged user wants to set the DNS servers due to
information from interface NETIF, it should invoke:

    resolvconf-admin add NETIF [-s SEARCH] [-d DOMAIN] NAMESERVER [...]

(note that DNS search path and domain name are optional; however,
at least one nameserver is required)

When the non-privileged users wants to tear down the DNS servers
that had been set for interface NETIF, it should invoke:

    resolvconf-admin del NETIF

WARNING
=======

A better (non-suid) approach for setting up the DNS in a
non-privileged way is to make an authenticated IPC call to some
running daemon that already manages (e.g., systemd-resolved(8)).
However, some systems do not run such a daemon, so we offer this
setuid approach instead, for those limited systems.

This setuid program *should not* be installed on systems that already run
such a daemon, because every setuid program increases the attack surface of
the operating system.

DO NOT INSTALL THIS TOOL IF YOU HAVE BETTER OPTIONS AVAILABLE TO YOU!

SEE ALSO
========

resolvconf(8), resolv.conf(5), systemd-resolved(8) 
