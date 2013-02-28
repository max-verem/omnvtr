
// EditDlg.cpp : implementation file
//

#include "stdafx.h"
#include <afxtoolbarimages.h>
#include <Specstrings.h>
#include <gdiplus.h>
#include "omnvtrGUI.h"
#include "EditDlg.h"
#include "timecode.h"
#include "ReelNameInput.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// CEditDlg dialog

extern ComnvtrGUIApp theApp;

static char* OmPlrState_text(enum OmPlrState state)
{
    switch(state)
    {
        case omPlrStateStopped: return "STOPPED";
        case omPlrStateCuePlay: return "CUE PLAY";
        case omPlrStatePlay: return "PLAY";
        case omPlrStateCueRecord: return "CUE RECORD";
        case omPlrStateRecord: return "RECORD";
    };

    return "UNDEFINED";
};


CEditDlg::CEditDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditDlg::IDD, pParent)
{
    mark_input_id = 0;
    key_W_pressed = 0;
    m_rec_blink = 0;

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CEditDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_WM_TIMER()
    ON_EN_KILLFOCUS(IDC_EDIT_MARK_OUT, &CEditDlg::OnEnKillfocusEditMarkOut)
    ON_EN_SETFOCUS(IDC_EDIT_MARK_OUT, &CEditDlg::OnEnSetfocusEditMarkOut)
    ON_EN_KILLFOCUS(IDC_EDIT_MARK_IN, &CEditDlg::OnEnKillfocusEditMarkIn)
    ON_EN_SETFOCUS(IDC_EDIT_MARK_IN, &CEditDlg::OnEnSetfocusEditMarkIn)
#if 1
    ON_CONTROL(BN_CLICKED, IDC_LIST_SHOW, &CEditDlg::list_area_notify_IDC_LIST_SHOW)
    ON_CONTROL(BN_CLICKED, IDC_LIST_HIDE, &CEditDlg::list_area_notify_IDC_LIST_HIDE)
    ON_CONTROL(BN_CLICKED, IDC_BUTTON_LIST_LIST, &CEditDlg::list_area_notify_IDC_BUTTON_LIST_LIST)
    ON_CONTROL(BN_CLICKED, IDC_BUTTON_LIST_JUNK, &CEditDlg::list_area_notify_IDC_BUTTON_LIST_JUNK)
#endif
#if 0
    ON_COMMAND_EX(IDC_LIST_SHOW, &CEditDlg::list_area_notify)
    ON_COMMAND_EX(IDC_LIST_HIDE, &CEditDlg::list_area_notify)
    ON_COMMAND_EX(IDC_BUTTON_LIST_LIST, &CEditDlg::list_area_notify)
    ON_COMMAND_EX(IDC_BUTTON_LIST_JUNK, &CEditDlg::list_area_notify)
#endif
    ON_COMMAND_EX(IDC_BUTTON_GOTO_IN, &CEditDlg::ctl_button)
    ON_COMMAND_EX(IDC_BUTTON_GOTO_OUT, &CEditDlg::ctl_button)
    ON_COMMAND_EX(IDC_BUTTON_MARK_IN, &CEditDlg::ctl_button)
    ON_COMMAND_EX(IDC_BUTTON_MARK_OUT, &CEditDlg::ctl_button)
    ON_COMMAND_EX(IDC_BUTTON_CTL_FAST_REV, &CEditDlg::ctl_button)
    ON_COMMAND_EX(IDC_BUTTON_CTL_STEP_REV, &CEditDlg::ctl_button)
    ON_COMMAND_EX(IDC_BUTTON_CTL_PAUSE, &CEditDlg::ctl_button)
    ON_COMMAND_EX(IDC_BUTTON_CTL_PLAY, &CEditDlg::ctl_button)
    ON_COMMAND_EX(IDC_BUTTON_CTL_STEP_FF, &CEditDlg::ctl_button)
    ON_COMMAND_EX(IDC_BUTTON_CTL_FAST_FF, &CEditDlg::ctl_button)
    ON_COMMAND_EX(IDC_BUTTON_CTL_RECORD, &CEditDlg::ctl_button)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_RECORDS, &CEditDlg::OnNMDblclkListRecords)
    ON_BN_CLICKED(IDC_BUTTON_LIST_CREATE, &CEditDlg::OnBnClickedButtonListCreate)
    ON_BN_CLICKED(IDC_BUTTON_LIST_DELETE, &CEditDlg::OnBnClickedButtonListDelete)
    ON_BN_CLICKED(IDC_BUTTON_LIST_DELETE_PERM, &CEditDlg::OnBnClickedButtonListDelete)
    ON_BN_CLICKED(IDC_BUTTON_LIST_RESTORE, &CEditDlg::OnBnClickedButtonListRestore)
    ON_BN_CLICKED(IDC_BUTTON_LIST_OPEN, &CEditDlg::OnBnClickedButtonListOpen)
    ON_BN_CLICKED(IDC_BUTTON_EXPORT, &CEditDlg::OnBnClickedButtonExport)
    ON_NOTIFY(NM_RDBLCLK, IDC_LIST_RECORDS, &CEditDlg::OnNMRDblclkListRecords)
    ON_WM_DRAWITEM()
    ON_WM_MEASUREITEM()
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BUTTON_UNDO, &CEditDlg::OnBnClickedButtonUndo)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RECORDS, &CEditDlg::OnLvnItemchangedListRecords)
END_MESSAGE_MAP()


