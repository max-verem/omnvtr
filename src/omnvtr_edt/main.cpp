#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <omplrclnt.h>
#include <ommedia.h>

#pragma comment(lib, "omplrlib.lib")
#pragma comment(lib, "ommedia.lib")

#ifdef QQQ

#include "ommedia.h"
#include <stdio.h>
#ifdef WIN32
#include <process.h>
#endif
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

bool pauseAtExit = false;

int pauseExec(int value)
{
    char buf[ 128 ];
    if (pauseAtExit) {
        printf("hit return to exit...");
        fgets(buf, sizeof(buf) - 1, stdin);
    }
    exit(value);
}

static bool errMsg(const char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    return false;
}

static void usage(const char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    printf("Usage: ommcp <options> movie [movie...]\n"
       "where the options are:\n"
       "  -in file      specifies an input; multiple -in args are ok.\n"
       "  -out file     specifies the output.\n"
       "  -flatten      copy all input media to the output\n"
       "  -ref          opposite of -flatten; create a reference movie\n"
       "                without copying media\n"
       "  -ex in:len    specifies an extraction range applied to the\n"
       "                inputs as a whole and expressed in frame numbers;\n"
       "                if 'in' is ommitted, it defaults to 0; if 'len' is\n"
       "                ommited, it defaults to ~0\n"
       "  -trimaudio    this specifies that when creating a new movie file,\n"
       "                to trim audio tracks so that the movie is not longer\n"
       "                that the video track.  You can also do this with -ex\n"
       "                by specifying a length, but this is easier.\n"
       "  -discrete     create separate media files (implies -flatten)\n"
       "  -embedded     opposite of -discrete; embed media files in output\n"
       "                movie (implies -flatten)\n"
       "  -replace      if the specified out file exists, remove it (along\n"
       "                with any referenced media files\n"
       "  -map so=sn    specify a new suffix 'sn' to replace suffix 'so'\n"
       "  -maxage secs  change the max. record age for the source clip\n"
       "                (default is 20 seconds)\n"
       "  -pause        wait for a CR from user before exiting\n"
       "\nThe default options are -flatten -discrete\n");
    pauseExec(1);
}

static char* getArg(int& argc, char**& argv)
{
    if (argc <= 1) {
        printf("missing %s argument", *argv);
        exit(1);
    }
    argc--, argv++;
    return *argv;
}

struct CopyInfo {
    enum { maxInputs = 16 };
    struct Inp {
        char* path;
        uint  srcTrack;
        uint  dstTrack;
        uint  inFrame;
        uint  outFrame;
    } inputs[ maxInputs];
    int nInputs;
    char* output;
    enum { maxMappings = 16 };
    struct Map {
        OmMediaFileType type;
        const char* suffix;
    };
    Map mappings[ maxMappings ];
    int nMappings;
    bool replace;
    bool reference;
    bool discrete;
    bool trimAudio;
    uint inFrame;
    uint length;
    int maxAge;

    CopyInfo()
      : nInputs(0),
        output(0),
        nMappings(0),
        replace(false),
        reference(false),
        discrete(true),
        trimAudio(false),
        inFrame(0),
        length(~0),
        maxAge(-1)
    {}

    void badSuffix(const char* bs);
};

void CopyInfo::badSuffix(const char* bs)
{
    OmMediaCopier omc;
    uint type, sLen = 0;
    for (type = 0; type < nOmMediaFileTypes; type++) {
        const char* suffix =
            omc.getOutputSuffix(OmMediaFileType(type));
        if (suffix != 0 && type != omMediaFileTypeUnknown)
            sLen += strlen(suffix) + 2;
    }

    char* s = new char[ sLen+1 ]; s[0] = '\0';
    for (type = 0; type < nOmMediaFileTypes; type++) {
        const char* suffix =
            omc.getOutputSuffix(OmMediaFileType(type));
        if (suffix == 0 || type == omMediaFileTypeUnknown)
            continue;
        sprintf(s + strlen(s), "%s%s", s[0] != 0 ? ", " : "", suffix);
    }

    errMsg("suffix \"%s\" is not cannonical: use one of\n    %s\n", bs, s);
    pauseExec(1);
}

