// C:\projects\omnvtr\src\omnvtrGUI\ExportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "omnvtrGUI.h"
#include "ExportDlg.h"
#include "timecode.h"

#include <ommedia.h>
#pragma comment(lib, "ommedia.lib")

#include <ctype.h>

typedef struct export_ctx_desc
{
    int r;
    OmMediaCopier* omc;
    HANDLE th, *lock;
}
export_ctx_t;

extern char* strncpy_rev_slash(char* dst, char* src, int lim);

static unsigned long WINAPI worker(LPVOID pParam)
{
    int r;
    export_ctx_t* ctx = (export_ctx_t*)pParam;

    TRACE("[%4d] worker start copy\n", __LINE__);

    r = ctx->omc->copy();

    TRACE("[%4d] finished copy, r = %d\n", __LINE__, r);

    /* lock */
    WaitForSingleObject(*ctx->lock, INFINITE);

    /* release media copier */
    delete ctx->omc;

    /* reset value */
    ctx->omc = NULL;

    /* set result */
    if(r)
        ctx->r = 1;
    else
        ctx->r = -1;

    /* unlock */
    ReleaseMutex(*ctx->lock);

    TRACE("[%4d] exiting\n", __LINE__, r);

    return 0;
};

// CExportDlg dialog

IMPLEMENT_DYNAMIC(CExportDlg, CDialog)

CExportDlg::CExportDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExportDlg::IDD, pParent)
{
    mark_input_id = 0;
    lock = CreateMutex(NULL, FALSE, NULL);
    ctx = NULL;
}

CExportDlg::~CExportDlg()
{
    CloseHandle(lock);
}

void CExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

static const int buttons_desc[] =
{
    IDC_BUTTON_START,           1,  IDB_EXPORT_START,   IDB_EXPORT_START_PUSHED,
    IDC_BUTTON_ABORT,           1,  IDB_EXPORT_ABORT,   IDB_EXPORT_ABORT_PUSHED,
    0, 0, 0, 0
};


BEGIN_MESSAGE_MAP(CExportDlg, CDialog)
    ON_WM_TIMER()
    ON_EN_KILLFOCUS(IDC_EDIT_MARK_OUT, &CExportDlg::OnEnKillfocusEditMarkOut)
    ON_EN_SETFOCUS(IDC_EDIT_MARK_OUT, &CExportDlg::OnEnSetfocusEditMarkOut)
    ON_EN_KILLFOCUS(IDC_EDIT_MARK_IN, &CExportDlg::OnEnKillfocusEditMarkIn)
    ON_EN_SETFOCUS(IDC_EDIT_MARK_IN, &CExportDlg::OnEnSetfocusEditMarkIn)
    ON_BN_CLICKED(IDC_BUTTON_START, &CExportDlg::OnBnClickedButtonStart)
    ON_BN_CLICKED(IDC_BUTTON_ABORT, &CExportDlg::OnBnClickedButtonAbort)
    ON_WM_DRAWITEM()
    ON_WM_MEASUREITEM()
END_MESSAGE_MAP()