// CEditDlg message handlers
#define SET_TEXT(TYPE, ID, TEXT) \
    ((TYPE*)GetDlgItem(ID))->SetWindowText(TEXT);

#define TIMER_STATUS 2
#define TIMER_BLINK_REC 3

void CEditDlg::OnTimer(UINT nIDEvent)
{
    if(TIMER_STATUS == nIDEvent)
        theApp.m_ctl->status();
    else if(TIMER_BLINK_REC == nIDEvent)
    {
        m_rec_blink++;
        GetDlgItem(IDC_BUTTON_CTL_RECORD)->RedrawWindow(); //UpdateWindow();
    };
};

void CEditDlg::update_list(int f_deleted)
{
    char buf[1024];
    time_t t;
    int i, c;
    COmnReel** list;
    static char *templ = "%Y-%m-%d %H:%M:%S";
    struct tm *rtime;

    TRACE("update_list(%d)\n", f_deleted);

    ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->DeleteAllItems();

    list = theApp.m_ctl->list_reels();
    if(list)
    {
        for(i = 0, c = 0; list[i]; i++)
        {
            if
            (
                f_deleted && list[i]->deleted_on
                ||
                !f_deleted && !list[i]->deleted_on
            )
            {
                /* set title */
                ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->InsertItem
                (
                    LVIF_TEXT | LVIF_STATE | LVIF_PARAM, c, list[i]->title,
                    (i%2)==0 ? LVIS_SELECTED : 0, LVIS_SELECTED,
                    0, list[i]->created_on
                );

                /* set duration */
                ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
                    SetItemText(c, 1, tc_frames2txt(list[i]->dur(), buf));

                /* set creation details */
                t = list[i]->created_on;
                rtime = localtime(&t);
                strftime(buf, sizeof(buf), templ, rtime);
                ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
                    SetItemText(c, 2, buf);
            };

            delete list[i];
        };

        free(list);
    };

    OnLvnItemchangedListRecords(NULL, NULL);
};

static const int buttons_desc[] =
{
    IDC_LIST_SHOW,              1,  IDB_PANEL_SHOW,     IDB_PANEL_SHOW_PUSHED,
    IDC_LIST_HIDE,              1,  IDB_PANEL_HIDE,     IDB_PANEL_HIDE_PUSHED,
    IDC_BUTTON_LIST_LIST,       1,  IDB_VIEW_LIST,      IDB_VIEW_LIST_PUSHED,
    IDC_BUTTON_LIST_JUNK,       1,  IDB_VIEW_TRASH,     IDB_VIEW_TRASH_PUSHED,
    IDC_BUTTON_LIST_CREATE,     1,  IDB_CREATE,         IDB_CREATE_PUSHED,
    IDC_BUTTON_LIST_OPEN,       1,  IDB_OPEN,           IDB_OPEN_PUSHED,
    IDC_BUTTON_LIST_DELETE,     1,  IDB_DELETE,         IDB_DELETE_PUSHED,
    IDC_BUTTON_LIST_DELETE_PERM,1,  IDB_DELETE_PERM,    IDB_DELETE_PERM_PUSHED,
    IDC_BUTTON_LIST_RESTORE,    1,  IDB_RESTORE,        IDB_RESTORE_PUSHED,
    IDC_BUTTON_UNDO,            1,  IDB_UNDO,           IDB_UNDO_PUSHED,
    IDC_BUTTON_EXPORT,          1,  IDB_EXPORT,         IDB_EXPORT_PUSHED,
    IDC_BUTTON_GOTO_IN,         1,  0,                  0,
    IDC_BUTTON_GOTO_OUT,        1,  0,                  0,
    IDC_BUTTON_MARK_IN,         1,  0,                  0,
    IDC_BUTTON_MARK_OUT,        1,  0,                  0,
    IDC_BUTTON_CTL_FAST_REV,    1,  0,                  0,
    IDC_BUTTON_CTL_STEP_REV,    1,  0,                  0,
    IDC_BUTTON_CTL_PAUSE,       1,  0,                  0,
    IDC_BUTTON_CTL_PLAY,        1,  0,                  0,
    IDC_BUTTON_CTL_STEP_FF,     1,  0,                  0,
    IDC_BUTTON_CTL_FAST_FF,     1,  0,                  0,
    IDC_BUTTON_CTL_RECORD,      1,  IDB_REC_GREY,       IDB_REC_GREY_PUSHED,
    IDC_BUTTON_CTL_RECORD,      0,  IDB_REC_RED,        IDB_REC_RED_PUSHED,
    0, 0, 0, 0
};

