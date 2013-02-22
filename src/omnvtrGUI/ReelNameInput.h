#pragma once


// CReelNameInput dialog

class CReelNameInput : public CDialog
{
	DECLARE_DYNAMIC(CReelNameInput)

public:
	CReelNameInput(CWnd* pParent = NULL);   // standard constructor
	virtual ~CReelNameInput();

// Dialog Data
	enum { IDD = IDD_REEL_NAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
    CString m_title;
};
