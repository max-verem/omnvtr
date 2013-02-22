// C:\projects\omnvtr\src\omnvtrGUI\ReelNameInput.cpp : implementation file
//

#include "stdafx.h"
#include "omnvtrGUI.h"
#include "ReelNameInput.h"


// CReelNameInput dialog

IMPLEMENT_DYNAMIC(CReelNameInput, CDialog)

CReelNameInput::CReelNameInput(CWnd* pParent /*=NULL*/)
	: CDialog(CReelNameInput::IDD, pParent)
    , m_title(_T(""))
{

}

CReelNameInput::~CReelNameInput()
{
}

void CReelNameInput::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT1, m_title);
	DDV_MaxChars(pDX, m_title, 100);
}

BOOL CReelNameInput::OnInitDialog()
{
    CDialog::OnInitDialog();
    GotoDlgCtrl(GetDlgItem(IDC_EDIT1));
    ((CEdit*)GetDlgItem(IDC_EDIT1))->SetSel(0, -1, FALSE);
    return FALSE;
};

BEGIN_MESSAGE_MAP(CReelNameInput, CDialog)
END_MESSAGE_MAP()


// CReelNameInput message handlers
