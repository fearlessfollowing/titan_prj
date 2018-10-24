#ifndef _MID_PROTO_H
#define _MID_PROTO_H

#include <thread>
#include <vector>
#include <common/sp.h>
#include <hw/battery_interface.h>
#include <sys/net_manager.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <hw/oled_module.h>
#include <sys/VolumeManager.h>

#include <hw/MenuUI.h>

#include <json/value.h>
#include <json/json.h>

class fifo;

enum {
    SEND_PROTO_SRC_UI,
    SEND_PROTO_SRC_VOL,
    SEND_PROTO_SRC_MAX
};

class MidProtoManager {

public:
    virtual ~MidProtoManager();
    
    /*
     * 发送消息 - 通过下层的传输层对象，将数据发送出去
     */
    bool     sendProtoMsg(int iSendSrc, int iEventType, int iAction);  
    
    /*
     * 派发消息 - 根据消息的类型派发给不通的对象
     */
    int     disptachMsg(int iMsgType, Json::Value* pJsonRoot);
    
    void    setNotify(sp<ARMessage> notify);

    void    setRecvUi(sp<MenuUI> pMenuUI);

    static MidProtoManager*     Instance();

    void    start();


private:
    static MidProtoManager*   sInstance;

    MidProtoManager();
    sp<fifo>        mTranFifo;
    sp<MenuUI>      mpUI;
    sp<ARMessage>   mNotify;    
};


#endif  /* _MID_PROTO_H */