
                    /* DO NOT MODIFY - GENERATED BY script */
                    #ifndef DCE_HEADER_NETDB_H
                    #define DCE_HEADER_NETDB_H
                    // TODO add extern "C" ?
                    #include <netdb.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif

                    extern int dce_getnameinfo (__restrict__ ::sockaddr const *,socklen_t,__restrict__ char *,socklen_t,__restrict__ char *,socklen_t,int);

extern hostent * dce_gethostbyname (char const *);

extern hostent * dce_gethostbyname2 (char const *,int);

extern int dce_getaddrinfo (__restrict__ char const *,__restrict__ char const *,__restrict__ ::addrinfo const *,__restrict__ ::addrinfo * *);

extern void dce_freeaddrinfo (addrinfo *);

extern char const * dce_gai_strerror (int);




extern void dce_herror (char const *);













                    #ifdef __cplusplus
}
#endif
                    #endif
                    