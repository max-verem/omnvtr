#include <stdio.h>
#include "../mcs3/mcs3.h"

void cb(void* cookie, int button, int value)
{
    if(MCS3_SHUTTLE == button)
        fprintf(stderr, "SHUTTLE: %d\n", value);
    else if(MCS3_JOG == button)
        fprintf(stderr, "JOG: %d\n", value);
    else
    {
        char* btn[] = { "RECORD", "F6", "F4", "W7", "PLAY", "STOP", "FORWARD", "BACKWARD", "BOTTOM", "TOP", "LEFT", "RIGHT", "F3", "F2", "F1", "F5", "W1", "W2", "W3", "W4", "W5", "W6" };
        if(button > 0x15)
            fprintf(stderr, "unrecognized button %.2X\n", button);
        else
            fprintf(stderr, "%s %s\n", btn[button], (value)?"PRESSED":"RELEASE");
    };
};

int main(int argc, char** argv)
{
    int r;
    void* p;

    r = mcs3_open(&p, 8);
    mcs3_callback(p, cb, NULL);

    getc(stdin);

    mcs3_close(p);

    return 0;
};
