
/* DO NOT MODIFY - GENERATED BY script */
#ifndef DCE_HEADER_SYS_SYSLOG_H
#define DCE_HEADER_SYS_SYSLOG_H
// TODO add extern "C" ?
#include <sys/syslog.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif
                 void dce_openlog (char const * __ident,int __option,int __facility) ;

 void dce_closelog () ;

 int dce_setlogmask (int __mask) noexcept;

 void dce_syslog (int __pri,char const * __fmt,... ) ;

 void dce_vsyslog (int __pri,char const * __fmt,va_list) ;


#ifdef __cplusplus
}
#endif
#endif
