
                    /* DO NOT MODIFY - GENERATED BY script */
                    #ifndef DCE_HEADER_TIMERFD_H
                    #define DCE_HEADER_TIMERFD_H
                    // TODO add extern "C" ?
                    #include <timerfd.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif

                    extern int dce_timerfd_create (clockid_t,int);

extern int dce_timerfd_settime (int,int,itimerspec const *,itimerspec *);

extern int dce_timerfd_gettime (int,itimerspec *);


                    #ifdef __cplusplus
}
#endif
                    #endif
                    