#pragma once

#include <omplrclnt.h>
#include "OmnReel.h"

class COmnCallback
{
public:
    static const int Status = 0;
    static const int State = 1;
    static const int Pos = 2;
    static const int MarkIN = 3;
    static const int MarkOUT = 4;
    static const int Rate = 5;
    static const int Reel = 6;
    static const int Rem = 7;
    static const int ReelsUpdated = 8;
    virtual void COmnCallbackNotify(int id, void* data) = 0;
};

class COmnCtl
{
    COmnCallback* cb;
    char host[1024], player[1024], dir[1024];
    OmPlrHandle handle;
    HANDLE lock;
    OmPlrStatus status_curr, status_prev;
    int f_first, record_id, mark_in, mark_out, mark_curr;
public:
    COmnReel *reel;
    COmnCtl(char* host, char* player, char* dir);
    ~COmnCtl();
    int connect();
    int disconnect();
    int status();
    int get_mark_in() { return mark_in; };
    int get_mark_out() { return mark_out; };
    int get_pos();
    double get_rate();
    int get_state();
    int get_rem();
    char* get_reel_title();
    int reload_reel(int offset);
    int load_reel(int id);
    int unload_reel();
    int new_reel(char *title);
    COmnReel** list_reels();
    COmnReel* list_reel(int id);
    void oper_record();
    void oper_fast_ff();
    void oper_step_ff();
    void oper_play();
    void oper_pause();
    void oper_step_rev();
    void oper_fast_rev();
    void oper_mark_out();
    void oper_mark_in();
    void oper_goto_out();
    void oper_goto_in();
    void oper_record_cue();
    void oper_play_stop();
    void oper_play_record_stop();
    void oper_vary_play(double s);
    void set_cb(COmnCallback* c) { cb = c; };
    int set_mark_in(int m);
    int set_mark_out(int m);
    int destroy_reel(int id);
    int undelete_reel(int id);
    int retitle_reel(int id, char *title);
    int undo();
};