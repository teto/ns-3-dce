
/* DO NOT MODIFY - GENERATED BY script */
#ifndef DCE_HEADER_IF_H
#define DCE_HEADER_IF_H
// TODO add extern "C" ?
#include <if.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif
                 unsigned int dce_if_nametoindex (char const * __ifname) noexcept;

 char * dce_if_indextoname (unsigned int __ifindex,char * __ifname) noexcept;


#ifdef __cplusplus
}
#endif
#endif
