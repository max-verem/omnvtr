
// omnvtrUI.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "omnvtrUI.h"
#include "omnvtrUIDlg.h"
#include "ReelSelectDlg.h"
#include "ExportDlg.h"
#include "../mcs3/mcs3.h"
#include <Mmsystem.h>

extern void ComnvtrUIDlg_mcs3(void* cookie, int button, int value);

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib, "Winmm.lib")

// ComnvtrUIApp

BEGIN_MESSAGE_MAP(ComnvtrUIApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// ComnvtrUIApp construction

ComnvtrUIApp::ComnvtrUIApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only ComnvtrUIApp object

ComnvtrUIApp theApp;


// ComnvtrUIApp initialization

BOOL ComnvtrUIApp::InitInstance()
{
    int r, i;

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

    /* replace slashes for path due to micrecognize of slash */
    for(r = 0; 0 != cmdInfo.m_omneon_dir[r]; r++)
        if('\\' == cmdInfo.m_omneon_dir[r])
            cmdInfo.m_omneon_dir[r] = '/';
    for(i = 0; i < cmdInfo.m_omneon_dirs_cnt; i++)
        for(r = 0; 0 != cmdInfo.m_omneon_dirs[i][r]; r++)
            if('\\' == cmdInfo.m_omneon_dirs[i][r])
                cmdInfo.m_omneon_dirs[i][r] = '/';

    /* check mode */
    if(cmdInfo.m_mode)
    {
        ExportDlg d1;
        r = d1.DoModal();
    }
    else
    {
        OmPlrHandle handle;

        /* edl (reel) select dialog, if not supplies */
        if(!cmdInfo.m_edl[0])
        {
            CReelSelectDlg d1;
            r = d1.DoModal();
        };

        if(r != IDOK)
            return FALSE;

        /* connect to MCS3 */
        m_mcs3 = NULL;
        if(mcs3_open(&m_mcs3, cmdInfo.m_msc3_serial_port))
        {
            MessageBox(GetMainWnd()->GetSafeHwnd(),
                "Failed to open serial port for MSC3", "ERROR!" ,
                MB_OK | MB_ICONEXCLAMATION);
        };

        /* open director */
        r = OmPlrOpen
        (
            cmdInfo.m_omneon_host,
            cmdInfo.m_omneon_player,
            &handle
        );
        if(0 != r)
        {
            MessageBox(GetMainWnd()->GetSafeHwnd(),
                "Failed connect to Omneon", "ERROR!" ,
                MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
        };

        /* setup directory */
        r = OmPlrClipSetDirectory(handle, cmdInfo.m_omneon_dir);
        if(0 != r)
        {
            OmPlrClose(handle);
            MessageBox(GetMainWnd()->GetSafeHwnd(),
                "Failed connect to change directory on Omneon", "ERROR!" ,
                MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
        };

        {
            ComnvtrUIDlg dlg;
            m_pMainWnd = &dlg;

            dlg.m_director.handle = handle;

            /* setup callback of MCS3 */
            if(m_mcs3)
                mcs3_callback(m_mcs3, ComnvtrUIDlg_mcs3, &dlg);

            /* create lock */
            dlg.m_director.lock = CreateMutex(NULL, FALSE, NULL);
            dlg.m_director.f_online = 0;
            r = dlg.DoModal();
            CloseHandle(dlg.m_director.lock);
        };

        if(m_mcs3)
            mcs3_close(m_mcs3);
        OmPlrClose(handle);
    };

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
