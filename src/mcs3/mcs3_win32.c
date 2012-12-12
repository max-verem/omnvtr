#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <errno.h>

#include "mcs3.h"

#define MCS3_DEBUG
#define SERIAL_READ_TIMEOUT 100 // milliseconds
#define SERIAL_BUF_SIZE     128

#pragma comment(lib, "winmm.lib") 

typedef struct mcs3_ctx_desc
{
    HANDLE fd;
    HANDLE th;
    DWORD th_id;
    int serial_port;
    int f_exit;
    unsigned char* buf;
    int buf_len;
    mcs3_cb_proc cb_proc;
    void* cb_cookie;
}
mcs3_ctx_t;

static int mcs3_serial_data(mcs3_ctx_t* ctx, unsigned char* buf, int len)
{
#ifdef MCS3_DEBUG
    fprintf(stderr, "%.8d: %d\n", timeGetTime(), len);
#endif

    /* append buf */
    memcpy(ctx->buf + ctx->buf_len, buf, len);
    ctx->buf_len += len;

    /* process buffer */
    while(ctx->buf_len >= 2)
    {
        int s = (unsigned)ctx->buf[1];
        if(s > 64) s -= 128;

        if(MCS3_BUTTON == ctx->buf[0])
        {
#ifdef MCS3_DEBUG
            fprintf(stderr, "\tbutton: %.2X [%s]\n",
                ctx->buf[1] & 0x3F, (ctx->buf[1] & 0x40)?"PRESSED":"RELEASED");
#endif
            if(ctx->cb_proc)
                ctx->cb_proc(ctx->cb_cookie, ctx->buf[1] & 0x3F, (ctx->buf[1] & 0x40)?1:0);
        }
        else if (MCS3_JOG == ctx->buf[0])
        {
#ifdef MCS3_DEBUG
            fprintf(stderr, "\tjog: %d\n", s);
#endif
            if(ctx->cb_proc)
                ctx->cb_proc(ctx->cb_cookie, MCS3_JOG, s);
        }
        else if (MCS3_SHUTTLE == ctx->buf[0])
        {
#ifdef MCS3_DEBUG
            fprintf(stderr, "\tshuttle: %d\n", s);
#endif
            if(ctx->cb_proc)
                ctx->cb_proc(ctx->cb_cookie, MCS3_SHUTTLE, s);
        }
        else
        {
#ifdef MCS3_DEBUG
            fprintf(stderr, "UNRECOGNIZED SEQUENCE %.2X %.2X\n", ctx->buf[0], ctx->buf[1]);
#endif
        };

        memmove(ctx->buf, ctx->buf + 2, ctx->buf_len - 2);
        ctx->buf_len -= 2;
    };

#ifdef MCS3_DEBUG
    fprintf(stderr, "----------- %d\n", ctx->buf_len);
#endif

    return 0;
};

static unsigned long WINAPI mcs3_loop(void* p)
{
    DWORD dwRes, dwRead;
    BOOL fWaitingOnRead = FALSE;
    OVERLAPPED osReader;
    unsigned char* lpBuf = (unsigned char*)malloc(SERIAL_BUF_SIZE);
    mcs3_ctx_t* ctx = (mcs3_ctx_t*)p;

    // Create the overlapped event. Must be closed before exiting
    memset(&osReader, 0, sizeof(osReader));
    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    fWaitingOnRead = FALSE;

    while(!ctx->f_exit)
    {
        if (!fWaitingOnRead)
        {
            // Issue read operation.
            if(!ReadFile(ctx->fd, lpBuf, SERIAL_BUF_SIZE, &dwRead, &osReader))
            {
                if (GetLastError() != ERROR_IO_PENDING) // read not delayed?
                    break; // Error in communications; report it.
                else
                    fWaitingOnRead = TRUE;
            }
            else
            {
                // read completed immediately
                mcs3_serial_data(ctx, lpBuf, dwRead);
            }
        }

        if (fWaitingOnRead)
        {
            dwRes = WaitForSingleObject(osReader.hEvent, SERIAL_READ_TIMEOUT);
            switch(dwRes)
            {
                // Read completed.
                case WAIT_OBJECT_0:
                    if(!GetOverlappedResult(ctx->fd, &osReader, &dwRead, FALSE))
                        break;// Error in communications; report it.
                    else
                        // Read completed successfully.
                        mcs3_serial_data(ctx, lpBuf, dwRead);

                    // Reset flag so that another opertion can be issued.
                    fWaitingOnRead = FALSE;
                    break;

                // read timeout
                case WAIT_TIMEOUT:
                    // Operation isn't complete yet. fWaitingOnRead flag isn't
                    // changed since I'll loop back around, and I don't want
                    // to issue another read until the first one finishes.
                    //
                    // This is a good time to do some background work.
                    dwRes = 0;
                    break;

                default:
                    // Error in the WaitForSingleObject; abort.
                    // This indicates a problem with the OVERLAPPED structure's
                    // event handle.
                    dwRes = 0;
                    break;
            }
        }
    };

    free(lpBuf);
    CloseHandle(osReader.hEvent);

    return 0;
};