BOOL CEditDlg::OnInitDialog()
{
    int i;
    static const int font_sizes[FONTS_SIZED_LIST] = {150, 400, 180, 500};

	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    /* show list area */
    list_area_state = LIST_AREA_STATE_VISIBLE;
    list_area_state_adopt();

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

    /* create status bar */
    m_StatusBar = new CStatusBarCtrl;
    m_StatusBar->Create(WS_CHILD|WS_VISIBLE|SBT_OWNERDRAW, CRect(0,0,0,0), this, 0);
    int strPartDim[4]= {160, 260, -1};
    m_StatusBar->SetParts(3, strPartDim);
    m_StatusBar->SetText(_T("Dialog / StatusBar / Toolbar"),0,0);
    m_StatusBar->SetText(_T("Example"), 1, 0);

    /* init fonts */
    for(i = 0; i < FONTS_SIZED_LIST; i++)
    {
        m_Fonts[i] = new CFont();
        m_Fonts[i]->CreatePointFont(font_sizes[i], "Arial");
    };

    /* setup fonts */
    GetDlgItem(IDC_LABEL_TITLE)->SetFont(m_Fonts[0]);
    GetDlgItem(IDC_LABEL_TC)->SetFont(m_Fonts[1]);
    GetDlgItem(IDC_LABEL_SPEED)->SetFont(m_Fonts[2]);

    /* setup dir label */
    char buf[1024];
    _snprintf
    (
        buf, sizeof(buf),
        "%s@%s%s",
        theApp.cmdInfo.m_omneon_player,
        theApp.cmdInfo.m_omneon_host,
        theApp.cmdInfo.m_omneon_dir
    );
    SetWindowText(buf);

    /* run timer */
    theApp.m_ctl->set_cb(this);
    SetTimer(TIMER_STATUS, 100, NULL);
    SetTimer(TIMER_BLINK_REC, 900, NULL);

    /* configure list dialog */
    i = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->GetExtendedStyle();
    i |= LVS_EX_COLUMNOVERFLOW | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
    ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->SetExtendedStyle(i);
    ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        InsertColumn(0, "Record NAME", LVCFMT_LEFT, 210, -1);
    ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        InsertColumn(1, "DURATION", LVCFMT_LEFT, 80, -1);
    ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        InsertColumn(2, "DATE", LVCFMT_CENTER, 150, -1);
    update_list(0);

    /* try to set images for buttons */
    for(i = 0; buttons_desc[i]; i += 4)
    {
        CPngImage image;

        if(buttons_desc[i + 2] && image.Load(buttons_desc[i + 2], GetModuleHandle(NULL)))
            bmps[i / 4][0] = (HBITMAP)image.Detach();
        if(buttons_desc[i + 3] && image.Load(buttons_desc[i + 3], GetModuleHandle(NULL)))
            bmps[i / 4][1] = (HBITMAP)image.Detach();
    };

    GetDlgItem(IDC_BUTTON_LIST_JUNK)->
        ModifyStyle(SS_TYPEMASK, BS_OWNERDRAW, SWP_FRAMECHANGED);
    GetDlgItem(IDC_BUTTON_LIST_LIST)->
        ModifyStyle(SS_TYPEMASK, BS_OWNERDRAW, SWP_FRAMECHANGED);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEditDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEditDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CEditDlg::PreTranslateMessage(MSG* pMsg)
{
    m_ToolTip->RelayEvent(pMsg);

    if(pMsg->message == WM_KEYUP)
    {
        switch(pMsg->wParam)
        {
            case 'W':
                key_W_pressed = 0;
                return TRUE;
                break;
        };
    };

    if(pMsg->message == WM_KEYDOWN)
    {
        switch(pMsg->wParam)
        {
            case 'z':
            case 'Z':
                if(GetKeyState(VK_CONTROL)< 0)
                {
                    OnBnClickedButtonUndo();
                    return TRUE;
                };
                break;
            case 'e':
            case 'E':
                if(GetKeyState(VK_CONTROL)< 0)
                {
                    OnBnClickedButtonExport();
                    return TRUE;
                };
                break;
            case 'W':
                key_W_pressed = 1;
                return TRUE;
                break;

            case 'I':
                theApp.m_ctl->oper_mark_in();
                return TRUE;
                break;

            case 'O':
                theApp.m_ctl->oper_mark_out();
                return TRUE;
                break;

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
                    SET_TEXT(CStatic, mark_input_id, buf);
                    ((CEdit*)GetDlgItem(mark_input_id))->SetSel(0, -1, FALSE);
                };
                return TRUE;
                break;

            case VK_TAB:
                if(mark_input_id)
                {
                    TRACE("VK_TAB on edit\n");
                    parse_input_tc();
                    GotoDlgCtrl(GetDlgItem(IDC_BUTTON_CTL_PAUSE));
                    return TRUE;
                };
                break;

            case VK_RETURN:
                if(mark_input_id)
                {
                    TRACE("VK_RETURN on edit\n");
                    parse_input_tc();
                    GotoDlgCtrl(GetDlgItem(IDC_BUTTON_CTL_PAUSE));
                };
                return TRUE;
                break;

            case ' ':
            case 'S':
                theApp.m_ctl->oper_play_stop();
                return TRUE;
                break;

            case 'A':
                if(key_W_pressed)
                    theApp.m_ctl->oper_step_rev();
                else
                    theApp.m_ctl->oper_fast_rev();
                return TRUE;
                break;

            case 'D':
                if(key_W_pressed)
                    theApp.m_ctl->oper_step_ff();
                else
                    theApp.m_ctl->oper_fast_ff();
                return TRUE;
                break;

            case 'R':
                theApp.m_ctl->oper_record();
                return TRUE;
                break;
        };
    };

    return CDialog::PreTranslateMessage(pMsg);
};


