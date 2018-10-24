#include <util/ARMessage.h>
#include <util/ARHandler.h>
#include <util/ARLooper.h>

void ARMessage::postWithDelayMs(int ms)
{
    sp<ARHandler> handler = mHandler.lock();
    if (handler == nullptr) {
        Log.w(NULL, "ARMessage.post(), handler not exits now");
        return;
    }

    wp<ARLooper> looperWp = handler->getLooperWp();
    sp<ARLooper> looper = looperWp.lock();
    if (looper == nullptr) {
        Log.w(NULL, "ARMessage.post(), looper not exits now");
        return;
    }

    looper->sendMessageWithDelayMs(shared_from_this(), ms);
}