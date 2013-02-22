
// omnvtrGUI.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#include "CmdLine.h"
#include "OmnCtl.h"

// ComnvtrGUIApp:
// See omnvtrGUI.cpp for the implementation of this class
//

class ComnvtrGUIApp : public CWinAppEx
{
public:
	ComnvtrGUIApp();
    CmdLine cmdInfo;
    void* m_mcs3;
    COmnCtl* m_ctl;

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern ComnvtrGUIApp theApp;