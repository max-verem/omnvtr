
// omnvtrUIDlg.h : header file
//

#pragma once
#include "EDLFile.h"
#include <omplrclnt.h>
#pragma comment(lib, "omplrlib.lib") 

// ComnvtrUIDlg dialog
class ComnvtrUIDlg : public CDialog
{
// Construction
public:
	ComnvtrUIDlg(CWnd* pParent = NULL);	// standard constructor
    CFont times;
    struct
    {
        OmPlrHandle handle;
        HANDLE lock;
        OmPlrStatus status_curr, status_prev;
        int f_online, f_first;
        int mark_in, mark_out, mark_curr;
        int record_id;
        int jog_accum, jog_step;
        int keys;
    } m_director;

    CEDLFile* edl;

// Dialog Data
	enum { IDD = IDD_OMNVTRUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
    void oper_mark_in();
    void oper_mark_out();
    void oper_stop();
    void oper_play();
    void oper_backward();
    void oper_forward();
    void oper_record_record();
    void oper_cue_record();

protected:
	HICON m_hIcon;
    CToolTipCtrl m_ToolTip;
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
    afx_msg BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButtonExit();
    afx_msg void OnEnUpdateEditMarkIn();
    afx_msg LRESULT OnMCS3Jog(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMCS3Shuttle(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMCS3Button(WPARAM wParam, LPARAM lParam);
    afx_msg void OnBnClickedButtonExport();
    afx_msg BOOL ComnvtrUIDlg::OnOperButton(unsigned int id);
};

