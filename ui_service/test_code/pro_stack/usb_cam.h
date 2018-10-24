//
// Created by vans on 30/8/17.
//

#ifndef PROJECT_USB_CAM_H
#define PROJECT_USB_CAM_H
#include "../ins_types.h"

class usb_cam
{
public:
    explicit usb_cam(u32 iIndex);
    ~usb_cam();

private:
    void init();
    void deinit();

    u32 i_cam_index_;
};
#endif //PROJECT_USB_CAM_H
