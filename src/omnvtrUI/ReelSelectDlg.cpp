// ReelSelectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "omnvtrUI.h"
#include "ReelSelectDlg.h"


// CReelSelectDlg dialog

IMPLEMENT_DYNAMIC(CReelSelectDlg, CDialog)

CReelSelectDlg::CReelSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReelSelectDlg::IDD, pParent)
{

}

CReelSelectDlg::~CReelSelectDlg()
{
}

void CReelSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CReelSelectDlg, CDialog)
END_MESSAGE_MAP()

BOOL CReelSelectDlg::OnInitDialog()
{
    char path[512];
    WIN32_FIND_DATA ffd;
    CDialog::OnInitDialog();

    CComboBox *edl_list_ui = (CComboBox*)GetDlgItem(IDC_COMBO_REELNAME);

    _snprintf(path, sizeof(path), "\\\\%s%s\\*.edl",
        theApp.cmdInfo.m_omneon_host, theApp.cmdInfo.m_omneon_dir);

    HANDLE hFind = FindFirstFile(path, &ffd);
    if (INVALID_HANDLE_VALUE != hFind)
    {
        do
        {
            char* a;

            if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            a = strrchr(ffd.cFileName, '.');
            if(!a) continue;

            *a = 0;

            edl_list_ui->AddString(ffd.cFileName);
        }
        while (FindNextFile(hFind, &ffd) != 0);

        FindClose(hFind);
    };

    edl_list_ui->SetCurSel(0);
    edl_list_ui->SetFocus();

    return FALSE;
}

void CReelSelectDlg::OnOK()
{
    CComboBox
        *edl_list_ui = (CComboBox*)GetDlgItem(IDC_COMBO_REELNAME);

    theApp.cmdInfo.m_edl[0] = 0;
    edl_list_ui->GetWindowText(theApp.cmdInfo.m_edl, sizeof(theApp.cmdInfo.m_edl));

    if(theApp.cmdInfo.m_edl[0])
        CDialog::OnOK();
};

// CReelSelectDlg message handlers
