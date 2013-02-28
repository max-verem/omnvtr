#include "stdafx.h"
#include <string.h>

#include "OmnCtl.h"

#pragma comment(lib, "omplrlib.lib") 

char* strncpy_rev_slash(char* dst, char* src, int lim)
{
    int i;

    for(i = 0; i < lim && src[i]; i++)
    {
        dst[i] = src[i];
        if('\\' == dst[i])
            dst[i] = '/';
    };
    dst[i] = 0;

    return dst;
};

COmnCtl::COmnCtl(char* host, char* player, char* dir)
{
    cb = NULL;
    record_id = 0;
    mark_in = mark_out = mark_curr = -1;
    reel = NULL;
    handle = NULL;
    lock = CreateMutex(NULL, FALSE, NULL);
    strncpy_rev_slash(this->host, host, sizeof(this->host));
    strncpy_rev_slash(this->player, player, sizeof(this->player));
    strncpy_rev_slash(this->dir, dir, sizeof(this->dir));
};

COmnCtl::~COmnCtl()
{
    disconnect();
    CloseHandle(lock);
};

int COmnCtl::disconnect()
{
    if(!handle)
        return -1;
    OmPlrClose(handle);
    handle = NULL;
    return 0;
};

int COmnCtl::connect()
{
    int r;

    if(handle)
        return 0;

    f_first = 1;

    /* open director */
    r = OmPlrOpen
    (
        host,
        player,
        &handle
    );
    if(r)
        return -1;

    /* setup directory */
    r = OmPlrClipSetDirectory(handle, dir);
    if(r)
    {
        disconnect();
        return -2;
    };

    return 0;
};

int COmnCtl::status()
{
    int r;

    if(!handle)
    {
        connect();
        if(cb)
            cb->COmnCallbackNotify(COmnCallback::Status,
                (void*)((handle)?0:-1));
        return -1;
    };

    WaitForSingleObject(lock, INFINITE);
    status_curr.size = sizeof(OmPlrStatus);
    r = OmPlrGetPlayerStatus(handle, &status_curr);
    if(r)
    {
        ReleaseMutex(lock);
        disconnect();
        if(cb)
            cb->COmnCallbackNotify(COmnCallback::Status, (void*)-1);
        return -1;
    };

    if(f_first)
    {
        f_first = 0;
        status_prev = status_curr;
        if(cb)
        {
            cb->COmnCallbackNotify(COmnCallback::Status, (void*)0);
            cb->COmnCallbackNotify(COmnCallback::State, (void*)&status_curr.state);
            cb->COmnCallbackNotify(COmnCallback::Pos, (void*)&status_curr.pos);
            cb->COmnCallbackNotify(COmnCallback::Rate, (void*)&status_curr.rate);
            cb->COmnCallbackNotify(COmnCallback::MarkIN, (void*)&mark_in);
            cb->COmnCallbackNotify(COmnCallback::MarkOUT, (void*)&mark_out);
            if(reel)
                cb->COmnCallbackNotify(COmnCallback::Reel, (void*)reel->title);
        };
        ReleaseMutex(lock);
        return -1;
    };

    /* nofity */
    if(cb)
    {
        unsigned int s;
        if(status_prev.state != status_curr.state)
            cb->COmnCallbackNotify(COmnCallback::State, (void*)&status_curr.state);
        if(status_prev.pos != status_curr.pos)
            cb->COmnCallbackNotify(COmnCallback::Pos, (void*)&status_curr.pos);
        if(status_prev.rate != status_curr.rate)
            cb->COmnCallbackNotify(COmnCallback::Rate, (void*)&status_curr.rate);
        OmPlrGetRecordTime(handle, &s);
        cb->COmnCallbackNotify(COmnCallback::Rem, (void*)&s);
    };

    /* omPlrStateCueRecord -> omPlrStateStopped */
    if
    (
        reel
        &&
        record_id
        &&
        status_prev.state == omPlrStateCueRecord
        &&
        status_curr.state == omPlrStateStopped
    )
    {
        char buf[1024];

        /* delete zero-recorded id */
        OmPlrDetachAllClips(handle);
        _snprintf(buf, sizeof(buf), "%d", record_id);
        OmPlrClipDelete(handle, buf);

        /* load clip and seek to pos */
        reload_reel(mark_curr);
    };

    /* omPlrStateRecord -> omPlrStateStopped */
    if
    (
        reel
        &&
        record_id
        &&
        status_prev.state == omPlrStateRecord
        &&
        status_curr.state == omPlrStateStopped
    )
    {
        char buf[1024];
        OmPlrClipInfo clip_info;
        OmPlrDetachAllClips(handle);

        _snprintf(buf, sizeof(buf), "%d", record_id);
        clip_info.maxMsTracks = 0;
        clip_info.size = sizeof(clip_info);
        r = OmPlrClipGetInfo(handle, (LPCSTR)buf, &clip_info);
        if(!r)
        {
            reel->add(record_id, clip_info.defaultIn, clip_info.defaultOut, mark_in, mark_out, 1);
            if(cb)
                cb->COmnCallbackNotify(COmnCallback::ReelsUpdated, NULL);
        };

        /* delete undo item*/
        r = reel->undo_clear();
        if(r)
        {
            _snprintf(buf, sizeof(buf), "%d", r);
            OmPlrClipDelete(handle, buf);
        };

        /* load clip and seek to pos */
        if(mark_in < 0)
            reload_reel(reel->dur());
        else
            reload_reel(mark_in + clip_info.defaultOut - clip_info.defaultIn);
    };

    ReleaseMutex(lock);

    status_prev = status_curr;

    return 0;
};

