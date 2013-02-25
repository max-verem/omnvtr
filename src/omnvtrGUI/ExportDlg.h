#pragma once


// CExportDlg dialog

class CExportDlg : public CDialog
{
	DECLARE_DYNAMIC(CExportDlg)

public:
    COmnReel* m_reel;
	CExportDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CExportDlg();

// Dialog Data
	enum { IDD = IDD_EXPORT_DIALOG };

protected:
    HBITMAP bmps[32][2];
    CToolTipCtrl* m_ToolTip;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    int mark_input_id;
    int parse_input_tc();
    BOOL PreTranslateMessage(MSG* pMsg);
public:
    afx_msg void OnEnKillfocusEditMarkOut();
    afx_msg void OnEnSetfocusEditMarkOut();
    afx_msg void OnEnKillfocusEditMarkIn();
    afx_msg void OnEnSetfocusEditMarkIn();
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpdis);

	DECLARE_MESSAGE_MAP()
    afx_msg void OnBnClickedButtonStart();
    afx_msg void OnBnClickedButtonAbort();
    HANDLE lock;
    void* ctx;
};
