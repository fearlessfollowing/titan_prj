#ifndef _ST_LOG_H
#define _ST_LOG_H

class STLog {
    
public:
    void v(const char *tag, const char *format, ...);
    void d(const char *tag, const char *format, ...);
    void i(const char *tag, const char *format, ...);
    void w(const char *tag, const char *format, ...);
    void e(const char *tag, const char *format, ...);
};

extern STLog Log;

#define STLOG_DEFAULT_TAG "Insta360"

#endif  /* _ST_LOG_H */