void CEditDlg::OnEnKillfocusEditMarkOut()
{
    TRACE("CEditDlg::OnEnKillfocusEditMarkOut()\n");
    parse_input_tc();
    mark_input_id = 0;
}

void CEditDlg::OnEnSetfocusEditMarkOut()
{
    TRACE("CEditDlg::OnEnSetfocusEditMarkOut()\n");
    ((CEdit*)GetDlgItem(mark_input_id = IDC_EDIT_MARK_OUT))->
        SetSel(0, -1, FALSE);
}

void CEditDlg::OnEnKillfocusEditMarkIn()
{
    TRACE("CEditDlg::OnEnKillfocusEditMarkIn()\n");
    parse_input_tc();
    mark_input_id = 0;
}

void CEditDlg::OnEnSetfocusEditMarkIn()
{
    TRACE("CEditDlg::OnEnSetfocusEditMarkIn()\n");
    ((CEdit*)GetDlgItem(mark_input_id = IDC_EDIT_MARK_IN))->
        SetSel(0, -1, FALSE);
}

void CEditDlg::list_area_state_adopt()
{
    CRect rc_btn, rc_wnd;
    int
        h_g = (list_area_state & LIST_AREA_STATE_VISIBLE)?SW_HIDE:SW_SHOW,
        s_g = (list_area_state & LIST_AREA_STATE_VISIBLE)?SW_SHOW:SW_HIDE,
        s_j = ((list_area_state & LIST_AREA_STATE_VISIBLE) && (list_area_state & LIST_AREA_STATE_JUNK))?SW_SHOW:SW_HIDE,
        s_n = ((list_area_state & LIST_AREA_STATE_VISIBLE) && !(list_area_state & LIST_AREA_STATE_JUNK))?SW_SHOW:SW_HIDE;

    /* show / hide objects */
    GetDlgItem(IDC_LIST_RECORDS)->ShowWindow(s_g);
    GetDlgItem(IDC_BUTTON_LIST_LIST)->ShowWindow(s_g);
    GetDlgItem(IDC_BUTTON_LIST_JUNK)->ShowWindow(s_g);
    GetDlgItem(IDC_BUTTON_LIST_CREATE)->ShowWindow(s_n);
    GetDlgItem(IDC_BUTTON_LIST_OPEN)->ShowWindow(s_n);
    GetDlgItem(IDC_BUTTON_LIST_DELETE)->ShowWindow(s_n);
    GetDlgItem(IDC_BUTTON_LIST_DELETE_PERM)->ShowWindow(s_j);
    GetDlgItem(IDC_BUTTON_LIST_RESTORE)->ShowWindow(s_j);
    GetDlgItem(IDC_LIST_HIDE)->ShowWindow(s_g);
    GetDlgItem(IDC_LIST_AREA)->ShowWindow(s_g);
    GetDlgItem(IDC_LIST_SHOW)->ShowWindow(h_g);

    /* buttons push state */
    int s, r;
    r = !(list_area_state & LIST_AREA_STATE_JUNK);
    s = ((CButton*)GetDlgItem(IDC_BUTTON_LIST_LIST))->GetState();
    if(r != (s & BST_PUSHED))
        ((CButton*)GetDlgItem(IDC_BUTTON_LIST_LIST))->SetState(r);

    r = list_area_state & LIST_AREA_STATE_JUNK;
    s = ((CButton*)GetDlgItem(IDC_BUTTON_LIST_JUNK))->GetState();
    if(r != (s & BST_PUSHED))
        ((CButton*)GetDlgItem(IDC_BUTTON_LIST_JUNK))->SetState(r);

    if(list_area_state & LIST_AREA_STATE_VISIBLE)
        GetDlgItem(IDC_LIST_HIDE)->GetWindowRect(&rc_btn);
    else
        GetDlgItem(IDC_LIST_SHOW)->GetWindowRect(&rc_btn);
    GetWindowRect(&rc_wnd);
    rc_wnd.right = rc_btn.right + (rc_btn.right - rc_btn.left) / 2;
    MoveWindow(&rc_wnd, 1);
};

