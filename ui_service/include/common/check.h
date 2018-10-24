#ifndef _CHECK_H
#define _CHECK_H
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <common/sp.h>
#include <log/stlog.h>
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
#ifdef ENABLE_ABORT
#define CHECK_EQ(a, b) \
    do { \
    if((a) != (b)) {\
        Log.e(NULL, "CHECK_EQ(%s, %s) failed<%s, %s>(%s:%s:%d)", \
            #a, #b, ToString((a)).c_str(), ToString((b)).c_str(),\
            __FILE__, __FUNCTION__, __LINE__);\
        abort();\
    }\
    } while(0)


#define CHECK_NE(a, b) \
    do { \
    if((a) == (b)) {\
        Log.e(NULL, "CHECK_NE(%s, %s) failed<%s, %s>(%s:%s:%d)", \
            #a, #b, ToString((a)).c_str(), ToString((b)).c_str(),\
            __FILE__, __FUNCTION__, __LINE__);\
        abort();\
    }\
    } while(0)

#define CHECK_OP(a, b, op) \
    do { \
    if(!((a) op (b))) {\
        Log.e(NULL, "CHECK_OP(%s, %s, %s) failed<%s, %s>(%s:%s:%d)", \
            #a, #b, #op, ToString((a)).c_str(), ToString((b)).c_str(),\
            __FILE__, __FUNCTION__, __LINE__);\
        abort();\
    }\
    } while(0)

#define CHECK(a, ...) \
    do { \
    if(!(a)) {\
        Log.e(NULL, "CHECK(%s) failed: %s.(%s:%s:%d)", \
            #a, StringFormat(__VA_ARGS__).c_str(), \
            __FILE__, __FUNCTION__, __LINE__);\
        abort(); \
    }\
    } while(0)

#else
#define CHECK_EQ(a, b) \
    do { \
    if((a) != (b)) {\
        Log.e(NULL, "CHECK_EQ(%s, %s) failed<%s, %s>(%s:%s:%d)", \
            #a, #b, ToString((a)).c_str(), ToString((b)).c_str(),\
            __FILE__, __FUNCTION__, __LINE__);\
    }\
    } while(0)

#define CHECK_NE(a, b) \
    do { \
    if((a) == (b)) {\
        Log.e(NULL, "CHECK_NE(%s, %s) failed<%s, %s>(%s:%s:%d)", \
            #a, #b, ToString((a)).c_str(), ToString((b)).c_str(),\
            __FILE__, __FUNCTION__, __LINE__);\
    }\
    } while(0)

#define CHECK_OP(a, b, op) \
    do { \
    if(!((a) op (b))) {\
        Log.e(NULL, "CHECK_OP(%s, %s, %s) failed<%s, %s>(%s:%s:%d)", \
            #a, #b, #op, ToString((a)).c_str(), ToString((b)).c_str(),\
            __FILE__, __FUNCTION__, __LINE__);\
    }\
    } while(0)

#define CHECK(a, ...) \
    do { \
    if(!(a)) {\
        Log.e(NULL, "CHECK(%s) failed: %s.(%s:%s:%d)", \
            #a, StringFormat(__VA_ARGS__).c_str(), \
            __FILE__, __FUNCTION__, __LINE__);\
    }\
    } while(0)

#endif
#endif
