// ExportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "omnvtrUI.h"
#include "omnvtrUIDlg.h"
#include "ExportDlg.h"

#include <ommedia.h>

// ExportDlg dialog

IMPLEMENT_DYNAMIC(ExportDlg, CDialog)

ExportDlg::ExportDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ExportDlg::IDD, pParent)
{
    pomc = NULL;
    homc = INVALID_HANDLE_VALUE;
    eomc = 0;
}

ExportDlg::~ExportDlg()
{
}

void ExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ExportDlg, CDialog)
    ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL ExportDlg::OnInitDialog()
{
    int i;
    CDialog::OnInitDialog();

    CComboBox
        *combo_dirs = (CComboBox*)GetDlgItem(IDC_COMBO_DIRS),
        *combo_types = (CComboBox*)GetDlgItem(IDC_COMBO_TYPES);

    for(i = 0; i < theApp.cmdInfo.m_omneon_dirs_cnt; i++)
        combo_dirs->SetItemData
        (
            combo_dirs->AddString(theApp.cmdInfo.m_omneon_dirs[i]),
            i
        );
    combo_dirs->SetCurSel(0);

    combo_types->SetItemData(combo_types->AddString("Reference Copy (pure reference)"), omReferenceCopy);
    combo_types->SetItemData(combo_types->AddString("Flattened With DiscreteMedia (reference with copy)"), omFlattenedWithDiscreteMedia);
    combo_types->SetItemData(combo_types->AddString("Flattened With Embedded Media (self-contained)"), omFlattenedWithEmbeddedMedia);
    combo_types->SetCurSel(0);

    /* run timer */
    SetTimer(2, 40, NULL);

    GetDlgItem(IDC_EDIT_ID)->SetFocus();

    return FALSE;
}

void ExportDlg::OnTimer(UINT nIDEvent)
{
    int r = 0;
    OmCopyProgress progress;
    ComnvtrUIDlg* dlg = (ComnvtrUIDlg*)GetParent();
    CProgressCtrl* p_ui = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS_COPY);

    WaitForSingleObject(dlg->m_director.lock, INFINITE);
    if(pomc)
    {
        progress = ((OmMediaCopier*)pomc)->getProgress();
        r = 1;
    }
    ReleaseMutex(dlg->m_director.lock);

    if(r)
    {
        p_ui->SetRange32(0, progress.nTotalFrames);
        p_ui->SetPos(progress.nComplete);
        UpdateWindow();
    }
    else
        p_ui->SetPos(0);

    if(eomc)
    {
        int r = eomc;
        eomc = 0;
        WaitForSingleObject(homc, INFINITE);
        CloseHandle(homc);
        homc = INVALID_HANDLE_VALUE;
        GetDlgItem(IDC_COMBO_DIRS)->EnableWindow(1);
        GetDlgItem(IDC_COMBO_TYPES)->EnableWindow(1);
        GetDlgItem(IDC_EDIT_ID)->EnableWindow(1);
        GetDlgItem(IDOK)->EnableWindow(1);
        if(r == 1)
            CDialog::OnOK();
        else
            MessageBox("Operation failed", "Error!");
    };
}

static unsigned long WINAPI worker(LPVOID pParam)
{
    int r;

    ExportDlg* main = (ExportDlg*)pParam;
    ComnvtrUIDlg* dlg = (ComnvtrUIDlg*)main->GetParent();

    r = ((OmMediaCopier*)main->pomc)->copy();

    WaitForSingleObject(dlg->m_director.lock, INFINITE);
    delete (OmMediaCopier*)main->pomc;
    main->pomc = NULL;
    ReleaseMutex(dlg->m_director.lock);

    if(r)
        main->eomc = 1;
    else
        main->eomc = -1;

    return 0;
};

void ExportDlg::OnCancel()
{
    int c = 0;
    ComnvtrUIDlg* dlg = (ComnvtrUIDlg*)GetParent();

    WaitForSingleObject(dlg->m_director.lock, INFINITE);
    if(pomc)
        ((OmMediaCopier*)pomc)->cancel();
    else
    {
        if(homc == INVALID_HANDLE_VALUE)
            c = 1;
    };
    ReleaseMutex(dlg->m_director.lock);

    if(c)
        CDialog::OnCancel();
};

void ExportDlg::OnOK()
{
    int r, d = 0, mark_out, mark_in;
    char file_origin[256], id_selected[256], file_target[256];
    CComboBox
        *combo_dirs = (CComboBox*)GetDlgItem(IDC_COMBO_DIRS),
        *combo_types = (CComboBox*)GetDlgItem(IDC_COMBO_TYPES);
    CEdit
        *id_edit = (CEdit*)GetDlgItem(IDC_EDIT_ID);

    ComnvtrUIDlg* dlg = (ComnvtrUIDlg*)GetParent();

    if(homc != INVALID_HANDLE_VALUE)
        return;

    /* get entered id */
    id_edit->GetWindowText(id_selected, sizeof(id_selected));

    /* check if it empty */
    if(!id_selected[0])
    {
        MessageBox("id not specified", "Error!");
        return;
    };

    /* request selected args */
    char* target_dir = theApp.cmdInfo.m_omneon_dirs[combo_dirs->GetItemData(combo_dirs->GetCurSel())];
    enum OmMediaCopyType t = (enum OmMediaCopyType)combo_types->GetItemData(combo_types->GetCurSel());

    /* build filenames */
    _snprintf
    (
        file_origin, sizeof(file_origin),
        "%s/%s.%s",
        theApp.cmdInfo.m_omneon_dir,
        dlg->m_director.id,
        "mov"
    );
    _snprintf
    (
        file_target, sizeof(file_target),
        "%s/%s.%s",
        target_dir,
        id_selected,
        "mov"
    );

    OmMediaCopier* omcp = new OmMediaCopier();
#ifdef _DEBUG
    omcp->setDebug("c:\\temp\\omnvtr.log1");
#endif
    omcp->setRemoteHost(theApp.cmdInfo.m_omneon_host);
    omcp->setDestination(file_target, 1);
    omcp->setOutputSuffix(omMediaFileTypeQt, "mov");
    omcp->setCopyType(t);

    if(dlg->m_director.mark_in < 0)
        mark_in = 0;
    else
        mark_in = dlg->m_director.mark_in;
    if(dlg->m_director.mark_out < 0)
        mark_out = ~0;
    else
        mark_out = dlg->m_director.mark_out;

    r = omcp->appendTracks(0, file_origin, mark_in, mark_out);

    WaitForSingleObject(dlg->m_director.lock, INFINITE);
    pomc = omcp;
    ReleaseMutex(dlg->m_director.lock);

    homc = ::CreateThread(NULL, 0, worker, this, 0, NULL);

    GetDlgItem(IDC_COMBO_DIRS)->EnableWindow(0);
    GetDlgItem(IDC_COMBO_TYPES)->EnableWindow(0);
    GetDlgItem(IDC_EDIT_ID)->EnableWindow(0);
    GetDlgItem(IDOK)->EnableWindow(0);
};

// ExportDlg message handlers