BOOL CEditDlg::list_area_notify(unsigned int id)
{
    switch(id)
    {
        case IDC_LIST_SHOW:
            list_area_state |= LIST_AREA_STATE_VISIBLE;
            break;

        case IDC_LIST_HIDE:
            list_area_state &= ~LIST_AREA_STATE_VISIBLE;
            break;

        case IDC_BUTTON_LIST_LIST:
            TRACE("IDC_BUTTON_LIST_LIST\n");
            if(list_area_state & LIST_AREA_STATE_JUNK)
            {
                list_area_state &= ~LIST_AREA_STATE_JUNK;
                update_list(0);
            };
            break;

        case IDC_BUTTON_LIST_JUNK:
            TRACE("IDC_BUTTON_LIST_JUNK\n");
            if(!(list_area_state & LIST_AREA_STATE_JUNK))
            {
                list_area_state |= LIST_AREA_STATE_JUNK;
                update_list(1);
            };
            break;
    };

    list_area_state_adopt();

    return TRUE;
};

void CEditDlg::OnNMDblclkListRecords(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: Add your control notification handler code here
    *pResult = 0;

    int id = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->GetItemData(pNMItemActivate->iItem);
    theApp.m_ctl->load_reel(id);
}

BOOL CEditDlg::ctl_button(unsigned int id)
{
    switch(id)
    {
        case IDC_BUTTON_GOTO_IN:
            theApp.m_ctl->oper_goto_in();
            break;
        case IDC_BUTTON_GOTO_OUT:
            theApp.m_ctl->oper_goto_out();
            break;
        case IDC_BUTTON_MARK_IN:
            theApp.m_ctl->oper_mark_in();
            break;
        case IDC_BUTTON_MARK_OUT:
            theApp.m_ctl->oper_mark_out();
            break;
        case IDC_BUTTON_CTL_FAST_REV:
            theApp.m_ctl->oper_fast_rev();
            break;
        case IDC_BUTTON_CTL_STEP_REV:
            theApp.m_ctl->oper_step_rev();
            break;
        case IDC_BUTTON_CTL_PAUSE:
            theApp.m_ctl->oper_play_stop();
            break;
        case IDC_BUTTON_CTL_PLAY:
            theApp.m_ctl->oper_play();
            break;
        case IDC_BUTTON_CTL_STEP_FF:
            theApp.m_ctl->oper_step_ff();
            break;
        case IDC_BUTTON_CTL_FAST_FF:
            theApp.m_ctl->oper_fast_ff();
            break;
        case IDC_BUTTON_CTL_RECORD:
            theApp.m_ctl->oper_record();
            break;
    };

    return TRUE;
};

