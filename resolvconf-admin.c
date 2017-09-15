/* setuid helper program for tools that need to be able to set up the local DNS.
 *
 * Author: Daniel Kahn Gillmor <dkg@fifthhorseman.net
 * Copyright: 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/* TODO: handle DHCPv6 */

/* TODO: discourage ecs (client subnet) see
   https://sourceware.org/bugzilla/show_bug.cgi?id=18419 */

/* TODO: accept non-ascii domain names for -d and -s */

/* TODO: there are some small TOCTOU race conditions (probably fewer than
   resolvconf itself) */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>

typedef struct { char data[INET_ADDRSTRLEN]; } addrstr;

#ifndef PROGNAME
#define PROGNAME "resolvconf-admin"
#endif

#ifndef SBINRESOLVCONF
#define SBINRESOLVCONF "/sbin/resolvconf"
#endif

#ifndef ETCRESOLVCONF
#define ETCRESOLVCONF "/etc/resolv.conf"
#endif

#ifndef PREAMBLE
#define PREAMBLE "# Generated by " PROGNAME
#endif

#ifndef BACKUPNAME
#define BACKUPNAME ETCRESOLVCONF ".bak." PROGNAME
#endif

/* return 0 on success */
int
canonicalize_ip(const char *in, addrstr* out)
{
    struct in_addr a;
    if (inet_pton(AF_INET, in, &a) != 1)
        return 1;
    if (NULL == inet_ntop(AF_INET, &a, out->data, sizeof(out->data))) {
        perror("inet_ntop failed");
        return 2;
    }
    return 0;
}

/* returns 1 if ifname is the name of a legitimate network interface on the
   current system, 0 if not */
int
is_valid_interface(const char *ifname)
{
    struct ifaddrs *ifa, *cur;
    if (getifaddrs(&ifa)) {
        perror("Failed to getifaddrs()");
        return 0;
    }
    int found = 0;

    for (cur = ifa; cur; cur = cur->ifa_next) {
        if (strcmp(ifname, cur->ifa_name) == 0) {
            found = 1;
            break;
        }
    }

    freeifaddrs(ifa);
    return found;
}


/* returns 1 if we think domain is a valid argument for "search" or "domain"
   paths, 0 if not.

   allowblank should be 1 for "search", 0 for "domain".  see resolv.conf(5)
   for more details.
  */
int
is_valid_domain(const char *domain, int allowblank)
{
    if (domain == NULL || *domain == 0)
        return 0;
    while (*domain) {
        if ((!(allowblank && isblank(*domain))) &&
            (isspace(*domain) ||
             (!isgraph(*domain)) ||
             (ispunct(*domain) && *domain != '.')))
            return 0;
        domain++;
    }
    return 1;
}

void
usage(FILE* f, const char *arg0)
{
    if (!arg0)
        arg0 = PROGNAME;
    fprintf(stderr, "Usage: %s add IFNAME [-d DOMAIN] [-s SEARCH] NAMESERVERIP [...]\n"
            "\t%s del IFNAME\n"
            "\t%s help\n"
            "\n"
            "invokes " SBINRESOLVCONF " if present,\n"
            "otherwise updates " ETCRESOLVCONF " directly\n",
            arg0, arg0, arg0);
}

