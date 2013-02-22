
// EditDlg.h : header file
//

#pragma once

#define FONTS_SIZED_LIST 4

// CEditDlg dialog
class CEditDlg : public CDialog, public COmnCallback
{
// Construction
public:
	CEditDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_OMNVTRGUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
    CToolTipCtrl* m_ToolTip;
    CStatusBarCtrl* m_StatusBar;
    CFont* m_Fonts[FONTS_SIZED_LIST];
    int mark_input_id;
    int parse_input_tc();
    int key_W_pressed;

#define LIST_AREA_STATE_VISIBLE 2
#define LIST_AREA_STATE_JUNK 1

    int list_area_state;
    void list_area_state_adopt();
    BOOL list_area_notify(unsigned int id);
    void update_list(int f_deleted);
    BOOL ctl_button(unsigned int id);
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
    BOOL PreTranslateMessage(MSG* pMsg);
public:
    virtual void COmnCallbackNotify(int id, void* data);
    afx_msg void OnEnKillfocusEditMarkOut();
    afx_msg void OnEnSetfocusEditMarkOut();
    afx_msg void OnEnKillfocusEditMarkIn();
    afx_msg void OnEnSetfocusEditMarkIn();
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnNMDblclkListRecords(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnBnClickedButtonListCreate();
    afx_msg void OnBnClickedButtonListDelete();
    afx_msg void OnBnClickedButtonListRestore();
    afx_msg void OnBnClickedButtonListRename();
    afx_msg void OnBnClickedButtonListOpen();
    afx_msg void OnBnClickedButtonExport();
    afx_msg void OnNMRDblclkListRecords(NMHDR *pNMHDR, LRESULT *pResult);
};
