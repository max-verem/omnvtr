#include "timecode.h"

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
        buf[0] = '-';
        frames = -frames;
        buf++;
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

