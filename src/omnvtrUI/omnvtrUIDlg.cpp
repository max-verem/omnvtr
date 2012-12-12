
// omnvtrUIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "omnvtrUI.h"
#include "omnvtrUIDlg.h"
#include "IDInputDlg.h"
#include "ExportDlg.h"

#include "../mcs3/mcs3.h"
#include "timecode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <ommedia.h>

#pragma comment(lib, "ommedia.lib")

static int omn_concat_videos
(
    char* omn_host,
    char* omn_dir,
    char* id,
    int f_new,
    int record_id,
    int mark_in,
    int mark_out,
    char* ext
)
{
    int r, d = 0;
    OmMediaCopier omc;
    OmMediaInfo omi;
    OmMediaSummary oms;
    OmMediaQuery omq;
    char id_origin[256], id_recorded[256], id_target[256];

#ifdef _DEBUG
    omc.setDebug("c:\\temp\\omnvtr.log1");
#endif

    /* build filenames */
    _snprintf(id_origin, sizeof(id_origin),
        "\\\\%s%s/%s.%s", omn_host, omn_dir, id, ext);
    _snprintf(id_recorded, sizeof(id_recorded),
        "\\\\%s%s/%s-%d.%s", omn_host, omn_dir, id, record_id, ext);
    _snprintf(id_target, sizeof(id_target),
        "\\\\%s%s/%s-temp.%s", omn_host, omn_dir, id, ext);

    omc.setDestination(id_target, 1);

    /* append first segment */
    if(!f_new && mark_in)
    {
        r = omc.appendTracks(0, id_origin, 0, mark_in);
        d += mark_in;
    };

    /* append recorded segment */
    r = omc.appendTracks(0, id_recorded);
    r = omq.setFile(id_recorded);
    r = omq.getMovieInfo(omi);
    d += omi.defaultOut - omi.defaultIn;

    /* append last segment */
    if(mark_out >= 0)
    {
        r = omc.appendTracks(0, id_origin, mark_out, ~0);
        r = omq.setFile(id_origin);
        r = omq.getMovieInfo(omi);
        d += omi.defaultOut - omi.defaultIn - mark_out;
    };

    omc.setOutputSuffix(omMediaFileTypeQt, ext);
    omc.setCopyType(omReferenceCopy);
    r = omc.setRange(0, d);
    r = omc.copy();
    omc.release();

    /* cleanup files */
    r = DeleteFile(id_origin);
    r = MoveFile(id_target, id_origin);
    r = DeleteFile(id_recorded);

    return 0;
};


// ComnvtrUIDlg dialog

#define SET_TEXT(TYPE, ID, TEXT) \
    ((TYPE*)GetDlgItem(ID))->SetWindowText(TEXT);

#define ID_MCS3_BUTTON  (WM_APP + 100)
#define ID_MCS3_JOG     (WM_APP + 101)
#define ID_MCS3_SHUTTLE (WM_APP + 102)
#define MCS3_BUTTON_MARK_IN MCS3_BUTTON_W3
#define MCS3_BUTTON_MARK_OUT MCS3_BUTTON_W5


void ComnvtrUIDlg_mcs3(void* cookie, int button, int value)
{
    ComnvtrUIDlg* dlg = (ComnvtrUIDlg*)cookie;

    /* do not send any message in OFFLINE mode */
    if(!dlg->m_director.f_online)
        return;

    if(MCS3_SHUTTLE == button)
        ::SendMessage(dlg->GetSafeHwnd(), ID_MCS3_SHUTTLE, button, (LPARAM)value);
    else if(MCS3_JOG == button)
        ::SendMessage(dlg->GetSafeHwnd(), ID_MCS3_JOG, button, (LPARAM)value);
    else
        ::SendMessage(dlg->GetSafeHwnd(), ID_MCS3_BUTTON, button, (LPARAM)value);
};

ComnvtrUIDlg::ComnvtrUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ComnvtrUIDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void ComnvtrUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(ComnvtrUIDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BUTTON_EXIT, &ComnvtrUIDlg::OnBnClickedButtonExit)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_BUTTON_NEW, &ComnvtrUIDlg::OnBnClickedButtonNew)
    ON_BN_CLICKED(IDC_BUTTON_LOAD, &ComnvtrUIDlg::OnBnClickedButtonLoad)
    ON_EN_UPDATE(IDC_EDIT_MARK_IN, &ComnvtrUIDlg::OnEnUpdateEditMarkIn)
    ON_WM_CTLCOLOR()
    ON_MESSAGE(ID_MCS3_BUTTON, OnMCS3Button)
    ON_MESSAGE(ID_MCS3_JOG, OnMCS3Jog)
    ON_MESSAGE(ID_MCS3_SHUTTLE, OnMCS3Shuttle)
    ON_BN_CLICKED(IDC_BUTTON_EXPORT, &ComnvtrUIDlg::OnBnClickedButtonExport)
