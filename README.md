resolvconf-admin
================

`resolvconf-admin` is a setuid helper program for tools that need to
be able to set up the local DNS resolver configuration.

This program deals with setting the local DNS resolver configuration
(i.e. `/etc/resolv.conf`), which needs to be done as root on some
systems.  One example use case is to run a DHCP client without giving
that DHCP client full superuser privileges.

Theory of Operation
-------------------

If `/sbin/resolvconf` is present and executable, it is invoked as root
with the specified configuration.  If `/sbin/resolvconf` is not
present (or is present but not executable), then `/etc/resolv.conf` is
updated directly.

WARNING!!!
----------

A better approach for setting up the DNS in a non-privileged way is to
make an authenticated IPC call to some [running daemon that already
manages
`/etc/resolv.conf`](https://www.freedesktop.org/wiki/Software/systemd/resolved/).
However, some systems do not run such a daemon, so we offer this
setuid approach instead, for those limited systems only.

This setuid program *should not* be installed on systems that already
run such a daemon, because every setuid program increases the attack
surface of the operating system.

*DO NOT INSTALL THIS TOOL IF YOU HAVE BETTER OPTIONS AVAILABLE TO YOU!*

Installation
------------

It should probably be installed as `/usr/bin/resolvconf-admin`
something like this:

    getent group resolvconf-admins >/dev/null || addgroup --system resolvconf-admins
    chown root:resolvconf-admins /usr/bin/resolvconf-admin
    chmod 4754 /usr/bin/resolvconf-admin

and then make sure the user that you care about has access, by
adding them to this group:

    adduser my-nonpriv-dhcp-daemon resolvconf-admins

Usage
-----

When the non-privileged user wants to set local DNS resolvers due to
information it learned from interface NETIF, it should invoke:

    resolvconf-admin add NETIF [-s SEARCH] [-d DOMAIN] NAMESERVER [...]

Note that DNS search path and domain name are optional.  However, at
least one nameserver is required.

When the non-privileged user wants to tear down the DNS resolver
information that it had previously set for interface NETIF, it should
invoke:

    resolvconf-admin del NETIF
