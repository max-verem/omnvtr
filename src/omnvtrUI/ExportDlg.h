#pragma once

#include "EDLFile.h"

// ExportDlg dialog

class ExportDlg : public CDialog
{
	DECLARE_DYNAMIC(ExportDlg)
public:
	ExportDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~ExportDlg();

// Dialog Data
	enum { IDD = IDD_EXPORTDLG };

    void* pomc;
    HANDLE homc;
    int eomc;
    CEDLFile* edl;
    HANDLE lock;

protected:
    afx_msg void OnTimer(UINT nIDEvent);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
