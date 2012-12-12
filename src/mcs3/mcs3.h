#ifndef MCS3_H
#define MCS3_H

#ifdef __linux__
#define MCS3_API
#else
#ifdef MCS3_EXPORTS
#define MCS3_API __declspec(dllexport)
#else /* !MCS3_EXPORTS */
#define MCS3_API __declspec(dllimport)
#pragma comment(lib, "mcs3.lib") 
#endif /* MCS3_EXPORTS */
#endif /* __linux__ */

#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define MCS3_BUTTON_W1          0x10
#define MCS3_BUTTON_W2          0x11
#define MCS3_BUTTON_W3          0x12
#define MCS3_BUTTON_W4          0x13
#define MCS3_BUTTON_W5          0x14
#define MCS3_BUTTON_W6          0x15
#define MCS3_BUTTON_W7          0x03
#define MCS3_BUTTON_F1          0x0E
#define MCS3_BUTTON_F2          0x0D
#define MCS3_BUTTON_F3          0x0C
#define MCS3_BUTTON_F4          0x02
#define MCS3_BUTTON_F5          0x0F
#define MCS3_BUTTON_F6          0x01
#define MCS3_BUTTON_BACKWARD    0x07
#define MCS3_BUTTON_FORWARD     0x06
#define MCS3_BUTTON_STOP        0x05
#define MCS3_BUTTON_PLAY        0x04
#define MCS3_BUTTON_RECORD      0x00
#define MCS3_BUTTON_RIGHT       0x0B
#define MCS3_BUTTON_LEFT        0x0A
#define MCS3_BUTTON_TOP         0x09
#define MCS3_BUTTON_BOTTOM      0x08

#define MCS3_BUTTON             0x80
#define MCS3_JOG                0x81
#define MCS3_SHUTTLE            0x82

typedef void (*mcs3_cb_proc)(void* cookie, int button, int value);

extern MCS3_API int mcs3_open(void** phandle, int serial_port);
extern MCS3_API int mcs3_close(void* handle);
extern MCS3_API int mcs3_callback(void* handle, mcs3_cb_proc proc, void* cookie);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif
