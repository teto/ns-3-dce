
/* DO NOT MODIFY - GENERATED BY script */
#ifndef DCE_HEADER_SYS_UIO_H
#define DCE_HEADER_SYS_UIO_H
// TODO add extern "C" ?
#include <sys/uio.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif
                 ssize_t dce_readv (int __fd,iovec const * __iovec,int __count);

 ssize_t dce_writev (int __fd,iovec const * __iovec,int __count);


#ifdef __cplusplus
}
#endif
#endif
