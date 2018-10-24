//
// Created by vans on 16-12-6.
//

#ifndef PROJECT_TIMER_H
#define PROJECT_TIMER_H
#if 0
#include <thread>
#include <sp.h>

class net_manager;
class ARMessage;
class gyro;
class poll_timer
{
public:
    explicit poll_timer(const sp<ARMessage> &mNotifyMsg);
    ~poll_timer();
    enum
    {
        UPDATE_BATTERY,
        UPDATE_NET,
        READ_KEY,
    };
    void start_timer_thread();
    void stop_timer_thread();
//    void send_read_key();
private:
    void init();
    void deinit();
    void timer_thread();
    void check_net_status();

    bool bExitTimer = false;
    std::thread th_timer_;
    sp<net_manager> mpNetManager;
    sp<gyro> mGyro;
    sp<ARMessage> mNotify;
};
#endif
#endif //PROJECT_TIMER_H
