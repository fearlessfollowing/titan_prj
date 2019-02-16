#include "UiListener.h"
#include <log/log_wrapper.h>

#include <util/util.h>
#include <util/SingleInstance.h>

#undef  TAG 
#define TAG "UiListener"


UiListener::UiListener(int socket): SocketListener(socket, true)
{

}


/*
 * 接收数据并转换为一个Json对象构造成AEvent,投递到EventLooper中处理
 */
bool UiListener::onDataAvailable(SocketClient * cli)
{
    std::shared_ptr<AEvent> pEvt = std::make_shared<AEvent>();
    if (pEvt) {
        pEvt->iEventSrcType = AEVENT_SRC_UI;
        pEvt->priv = cli;

        memset(mRecvBuf, 0, sizeof(mRecvBuf));
        int r = TEMP_FAILURE_RETRY(recv(cli->getSocket(), &mRecvBuf, sizeof(mRecvBuf), 0));
        if (r < 0) {
            LOGERR(TAG, "recv data from ui failed, maybe need restart it");
            return false;
        } else if (r == 0) {
            LOGWARN(TAG, "Maybe client closed.");
            return false;
        }

        mRecvBuf[r] = '\0';
        LOGERR(TAG, "onDataAvailable: %s, data len = %d", mRecvBuf, r);

        if (loadJsonFromCString(mRecvBuf, &(pEvt->jInputData))) {
            printJson(pEvt->jInputData);
            auto eventServer = Singleton<EventServer>::getInstance();
            return eventServer->handleUiEvent(pEvt);
        } else {
            LOGERR(TAG, "Parse recv data 2 Json Failed!");
            return false;
        }
    } else {
        return false;
    }
}