void CEditDlg::COmnCallbackNotify(int id, void* data)
{
    static const int online_sencitives[] =
    {
        IDC_BUTTON_GOTO_IN,
        IDC_BUTTON_GOTO_OUT,
        IDC_BUTTON_MARK_IN,
        IDC_BUTTON_MARK_OUT,
        IDC_BUTTON_CTL_FAST_REV,
        IDC_BUTTON_CTL_STEP_REV,
        IDC_BUTTON_CTL_PAUSE,
        IDC_BUTTON_CTL_PLAY,
        IDC_BUTTON_CTL_STEP_FF,
        IDC_BUTTON_CTL_FAST_FF,
        IDC_BUTTON_CTL_RECORD,
        IDC_EDIT_MARK_IN,
        IDC_EDIT_MARK_OUT,
        0
    };

    int r, i;
    char buf[1024];

    switch(id)
    {
        case COmnCallback::Rem:
            r = *((int*)data);
            _snprintf(buf, sizeof(buf), "Free: %dh %dm",
                r / 3600, (r % 3600) / 60);
            m_StatusBar->SetText(buf, 0, 0);
            break;

        case COmnCallback::Status:
            for(i = 0; online_sencitives[i]; i++)
                GetDlgItem(online_sencitives[i])->EnableWindow(!data);
            if(data)
            {
                SET_TEXT(CStatic, IDC_LABEL_TC, "");
                SET_TEXT(CStatic, IDC_LABEL_SPEED, "");
                SET_TEXT(CStatic, IDC_LABEL_TITLE, "");
                m_StatusBar->SetText("", 0, 0);
                m_StatusBar->SetText("OFFLINE", 1, 0);
                m_StatusBar->SetText("", 2, 0);
            };
            break;

        case COmnCallback::State:
            r = *((int*)data);
            if(r != m_omn_state)
            {
                m_omn_state = r;
                RedrawWindow();
            };
            m_StatusBar->SetText(OmPlrState_text((OmPlrState)m_omn_state), 1, 0);
            break;

        case COmnCallback::Pos:
            tc_frames2txt(theApp.m_ctl->get_pos(), buf);
            SET_TEXT(CStatic, IDC_LABEL_TC, buf);
            break;

        case COmnCallback::Rate:
            _snprintf(buf, sizeof(buf), "%.2f X", *((double*)data));
            SET_TEXT(CStatic, IDC_LABEL_SPEED, buf);
            break;

        case COmnCallback::MarkIN:
            r = *((int*)data);
            if(r < 0)
                buf[0] = 0;
            else
                tc_frames2txt(r, buf);
            SET_TEXT(CStatic, IDC_EDIT_MARK_IN, buf);
            TRACE("COmnCallback::MarkIN [%s]\n", buf);
            break;

        case COmnCallback::MarkOUT:
            r = *((int*)data);
            if(r < 0)
                buf[0] = 0;
            else
                tc_frames2txt(r, buf);
            SET_TEXT(CStatic, IDC_EDIT_MARK_OUT, buf);
            TRACE("COmnCallback::MarkOUT [%s]\n", buf);
            break;

        case COmnCallback::Reel:
            m_StatusBar->SetText((char*)data, 2, 0);
            SET_TEXT(CStatic, IDC_LABEL_TITLE, (char*)data);
            break;

        case COmnCallback::ReelsUpdated:
            update_list(list_area_state & LIST_AREA_STATE_JUNK);
            break;
    };
};


int CEditDlg::parse_input_tc()
{
    int r;
    char buf[1024];

    /* get text */
    GetDlgItem(mark_input_id)->GetWindowText(buf, sizeof(buf));

    /* try to parse */
    r = tc_txt2frames2(buf);

    /* set */
    if(mark_input_id == IDC_EDIT_MARK_OUT)
        theApp.m_ctl->set_mark_out(r);
    else
        theApp.m_ctl->set_mark_in(r);

    ((CEdit*)GetDlgItem(mark_input_id))->SetSel(0, -1, FALSE);

    return 0;
};

