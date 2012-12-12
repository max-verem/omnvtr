#pragma once
#include "afxwin.h"


// CIDInputDlg dialog

class CIDInputDlg : public CDialog
{
	DECLARE_DYNAMIC(CIDInputDlg)

public:
	CIDInputDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CIDInputDlg();

// Dialog Data
	enum { IDD = IDD_IDINPUTDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    CEdit m_id;
    CString m_id_val;
};