END_MESSAGE_MAP()

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

void ComnvtrUIDlg::OnTimer(UINT nIDEvent)
{
    int r;

    WaitForSingleObject(m_director.lock, INFINITE);

    m_director.status_curr.size = sizeof(OmPlrStatus);
    r = OmPlrGetPlayerStatus(m_director.handle, &m_director.status_curr);
    if(r != 0)
        m_director.f_online = 0;
    else
        m_director.f_online = 1;

    ReleaseMutex(m_director.lock);

    if(m_director.f_online && m_director.f_first)
    {
        char buf[1024];

        if(m_director.status_curr.state == omPlrStateCueRecord ||
            m_director.status_curr.state == omPlrStateRecord)
        {
            char buf2[1024];
            _snprintf(buf, sizeof(buf), "%s [%s]",
                OmPlrState_text(m_director.status_curr.state),
                tc_frames2txt(m_director.status_curr.pos, buf2));
        }
        else
            strcpy(buf, OmPlrState_text(m_director.status_curr.state));
        SET_TEXT(CStatic, IDC_STATIC_STATUS, buf);

        if(m_director.id[0])
        {
            SET_TEXT(CStatic, IDC_STATIC_VIDEO_ID, m_director.id);
        }
        else
        {
            _snprintf(buf, sizeof(buf), "*%s*", m_director.status_curr.currClipName);
            SET_TEXT(CStatic, IDC_STATIC_VIDEO_ID, buf);
        };

        _snprintf(buf, sizeof(buf), "%.2f", m_director.status_curr.rate);
        SET_TEXT(CStatic, IDC_STATIC_SPEED, buf);

        if(m_director.status_curr.state == omPlrStateCueRecord ||
            m_director.status_curr.state == omPlrStateRecord)
            tc_frames2txt(m_director.status_curr.pos + m_director.mark_in, buf);
        else
            tc_frames2txt(m_director.status_curr.pos, buf);

        SET_TEXT(CStatic, IDC_EDIT_POS, buf);

        /* omPlrStateCueRecord -> omPlrStateStopped */
        if
        (
            m_director.status_prev.state == omPlrStateCueRecord
            &&
            m_director.status_curr.state == omPlrStateStopped
            &&
            m_director.record_id
        )
        {
            char buf[1024];

            /* delete recorded id */
            OmPlrDetachAllClips(m_director.handle);
            _snprintf(buf, sizeof(buf), "%s-%d", m_director.id, m_director.record_id);
            OmPlrClipDelete(m_director.handle, buf);

            /* load clip and seek to pos */
            load_clip(m_director.mark_curr);
        };

        /* omPlrStateRecord -> omPlrStateStopped */
        if
        (
            m_director.status_prev.state == omPlrStateRecord
            &&
            m_director.status_curr.state == omPlrStateStopped
            &&
            m_director.record_id
        )
        {
            window_lock(0, "creating files...");

            OmPlrDetachAllClips(m_director.handle);

            /* concat id */
            omn_concat_videos
            (
                theApp.cmdInfo.m_omneon_host,
                theApp.cmdInfo.m_omneon_dir,
                m_director.id,
                m_director.f_new,
                m_director.record_id,
                m_director.mark_in,
                m_director.mark_out,
                "mov"
            );

            /* load clip and seek to pos */
            m_director.f_new = 0;

            /* load clip and seek to pos */
            load_clip(m_director.mark_in + m_director.status_curr.pos);

            window_lock(1);

        };
    }
    else
    {
        m_director.f_first = 0;
        SET_TEXT(CStatic, IDC_STATIC_STATUS, "OFFLINE");
        SET_TEXT(CStatic, IDC_STATIC_VIDEO_ID, "");
        SET_TEXT(CStatic, IDC_STATIC_SPEED, "");
        SET_TEXT(CStatic, IDC_EDIT_POS, "");
    };

    if(m_director.f_online)
    {
        m_director.f_first = 1;
        m_director.status_prev = m_director.status_curr;
    };
}