void CEditDlg::OnBnClickedButtonListCreate()
{
    int r;
    CReelNameInput dlg;

    dlg.m_title = "New Record";
    r = dlg.DoModal();
    if(r == IDOK)
    {
        theApp.m_ctl->new_reel((LPTSTR)(LPCSTR)dlg.m_title);
        update_list(0);
    };
}

void CEditDlg::OnBnClickedButtonListDelete()
{
    int idx, id;
    POSITION pos;

    pos = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetFirstSelectedItemPosition();
    if(!pos)
        return;

    idx = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetNextSelectedItem(pos);

    id = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetItemData(idx);

    theApp.m_ctl->destroy_reel(id);

    update_list(list_area_state & LIST_AREA_STATE_JUNK);
}

void CEditDlg::OnBnClickedButtonListRestore()
{
    int idx, id;
    POSITION pos;

    pos = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetFirstSelectedItemPosition();
    if(!pos)
        return;

    idx = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetNextSelectedItem(pos);

    id = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetItemData(idx);

    theApp.m_ctl->undelete_reel(id);

    update_list(list_area_state & LIST_AREA_STATE_JUNK);
}

void CEditDlg::OnBnClickedButtonListOpen()
{
    POSITION pos;
    int r, idx, id;

    pos = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetFirstSelectedItemPosition();
    if(!pos)
        return;

    idx = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetNextSelectedItem(pos);

    id = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetItemData(idx);

    r = theApp.m_ctl->load_reel(id);
};

void CEditDlg::OnBnClickedButtonListRename()
{
    POSITION pos;
    int r, idx, id;
    COmnReel* reel;
    CReelNameInput dlg;

    pos = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetFirstSelectedItemPosition();
    if(!pos)
        return;

    idx = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetNextSelectedItem(pos);

    id = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetItemData(idx);

    reel = theApp.m_ctl->list_reel(id);

    dlg.m_title = reel->title;

    delete reel;

    r = dlg.DoModal();
    if(r == IDOK)
    {
        theApp.m_ctl->retitle_reel(id, (LPTSTR)(LPCSTR)dlg.m_title);
        update_list(list_area_state & LIST_AREA_STATE_JUNK);
    };
}

void CEditDlg::OnBnClickedButtonExport()
{
    int j;
    char buf[512];
    char export_cmd_line[1024];

    if(theApp.m_ctl->reel)
    {
        export_cmd_line[0] = 0;

        strncat(export_cmd_line, " /export ", sizeof(export_cmd_line));
        strncat(export_cmd_line, " /edl ", sizeof(export_cmd_line));
        _snprintf(buf, sizeof(buf), " %d ", theApp.m_ctl->reel->created_on);
        strncat(export_cmd_line, buf, sizeof(export_cmd_line));

        if(theApp.m_ctl->get_mark_in() >= 0)
        {
            strncat(export_cmd_line, " /mark_in ", sizeof(export_cmd_line));
            _snprintf(buf, sizeof(buf), " %d ", theApp.m_ctl->get_mark_in());
            strncat(export_cmd_line, buf, sizeof(export_cmd_line));
        };
        if(theApp.m_ctl->get_mark_out() >= 0)
        {
            strncat(export_cmd_line, " /mark_out ", sizeof(export_cmd_line));
            _snprintf(buf, sizeof(buf), " %d ", theApp.m_ctl->get_mark_out());
            strncat(export_cmd_line, buf, sizeof(export_cmd_line));
        };

        strncat(export_cmd_line, " /omneon_host ", sizeof(export_cmd_line));
        strncat(export_cmd_line, theApp.cmdInfo.m_omneon_host, sizeof(export_cmd_line));

        strncat(export_cmd_line, " /omneon_player ", sizeof(export_cmd_line));
        strncat(export_cmd_line, theApp.cmdInfo.m_omneon_player, sizeof(export_cmd_line));

        strncat(export_cmd_line, " /omneon_dir ", sizeof(export_cmd_line));
        strncpy(buf, theApp.cmdInfo.m_omneon_dir, sizeof(buf));
        strncat(export_cmd_line, theApp.cmdInfo.m_omneon_dir, sizeof(export_cmd_line));

        for(j = 0; j < theApp.cmdInfo.m_omneon_dirs_cnt; j++)
        {
            strncat(export_cmd_line, " /export_dir ", sizeof(export_cmd_line));
            strncat(export_cmd_line, theApp.cmdInfo.m_omneon_dirs[j], sizeof(export_cmd_line));
        };

        strncat(export_cmd_line, " /msc3_serial_port ", sizeof(export_cmd_line));
        _snprintf(buf, sizeof(buf), " %d ", theApp.cmdInfo.m_msc3_serial_port);
        strncat(export_cmd_line, buf, sizeof(export_cmd_line));

        GetModuleFileName(NULL, buf, sizeof(buf));
        TRACE("Starting APP:\n\t%s\nwith ARGS:\n\t%s\n", buf, export_cmd_line);
        ShellExecute(NULL, NULL, buf, export_cmd_line, NULL, SW_SHOW);
    };
}

