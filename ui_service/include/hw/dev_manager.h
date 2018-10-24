#ifndef PROJECT_DEV_MANAGER_H
#define PROJECT_DEV_MANAGER_H
#include <thread>
#include <vector>
#include <common/sp.h>

// #include <sys/StorageManager.h>


struct net_link_info;
class ARMessage;

enum {
    SET_STORAGE_SD,
    SET_STORAGE_USB,
    SET_STORAGE_MAX
};

enum
{
    ADD,
    REMOVE,
    CHANGE,
};

class dev_manager
{
public:
    explicit dev_manager(const sp<ARMessage> &notify);
    ~dev_manager();
    bool start_scan(int action = CHANGE);
    int check_block_mount(char *dev);
    void resetDev(int count);
	
private:

    void dev_change(int action,const int MAX_TIMES);
    bool parseAsciiNetlinkMessage(char *buffer,int size);
    void uevent_detect_thread();
    void init();
    void start_detect_thread();
    void deinit();
    void stop_detect_thread();
    void handle_block_event(sp<struct net_link_info> &mLink);

    void send_notify_msg(std::vector<sp<Volume>> &dev_list);
	
    std::thread th_detect_;

    sp<ARMessage> mNotify;

    int mCtrlPipe[2];

    bool bThExit_ = false;
    unsigned int org_dev_count = 0;

};
#endif //PROJECT_DEV_MANAGER_H