// ComnvtrUIDlg message handlers

BOOL ComnvtrUIDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

    /* clear */
    m_director.id[0] = 0;
    m_director.f_first = 0;
    m_director.record_id = 0;
    m_director.jog_step = 60;
    m_director.keys = 0;

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
    SetTimer(2, 40, NULL);

    SET_TEXT(CStatic, IDC_STATIC_INFO1, "Load existing clip or create new");

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void ComnvtrUIDlg::OnPaint()
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
HCURSOR ComnvtrUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void ComnvtrUIDlg::OnBnClickedButtonExit()
{
    CDialog::OnCancel();
}

void ComnvtrUIDlg::load_clip(int pos)
{
    int r;
    unsigned int l;

    WaitForSingleObject(m_director.lock, INFINITE);

    /* stop omneon */
    OmPlrStop(m_director.handle);
    OmPlrDetachAllClips(m_director.handle);

    if(!m_director.f_new)
        OmPlrAttach
        (
            m_director.handle,
            m_director.id,
            omPlrClipDefaultIn,
            omPlrClipDefaultOut,
            0,
            omPlrShiftModeAfter,
            &l
        );

    /* 4. Set timeline min/max */
    r = OmPlrSetMinPosMin(m_director.handle);
    r = OmPlrSetMaxPosMax(m_director.handle);

    /* 5. Set timeline position */
    r = OmPlrSetPos(m_director.handle, pos);

    /* 6. Cue */
    r = OmPlrCuePlay(m_director.handle, 0.0);

    /* 7. Cue */
    r = OmPlrPlay(m_director.handle, 0.0);

    m_director.record_id = 0;

    ReleaseMutex(m_director.lock);
};

void ComnvtrUIDlg::ui_load_clip(int f_new)
{
    if(!m_director.f_online)
        return;

    int r;
    CIDInputDlg dlg;
    OmPlrClipInfo clip_info;

    if (dlg.DoModal() == IDOK)
    {
        /* get clip info */
        WaitForSingleObject(m_director.lock, INFINITE);
        clip_info.maxMsTracks = 0;
        clip_info.size = sizeof(clip_info);
        r = OmPlrClipGetInfo(m_director.handle,
            (LPCSTR)dlg.m_id_val, &clip_info);
        ReleaseMutex(m_director.lock);

        if(r && !f_new)
        {
            MessageBox("ID NOT FOUND", "Error");
            return;
        }
        if(!r && f_new)
        {
            MessageBox("ID ALREADY EXIST", "Error");
            return;
        }

        /* setup data */
        strcpy(m_director.id, (LPCSTR)dlg.m_id_val);
        m_director.f_new = f_new;

        window_lock(0, "loading clip...");

        /* load clip */
        load_clip(0);

        window_lock(1);
    };
};

#define RED        RGB(127,  0,  0)
#define GREEN      RGB(  0,127,  0)

void ComnvtrUIDlg::OnEnUpdateEditMarkIn()
{

    CEdit *e = (CEdit*)GetDlgItem(IDC_EDIT_MARK_IN);


    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function to send the EM_SETEVENTMASK message to the control
    // with the ENM_UPDATE flag ORed into the lParam mask.

    // TODO:  Add your control notification handler code here
}

HBRUSH ComnvtrUIDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    int id = pWnd->GetDlgCtrlID();

    HBRUSH h = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

    if(id == IDC_EDIT_MARK_IN || id == IDC_EDIT_MARK_OUT)
    {
        int tc_int;
        char tc_txt_src[32], tc_txt_dst[32];

        pWnd->GetWindowText(tc_txt_src, sizeof(tc_txt_src));

        /* empty timecode string */
        if(!strlen(tc_txt_src))
        {
            if(id == IDC_EDIT_MARK_IN)
                m_director.mark_in = -1;
            else
                m_director.mark_out = -1;
        };

        tc_int = tc_txt2frames(tc_txt_src);
        tc_frames2txt(tc_int, tc_txt_dst);
        if(strcmp(tc_txt_src, tc_txt_dst))
        {
//            pDC->SetBkColor(RGB(0,0,255));
            pDC->SetTextColor(RGB(127, 0, 0));
        }
        else
        {
//            pDC->SetBkColor(RGB(0,0,255));
            pDC->SetTextColor(RGB(0, 127, 0));
            if(id == IDC_EDIT_MARK_IN)
                m_director.mark_in = tc_int;
            else
                m_director.mark_out = tc_int;
        };
    };

    return h;
};

