#include "StdAfx.h"
#include "omnvtrUICmdLine.h"
#include "omnvtrUI.h"

extern ComnvtrUIApp theApp;

ComnvtrUICmdLine::ComnvtrUICmdLine(void)
{
    m_syncplay_omneon_host[0] = m_syncplay_omneon_player[0] =
    m_omneon_host[0] = m_omneon_player[0] = m_omneon_dir[0] = 0;
    m_msc3_serial_port = -1;
    m_omneon_dirs_cnt = 0;
    m_syncplay_delay = 0;
    m_mode = 0;
    m_mark_in = m_mark_out = -1;
}

ComnvtrUICmdLine::~ComnvtrUICmdLine(void)
{
};

enum ComnvtrUICmdLineOpts
{
    opt_none = 0,
    opt_msc3_serial_port,
    opt_omneon_host,
    opt_omneon_player,
    opt_omneon_dir,
    opt_export_dir,
    opt_syncplay_omneon_host,
    opt_syncplay_omneon_player,
    opt_syncplay_delay,
    opt_edl,
    opt_mark_in,
    opt_mark_out,
    opt_export
};

static struct
{
    char* name;
    int id;
} opts[] = 
{
    {"msc3_serial_port",    opt_msc3_serial_port},
    {"omneon_host",         opt_omneon_host},
    {"omneon_player",       opt_omneon_player},
    {"omneon_dir",          opt_omneon_dir},
    {"export_dir",          opt_export_dir},
    {"syncplay_omneon_host", opt_syncplay_omneon_host},
    {"syncplay_omneon_player", opt_syncplay_omneon_player},
    {"syncplay_delay",      opt_syncplay_delay},
    {"export",              opt_export},
    {"edl",                 opt_edl},
    {"mark_in",             opt_mark_in},
    {"mark_out",            opt_mark_out},
    {NULL,                  opt_none}
};

void ComnvtrUICmdLine::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
    int i;
    char* buf = NULL;

    /* flag given */
    if(bFlag)
    {
        /* check if previous option processed */
        if(opt_none != opt)
        {
            MessageBox(theApp.GetMainWnd()->GetSafeHwnd(), pszParam, "OPTIONS: ARGUMENT MISSED BEFORE" , MB_OK | MB_ICONEXCLAMATION);
            m_fails++;
        };

        /* lookup option id */
        for(opt = opt_none, i = 0; (NULL != opts[i].name) && (opt_none == opt); i++)
            if(0 == _stricmp(pszParam, opts[i].name))
                opt = opts[i].id;

        /* check options */
        switch(opt)
        {
            case opt_export:
                m_mode = 1;
                opt = opt_none;
                break;

            /* if unknown option found */
            case opt_none:
                MessageBox(theApp.GetMainWnd()->GetSafeHwnd(), pszParam, "OPTIONS: WRONG OPTION" , MB_OK | MB_ICONEXCLAMATION);
                m_fails++;
                break;

            /* required parameters - must not be last */
            case opt_msc3_serial_port:
            case opt_omneon_host:
            case opt_omneon_player:
            case opt_omneon_dir:
            case opt_export_dir:
            case opt_syncplay_omneon_host:
            case opt_syncplay_omneon_player:
            case opt_syncplay_delay:
            case opt_edl:
            case opt_mark_in:
            case opt_mark_out:
                if(bLast)
                {
                    MessageBox
                    (
                        theApp.GetMainWnd()->GetSafeHwnd(), 
                        pszParam, 
                        "OPTIONS: OPTION REQUIRED PARAMETER",
                        MB_OK | MB_ICONEXCLAMATION
                    );
                    m_fails++;
                };
                break;
        };
    }
    else
    {
        /* check what option was previously */
        switch(opt)
        {
            case opt_mark_in:
                m_mark_in = atol((char*)pszParam);
                opt = opt_none;
                break;
            case opt_mark_out:
                m_mark_out = atol((char*)pszParam);
                opt = opt_none;
                break;
            case opt_edl:                   buf = m_edl; break;
            case opt_msc3_serial_port:
                m_msc3_serial_port = atol((char*)pszParam);
                opt = opt_none;
                break;
            case opt_syncplay_omneon_host:  buf = m_omneon_host; break;
            case opt_syncplay_omneon_player:buf = m_omneon_host; break;
            case opt_syncplay_delay:
                m_syncplay_delay = atol((char*)pszParam);
                opt = opt_none;
                break;
            case opt_omneon_host:           buf = m_omneon_host; break;
            case opt_omneon_player:         buf = m_omneon_player; break;
            case opt_omneon_dir:            buf = m_omneon_dir; break;
            case opt_export_dir:
                buf = m_omneon_dirs[m_omneon_dirs_cnt];
                m_omneon_dirs_cnt++;
                break;

            default:
                /* notify about wrong data */
                MessageBox(theApp.GetMainWnd()->GetSafeHwnd(), pszParam, "OPTIONS: EXTRA DATA" , MB_OK | MB_ICONEXCLAMATION);
                m_fails++;
                opt = opt_none;
                break;
        };

        /* copy param to buf */
        if(NULL != buf)
        {
            strncpy(buf, (char*)pszParam, MAX_LEN_CONF_PARAM);

            /* clear option */
            opt = opt_none;
        };
    };
};

int ComnvtrUICmdLine::check_params()
{
    char* err_msg_not_set = NULL;

#define DEF_CHECK(V) if(!m_##V[0]) err_msg_not_set = #V;
    DEF_CHECK(omneon_host);
    DEF_CHECK(omneon_player);
    DEF_CHECK(omneon_dir);
    if(m_msc3_serial_port < 0) err_msg_not_set = "msc3_serial_port";

    if(err_msg_not_set)
    {
        MessageBox(theApp.GetMainWnd()->GetSafeHwnd(), err_msg_not_set, "OPTION NOT SET" , MB_OK | MB_ICONEXCLAMATION);
        return -1;
    };

    return 0;
};