int COmnCtl::reload_reel(int pos)
{
    int r, i;
    unsigned int l;

    WaitForSingleObject(lock, INFINITE);

    /* stop omneon */
    OmPlrStop(handle);
    OmPlrDetachAllClips(handle);

    /* load playlist */
    if(reel)
    {
        for(i = 0; i < reel->play_cnt; i++)
        {
            char buf[512];

            _snprintf(buf, sizeof(buf), "%d", reel->play_list[i].id);

            r = OmPlrAttach
            (
                handle,
                buf,
                reel->play_list[i].clip_in,
                reel->play_list[i].clip_out,
                0,
                omPlrShiftModeAfter,
                &l
            );
        };
    };

    /* 4. Set timeline min/max */
    r = OmPlrSetMinPosMin(handle);
    r = OmPlrSetMaxPosMax(handle);

    /* 5. Set timeline position */
    r = OmPlrSetPos(handle, pos);

    /* 6. Cue */
    r = OmPlrCuePlay(handle, 0.0);

    /* 7. Cue */
    r = OmPlrPlay(handle, 0.0);

    record_id = 0;

    ReleaseMutex(lock);

    return 0;
};

int COmnCtl::new_reel(char *title)
{
    time_t id;
    char buf[512];

    if(reel)
        delete reel;

    time((time_t*)&id);
    _snprintf(buf, sizeof(buf), "\\\\%s%s\\%d.vtr", host, dir, id);

    reel = new COmnReel(buf, (int)id, title);

    set_mark_in(-1);
    set_mark_out(-1);

    reload_reel(mark_curr = 0);

    return 0;
};

int COmnCtl::load_reel(int id)
{
    char buf[512];

    if(reel)
        delete reel;

    _snprintf(buf, sizeof(buf), "\\\\%s%s\\%d.vtr", host, dir, id);

    reel = new COmnReel(buf);

    set_mark_in(-1);
    set_mark_out(-1);

    reload_reel(mark_curr = 0);

    if(cb)
        cb->COmnCallbackNotify(COmnCallback::Reel, reel->title);

    return 0;
};

int COmnCtl::get_pos()
{
    int p = status_curr.pos;

    /* update position */
    if
    (
        status_curr.state == omPlrStateCueRecord
        ||
        status_curr.state == omPlrStateRecord
    )
    {
        if(mark_in >= 0)
            p += mark_in;
        else if(reel)
            p += reel->dur();
    };

    return p;
};

double COmnCtl::get_rate()
{
    return status_curr.rate;
};

int COmnCtl::get_state()
{
   return status_curr.state;
};

int COmnCtl::get_rem()
{
    return 0;
};

char* COmnCtl::get_reel_title()
{
    if(reel)
        return reel->title;
    return "";
};

COmnReel* COmnCtl::list_reel(int id)
{
    char buf[1024];

    _snprintf(buf, sizeof(buf), "\\\\%s%s\\%d.vtr", host, dir, id);

    return new COmnReel(buf);
};