LRESULT ComnvtrUIDlg::OnMCS3Jog(WPARAM wParam, LPARAM lParam)
{
    int value = lParam;

    if(m_director.jog_accum * value < 0)
        m_director.jog_accum = 0;

    m_director.jog_accum += value;
    if(abs(m_director.jog_accum) > m_director.jog_step)
    {
        m_director.jog_accum = m_director.jog_accum % m_director.jog_step;

        WaitForSingleObject(m_director.lock, INFINITE);
        OmPlrStep(m_director.handle, (m_director.jog_accum < 0)?-1:1);
        ReleaseMutex(m_director.lock);
    };

    return 0;
}
LRESULT ComnvtrUIDlg::OnMCS3Shuttle(WPARAM wParam, LPARAM lParam)
{
    double s;
    int value = lParam;
    static const double shuttle_speeds[13] =
    {
        0.0, 0.03, 0.1, 0.2, 0.5,
        1.0, 2.0, 3.0, 5.0, 7.0,
        10.0, 24.0, 42.0
    };

    if(value < 0)
        s = -shuttle_speeds[-value];
    else
        s = shuttle_speeds[value];

    WaitForSingleObject(m_director.lock, INFINITE);
    OmPlrPlay(m_director.handle, s);
    ReleaseMutex(m_director.lock);

    return 0;
}

void ComnvtrUIDlg::oper_mark_in()
{
    char buf[1024];

    if(m_director.status_curr.state == omPlrStatePlay)
        SET_TEXT(CEdit, IDC_EDIT_MARK_IN,
            tc_frames2txt(m_director.mark_in = m_director.status_curr.pos, buf));
};

void ComnvtrUIDlg::oper_mark_out()
{
    char buf[1024];

    if(m_director.status_curr.state == omPlrStatePlay)
        SET_TEXT(CEdit, IDC_EDIT_MARK_OUT,
            tc_frames2txt(m_director.mark_out = m_director.status_curr.pos, buf));
};

void ComnvtrUIDlg::oper_stop()
{
    WaitForSingleObject(m_director.lock, INFINITE);
    if(m_director.status_curr.state == omPlrStatePlay)
        OmPlrPlay(m_director.handle, 0.0);
    else
        OmPlrStop(m_director.handle);
    ReleaseMutex(m_director.lock);
};

void ComnvtrUIDlg::oper_play()
{
    WaitForSingleObject(m_director.lock, INFINITE);
    if(m_director.status_curr.state == omPlrStatePlay)
        OmPlrPlay(m_director.handle, 1.0);
    ReleaseMutex(m_director.lock);
};

void ComnvtrUIDlg::oper_backward()
{
    WaitForSingleObject(m_director.lock, INFINITE);
    if(m_director.status_curr.state == omPlrStatePlay)
    {
        double s;

        if(m_director.status_curr.rate >= 0)
            s = -1.0;
        else
            s = 2.0 * m_director.status_curr.rate;

        OmPlrPlay(m_director.handle, s);
    };
    ReleaseMutex(m_director.lock);
}

void ComnvtrUIDlg::oper_forward()
{
    WaitForSingleObject(m_director.lock, INFINITE);
    if(m_director.status_curr.state == omPlrStatePlay)
    {
        double s;

        if(m_director.status_curr.rate <= 0)
            s = 1.0;
        else
            s = 2.0 * m_director.status_curr.rate;

        OmPlrPlay(m_director.handle, s);
    };
    ReleaseMutex(m_director.lock);
}

void ComnvtrUIDlg::oper_record_record()
{
    WaitForSingleObject(m_director.lock, INFINITE);
    if(m_director.status_curr.state == omPlrStateCueRecord)
        OmPlrRecord(m_director.handle);
    ReleaseMutex(m_director.lock);
};

