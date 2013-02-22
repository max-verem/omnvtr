
// omnvtrGUI.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "omnvtrGUI.h"
#include "EditDlg.h"
#include "ExportDlg.h"

#include <Mmsystem.h>
#include "../mcs3/mcs3.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib, "Winmm.lib")

typedef struct mcs3_cb_desc
{
    COmnCtl* m_ctl;
    int jog_accum, jog_step;
} mcs3_cb_t;

#define MCS3_JOG_STEP 60
#define MCS3_BUTTON_MARK_IN MCS3_BUTTON_W3
#define MCS3_BUTTON_MARK_OUT MCS3_BUTTON_W5
#define MCS3_BUTTON_GOTO_IN MCS3_BUTTON_W1
#define MCS3_BUTTON_GOTO_OUT MCS3_BUTTON_W7


void mcs3_cb(void* cookie, int button, int value)
{
    mcs3_cb_t* ctx = (mcs3_cb_t*)cookie;

    if(MCS3_SHUTTLE == button)
    {
        double s;
        static const double shuttle_speeds[13] =
        {
            0.0, 0.03, 0.1, 0.2, 0.5,
            1.0, 2.0, 3.0, 5.0, 7.0,
            10.0, 24.0, 42.0
        };

        if(value < 0)
            s = -shuttle_speeds[-value];
        else
            s = shuttle_speeds[value];

        ctx->m_ctl->oper_vary_play(s);
    }
    else if(MCS3_JOG == button)
    {
        if(ctx->jog_accum * value < 0)
            ctx->jog_accum = 0;

        ctx->jog_accum += value;

        if(abs(ctx->jog_accum) > ctx->jog_step)
        {
            ctx->jog_accum = ctx->jog_accum % ctx->jog_step;
            if(ctx->jog_accum > 0)
                ctx->m_ctl->oper_step_ff();
            else
                ctx->m_ctl->oper_step_rev();
        };
    }
    else
    {
        /* ignore relase button */
        if(!value)
            return;

        switch(button)
        {
            case MCS3_BUTTON_MARK_IN:
                ctx->m_ctl->oper_mark_in();
                break;

            case MCS3_BUTTON_MARK_OUT:
                ctx->m_ctl->oper_mark_out();
                break;

            case MCS3_BUTTON_STOP:
                ctx->m_ctl->oper_play_stop();
                break;

            case MCS3_BUTTON_PLAY:
                ctx->m_ctl->oper_play();
                break;

            case MCS3_BUTTON_BACKWARD:
                ctx->m_ctl->oper_fast_rev();
                break;

            case MCS3_BUTTON_FORWARD:
                ctx->m_ctl->oper_fast_ff();
                break;

            case MCS3_BUTTON_RECORD:
                ctx->m_ctl->oper_record();
                break;

            case MCS3_BUTTON_GOTO_IN:
                ctx->m_ctl->oper_goto_in();
                break;

            case MCS3_BUTTON_GOTO_OUT:
                ctx->m_ctl->oper_goto_out();
                break;
        };
    };
};



// ComnvtrGUIApp

BEGIN_MESSAGE_MAP(ComnvtrGUIApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// ComnvtrGUIApp construction

ComnvtrGUIApp::ComnvtrGUIApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only ComnvtrGUIApp object

ComnvtrGUIApp theApp;


// ComnvtrGUIApp initialization

BOOL ComnvtrGUIApp::InitInstance()
{
    timeBeginPeriod(1);

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

    /* Parse command line */
    ParseCommandLine(cmdInfo);
    if(0 != cmdInfo.m_fails)
        return FALSE;
    if(0 != cmdInfo.check_params())
        return FALSE;

    /* create control object */
    m_ctl = new COmnCtl
    (
        cmdInfo.m_omneon_host,
        cmdInfo.m_omneon_player,
        cmdInfo.m_omneon_dir
    );

    /* check mode */
    if(cmdInfo.m_mode)
    {
        CExportDlg dlg;

        /* load specified reel */
        dlg.m_reel = m_ctl->list_reel(atol(cmdInfo.m_edl));
        if(dlg.m_reel->title[0])
        {
            m_pMainWnd = &dlg;
            dlg.DoModal();
        }
        else
            MessageBox(GetMainWnd()->GetSafeHwnd(),
                "Failed to load specified reel, specify /edl parameter in proper way", "ERROR!",
                MB_OK | MB_ICONEXCLAMATION);
        delete dlg.m_reel;
    }
    else
    {
        mcs3_cb_t m_mcs3_ctx;
        void* m_mcs3 = NULL;

        /* connect control object */
        m_ctl->connect();

        /* connect to MCS3 */
        m_mcs3 = NULL;
        if(mcs3_open(&m_mcs3, cmdInfo.m_msc3_serial_port))
        {
            MessageBox(GetMainWnd()->GetSafeHwnd(),
                "Failed to open serial port for MSC3", "ERROR!" ,
                MB_OK | MB_ICONEXCLAMATION);
        }
        else
        {
            /* setup context */
            m_mcs3_ctx.m_ctl = m_ctl;
            m_mcs3_ctx.jog_accum = 0;
            m_mcs3_ctx.jog_step = MCS3_JOG_STEP;

            /* setup callback of MCS3 */
            mcs3_callback(m_mcs3, mcs3_cb, &m_mcs3_ctx);
        };

        CEditDlg dlg;
        m_pMainWnd = &dlg;
        dlg.DoModal();

        if(m_mcs3)
            mcs3_close(m_mcs3);

    };

    delete m_ctl;

    return FALSE;
}