COmnReel** COmnCtl::list_reels()
{
    int c = 0;
    char path[512];
    WIN32_FIND_DATA ffd;
    COmnReel** list = NULL;

    _snprintf(path, sizeof(path), "\\\\%s%s\\*.vtr", host, dir);

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

            _snprintf(path, sizeof(path), "\\\\%s%s\\%s", host, dir, ffd.cFileName);

            if(!list)
                list = (COmnReel**)malloc(sizeof(COmnReel*) * 2);
            else
                list = (COmnReel**)realloc(list, sizeof(COmnReel*) * (c + 2));
            list[c] = new COmnReel(path);
            list[c + 1] = NULL;
            c++;
        }
        while (FindNextFile(hFind, &ffd) != 0);

        FindClose(hFind);
    };

    return list;
};

void COmnCtl::oper_record_record()
{
#if 0
    if(theApp.cmdInfo.m_syncplay_omneon_player[0] &&
        theApp.cmdInfo.m_syncplay_omneon_host[0])
    {
        int r;
        OmPlrHandle h;

        /* open director */
        r = OmPlrOpen
        (
            theApp.cmdInfo.m_syncplay_omneon_host,
            theApp.cmdInfo.m_syncplay_omneon_player,
            &h
        );
        if(!r)
        {
            OmPlrPlay(h, 1.0);
            OmPlrClose(h);
            Sleep(theApp.cmdInfo.m_syncplay_delay);
        }
    };
#endif
    OmPlrRecord(handle);
};

void COmnCtl::oper_record_cue()
{
    int r;
    time_t t;
    unsigned int l;
    char buf[1024];

    /* create a time id */
    time(&t);
    record_id = (int)t;

    /* compose new id */
    _snprintf(buf, sizeof(buf), "%d", record_id);

    /* save curr pos */
    mark_curr = status_curr.pos;

    /* 1. stop */
    OmPlrStop(handle);
    OmPlrDetachAllClips(handle);

    /* 2. remove from server */
    OmPlrClipDelete(handle, buf);

    /* 3. attach the new clip */
    r = OmPlrAttach
    (
        handle,
        buf,                    /* pClipName */
        0,                      /* clipIn */
        25*3600,                /* clipOut */ 
        0,                      /* attachBeforeClip */
        omPlrShiftModeAfter,    /* shift */
        &l                      /* *pClipAttachHandle */
    );

    /* 4. setup MIN/MAX positions of timeline */
    OmPlrSetMinPosMin(handle);
    OmPlrSetMaxPosMax(handle);

    /* 5. setup start pos */
    r = OmPlrSetPos(handle, 0);

    /* 6. cue record */
    r = OmPlrCueRecord(handle);
};

void COmnCtl::oper_record()
{
    WaitForSingleObject(lock, INFINITE);
    if(handle && reel)
    {
        if(status_curr.state == omPlrStatePlay)
            oper_record_cue();
        else if(status_curr.state == omPlrStateCueRecord)
            oper_record_record();
    };
    ReleaseMutex(lock);
};

void COmnCtl::oper_fast_ff()
{
    int r;
    double s;
    WaitForSingleObject(lock, INFINITE);
    if(handle)
    {
        if(status_curr.rate > 0)
            s = 1.5 * status_curr.rate;
        else
            s = 1.0;
        r = OmPlrPlay(handle, s);
    };
    ReleaseMutex(lock);
};

void COmnCtl::oper_vary_play(double s)
{
    int r;
    WaitForSingleObject(lock, INFINITE);
    if(handle)
        r = OmPlrPlay(handle, s);
    ReleaseMutex(lock);
};

void COmnCtl::oper_step_ff()
{
    int r;
    WaitForSingleObject(lock, INFINITE);
    if(handle)
        r = OmPlrStep(handle, 1);
    ReleaseMutex(lock);
};

void COmnCtl::oper_play()
{
    int r;
    WaitForSingleObject(lock, INFINITE);
    if(handle)
        r = OmPlrPlay(handle, 1.0);
    ReleaseMutex(lock);
};

void COmnCtl::oper_pause()
{
    int r;
    WaitForSingleObject(lock, INFINITE);
    if(handle)
        r = OmPlrPlay(handle, 0.0);
    ReleaseMutex(lock);
};

