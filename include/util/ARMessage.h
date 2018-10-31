#ifndef _AR_MESSAGE_H
#define _AR_MESSAGE_H
#include <unordered_map>
#include <util/Any.h>
#include <common/sp.h>

class ARHandler;
class ARLooper;

class ARMessage : public std::enable_shared_from_this<ARMessage>
{
public:
    friend class ARHandler;

    ~ARMessage() {}

    // FIXME:
    // NOTE: we SHOULD NOT pass raw pointer as data, as we may copy it in dup() ?
    template<typename U>
    void set(const char *key, const typename identity<U>::type &data)
    {
        mFields[key] = Any::from<U>(data);
    }

    template<typename U>
    bool find(const char *key, typename identity<U>::type *data)
    {
        if(mFields.count(key) == 0)
            return false;
        *data = mFields[key].AnyCast<U>();
        return true;
    }

    void post() {
        postWithDelayMs(0);
    }

    wp<ARHandler> getHandler() {
        return mHandler;
    }

    void postWithDelayMs(int ms);

    sp<ARMessage> dup() {
        sp<ARMessage> msg(new ARMessage(mWhat));
        msg->mHandler = mHandler;
        msg->mFields = mFields;
        return msg;
    }

    uint32_t what() {
        return mWhat;
    }

    void setWhat(uint32_t what) {
        mWhat = what;
    }
	
    void setHandler(sp<ARHandler> handler) {
        mHandler = handler;
    }

    ARMessage(uint32_t what) : mWhat(what) {}

    ARMessage(const ARMessage &) = delete;
    ARMessage &operator=(ARMessage &) = delete;

private:
    /* data */
    std::unordered_map<std::string, Any> mFields;
    wp<ARHandler> mHandler;
    uint32_t mWhat;



};

#endif
