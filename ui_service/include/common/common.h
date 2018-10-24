
#ifndef _INS_UTIL_UTIL_H_
#define _INS_UTIL_UTIL_H_

#define INS_OK    0  
#define INS_ERR   -1
#define INS_ERR_TIME_OUT -2
#define INS_ERR_OVER   -3

#define INS_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define INS_MIN(a,b) (((a) < (b)) ? (a) : (b))

#define INS_DELETE(p) \
if (p) \
{\
delete p;\
p = nullptr;\
}

#define INS_DELETE_ARRAY(p) \
if (p) \
{\
delete[] p;\
p = nullptr;\
}

#define INS_FREE(p) \
if (p) \
{\
free(p);\
p = nullptr;\
}

#endif