bool copy(const CopyInfo& cpi)
{
    OmMediaCopier omc;

    if (cpi.reference) {
        if (cpi.discrete)
            omc.setCopyType(omReferenceCopy);
        else
            return errMsg("-ref -flatten is not possible\n");
    } else {
        if (cpi.discrete)
            omc.setCopyType(omFlattenedWithDiscreteMedia);
        else
            omc.setCopyType(omFlattenedWithEmbeddedMedia);
    }

    int i;
    for (i = 0; i < cpi.nInputs; i++) {
        const struct CopyInfo::Inp *inp = &cpi.inputs[i];
        if (inp->srcTrack != (unsigned int)~0) {
            if (inp->dstTrack != (unsigned int)~0) {
                if (! omc.appendTrack(inp->dstTrack, inp->srcTrack,
                    inp->path, inp->inFrame, inp->outFrame)) {
                    return errMsg(
                        "can't append %s (track 0x%x)--is it mpeg long GOP?\n",
                        inp->path, inp->srcTrack);
                }
            } else {
                if (! omc.addSourceTrack(inp->path, inp->srcTrack)) {
                    return errMsg(
                        "can't add %s (track 0x%x)--is it mpeg long GOP?\n",
                        inp->path, inp->srcTrack);
                }
                if (inp->inFrame != 0 || inp->outFrame != (unsigned int)~0) {
                    return errMsg("can't specify in/outframe without dsttrack\n");
                }
            }
        } else if (inp->dstTrack != (unsigned int)~0 || inp->inFrame != 0 ||
                   inp->outFrame != (unsigned int)~0) {
            if (! omc.appendTracks(inp->dstTrack, inp->path,
                inp->inFrame, inp->outFrame)) {
                return errMsg(
                    "can't append %s (all tracks)--is it mpeg long GOP?\n",
                    inp->path);
            }
        } else {
            if (! omc.addSourceTracks(inp->path))
                return errMsg("can't add source track %s\n", inp->path);
        }
    }
    for (i = 0; i < cpi.nMappings; i++) {
        const CopyInfo::Map& map = cpi.mappings[i];
        omc.setOutputSuffix(map.type, map.suffix);
    }
    if ((cpi.inFrame != 0 || cpi.length != (unsigned int)~0)
     && ! omc.setRange(cpi.inFrame, cpi.inFrame+cpi.length)) {
        return errMsg("can't set range to %d:%d\n",
            cpi.inFrame, cpi.inFrame+cpi.length);
    }        

    if (! omc.setDestination(cpi.output, cpi.replace))
        return errMsg("can't set output %s\n", cpi.output);

    if (cpi.trimAudio)
        omc.setTrimmedAudio();

    if (cpi.maxAge > 0)
        omc.setMaxRecordAge(cpi.maxAge);

    if (! omc.copy()) {
        printf("copy failed\n");
        return false;
    }
    return true;
}

