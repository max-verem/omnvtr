#include "stdafx.h"

#define FPS 25.0

static int is_digit(char c)
{
	return ((c>='0')&&(c<='9'))?1:0;
};

char* tc_frames2txt(int frames, char* buf)
{
    static int bs[] = {24, 60, 60, (long)FPS};
    char *buf_out = buf;
    int i, r;

    /* check for negative */
    if(frames < 0)
    {
#ifdef _DEBUG
        strcpy(buf, "--:--:--:--");
        return buf;
#else
        buf[0] = '-';
        frames = -frames;
        buf++;
#endif
    };

    /* setup datas */
    for(i = 3; i >= 0; i--)
    {
        /* get part */
        r = frames % bs[i]; frames /= bs[i];

        /* store part */
        buf[3*i + 0] = (r/10) | 0x30;
        buf[3*i + 1] = (r%10) | 0x30;
        buf[3*i + 2] = ':';
    };

    /* terminate with zero */
    buf[11] = 0;

    /* return */
    return buf_out;
};

long tc_txt2frames(char* txt)
{
//	char* txt = (char*)tc_txt;

	unsigned char part_found[4] = {0,0,0,0};
	long part_tc[4] = {0,0,0,0};
	long part_no = 0;
	while((*txt)&&(part_no < 4))
	{
		if(is_digit(*txt))
		{
			part_found[part_no] = 1;
			part_tc[part_no] *= 10;
			part_tc[part_no] += (unsigned long)(*txt - '0');
			txt++;
			if(!is_digit(*txt))
				part_no++;
		}
		else
		{
			txt++;
		};
	};

	return (part_tc[0]*3600 + part_tc[1]*60 + part_tc[2])*((long)FPS) + part_tc[3];
};

long tc_txt2frames2(char* src)
{
    int i, j;
    int tcs[4] = {0, 0, 0, 0};
    int digits[12];
    int cnt;

    for(cnt = -1, i = 0; src[i] && cnt < 11; i++)
    {
        if(src[i] < '0' || src[i] > '9')
            continue;

        /* step index */
        if(cnt < 0 || src[i -  1] < '0' || src[i - 1] > '9')
        {
            cnt++;
            digits[cnt] = 0;
        };

        digits[cnt] = digits[cnt] * 10 + (src[i] - '0');

//fprintf(stderr, "i = %d, digits[cnt = %d] = %d\n", i, cnt, digits[cnt]);
    };

    if(cnt < 0)
        return -1;

    for(j = 0, i = cnt; i > -1 && j < 4; i--)
    {
        while(j < 4)
        {
            tcs[j++] = digits[i] % 100;
            digits[i] /= 100;
            if(!digits[i])
                break;
        };
    };

//for(j = 3; j >= 0; j--) printf(" {%d}", tcs[j]); printf("\n");

    return tcs[0] + 25 * (tcs[1] + 60 * (tcs[2] + 60 * tcs[3]));
};
