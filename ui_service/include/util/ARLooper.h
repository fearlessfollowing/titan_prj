#ifndef _AR_LOOPER_H
#define _AR_LOOPER_H
#include <mutex>
#include <thread>
#include <list>
#include <unordered_map>
#include <chrono>
#include <common/sp.h>
#include <util/ARMessage.h>
#include <util/ev++.h>

class ARHandler;
class ARMessage;

struct MsgInfo
{
    bool is_delay_msg;
    std::chrono::time_point<std::chrono::steady_clock> excute_time;
    sp<ARMessage> msg;
};

struct TimerInfo
{
    sp<ev::timer> timer;
    sp<ARMessage> msg;
};

class ARLooper
{
public:
    friend class ARHandler;
    friend class ARMessage;

    explicit ARLooper();
    ~ARLooper();

    void run();
    void quit();

private:
    std::list<sp<MsgInfo> > mMsgs;
    std::unordered_map<ev::timer*, TimerInfo > mTimers;
    std::thread::id mThreadID;
    std::mutex mMutex;
    ev::dynamic_loop mLoop;
    ev::async mAsync;
    volatile bool mRunning = false;
    volatile bool mQuit = false;

private:
    void sendMessageWithDelayMs(const sp<ARMessage> &msg, int ms);
    void dispatchMessage(const sp<ARMessage> &msg);
    void performAsync(ev::async &watcher, int events);
    void performTimer(ev::timer &watcher, int events);

    ARLooper(const ARLooper &) = delete;
    ARLooper &operator=(ARLooper &) = delete;
};

#endif
