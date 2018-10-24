//
// Created by vans on 17-4-22.
//

#ifndef PROJECT_DBG_UTIL_H
#define PROJECT_DBG_UTIL_H

#define UPDATE_BASE_PATH "/sdcard/"

#define DBG_PRINT (printf("%s:%u:\t", __FILE__, __LINE__), printf)
#define DBG_ERR (printf("ERR:%s:%u:\t", __FILE__, __LINE__), printf)

#endif //PROJECT_DBG_UTIL_H
