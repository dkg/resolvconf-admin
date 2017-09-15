resolvconf-admin
================

`resolvconf-admin` is a setuid helper program for tools that need to
be able to set up the local DNS.

This program deals with setting the local DNS information, which needs
to be done by root on some systems.  It allows you to run things like
a DHCP client without giving that DHCP client full network privileges.

A better (non-suid) approach for setting up the DNS in a
non-privileged way is to make an authenticated IPC call to some
[running daemon that already manages
resolv.conf](https://www.freedesktop.org/wiki/Software/systemd/resolved/).
However, some systems do not run such a daemon, so we offer this
setuid approach instead, for those limited systems.

This setuid program *should not* be installed on systems that already run
such a daemon, because every setuid program increases the attack surface of
the operating system.

DO NOT USE THIS TOOL IF YOU CAN HELP IT!

If `/sbin/resolvconf` is present, it is invoked as root with the recommended
data.  If it is not present, then `/etc/resolv.conf` is overwritten with a
simple file.

It should probably be installed like so:

    getent group resolvconf-admins >/dev/null || addgroup --system resolvconf-admins
    chown root:resolvconf-admins /usr/bin/resolvconf-admin
    chmod 2754 /usr/bin/resolvconf-admin

and then make sure the user that you care about has access, by
adding them to this group:

    adduser my-nonpriv-dhcp-daemon resolvconf-admins

When the non-privileged user wants to set the DNS servers due to
information from interface NETIF, it should invoke:

    resolvconf-admin add NETIF [-s SEARCH] [-d DOMAIN] NAMESERVER [...]

(note that DNS search path and domain name are optional; however,
at least one nameserver is required)

When the non-privileged users wants to tear down the DNS servers
that had been set for interface NETIF, it should invoke:

    resolvconf-admin del NETIF
