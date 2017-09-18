#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>

/* print the name of the first discovered network interface on stdout */
int
main(int argc, const char **argv)
{
  struct ifaddrs *ifa, *cur;
  if (getifaddrs(&ifa)) {
    perror("Failed to getifaddrs()");
    return 1;
  }

  for (cur = ifa; cur; cur = cur->ifa_next) {
    if (cur->ifa_name && cur->ifa_name[0] != 0) {
      printf("%s\n", cur->ifa_name);
      freeifaddrs(ifa);
      return 0;
    }
  }

  freeifaddrs(ifa);
  perror("no interfaces found via getifaddrs()");
  return 2;
}
