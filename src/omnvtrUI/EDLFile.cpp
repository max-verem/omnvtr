#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EDLFile.h"

CEDLFile::CEDLFile(char* host, char* path, char* name)
{
    FILE* f;

    /* init basic vars */
    items_count = 0;
    items_list = (CEDLFileItem*)malloc(0);
    uses_count = 0;
    uses_list = (int*)malloc(0);

    /* compose filename */
    _snprintf(filename, sizeof(filename), "\\\\%s%s/%s.edl",
        host, path, name);

    /* try to load */
    f = fopen(filename, "rt");
    if(f)
    {
        char line[512];

        while(!feof(f))
        {
            line[0] = 0;
            fgets(line, sizeof(line), f);
            if(!line[0])
                continue;

            if(line[0] == '!')
            {
                uses_list = (int*)realloc(uses_list, sizeof(int) * (uses_count + 1));
                uses_list[uses_count] = atol(line + 1);
                uses_count++;
            }
            else if(line[0] == '*')
            {
                int r;

                items_list = (CEDLFileItem*)realloc(items_list, sizeof(CEDLFileItem) * (items_count + 1));
                r = sscanf
                (
                    line + 1,
                    "%d %d %d",
                    &items_list[items_count].id,
                    &items_list[items_count].mark_in,
                    &items_list[items_count].mark_out
                );
                if(3 == r)
                    items_count++;
            }
        };

        fclose(f);
    };
};

int CEDLFile::save()
{
    FILE* f;

    /* try to load */
    f = fopen(filename, "wt");
    if(f)
    {
        int i;

        for(i = 0; i < uses_count; i++)
            fprintf(f, "!%d\n", uses_list[i]);

        for(i = 0; i < items_count; i++)
            fprintf
            (
                f, "*%d %d %d\n",
                items_list[i].id,
                items_list[i].mark_in,
                items_list[i].mark_out
            );

        fclose(f);
    };

    return 0;
};

int CEDLFile::mark_in(int idx)
{
    if(idx < 0 || idx >= items_count)
        return -1;
    return items_list[idx].mark_in;
};

int CEDLFile::mark_out(int idx)
{
    if(idx < 0 || idx >= items_count)
        return -1;
    return items_list[idx].mark_out;
};

int CEDLFile::dur(int idx)
{
    if(idx < 0 || idx >= items_count)
        return -1;
    return items_list[idx].mark_out - items_list[idx].mark_in;
};

int CEDLFile::id(int idx)
{
    if(idx < 0 || idx >= items_count)
        return -1;
    return items_list[idx].id;
};

int CEDLFile::count()
{
    return items_count;
};

int CEDLFile::add(int mark_in, int mark_out, int id, int clip_in, int clip_out)
{
    int idx_in, idx_out, offset_in, offset_out;

    uses_list = (int*)realloc(uses_list, sizeof(int) * (uses_count + 1));
    uses_list[uses_count] = id;
    uses_count++;

    /* find item index where to overlap */
    idx_in = lookup(mark_in, &offset_in);
    if(idx_in < 0)
    {
        /* just append item */
        items_list = (CEDLFileItem*)realloc(items_list, sizeof(CEDLFileItem) * (items_count + 1));
        items_list[items_count].id = id;
        items_list[items_count].mark_in = clip_in;
        items_list[items_count].mark_out = clip_out;
        items_count++;
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
            items_count = idx_in + 2;
            items_list = (CEDLFileItem*)realloc(items_list, sizeof(CEDLFileItem) * items_count);

            /* truncate existing item */
            items_list[idx_in + 0].mark_out =
                items_list[idx_in + 0].mark_in + offset_in;

            /* append new */
            items_list[idx_in + 1].id = id;
            items_list[idx_in + 1].mark_in = clip_in;
            items_list[idx_in + 1].mark_out = clip_out;
        }
        else
        {
            items_list = (CEDLFileItem*)realloc(items_list, sizeof(CEDLFileItem) * (items_count + 2));
            memmove(&items_list[idx_in + 2], &items_list[idx_out], 
                sizeof(CEDLFileItem)* (items_count - idx_out));
            items_count += 2 + idx_in - idx_out;

            /* fix IN first item */
            items_list[idx_in + 0].mark_out =
                items_list[idx_in + 0].mark_in + offset_in;

            /* set new */
            items_list[idx_in + 1].id = id;
            items_list[idx_in + 1].mark_in = clip_in;
            items_list[idx_in + 1].mark_out = clip_out;

            /* fix OUT last item */
            items_list[idx_in + 2].mark_in += offset_out;
        };
    };

    return save();
};

int CEDLFile::lookup(int tc, int* poffset)
{
    int i, t, d;

    for(t = 0, i = 0; i < items_count; i++)
    {
        d = items_list[i].mark_out - items_list[i].mark_in;

        if(t <= tc && tc < (t + d))
        {
            *poffset = tc - t;
            return i;
        };

        t += d;
    };

    return -1;
};
