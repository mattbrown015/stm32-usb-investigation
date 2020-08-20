#include "version-string.h"

#include "version-number.h"

#include <platform/mbed_version.h>

#define xstr(x) str(x)
#define str(s) #s

#if defined(NDEBUG)
# define NDEBUG_STR "N"
#else
# define NDEBUG_STR "D"
#endif

char version_string[] = "spi-master V" xstr(SM_MAJOR_VERSION) "." xstr(SM_MINOR_VERSION) "."  xstr(SM_PATCH_VERSION)NDEBUG_STR " " __DATE__ " " __TIME__ " ";
char mbed_os_version_string[] = "Mbed OS V" xstr(MBED_MAJOR_VERSION) "." xstr(MBED_MINOR_VERSION) "."  xstr(MBED_PATCH_VERSION);
