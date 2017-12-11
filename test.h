/* #if __USE_FORTIFY_LEVEL > 0 && defined __fortify_function */

#include <stdio.h>
#undef __USE_FORTIFY_LEVEL 
#define __USE_FORTIFY_LEVEL 3 
/* #define __fortify_function */
/* #define _STDIO_H */
// CXXFLAGS= " -D__USE_FORTIFY_LEVEL=2"
/* we need to set this in order to be able to include functions */
#include <bits/stdio2.h>
#include <libgen.h>
#include <wchar.h>
#include <link.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <ifaddrs.h>
#include <sys/uio.h>
#include <libgen.h>
#include <locale.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio_ext.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/dir.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <pthread.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
// xlocale.h is not provided in newer glibc https://bugs.freedesktop.org/show_bug.cgi?id=102454
/* #include <xlocale.h> */
#include <errno.h>
#include <setjmp.h>
#include <libintl.h>
#include <pwd.h>
#include <inttypes.h>
#include <error.h>
#include <netinet/ether.h>
#include <search.h>
#include <fnmatch.h>
#include <langinfo.h>
#include <sys/vfs.h>
#include <termio.h>
#include <math.h>
#include <assert.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <wctype.h>
#include <locale.h>
#include <utime.h>

