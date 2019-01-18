
#ifndef PROJECT_UTIL_H
#define PROJECT_UTIL_H

#ifdef ENABLE_ABORT
#define SWITCH_DEF_ERROR(item) \
default: \
    LOGERR(TAG,"%s:%s:%d error item %d",__FILE__,__FUNCTION__,__LINE__,item);\
    abort();
#else
#define SWITCH_DEF_ERROR(item) \
default: \
    LOGERR(TAG,"error item %d",item);\
    LOGDBG(TAG,"cancel ab");
#endif


#define ERR_ITEM(item) \
    LOGERR(TAG,"error item %d",item);\
    abort();


bool sh_isbig(void);
int read_line(int fd, void *vptr, int maxlen);
int exec_sh(const char *str);
bool check_path_exist(const char *path);
int move_cmd(const char *src, const char *dest);
int create_dir(const char *path);

bool check_dev_speed_good(const char *path);
int ins_rm_file(const char *name);

int create_socket(const char *name, int type, mode_t perm);

int updateFile(const char* filePath, const char* content, int iSize);

#endif //PROJECT_UTIL_H