void COmnCtl::oper_step_rev()
{
    int r;
    WaitForSingleObject(lock, INFINITE);
    if(handle)
        r = OmPlrStep(handle, -1);
    ReleaseMutex(lock);
};

void COmnCtl::oper_fast_rev()
{
    int r;
    double s;
    WaitForSingleObject(lock, INFINITE);
    if(handle)
    {
        if(status_curr.rate < 0)
            s = 1.5 * status_curr.rate;
        else
            s = -1.0;
        r = OmPlrPlay(handle, s);
    };
    ReleaseMutex(lock);
};

void COmnCtl::oper_mark_out()
{
    WaitForSingleObject(lock, INFINITE);
    if(handle)
        set_mark_out(status_curr.pos);
    ReleaseMutex(lock);
};

void COmnCtl::oper_mark_in()
{
    WaitForSingleObject(lock, INFINITE);
    if(handle)
        set_mark_in(status_curr.pos);
    ReleaseMutex(lock);
};

void COmnCtl::oper_goto_out()
{
    int r;
    WaitForSingleObject(lock, INFINITE);
    if(handle)
    {
        if(mark_out >= 0)
            r = mark_out;
        else
        {
            if(!reel)
                r = 0;
            else
                r = reel->dur();
            if(r)
                r--;
        };
        r = OmPlrSetPos(handle, r);
    };
    ReleaseMutex(lock);
};

void COmnCtl::oper_goto_in()
{
    int r;
    WaitForSingleObject(lock, INFINITE);
    if(handle)
    {
        if(mark_in >= 0)
            r = mark_in;
        else
            r = 0;
        r = OmPlrSetPos(handle, r);
    };
    ReleaseMutex(lock);
};

int COmnCtl::set_mark_in(int m)
{
    mark_in = m;
    if(cb)
        cb->COmnCallbackNotify(COmnCallback::MarkIN, (void*)&mark_in);
    return 0;
};

int COmnCtl::set_mark_out(int m)
{
    mark_out = m;
    if(cb)
        cb->COmnCallbackNotify(COmnCallback::MarkOUT, (void*)&mark_out);
    return 0;
};

void COmnCtl::oper_play_stop()
{
    int r;
    WaitForSingleObject(lock, INFINITE);
    if(handle)
    {
        if(status_curr.state == omPlrStatePlay)
        {
            if(0.0 == status_curr.rate)
                r = OmPlrPlay(handle, 1.0);
            else
                r = OmPlrPlay(handle, 0.0);
        }
        else if
        (
            status_curr.state == omPlrStateRecord
            ||
            status_curr.state == omPlrStateCueRecord
        )
            r = OmPlrStop(handle);
    };
    ReleaseMutex(lock);
};

int COmnCtl::undelete_reel(int id)
{
    if(reel && reel->created_on == id)
        reel->undeleted();
    else
    {
        COmnReel* reel = list_reel(id);
        reel->undeleted();
        delete reel;
    };
    return 0;
};

int COmnCtl::destroy_reel(int id)
{
    WaitForSingleObject(lock, INFINITE);

    if(reel && reel->created_on == id)
        reel->deleted();
    else if(handle)
    {
        int i, r;
        COmnReel* reel = list_reel(id);
        if(reel->deleted_on)
        {
            /* delete ids */
            for(i = 0; i < reel->hist_cnt; i++)
            {
                char b[1024];
                _snprintf(b, sizeof(b), "%d", reel->hist_list[i].id);
                r = OmPlrClipDelete(handle, b);
            };

            /* delete file */
            DeleteFile(reel->filename);
        }
        else
            reel->deleted();

        delete reel;
    };
    ReleaseMutex(lock);
    return 0;
};

int COmnCtl::retitle_reel(int id, char *title)
{
    if(reel && reel->created_on == id)
        reel->new_title(title);
    else
    {
        COmnReel* reel;
        reel = list_reel(id);
        reel->new_title(title);
        delete reel;
    };
    return 0;
};

int COmnCtl::undo()
{
    int r;

    if(!reel)
        return -1;

    if(!handle)
        return -1;

    r = reel->undo();
    if(r < 0)
        return r;

    /* reseek */
    reload_reel(reel->dur());

    return 0;
};
