#pragma once
#include "afxwin.h"

#define MAX_LEN_CONF_PARAM 1024
#define MAX_OMNEON_DIRS 10

class CmdLine :
    public CCommandLineInfo
{
    int opt;
public:
    int m_fails;
    CmdLine(void);
    virtual ~CmdLine(void);
    virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);
    int check_params();

    char
        m_syncplay_omneon_host[MAX_LEN_CONF_PARAM],
        m_syncplay_omneon_player[MAX_LEN_CONF_PARAM],
        m_omneon_host[MAX_LEN_CONF_PARAM],
        m_omneon_player[MAX_LEN_CONF_PARAM],
        m_omneon_dir[MAX_LEN_CONF_PARAM],
        m_omneon_dirs[MAX_OMNEON_DIRS][MAX_LEN_CONF_PARAM];
    int m_omneon_dirs_cnt;
    int m_msc3_serial_port;
    int m_syncplay_delay;
    int m_mode;
    char m_edl[MAX_LEN_CONF_PARAM];
    int m_mark_in, m_mark_out;
};
