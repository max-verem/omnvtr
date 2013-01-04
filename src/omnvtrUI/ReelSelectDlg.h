#pragma once


// CReelSelectDlg dialog

class CReelSelectDlg : public CDialog
{
	DECLARE_DYNAMIC(CReelSelectDlg)

public:
	CReelSelectDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CReelSelectDlg();

// Dialog Data
	enum { IDD = IDD_REELSELECT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
