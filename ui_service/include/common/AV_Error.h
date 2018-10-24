#ifndef _AV_ERROR_H
#define _AV_ERROR_H

extern "C"
{
#include <libavutil/error.h>
}

thread_local static char _errBuf[128] = { '\0' };

inline const char *AV_Err2Str(int err)
{
    if(av_strerror(err, _errBuf, sizeof(_errBuf)) == 0)
        return _errBuf;
    return "AV_Err2Str: unknown error";
}

#endif
