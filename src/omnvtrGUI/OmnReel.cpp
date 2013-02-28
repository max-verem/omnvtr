#include "stdafx.h"
#include "OmnReel.h"

COmnReel::COmnReel(char* filename, int id, char* title)
{
    hist_cnt = 0;
    play_cnt = 0;
    strncpy(this->filename, filename, sizeof(this->filename));
    strncpy(this->title, title, sizeof(this->title));
    created_on = modified_on = id;
    deleted_on = 0;
    undo_clear();
    save();
};

COmnReel::COmnReel(char* filename)
{
    int l;
    char buf[1024];

    hist_cnt = 0;
    play_cnt = 0;
    title[0] = 0;

    undo_clear();

    /* store filename */
    strncpy(this->filename, filename, sizeof(this->filename));

    /* load datas from file */
    FILE* f;

    f = fopen(filename, "rt");
    if(f)
    {
        /* read title */
        title[0] = 0;
        fgets(title, sizeof(this->title), f);

        /* cleanup newlines from title */
        l = strlen(title);
        if(l && (title[l - 1] == '\n' || title[l - 1] == '\r'))
            title[--l] = 0;
        if(l && (title[l - 1] == '\n' || title[l - 1] == '\r'))
            title[--l] = 0;

        /* read ids */
        buf[0] = 0;
        fgets(buf, sizeof(buf), f);
        sscanf(buf, "%d %d %d", &created_on, &modified_on, &deleted_on);

        /* read records list */
        while(!feof(f))
        {
            int id, def_in, def_out, mark_in, mark_out;

            buf[0] = 0;
            fgets(buf, sizeof(buf), f);
            if(!buf[0])
                continue;

            if(5 == sscanf(buf, "%d %d %d %d %d", &id, &def_in, &def_out, &mark_in, &mark_out))
                add(id, def_in, def_out, mark_in, mark_out);
        };

        fclose(f);
    };
};

int COmnReel::add_int(int id, int clip_in, int clip_out, int mark_in, int mark_out)
{
    int idx_in, idx_out, offset_in, offset_out;

    /* find item index where to overlap */
    if(mark_in < 0 || (idx_in = lookup(mark_in, &offset_in)) < 0)
    {
        /* just append item */
        play_list[play_cnt].id = id;
        play_list[play_cnt].clip_in = clip_in;
        play_list[play_cnt].clip_out = clip_out;
        play_cnt++;
    }
    else
    {
        /* calc new out */
        if(mark_out < 0)
            mark_out = mark_in + (clip_out - clip_in);

        /* find out mark */
        idx_out = lookup(mark_out, &offset_out);

        if(idx_out < 0)
        {
            /* new count */
            play_cnt = idx_in + 2;

            /* truncate existing item */
            play_list[idx_in + 0].clip_out =
                play_list[idx_in + 0].clip_in + offset_in;

            /* append new */
            play_list[idx_in + 1].id = id;
            play_list[idx_in + 1].clip_in = clip_in;
            play_list[idx_in + 1].clip_out = clip_out;
        }
        else
        {
            memmove(&play_list[idx_in + 2], &play_list[idx_out], 
                sizeof(COmnReelItem_t)* (play_cnt - idx_out));
            play_cnt += 2 + idx_in - idx_out;

            /* fix IN first item */
            play_list[idx_in + 0].clip_out =
                play_list[idx_in + 0].clip_in + offset_in;

            /* set new */
            play_list[idx_in + 1].id = id;
            play_list[idx_in + 1].clip_in = clip_in;
            play_list[idx_in + 1].clip_out = clip_out;

            /* fix OUT last item */
            play_list[idx_in + 2].clip_in += offset_out;
        };
    };

    /* recalc mark in/out */
    for(idx_in = 0; idx_in < play_cnt; idx_in++)
    {
        if(!idx_in)
            play_list[idx_in].mark_in = 0;
        else
            play_list[idx_in].mark_in = play_list[idx_in - 1].mark_out;

        play_list[idx_in].mark_out =
              play_list[idx_in].mark_in
            + play_list[idx_in].clip_out
            - play_list[idx_in].clip_in;
    };

    return 0;
};

int COmnReel::add(int id, int clip_in, int clip_out, int mark_in, int mark_out, int f_save)
{
    /* store data in history list */
    hist_list[hist_cnt].id = id;
    hist_list[hist_cnt].clip_in = clip_in;
    hist_list[hist_cnt].clip_out = clip_out;
    hist_list[hist_cnt].mark_in = mark_in;
    hist_list[hist_cnt].mark_out = mark_out;
    hist_cnt++;

    add_int(id, clip_in, clip_out, mark_in, mark_out);

    if(f_save)
    {
        time_t t;
        time(&t);
        modified_on = (int)t;
        save();
    };

    return 0;
};

int COmnReel::lookup(int tc, int* poffset)
{
    int i;

    for(i = 0; i < play_cnt; i++)
        if(tc >= play_list[i].mark_in && tc < play_list[i].mark_out)
        {
            *poffset = tc - play_list[i].mark_in;
            return i;
        };

    return -1;
};

int COmnReel::save()
{
    int l;

    /* load datas from file */
    FILE* f;

    f = fopen(filename, "wt");
    if(f)
    {
        /* write title title */
        fprintf(f, "%s\n", title);

        /* write ids */
        fprintf(f, "%d %d %d\n", created_on, modified_on, deleted_on);

        /* write edl */
        for(l = 0; l < hist_cnt; l++)
            fprintf(f, "%d %d %d %d %d\n",
                hist_list[l].id,
                hist_list[l].clip_in, hist_list[l].clip_out,
                hist_list[l].mark_in, hist_list[l].mark_out);

        fclose(f);
    };

    return 0;
};

int COmnReel::dur()
{
    if(!play_cnt)
        return 0;

    return play_list[play_cnt - 1].mark_out;
};

int COmnReel::deleted()
{
    time_t t;
    time(&t);
    deleted_on = (int)t;
    save();
    return 0;
};

int COmnReel::undeleted()
{
    deleted_on = 0;
    save();
    return 0;
};

int COmnReel::new_title(char* title)
{
    strncpy(this->title, title, sizeof(this->title));
    save();
    return 0;
};

int COmnReel::undo_clear()
{
    int r;

    r = undo_item.id;

    memset(&undo_item, 0, sizeof(undo_item));

    return r;
};

int COmnReel::undo()
{
    int i;

    /* check for undo item */
    if(undo_item.id)
    {
        add_int(undo_item.id, undo_item.clip_in, undo_item.clip_out, undo_item.mark_in, undo_item.mark_out);
        save();
        return 0;
    };

    if(!hist_cnt)
        return -1;

    /* save deleted id and decrement number */
    undo_item = hist_list[--hist_cnt];

    /* save reel */
    save();

    /* rebuild playlist */
    for(i = 0, play_cnt = 0; i < hist_cnt; i++)
        add_int
        (
            hist_list[i].id,
            hist_list[i].clip_in,
            hist_list[i].clip_out,
            hist_list[i].mark_in,
            hist_list[i].mark_out
        );

    return undo_item.id;
};
