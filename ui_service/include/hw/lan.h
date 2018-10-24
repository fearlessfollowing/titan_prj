#ifndef PROJECT_LAN_H
#define PROJECT_LAN_H

#include <sys/ins_types.h>

typedef enum _lan_ {
    ENGLISH,
    CHINESE,
    MAX_LAN,
} LAN;

typedef struct _str_info_ {
    const u8 x;
    const u8 y;
    const u8 *dat;
} STR_INFO;

enum {
    STR_INDEX_FAIL,
    STR_INDEX_SHOTTING,
    STR_INDEX_SAVING,
    STR_INDEX_REDAY,
    STR_INDEX_3DOT,
};


static const STR_INFO gstStrInfos[][MAX_LAN] = {
    { {96,16,(const u8 *)"FAIL"},{0,0, nullptr} },

#if 0
    { {96,16,"FAIL"},{} },
    { {96,16,"FAIL"},{} },
    { {96,16,"FAIL"},{} },
    { {96,16,"Origin"},{} },
#endif
};

#endif //PROJECT_LAN_H
