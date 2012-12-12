// IDInputDlg.cpp : implementation file
//

#include "stdafx.h"
#include "omnvtrUI.h"
#include "IDInputDlg.h"


// CIDInputDlg dialog

IMPLEMENT_DYNAMIC(CIDInputDlg, CDialog)

CIDInputDlg::CIDInputDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIDInputDlg::IDD, pParent)
    , m_id_val(_T(""))
{

}

CIDInputDlg::~CIDInputDlg()
{
}

void CIDInputDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_INPUT_ID, m_id);
    DDX_Text(pDX, IDC_EDIT_INPUT_ID, m_id_val);
}


BEGIN_MESSAGE_MAP(CIDInputDlg, CDialog)
END_MESSAGE_MAP()


// CIDInputDlg message handlers