BOOL CExportDlg::OnInitDialog()
{
    int i, j;
    char buf[1024];
    CDialog::OnInitDialog();

    CComboBox
        *combo_dirs = (CComboBox*)GetDlgItem(IDC_COMBO_DIRS),
        *combo_types = (CComboBox*)GetDlgItem(IDC_COMBO_TYPES);

    /* setup output dirs */
    for(i = 0; i < theApp.cmdInfo.m_omneon_dirs_cnt; i++)
        combo_dirs->SetItemData
        (
            combo_dirs->AddString(theApp.cmdInfo.m_omneon_dirs[i]),
            i
        );
    combo_dirs->SetCurSel(0);

    /* setup possible type */
    combo_types->SetItemData(combo_types->AddString("STANDALONE"), omFlattenedWithDiscreteMedia);
    combo_types->SetItemData(combo_types->AddString("LINK"), omReferenceCopy);
    combo_types->SetCurSel(0);

    /* run timer */
    SetTimer(2, 40, NULL);

    /* specify window title */
    _snprintf(buf, sizeof(buf), "Exporting [%s]", m_reel->title);
    SetWindowText(buf);

    ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS_COPY))->SetPos(0);

    /* fix marks in/out */
    if(theApp.cmdInfo.m_mark_in < 0)
        theApp.m_ctl->set_mark_in(0);
    else
        theApp.m_ctl->set_mark_in(theApp.cmdInfo.m_mark_in);
    if(theApp.cmdInfo.m_mark_out < 0)
        theApp.m_ctl->set_mark_out(m_reel->dur());
    else
        theApp.m_ctl->set_mark_out(theApp.cmdInfo.m_mark_out);

    /* set in/out */
    ((CStatic*)GetDlgItem(IDC_EDIT_MARK_IN))->
        SetWindowText(tc_frames2txt(theApp.m_ctl->get_mark_in(), buf));
    ((CStatic*)GetDlgItem(IDC_EDIT_MARK_OUT))->
        SetWindowText(tc_frames2txt(theApp.m_ctl->get_mark_out(), buf));

    /* config range slider */
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RANGE))->SetRange(0, m_reel->dur());
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RANGE))->SetSelection
    (
        theApp.m_ctl->get_mark_in(),
        theApp.m_ctl->get_mark_out()
    );

    /* copy title */
    memset(buf, 0, sizeof(buf));
    for(i = 0, j = 0; m_reel->title[i]; i++)
        if
        (
            (m_reel->title[i] >= 'a' && m_reel->title[i] <= 'z')
            ||
            (m_reel->title[i] >= 'A' && m_reel->title[i] <= 'Z')
            ||
            (m_reel->title[i] >= '0' && m_reel->title[i] <= '9')
            ||
            (m_reel->title[i] == '-') || (m_reel->title[i] == '_')
        )
            buf[j++] = toupper(m_reel->title[i]);
    ((CEdit*)GetDlgItem(IDC_EDIT_ID))->SetWindowText(buf);

    /* create a tooltips */
    m_ToolTip = new CToolTipCtrl();
    if(m_ToolTip->Create(this))
    {
        for(i = 0; buttons_desc[i]; i += 4)
            if(buttons_desc[i + 1])
                m_ToolTip->AddTool(GetDlgItem(buttons_desc[i]),
                    buttons_desc[i]);
        m_ToolTip->Activate(TRUE);
        m_ToolTip->SetMaxTipWidth(300);
    }

    /* try to set images for buttons */
    for(i = 0; buttons_desc[i]; i += 4)
    {
        CPngImage image;

        if(buttons_desc[i + 2] && image.Load(buttons_desc[i + 2], GetModuleHandle(NULL)))
            bmps[i / 4][0] = (HBITMAP)image.Detach();
        if(buttons_desc[i + 3] && image.Load(buttons_desc[i + 3], GetModuleHandle(NULL)))
            bmps[i / 4][1] = (HBITMAP)image.Detach();
    };

    return TRUE;
};

void CExportDlg::OnEnKillfocusEditMarkOut()
{
    parse_input_tc();
    mark_input_id = 0;
}

void CExportDlg::OnEnSetfocusEditMarkOut()
{
    ((CEdit*)GetDlgItem(mark_input_id = IDC_EDIT_MARK_OUT))->
        SetSel(0, -1, FALSE);
}

void CExportDlg::OnEnKillfocusEditMarkIn()
{
    parse_input_tc();
    mark_input_id = 0;
}

void CExportDlg::OnEnSetfocusEditMarkIn()
{
    ((CEdit*)GetDlgItem(mark_input_id = IDC_EDIT_MARK_IN))->
        SetSel(0, -1, FALSE);
}

int CExportDlg::parse_input_tc()
{
    int r;
    char buf[1024];

    /* get text */
    GetDlgItem(mark_input_id)->GetWindowText(buf, sizeof(buf));

    /* try to parse */
    r = tc_txt2frames2(buf);

    /* set */
    if(mark_input_id == IDC_EDIT_MARK_OUT)
    {
        if(r < 0)
            r = theApp.m_ctl->get_mark_out();
        else
            theApp.m_ctl->set_mark_out(r);
    }
    else
    {
        if(r < 0)
            r = theApp.m_ctl->get_mark_in();
        else
            theApp.m_ctl->set_mark_in(r);
    };

    /* set it back */
    GetDlgItem(mark_input_id)->SetWindowText(tc_frames2txt(r, buf));

    /* select */
    ((CEdit*)GetDlgItem(mark_input_id))->SetSel(0, -1, FALSE);

    /* update slider */
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RANGE))->SetSelection
    (
        theApp.m_ctl->get_mark_in(),
        theApp.m_ctl->get_mark_out()
    );
    ((CSliderCtrl*)GetDlgItem(IDC_SLIDER_RANGE))->SetRange(0, m_reel->dur(), 1);

    return 0;
};

void CExportDlg::OnCancel()
{
    OnBnClickedButtonAbort();
};

void CExportDlg::OnOK()
{
#if 0
    int r = 1;

    WaitForSingleObject(lock, INFINITE);
    if(ctx)
    {

    };
    ReleaseMutex(lock);

    if(r)
        CDialog::OnOK();
#endif
};

