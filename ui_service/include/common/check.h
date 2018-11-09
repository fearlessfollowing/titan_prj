#ifndef _CHECK_H
#define _CHECK_H
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <common/sp.h>
#include <util/StringFormat.h>


inline std::string ToString(const std::string &str)
{
    return str;
}

inline std::string ToString(int a)
{
    // return std::to_string(a);
    char buf[128] = {0};
    snprintf(buf, sizeof(buf), "%d", a);
    return buf;
}


#if 0
inline std::string ToString(int64_t a)
{
    // return std::to_string(a);
    char buf[128] = {0};
    snprintf(buf, sizeof(buf), "%" PRId64, a);
    return buf;
}
#endif

inline std::string ToString(bool a)
{
    return a ? "true" : "false";
}

inline std::string ToString(void *a)
{
    char buf[128] = {0};
    snprintf(buf, sizeof(buf), "%p", a);
    return std::string(buf);
}

inline std::string ToString(std::nullptr_t a)
{
    return "null";
}

template<typename T>
inline std::string ToString(T *a)
{
    char buf[128] = {0};
    snprintf(buf, sizeof(buf), "%p", a);
    return std::string(buf);
}

template<typename T>
inline std::string ToString(const sp<T> &a)
{
    char buf[128] = {0};
    snprintf(buf, sizeof(buf), "%p", a.get());
    return std::string(buf);
}

template<typename T>
inline std::string ToString(const T &a)
{
    return "unknown_obj";
}

#define CHECK_EQ(a, b)
#define CHECK_NE(a, b)
#define CHECK_OP(a, b, op) 
#define CHECK(a, ...) 

#endif