void ComnvtrUIDlg::oper_cue_record()
{
    int r;
    char buf[1024];
    unsigned int l;

    WaitForSingleObject(m_director.lock, INFINITE);
    if
    (
        (m_director.status_curr.state == omPlrStatePlay) &&
        (m_director.id[0] || m_director.f_new) &&
        m_director.mark_in >= 0
    )
    {
        window_lock(0, "preparing to record...");

        /* create a time id */
        time((time_t*)&m_director.record_id);

        /* compose new id */
        _snprintf(buf, sizeof(buf), "%s-%d", m_director.id, m_director.record_id);

        /* save curr pos */
        m_director.mark_curr = m_director.status_curr.pos;

        /* 1. stop */
        OmPlrStop(m_director.handle);
        OmPlrDetachAllClips(m_director.handle);

        /* 2. remove from server */
        OmPlrClipDelete(m_director.handle, buf);

        /* 3. attach the new clip */
        r = OmPlrAttach
        (
            m_director.handle,
            buf,                    /* pClipName */
            0,                      /* clipIn */
            25*3600,                /* clipOut */ 
            0,                      /* attachBeforeClip */
            omPlrShiftModeAfter,    /* shift */
            &l                      /* *pClipAttachHandle */
        );

        /* 4. setup MIN/MAX positions of timeline */
        OmPlrSetMinPosMin(m_director.handle);
        OmPlrSetMaxPosMax(m_director.handle);

        /* 5. setup start pos */
        r = OmPlrSetPos(m_director.handle, 0);

        /* 6. cue record */
        r = OmPlrCueRecord(m_director.handle);

        ReleaseMutex(m_director.lock);

        window_lock(1);
    };
    ReleaseMutex(m_director.lock);
}

LRESULT ComnvtrUIDlg::OnMCS3Button(WPARAM wParam, LPARAM lParam)
{
    int button = wParam;

    /* ignore relase button */
    if(!lParam) return 0;


    switch(button)
    {
        case MCS3_BUTTON_MARK_IN:
            oper_mark_in();
            break;

        case MCS3_BUTTON_MARK_OUT:
            oper_mark_out();
            break;

        case MCS3_BUTTON_STOP:
            oper_stop();
            break;

        case MCS3_BUTTON_PLAY:
            oper_play();
            break;

        case MCS3_BUTTON_BACKWARD:
            oper_backward();
            break;

        case MCS3_BUTTON_FORWARD:
            oper_forward();
            break;

        case MCS3_BUTTON_RECORD:
            oper_record_record();
            oper_cue_record();
            break;
        };

    return 0;
}

void ComnvtrUIDlg::window_lock(int l, char* msg)
{
    SET_TEXT(CStatic, IDC_STATIC_INFO1, msg);
    EnableWindow(l);
    UpdateWindow();
};
;
void ComnvtrUIDlg::OnBnClickedButtonExport()
{
    ExportDlg dlg;

    if(m_director.f_new || !m_director.id[0])
        return;

    dlg.DoModal();
}

BOOL ComnvtrUIDlg::PreTranslateMessage(MSG* pMsg)
{
    if(pMsg->message != WM_KEYDOWN && pMsg->message != WM_KEYUP)
        return CDialog::PreTranslateMessage(pMsg);

    if(pMsg->message == WM_KEYUP)
    {
        if(pMsg->wParam != 'W')
            return CDialog::PreTranslateMessage(pMsg);

        m_director.keys = 0;

        return TRUE;
    };

    switch(pMsg->wParam)
    {
        case 'W':
            m_director.keys = 1;
            break;

        case 'I':
            oper_mark_in();
            break;

        case 'O':
            oper_mark_out();
            break;

        case VK_ESCAPE:
            oper_stop();
            break;

        case ' ':
        case 'S':
            WaitForSingleObject(m_director.lock, INFINITE);
            if(m_director.status_curr.state == omPlrStatePlay)
            {
                if(0.0 == m_director.status_curr.rate)
                    OmPlrPlay(m_director.handle, 1.0);
                else
                    OmPlrPlay(m_director.handle, 0.0);
            }
            else if(m_director.status_curr.state == omPlrStateCueRecord)
                OmPlrRecord(m_director.handle);
            else if(m_director.status_curr.state == omPlrStateRecord)
                OmPlrStop(m_director.handle);
            ReleaseMutex(m_director.lock);
            break;

        case 'A':
            if(m_director.keys)
            {
                WaitForSingleObject(m_director.lock, INFINITE);
                OmPlrStep(m_director.handle, -1);
                ReleaseMutex(m_director.lock);
            }
            else
            {
                oper_backward();
            };
            break;

        case 'D':
            if(m_director.keys)
            {
                WaitForSingleObject(m_director.lock, INFINITE);
                OmPlrStep(m_director.handle, 1);
                ReleaseMutex(m_director.lock);
            }
            else
            {
                oper_forward();
            };
            break;

        case 'R':
            oper_record_record();
            oper_cue_record();
            break;

        default:
            return CDialog::PreTranslateMessage(pMsg);
    };

    return TRUE;
};

