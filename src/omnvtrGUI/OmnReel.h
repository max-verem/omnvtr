#pragma once

#define COmnReel_list_limit 4096

typedef struct COmnReelItem
{
    int id;
    int clip_in, clip_out;
    int mark_in, mark_out;
} COmnReelItem_t;

class COmnReel
{
//    int add(int id, int def_in, int def_out);
    int lookup(int tc, int* poffset);
    int save();
public:
    char filename[1024];
    int hist_cnt;
    COmnReelItem_t hist_list[COmnReel_list_limit];
    int play_dur;
    int play_cnt;
    COmnReelItem_t play_list[COmnReel_list_limit];

    COmnReel(char* filename, int id, char* title);
    COmnReel(char* filename);
    int created_on, modified_on, deleted_on;
    char title[1024];
    int add(int id, int def_in, int def_out, int mark_in, int mark_out, int f_save = 0);
    int dur();
    int deleted();
    int undeleted();
    int new_title(char* title);
};