void CExportDlg::OnTimer(UINT nIDEvent)
{
    int e = 0;
    OmCopyProgress progress;
    CProgressCtrl* p_ui = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS_COPY);

    WaitForSingleObject(lock, INFINITE);
    if(ctx)
    {
        export_ctx_t* ctx = (export_ctx_t*)this->ctx;
        if(ctx->r)
        {
            e = ctx->r;
            delete ctx->omc;
            WaitForSingleObject(ctx->th, INFINITE);
            CloseHandle(ctx->th);
            free(ctx);
            this->ctx = NULL;
        }
        else if(ctx->omc)
        {
            progress = ctx->omc->getProgress();
            p_ui->SetRange32(0, progress.nTotalFrames);
            p_ui->SetPos(progress.nComplete);
            UpdateWindow();
        };
    };
    ReleaseMutex(lock);

    if(e)
    {
        if(e < 0)
        {
            MessageBox("Error happens or aborted", "Error!");
            GetDlgItem(IDC_COMBO_DIRS)->EnableWindow(1);
            GetDlgItem(IDC_COMBO_TYPES)->EnableWindow(1);
            GetDlgItem(IDC_EDIT_ID)->EnableWindow(1);
            GetDlgItem(IDC_BUTTON_START)->EnableWindow(1);
            GetDlgItem(IDC_EDIT_MARK_IN)->EnableWindow(1);
            GetDlgItem(IDC_EDIT_MARK_OUT)->EnableWindow(1);
            p_ui->SetPos(0);
            UpdateWindow();
        }
        else
            CDialog::OnOK();
    };
};

BOOL CExportDlg::PreTranslateMessage(MSG* pMsg)
{
    if(pMsg->message == WM_KEYDOWN)
    {
        switch(pMsg->wParam)
        {
            case VK_ESCAPE:
                if(mark_input_id)
                {
                    int r;
                    char buf[1024];
                    if(mark_input_id == IDC_EDIT_MARK_OUT)
                        r = theApp.m_ctl->get_mark_out();
                    if(mark_input_id == IDC_EDIT_MARK_IN)
                        r = theApp.m_ctl->get_mark_in();
                    if(r < 0)
                        buf[0] = 0;
                    else
                        tc_frames2txt(r, buf);
                    ((CStatic*)GetDlgItem(mark_input_id))->SetWindowText(buf);
                    ((CEdit*)GetDlgItem(mark_input_id))->SetSel(0, -1, FALSE);
                };
                return TRUE;
                break;

            case VK_TAB:
                if(mark_input_id)
                {
                    TRACE("VK_TAB on edit\n");
                    parse_input_tc();
                };
                break;

            case VK_RETURN:
                if(mark_input_id)
                {
                    TRACE("VK_RETURN on edit\n");
                    parse_input_tc();
                };
                return TRUE;
                break;
        }
    };

    return CDialog::PreTranslateMessage(pMsg);
};