int main(int argc, char**argv)
{
    CopyInfo cpi;

    for (argc--, argv++; argc; argc--, argv++) {
        if (strcmp(*argv, "-in") == 0) {
            if (cpi.nInputs >= CopyInfo::maxInputs) {
                usage("too many inputs specified (max %d)\n",
                    CopyInfo::maxInputs);
            }

            char *pSrc = getArg(argc, argv);
            cpi.inputs[ cpi.nInputs ].path = pSrc;

            uint srcTrack = ~0;
            uint dstTrack = ~0;
            uint inFrame  =  0;
            uint outFrame = ~0;

            // Scan ahead for an '=' preceded by a ':'
            char *pCur = pSrc;
            while (*pCur && *pCur != '=') pCur++;
            if (*pCur) {
                while (pCur != pSrc && *(pCur-1) != ':') pCur--;
                if (pCur == pSrc) 
                    usage("-in does not have a colon following the file name");
                *(pCur-1) = '\0';

                while (*pCur) {
                    char *pKeyword = pCur;
                    while (*pCur && *pCur != '=') pCur++;
                    if (*pCur == '=') {
                        *pCur++ = '\0';
                        for (char*pch=pKeyword; *pch; pch++)
                            *pch = tolower(*pch);
                        uint val;
                        sscanf(pCur,"%d",&val);
                        if (!strcmp(pKeyword,"srctrack")) 
                            srcTrack = val;
                        else if (!strcmp(pKeyword,"dsttrack"))
                            dstTrack = val;
                        else if (!strcmp(pKeyword,"inframe"))
                            inFrame = val;
                        else if (!strcmp(pKeyword,"outframe"))
                            outFrame = val;
                        else
                            usage("-in has invalid keyword %s",pKeyword);
                        while (*pCur && *pCur != ':') pCur++;
                        if (*pCur) pCur++;
                    }
                }
            }
            cpi.inputs[ cpi.nInputs ].srcTrack = srcTrack;
            cpi.inputs[ cpi.nInputs ].dstTrack = dstTrack;
            cpi.inputs[ cpi.nInputs ].inFrame  = inFrame;
            cpi.inputs[ cpi.nInputs ].outFrame = outFrame;
            cpi.nInputs++;
        } else if (strcmp(*argv, "-out") == 0) {
            if (cpi.output == 0)
                cpi.output = getArg(argc, argv);
            else
                usage("too many outputs specified\n");
        } else if (strcmp(*argv, "-pause") == 0) {
            pauseAtExit = true;
        } else if (strcmp(*argv, "-ref") == 0) {
            cpi.reference = true;
        } else if (strcmp(*argv, "-flatten") == 0) {
            cpi.reference = false;
        } else if (strcmp(*argv, "-discrete") == 0) {
            cpi.reference = false;
            cpi.discrete = true;
        } else if (strcmp(*argv, "-embedded") == 0) {
            cpi.reference = false;
            cpi.discrete = false;
        } else if (strcmp(*argv, "-replace") == 0) {
            cpi.replace = true;
        } else if (strcmp(*argv, "-trimaudio") == 0) {
            cpi.trimAudio = true;
        } else if (strcmp(*argv, "-ex") == 0) {
            const char* arg = getArg(argc, argv);

            if (sscanf(arg, "%d:%d", &cpi.inFrame, &cpi.length) == 0
             && sscanf(arg, ":%d", &cpi.length) == 0
             && sscanf(arg, "%d:", &cpi.inFrame) == 0) {
                usage("-ex arg should be n:m or :m or n:");
            }
            // if a length wasn't specified, pick a number so that the
            // ***out frame*** will be ~0.
            if (cpi.length == ~0U)
                cpi.length = uint(~0) - cpi.inFrame;
        } else if (strcmp(*argv, "-map") == 0) {
            if (cpi.nMappings >= CopyInfo::maxMappings) {
                usage("too many mappings specified (max %d)\n",
                    CopyInfo::maxMappings);
            }
            const char* p0 = getArg(argc, argv);
            const char* p1 = p0;
            while (*p1 && *p1 != '=')
                p1++;
            if (*p1 == '\0' || *(p1+1) == '\0')
                usage("-map arg should have the form old=new\n");
            int length = p1-p0;
            char* pOld = new char[ length+1 ];
            strncpy(pOld, p0, length);
            pOld[ length ] = '\0';

            cpi.mappings[ cpi.nMappings ].type =
                OmMediaCopier::getOutputType(pOld);
            if (cpi.mappings[ cpi.nMappings ].type == omMediaFileTypeUnknown)
                cpi.badSuffix(pOld);
            if (OmMediaCopier::getOutputType(++p1) == omMediaFileTypeUnknown)
                cpi.badSuffix(p1);
            cpi.mappings[ cpi.nMappings++ ].suffix = p1;
            delete pOld;
        } else if (strcmp(*argv, "-maxage") == 0) {
            cpi.maxAge = atoi(getArg(argc, argv));
        }
    }

    if (cpi.nInputs == 0)
        usage("no inputs specified\n");

    if (cpi.output == 0)
        usage("no output specified\n");

    pauseExec( copy(cpi) ? 0 : 1 );
    return 0;
}

#else

int main(int argc, char** argv)
{
    int r;


    OmMediaCopier omc1, omc2;

    omc1.setDebug("c:\\temp\\omneon\\log");


    r = omc1.appendTracks(0, "c:\\temp\\omneon\\test001-1355156886.mov");
    r = omc1.setDestination("c:\\temp\\omneon\\test001-r1.mov", 1);
    r = omc1.setOutputSuffix(omMediaFileTypeQt, "mov");
    omc1.setCopyType(omReferenceCopy);
    omc1.setRange(0, ~0);
    r = omc1.copy();
    omc1.release();

    omc2.setDebug("c:\\temp\\omneon\\log");

#if 0
    r = omc2.appendTracks(0, "c:\\temp\\omneon\\test001-r1.mov");
#else
//    r = omc2.addSourceTracks("c:\\temp\\omneon\\test001-r1.mov");
#endif
    r = omc2.appendTracks(0, "c:\\temp\\omneon\\test001-1355156886.mov");
    r = omc2.appendTracks(0, "c:\\temp\\omneon\\test001-1355157050.mov");
    r = omc2.setDestination("c:\\temp\\omneon\\test001-r2.mov", 1);
    r = omc2.setOutputSuffix(omMediaFileTypeQt, "mov");
    omc2.setCopyType(omReferenceCopy);
    r = omc2.setRange(0, 15 * 25);
    r = omc2.copy();
    omc2.release();

    return 0;
};

#endif

