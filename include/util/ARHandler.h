/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: ARHandler.h
** 功能描述: 消息处理
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
******************************************************************************************************/
#ifndef _AR_HANDLER_H
#define _AR_HANDLER_H

// #include <util/ARLooper.h>
#include <util/ARMessage.h>
#include <common/sp.h>

class ARLooper;

class ARHandler : public std::enable_shared_from_this<ARHandler>
{
public:
    explicit ARHandler(const sp<ARLooper> &looper): mLooper(looper)
    {
    }

    ARHandler() {}

    virtual ~ARHandler() {}

    /* registerTo - 设置该ARHandler对应的Looper
     * @param
     *  looper - Looper对象
     */
    void registerTo(const sp<ARLooper>& looper) {
        mLooper = looper;
    }

    wp<ARLooper> getLooperWp() {
        return mLooper;
    }

    /* obtainMessage - 用于构造消息
     * @param
     *   what - 消息的类型值
     * 消息最终通过ARHandler传递到其对应的Looper中
     */
    sp<ARMessage> obtainMessage(uint32_t what) {
        sp<ARMessage> msg(new ARMessage(what));
        msg->setHandler(shared_from_this());
        return msg;
    }

    /* Handler提供消息处理：子类中可实现具体的消息处理 */
    virtual void handleMessage(const sp<ARMessage> &msg) = 0;

private:
    wp<ARLooper> mLooper;           /* 指向ARLooper的弱指针 */ 
    ARHandler(const ARHandler &) = delete;
    ARHandler &operator=(ARHandler &) = delete;
};

#endif
