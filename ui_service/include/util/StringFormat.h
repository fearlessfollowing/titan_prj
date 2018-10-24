#ifndef _STRING_FORMAT_H
#define _STRING_FORMAT_H

#include <string.h>
#include <stdio.h>
#include <string>

inline std::string StringFormat(const char *format, ...)
{
    std::string str;
    char *cstr = nullptr;
    va_list vaList;
    va_start(vaList, format);
    vasprintf(&cstr, format, vaList);
    str = cstr;
    free(cstr);
    va_end(vaList);
    return str;
}

#endif
