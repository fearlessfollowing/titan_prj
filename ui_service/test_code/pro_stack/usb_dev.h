//
// Created by vans on 30/8/17.
//

#ifndef PROJECT_USB_DEV_H
#define PROJECT_USB_DEV_H
#include "../ins_types.h"

class usb_dev
{
public:
    explicit usb_dev(u32 pid,u32 vid = 0x4255);
    ~usb_dev();

private:
    void init();
    void deinit();
    u32 pid_;
    u32 vid_;
};
#endif //PROJECT_USB_DEV_H
