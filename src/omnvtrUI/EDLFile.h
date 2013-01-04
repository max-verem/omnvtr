#pragma once

typedef struct CEDLFileItem_desc
{
    int id;
    int mark_in;
    int mark_out;
} CEDLFileItem;

class CEDLFile
{
    CEDLFileItem* items_list;
    int items_count;
    int* uses_list;
    int uses_count;
    char filename[1024];
    int save();
    int lookup(int tc, int* poffset);
public:
    CEDLFile(char* host, char* path, char* name);
    int add(int mark_in, int mark_out, int id, int clip_in, int clip_out);
    int mark_in(int idx);
    int mark_out(int idx);
    int dur(int idx);
    int id(int idx);
    int count();
};
