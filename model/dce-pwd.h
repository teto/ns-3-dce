
/* DO NOT MODIFY - GENERATED BY script */
#ifndef DCE_HEADER_PWD_H
#define DCE_HEADER_PWD_H
// TODO add extern "C" ?
#include <pwd.h>
#include <stdarg.h> // just in case there is an ellipsis
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif
                
 passwd * dce_getpwuid (__uid_t __uid)  ;

 void dce_endpwent ()  ;


#ifdef __cplusplus
}
#endif
#endif