void CEditDlg::OnNMRDblclkListRecords(NMHDR *pNMHDR, LRESULT *pResult)
{
    int id, r;
    COmnReel* reel;
    CReelNameInput dlg;

    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    *pResult = 0;

    id = ((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->GetItemData(pNMItemActivate->iItem);

    reel = theApp.m_ctl->list_reel(id);

    dlg.m_title = reel->title;

    delete reel;

    r = dlg.DoModal();
    if(r == IDOK)
    {
        theApp.m_ctl->retitle_reel(id, (LPTSTR)(LPCSTR)dlg.m_title);
        update_list(list_area_state & LIST_AREA_STATE_JUNK);
    };

}

/*
    http://public.googlecode.com/svn-history/r65/trunk/HDC.cpp
*/
BOOL FillSolidRect(HDC hDC, int x, int y, int cx, int cy, COLORREF clr)
{
    if (!hDC)
    {
        return FALSE;
    }
    RECT rect = {x, y, x + cx, y + cy};	
    COLORREF crOldBkColor = ::SetBkColor(hDC, clr);	
    ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
    ::SetBkColor(hDC, crOldBkColor);	
    return TRUE;
}
BOOL FillSolidRect(HDC hDC, const RECT* pRC, COLORREF crColor)
{
    HBRUSH hBrush = NULL, hOldBrush = NULL;
    if (!hDC || !pRC)
    {
        return FALSE;
    }
    return FillSolidRect(hDC, pRC->left, pRC->top, pRC->right - pRC->left, pRC->bottom - pRC->top, crColor);
}

/*
    http://support.microsoft.com/kb/179909/ru
    http://stackoverflow.com/questions/9293298/mfc-image-button-with-transparency
    http://www.cpp-home.com/tutorials/176_4.htm
    http://social.msdn.microsoft.com/Forums/en-US/vssmartdevicesnative/thread/c78a4c4c-bc31-4695-a229-59cfe6b56732/
    http://stackoverflow.com/questions/3005685/load-a-png-resource-into-a-cbitmap
*/
void CEditDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpdis)
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
//        TRACE("OnDrawItem: item %d, has state %.2X\n", nIDCtl, i);

        if
        (
            nIDCtl == IDC_BUTTON_CTL_RECORD
            &&
            (
                m_omn_state == omPlrStateRecord
                ||
                (
                    m_omn_state == omPlrStateCueRecord
                    &&
                    (m_rec_blink & 1)
                )
            )
        )
                idx++;

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

void CEditDlg::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpdis)
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

HBRUSH CEditDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    int id = pWnd->GetDlgCtrlID();

    HBRUSH h = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

    if
    (
        id == IDC_LABEL_TC
        &&
        m_omn_state == omPlrStateRecord
    )
        pDC->SetTextColor(RGB(219, 16, 16));

    return h;
};

void CEditDlg::OnBnClickedButtonUndo()
{
    theApp.m_ctl->undo();
    update_list(list_area_state & LIST_AREA_STATE_JUNK);
}

void CEditDlg::OnLvnItemchangedListRecords(NMHDR *pNMHDR, LRESULT *pResult)
{
    int i, r = 0;
    static const int ids[] =
    {
        IDC_BUTTON_LIST_CREATE,
        IDC_BUTTON_LIST_OPEN,
        IDC_BUTTON_LIST_RESTORE,
        IDC_BUTTON_LIST_DELETE,
        IDC_BUTTON_LIST_DELETE_PERM,
        0
    };

    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if(pResult)
        *pResult = 0;

    if(((CListCtrl*)GetDlgItem(IDC_LIST_RECORDS))->
        GetFirstSelectedItemPosition())
        r = 1;
    TRACE("OnLvnItemchangedListRecords: r=%d\n", r);
    for(i = 0; i < ids[i]; i++)
    {
        CWnd* w = GetDlgItem(i);
        if(w)
            w->EnableWindow(r);
    };
}