int
write_new_resolvconf(FILE* writer, int argc, const char **argv, const char *preamble) {
    int i;
    addrstr canonical;
    const char *search = NULL, *domain = NULL;

    fprintf(writer, "%s", preamble);
    int total_nameservers = 0;

    for (i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "-d") == 0) ||
            (strcmp(argv[i], "-s") == 0)) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "Need DOMAIN argument for %s\n", argv[i-1]);
                return 1;
            }
            if (argv[i-1][1] == 'd') {
                if (domain) {
                    fprintf(stderr, "only one -d DOMAIN option allowed\n");
                    return 1;
                }
                if (!is_valid_domain(argv[i], 0)) {
                    fprintf(stderr, "Non-domain-name argument for -d: \"%s\"\n",
                            argv[i]);
                    return 1;
                }
                domain = argv[i];
            } else if (argv[i-1][1] == 's') {
                if (search) {
                    fprintf(stderr, "only one -s SEARCH option allowed\n");
                    return 1;
                }
                if (!is_valid_domain(argv[i], 1)) {
                    fprintf(stderr, "bad argument for domain search list -s: \"%s\"\n",
                            argv[i]);
                    return 1;
                }
                search = argv[i];
            } else {
                fprintf(stderr, "something went wrong with argument parsing\n");
                return 1;
            }
        } else {
            /* this should be a nameserver */
            if (canonicalize_ip(argv[i], &canonical)) {
                fprintf(stderr, "Failed to canonicalize IP address '%s'\n",
                        argv[i]);
                return 1;
            }
            fprintf(writer, "nameserver %s\n", canonical.data);
            total_nameservers++;
        }
    }

    if (total_nameservers < 1) {
        fprintf(stderr, "not enough nameservers\n");
        return 1;
    }

    if (domain)
        fprintf(writer, "domain %s\n", domain);
    if (search)
        fprintf(writer, "search %s\n", search);

    return 0;
}

/* return 1 if it starts with the preamble; 0 if it exists and does not start
   with the preamble; -1 if there was any weirdness that prevented us from
   checking. */

int resolvconf_starts_with(const char *preamble) {
    int ret = -1;
    int oldfd = open(ETCRESOLVCONF, O_RDONLY);
    if (oldfd == -1) {
        perror("Failed to open " ETCRESOLVCONF " for reading");
    } else {
        size_t sz = strlen(preamble);
        char* oldcontents = (char *)mmap(NULL, sz,
                                         PROT_READ, MAP_PRIVATE, oldfd, 0);
        if (oldcontents == MAP_FAILED) {
            perror("Failed to mmap " ETCRESOLVCONF);
            return -1;
        } else {
            ret = (strncmp(preamble, oldcontents, sz) == 0);
            if (munmap(oldcontents, sz))
                perror("failed to mumap " ETCRESOLVCONF);
        }
        if (close(oldfd))
            perror("failed to close " ETCRESOLVCONF);
    }
    return ret;
}

