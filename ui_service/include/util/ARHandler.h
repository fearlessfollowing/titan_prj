#ifndef _AR_HANDLER_H
#define _AR_HANDLER_H
#include <util/ARLooper.h>
#include <util/ARMessage.h>
#include <common/sp.h>

class ARLooper;

class ARHandler : public std::enable_shared_from_this<ARHandler>
{
public:
    explicit ARHandler(const sp<ARLooper> &looper) : 
        mLooper(looper)
    {
    }

    ARHandler() {}

    virtual ~ARHandler() {}

    void registerTo(const sp<ARLooper> &looper)
    {
        mLooper = looper;
    }

    wp<ARLooper> getLooperWp()
    {
        return mLooper;
    }

    sp<ARMessage> obtainMessage(uint32_t what)
    {
        sp<ARMessage> msg(new ARMessage(what));
        msg->setHandler(shared_from_this());
        return msg;
    }

    virtual void handleMessage(const sp<ARMessage> &msg) = 0;


private:
    wp<ARLooper> mLooper; 

private:
    ARHandler(const ARHandler &) = delete;
    ARHandler &operator=(ARHandler &) = delete;
};

#endif
