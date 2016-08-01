
                    /* DO NOT MODIFY - GENERATED BY script */
                    #ifndef DCE_HEADER_SOCKET_H
                    #define DCE_HEADER_SOCKET_H
                    // TODO add extern "C" ?
                    #include <socket.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif

                    extern int dce_socket (int,int,int);

extern int dce_socketpair (int,int,int,int *);

extern int dce_getsockname (int,__restrict__ ::sockaddr *,__restrict__ ::socklen_t *);

extern int dce_getpeername (int,__restrict__ ::sockaddr *,__restrict__ ::socklen_t *);

extern int dce_bind (int,sockaddr const *,socklen_t);

extern int dce_connect (int,sockaddr const *,socklen_t);

extern int dce_setsockopt (int,int,int,void const *,socklen_t);

extern int dce_getsockopt (int,int,int,__restrict__ void *,__restrict__ ::socklen_t *);

extern int dce_listen (int,int);

extern int dce_accept (int,__restrict__ ::sockaddr *,__restrict__ ::socklen_t *);

extern int dce_shutdown (int,int);

extern ssize_t dce_send (int,void const *,size_t,int);

extern ssize_t dce_sendto (int,void const *,size_t,int,sockaddr const *,socklen_t);

extern ssize_t dce_sendmsg (int,msghdr const *,int);

extern ssize_t dce_recv (int,void *,size_t,int);

extern ssize_t dce_recvfrom (int,__restrict__ void *,size_t,int,__restrict__ ::sockaddr *,__restrict__ ::socklen_t *);

extern ssize_t dce_recvmsg (int,msghdr *,int);


                    #ifdef __cplusplus
}
#endif
                    #endif
                    