void CExportDlg::OnBnClickedButtonStart()
{
    int i, r;
    export_ctx_t* ctx;
    char buf[1024], file_origin[256], id_selected[256], file_target[256];

    /* check for empty id */
    ((CEdit*)GetDlgItem(IDC_EDIT_ID))->GetWindowText(id_selected, sizeof(id_selected));
    if(!id_selected[0])
    {
        MessageBox("id not specified", "Error!");
        return;
    }
    else
    {
        char* buf = id_selected;
        for(i = 0, r = 0; buf[i]; i++)
            if
            (
                (buf[i] >= 'a' && buf[i] <= 'z')
                ||
                (buf[i] >= 'A' && buf[i] <= 'Z')
                ||
                (buf[i] >= '0' && buf[i] <= '9')
                ||
                (buf[i] == '-') || (buf[i] == '_')
            )
                buf[i] = toupper(buf[i]);
            else
            {
                buf[i] = '_';
                r++;
            };
        ((CEdit*)GetDlgItem(IDC_EDIT_ID))->SetWindowText(buf);
        if(r)
            return;
    };
    CComboBox
        *combo_dirs = (CComboBox*)GetDlgItem(IDC_COMBO_DIRS),
        *combo_types = (CComboBox*)GetDlgItem(IDC_COMBO_TYPES);

    /* request selected args */
    char* target_dir = theApp.cmdInfo.m_omneon_dirs
        [combo_dirs->GetItemData(combo_dirs->GetCurSel())];
    enum OmMediaCopyType t = (enum OmMediaCopyType)combo_types->GetItemData
        (combo_types->GetCurSel());

    /* allocate context */
    ctx = (export_ctx_t*)malloc(sizeof(export_ctx_t));
    memset(ctx, 0, sizeof(export_ctx_t));

    ctx->omc = new OmMediaCopier();
    r = ctx->omc->setRemoteHost(theApp.cmdInfo.m_omneon_host);
    _snprintf
    (
        file_target, sizeof(file_target),
        "%s/%s.%s",
        strncpy_rev_slash(buf, target_dir, sizeof(buf)),
        id_selected,
        "mov"
    );
    r = ctx->omc->setDestination(file_target, 1);
    r = ctx->omc->setOutputSuffix(omMediaFileTypeQt, "mov");
    ctx->omc->setCopyType(t);

    for(i = 0; i < m_reel->play_cnt; i++)
    {
        _snprintf
        (
            file_origin, sizeof(file_origin),
            "%s/%d.%s",
            strncpy_rev_slash(buf, theApp.cmdInfo.m_omneon_dir, sizeof(buf)),
            m_reel->play_list[i].id,
            "mov"
        );

        r = ctx->omc->appendTracks(0, file_origin,
            m_reel->play_list[i].clip_in, m_reel->play_list[i].clip_out);
    };
    r = ctx->omc->setRange
    (
        theApp.m_ctl->get_mark_in(),
        theApp.m_ctl->get_mark_out()
    );

    WaitForSingleObject(lock, INFINITE);
    this->ctx = ctx;
    ctx->th = ::CreateThread(NULL, 0, worker, ctx, 0, NULL);
    ctx->lock = &lock;
    ReleaseMutex(lock);

    GetDlgItem(IDC_COMBO_DIRS)->EnableWindow(0);
    GetDlgItem(IDC_COMBO_TYPES)->EnableWindow(0);
    GetDlgItem(IDC_EDIT_ID)->EnableWindow(0);
    GetDlgItem(IDC_BUTTON_START)->EnableWindow(0);
    GetDlgItem(IDC_EDIT_MARK_IN)->EnableWindow(0);
    GetDlgItem(IDC_EDIT_MARK_OUT)->EnableWindow(0);
}

void CExportDlg::OnBnClickedButtonAbort()
{
    int e = 0;

    WaitForSingleObject(lock, INFINITE);
    if(ctx)
    {
        export_ctx_t* ctx = (export_ctx_t*)this->ctx;
        if(ctx->omc)
            ctx->omc->cancel();
    }
    else
        e = 1;
    ReleaseMutex(lock);

    if(e)
        CDialog::OnOK();
}

extern BOOL FillSolidRect(HDC hDC, int x, int y, int cx, int cy, COLORREF clr);
extern BOOL FillSolidRect(HDC hDC, const RECT* pRC, COLORREF crColor);

void CExportDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpdis)
{
    int idx, i;

    for(i = 0, idx = -1; buttons_desc[i * 4] && idx < 0; i++)
        if(buttons_desc[i * 4] == nIDCtl && bmps[i][0])
            idx = i;

    if(idx >= 0)
    {
        BITMAP bm;
        HBITMAP bmp, hBmpOld;
        BLENDFUNCTION bfn;
        HDC hCDC = ::CreateCompatibleDC(lpdis->hDC);

        FillSolidRect(lpdis->hDC, &lpdis->rcItem, ::GetSysColor(COLOR_BTNFACE));

        i = ((CButton*)GetDlgItem(nIDCtl))->GetState();

        if(i & BST_PUSHED)
            bmp = bmps[idx][1];
        else
            bmp = bmps[idx][0];

        hBmpOld = (HBITMAP)::SelectObject(hCDC, bmp);

        ::GetObject(bmp, sizeof(bm), &bm);

        bfn.BlendOp = AC_SRC_OVER;
        bfn.BlendFlags = 0;
        bfn.SourceConstantAlpha = 255;
        bfn.AlphaFormat = AC_SRC_ALPHA;

        i = ::AlphaBlend
        (
            lpdis->hDC,
            0, 0, bm.bmWidth, bm.bmHeight,
            hCDC,
            0, 0, bm.bmWidth, bm.bmHeight,
            bfn
        );
        if(!i)
        {
            i = GetLastError();
        };
        ::SelectObject(hCDC, hBmpOld);
        ::DeleteDC(hCDC);
    };
};

void CExportDlg::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpdis)
{
    int i, idx;

    for(i = 0, idx = -1; buttons_desc[i * 4] && idx < 0; i++)
        if(buttons_desc[i * 4] == nIDCtl && bmps[i][0])
            idx = i;

    if(idx >= 0)
    {
        BITMAP bm;
        HBITMAP bmp = bmps[idx][0];
        ::GetObject(bmp, sizeof(bm), &bm);
        lpdis->itemWidth = bm.bmWidth;
        lpdis->itemHeight = bm.bmHeight;
    };
};
