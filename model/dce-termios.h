
/* DO NOT MODIFY - GENERATED BY script */
#ifndef DCE_HEADER_TERMIOS_H
#define DCE_HEADER_TERMIOS_H
// TODO add extern "C" ?
#include <termios.h>
// TODO temporary hack
#define __restrict__

#ifdef __cplusplus
extern "C" {
#endif
                 int dce_tcgetattr (int __fd,termios * __termios_p) noexcept;

 int dce_tcsetattr (int __fd,int __optional_actions,termios const * __termios_p) noexcept;


#ifdef __cplusplus
}
#endif
#endif
