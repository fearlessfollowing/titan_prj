#include <util/ARLooper.h>
#include <mutex>
#include <thread>
#include <util/ev.h>
#include <util/ev++.h>
#include <util/ARHandler.h>

using namespace std;
using namespace std::chrono;

#define TAG "ARLooper"
#define TRACE 0

ARLooper::ARLooper() : mAsync(mLoop)
{
    mAsync.set<ARLooper, &ARLooper::performAsync>(this);
    mAsync.start();
}

ARLooper::~ARLooper()
{
}

void ARLooper::run()
{
    mThreadID = this_thread::get_id();
    mRunning = true;
    mLoop.run();

    for (auto &timerInfoPair : mTimers)
        timerInfoPair.first->stop();
    
    mAsync.stop();
    mRunning = false;
}

void ARLooper::quit()
{
    mQuit = true;
    mLoop.break_loop(ev::ALL);
}

void ARLooper::sendMessageWithDelayMs(const sp<ARMessage> &msg, int ms)
{
    {
        unique_lock<mutex> lock(mMutex);
        mMsgs.push_back(sp<MsgInfo>(new MsgInfo
                {ms > 0, steady_clock::now() + milliseconds(ms), msg}));
    }
    mAsync.send();
}

void ARLooper::dispatchMessage(const sp<ARMessage> &msg)
{
    wp<ARHandler> handlerWp = msg->getHandler();
    sp<ARHandler> handler = handlerWp.lock();
    if (handler == nullptr) {
        return;
    }

    handler->handleMessage(msg);
}

void ARLooper::performAsync(ev::async &watcher, int events)
{
    sp<MsgInfo> msgInfo;

    while (!mQuit) {
        {
            unique_lock<mutex> lock(mMutex);
            if (mMsgs.empty())
                break;
            msgInfo = mMsgs.front();
            mMsgs.pop_front();
        }

        if (!msgInfo->is_delay_msg) {
            dispatchMessage(msgInfo->msg);
            msgInfo = nullptr;
        } else {
            sp<ev::timer> timerWatcher(new ev::timer(mLoop));
            timerWatcher->set<ARLooper, &ARLooper::performTimer>(this);
            mTimers.insert({timerWatcher.get(), {timerWatcher, msgInfo->msg}});
            nanoseconds duration = msgInfo->excute_time - steady_clock::now();
            double dur = duration.count() / (1000 * 1000 * 1000.0);

            timerWatcher->start(dur);
            msgInfo = nullptr;
        }
    }
}

void ARLooper::performTimer(ev::timer &watcher, int events)
{
    if (mQuit)
        return;

    auto itr = mTimers.find(&watcher);

    dispatchMessage((*itr).second.msg);
    mTimers.erase(itr);
}