int mcs3_open(void** phandle, int serial_port)
{
    int r;
    DCB dcb;
    HANDLE fd;
    COMMTIMEOUTS CommTimeouts;
    char serial_name[64];
    mcs3_ctx_t* ctx;

    /* compose port name */
    _snprintf(serial_name, sizeof(serial_name), "\\\\.\\COM%d", serial_port);

    fd = CreateFile
    (
        serial_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        0
    );
    if (INVALID_HANDLE_VALUE == fd)
        return GetLastError();

    /* configure DCB */
    dcb.DCBlength = sizeof(DCB);
    GetCommState(fd, &dcb); // ger current serialport settings
    dcb.BaudRate = (UINT) CBR_2400;
    dcb.ByteSize = (BYTE) 8;
    dcb.fParity = 0;
    dcb.StopBits = (BYTE) ONESTOPBIT;
    dcb.fBinary = 1;
    dcb.fDtrControl = 0;
    dcb.fRtsControl = 0;
    dcb.fOutX = 0;
    dcb.fInX = 0;
    r = SetCommState (fd, &dcb);
    if(!r)
    {
        r = GetLastError();
        CloseHandle(fd);
        return r;
    };

    GetCommTimeouts(fd, &CommTimeouts);
    CommTimeouts.ReadIntervalTimeout = 1;
    SetCommTimeouts(fd, &CommTimeouts);

    ctx = (mcs3_ctx_t*)malloc(sizeof(mcs3_ctx_t));
    memset(ctx, 0, sizeof(mcs3_ctx_t));

    ctx->buf = (unsigned char*)malloc(2 * SERIAL_BUF_SIZE);
    ctx->fd = fd;
    ctx->serial_port = serial_port;
    ctx->th = CreateThread
    (
        NULL,               /* LPSECURITY_ATTRIBUTES lpThreadAttributes,*/
        8096,               /* SIZE_T dwStackSize,						*/
        mcs3_loop,          /* LPTHREAD_START_ROUTINE lpStartAddress,	*/
        ctx,                /* LPVOID lpParameter,						*/
        0,                  /* DWORD dwCreationFlags,					*/
        &ctx->th_id  /* LPDWORD lpThreadId						*/
    );

    *phandle = ctx;

    return 0;
};

int mcs3_close(void* handle)
{
    mcs3_ctx_t* ctx = (mcs3_ctx_t*)handle;

    /* notify thread */
    ctx->f_exit = 1;

    /* wait thread finish */
    WaitForSingleObject(ctx->th, INFINITE);

    /* close handles */
    CloseHandle(ctx->fd);
    CloseHandle(ctx->th);

    /* */
    free(ctx);

    return 0;
};

int mcs3_callback(void* handle, mcs3_cb_proc proc, void* cookie)
{
    mcs3_ctx_t* ctx = (mcs3_ctx_t*)handle;

    ctx->cb_proc = proc;
    ctx->cb_cookie = cookie;

    return 0;
};