int
main(int argc, const char **argv)
{
    int pipes[2] = { -1, -1 };
    int use_sbin_resolvconf = 0;
    char tmpname[] = ETCRESOLVCONF "." PROGNAME ".XXXXXX";
    int ret;
    const char *ifname = NULL;
    char label[1024];
    char preamble[1024];
    int do_add = 0;
    label[sizeof(label)-1] = 0;
    preamble[sizeof(preamble)-1] = 0;
    preamble[sizeof(preamble)-2] = '\n'; /* ensure that there is a trailing
                                            newline if ifname happens to be
                                            enormous */

    if (argc < 2) {
        usage(stderr, argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "help") == 0) {
        usage(stdout, argv[0]);
        return 0;
    }
    if (strcmp(argv[1], "add") == 0) {
        do_add = 1;
    } else if (strcmp(argv[1], "del") == 0) {
    } else {
        usage(stderr, argv[0]);
        return 1;
    }

    ifname = argv[2];

    if (!is_valid_interface(ifname)) {
        fprintf(stderr, "'%s' is not a valid network interface on this host\n",
                ifname);
        return 1;
    }

    if (access(SBINRESOLVCONF, X_OK)) {
        if (errno != ENOENT) {
            perror("cannot execute " SBINRESOLVCONF);
            fprintf(stderr, "falling back to overwriting " ETCRESOLVCONF " manually...\n");
        }
    } else {
        use_sbin_resolvconf = 1;
    }


    snprintf(preamble, sizeof(preamble)-2, "%s\n# (%s)\n", PREAMBLE, ifname);
    if (use_sbin_resolvconf) {
        snprintf(label, sizeof(label)-1, "%s." PROGNAME, ifname);
        if (do_add) {
            if (pipe(pipes)) {
                perror("Failed to open a pipe");
                return 1;
            }
        } else {
            execl(SBINRESOLVCONF, "-d", label, NULL);
            perror(SBINRESOLVCONF " -d failed.");
            return 1;
        }
    } else {
        if (do_add) {
            pipes[1] = mkstemp(tmpname);
            if (pipes[1] == -1) {
                perror("Failed to make a temporary file for overwriting " ETCRESOLVCONF);
                return 1;
            }
        } else {
            if (resolvconf_starts_with(preamble) == 1) {
                if (unlink(ETCRESOLVCONF))
                    perror("Failed to unlink " ETCRESOLVCONF);
            } else {
                fprintf(stderr, "%s is not created by %s for interface %s, not destroying.\n",
                        ETCRESOLVCONF, PROGNAME, ifname);
            }
            return 0;
        }
    }
    FILE *writer = fdopen(pipes[1], "w");

    if (writer == NULL) {
        perror("Failed to set up buffered writer");
        return 1;
    }

    ret = write_new_resolvconf(writer, argc-3, argv+3, preamble);
    if (ret) {
        if (writer)
            fclose(writer);
        if (!use_sbin_resolvconf) {
            if (unlink(tmpname))
                fprintf(stderr, "failed to unlink tempfile '%s': (%d) %s\n",
                        tmpname, errno, strerror(errno));
        }
        return ret;
    }


    if (use_sbin_resolvconf) {
        if (fclose(writer)) {
            perror("Failed to close write side of pipe");
            return 1;
        }

        if (-1 == dup2(pipes[0], 0)) {
            fprintf(stderr, "dup2(%d, 0) failed (%d) %s\n", pipes[0],
                    errno, strerror(errno));
            return 1;
        }
        execl(SBINRESOLVCONF, "-a", label, NULL);
        /* should never get here! */
        perror(SBINRESOLVCONF " -a failed");
        return 1;
    } else {
        struct stat statbuf;
        if (stat(ETCRESOLVCONF, &statbuf)) {
            perror("failed to stat " ETCRESOLVCONF);
            fprintf(stderr, "Not copying ownership and permissions\n");
        } else {
            /* if possible, copy ownership and permissions */
            if (fchown(fileno(writer), statbuf.st_uid, statbuf.st_gid))
                perror("failed to copy ownership for " ETCRESOLVCONF);
            if (fchmod(fileno(writer), statbuf.st_mode))
                perror("failed to copy permissions for " ETCRESOLVCONF);
            /* TODO: if possible, copy over ACL as well? would need to link to
               libacl, which expands attack surface */
        }
        if (fclose(writer)) {
            perror("Failed to close write side of pipe");
            return 1;
        }

        /* back up old /etc/resolv.conf which we are clobbering here, if the
           old file doesn't start with PREAMBLE.  note that we only check
           PREAMBLE here (that is, without the ifname), because we don't want
           two running instances (e.g. on different interfaces) to
           be backing up each other's data. */
        if (0 == access(ETCRESOLVCONF, F_OK)) {
            if (resolvconf_starts_with(PREAMBLE) != 1) {
                if (0 == access(BACKUPNAME, F_OK)) {
                    if (unlink(BACKUPNAME)) {
                        if (errno != ENOENT)
                            perror("failed to unlink(\"" BACKUPNAME
                                   "\") while backing up " ETCRESOLVCONF);
                    }
                }
                if (link(ETCRESOLVCONF, BACKUPNAME))
                    perror("failed to back up " ETCRESOLVCONF " to " BACKUPNAME);
            }
        }

        if (rename(tmpname, ETCRESOLVCONF)) {
            fprintf(stderr, "failed to rename \"%s\" to \"" ETCRESOLVCONF "\": (%d) %s\n",
                    tmpname, errno, strerror(errno));
        }
    }
}
