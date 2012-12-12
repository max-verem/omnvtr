
// omnvtrUIDlg.h : header file
//

#pragma once

#include <omplrclnt.h>
#pragma comment(lib, "omplrlib.lib") 

// ComnvtrUIDlg dialog
class ComnvtrUIDlg : public CDialog
{
// Construction
public:
	ComnvtrUIDlg(CWnd* pParent = NULL);	// standard constructor

    struct
    {
        OmPlrHandle handle;
        HANDLE lock;
        OmPlrStatus status_curr, status_prev;
        int f_online, f_new, f_first;
        char id[1023];
        int mark_in, mark_out, mark_curr;
        int record_id;
        int jog_accum, jog_step;
    } m_director;

// Dialog Data
	enum { IDD = IDD_OMNVTRUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;
    void ui_load_clip(int f_new);
    void load_clip(int pos);
    void window_lock(int l, char* msg = "");

	// Generated message map functions
	virtual BOOL OnInitDialog();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    virtual void OnOK(){};
    virtual void OnCancel(){};
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButtonExit();
    afx_msg void OnBnClickedButtonNew(){ ui_load_clip(1); };
    afx_msg void OnBnClickedButtonLoad(){ ui_load_clip(0); };
    afx_msg void OnEnUpdateEditMarkIn();
    afx_msg LRESULT OnMCS3Jog(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMCS3Shuttle(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMCS3Button(WPARAM wParam, LPARAM lParam);
    afx_msg void OnBnClickedButtonExport();
};

