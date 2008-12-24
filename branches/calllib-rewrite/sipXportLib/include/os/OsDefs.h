/*// 
// 
// Copyright (C) 2005-2006 SIPez LLC.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////
*/

/* NOTE THIS FILE MUST BE INCLUDABLE IN C CODE as well as C++ */
/* USE C Style comments */

#ifndef _OsDefs_h_
#define _OsDefs_h_

#include "qsTypes.h"    // Should be removed -- move contents here.

// SYSTEM INCLUDES
#ifdef _VXWORKS
#include "os/Vxw/OsVxwDefs.h"
#endif // _VXWORKS
#ifdef __pingtel_on_posix__
#include "os/linux/OsLinuxDefs.h"
#endif /* __pingtel_on_posix__ */

#if defined(_VXWORKS)
#  define IS_INET_RETURN_OK( x )    (x == 0)  /* wdn - OK == 0 defined in vxWorks.h but has issues ??? */
#else
#  define IS_INET_RETURN_OK( x )    (x > 0)
#endif

#ifdef _WIN32
  // Variadic macros not supported in MSVC++ before version 2005
  #if defined(_MSC_VER) && _MSC_VER < 1400
    #define SNPRINTF _snprintf
  #else
    #define SNPRINTF(buff, buffsize, ...) {_snprintf_s((buff), (buffsize), _TRUNCATE, __VA_ARGS__);}
  #endif
#else
#define SNPRINTF(buff, buffsize, ...)                   \
   {                                                    \
      if ((buff) && ((buffsize) > 0))                   \
      {                                                 \
         snprintf((buff), (buffsize) - 1, __VA_ARGS__); \
         (buff)[(buffsize) - 1] = 0;                    \
      }                                                 \
   }
#endif 

#ifdef _WIN32
#define SAFE_STRCHR(X, Y) (((X) == NULL) ? NULL : strchr((X), (Y)))
#else
#define SAFE_STRCHR(X, Y) (((X) == NULL) ? NULL : strchr((X), (Y)))
#endif

#define SAFE_PTR_CAST(CLASSTYPE, UNKNOWNPTR) \
   ((CLASSTYPE*)(UNKNOWNPTR) && ((CLASSTYPE*)(UNKNOWNPTR))->nSize == sizeof(CLASSTYPE)) ? (CLASSTYPE*)(UNKNOWNPTR) : NULL;

                       // strncpy_s introduced in MSVC++ 2005
#if defined(_WIN32) && defined(_MSC_VER) && _MSC_VER >= 1400
#define SAFE_STRNCPY(X, Y, Z) if ((X)) {strncpy_s((X), (Z), (Y), _TRUNCATE); }
#else
#define SAFE_STRNCPY(X, Y, Z) if ((X)) {strncpy((X), (Y), (Z)); if ((Z) > 0) (X)[(Z) - 1] = 0; }                              
#endif


#ifdef _WIN32
#define SAFE_STRDUP(X) (((X) == NULL) ? NULL : _strdup((X)))
#else
#define SAFE_STRDUP(X) (((X) == NULL) ? NULL : strdup((X)))
#endif

#define SAFE_STRLEN(X) (((X) == NULL) ? 0 : strlen((X)))

#define MAKESTR(X) #X 

/* Handle the case-insensitive string comparison functions, by making the Posix names
 * strcasecmp and strncasecmp available on all platforms.
 * (On newer Windows environments, str(n)casecmp are built-in, along with the older
 * str(n)icmp, but on older ones, they are not.) */
#ifdef WIN32
    #if defined(WINCE) || (defined(_MSC_VER) && (_MSC_VER >= 1400)) // if wince, or win and >= msvc8(vs2005)
        #ifndef strcasecmp
           #define strcasecmp _stricmp
        #endif
        #ifndef strncasecmp
           #define strncasecmp _strnicmp
        #endif
    #else
        #ifndef strcasecmp
           #define strcasecmp stricmp
        #endif
        #ifndef strncasecmp
           #define strncasecmp strnicmp
        #endif
    #endif
#endif

/* Define min and max if they're not already defined. */
#ifndef max
#  define max(x,y) (((x)>(y))?(x):(y))
#endif 

#ifndef min
#  define min(x,y) (((x)<(y))?(x):(y))
#endif 

/* APPLICATION INCLUDES  */
/* MACROS                */
/* EXTERNAL FUNCTIONS    */
/* DEFINES               */

#ifdef __cplusplus
extern "C" {
#endif

#if defined (_VXWORKS)  /*  Only needed for VxWorks --GAT */

int strcasecmp(const char *, const char *);
char * strdup (const char *);

/* These function names are for code compatibility with Windows. --GAT */
#ifndef strcmpi
#define strcmpi strcasecmp
#endif
#ifndef stricmp
#define stricmp strcasecmp
#endif
#ifndef _stricmp
#define _stricmp strcasecmp
#endif

#endif /* _VXWORKS */

extern unsigned int pspGetLocalMemLocalAddr(void);
extern unsigned int pspGetLocalMemSize(void);

#define SysLowerMemoryLimit   (pspGetLocalMemLocalAddr())
#define SysUpperMemoryLimit   (pspGetLocalMemLocalAddr() + pspGetLocalMemSize() - 4)


extern int hSipLogId;
void enableConsoleOutput(int bEnable) ;
void osPrintf(const char* format , ...)
#ifdef __GNUC__
            /* with the -Wformat switch, this enables format string checking */
            __attribute__ ((format (printf, 1, 2)))

#endif
         ;
         
/* A special value for "port number" which means that no port is specified.
*/
#define PORT_NONE (-1)

/* A special value for "port number" which means that some default port number
** should be used.  The default may be defined by the situation, or
** the OS may choose a port number.
** For use when PORT_NONE is used to mean "open no port", and in socket-opening
** calls.
*/
#define PORT_DEFAULT (-2)

/* Macro to test a port number for validity as a real port (and not PORT_NONE
** or PORT_DEFAULT).  Note that 0 is a valid port number for the protocol,
** but the Berkeley sockets interface makes it impossible to specify it.
** In addition, RTP treats port 0 as a special value.  Thus we forbid port 0.
*/
#define portIsValid(p) ((p) >= 1 && (p) <= 65535)

/* EXTERNAL VARIABLES   */
/* CONSTANTS            */
/* STRUCTS              */
/* TYPEDEFS             */
/* FORWARD DECLARATIONS */

#ifdef __cplusplus
}
#endif

#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "time.h"

#if defined(__sun) && defined(__SVR4)
#include <strings.h>
#include <sys/sockio.h>
#include <netdb.h>
#ifdef __cplusplus
extern "C"
#endif
extern int getdomainname(char *, int);
#endif

#endif  /* _OsDefs_h_ */
