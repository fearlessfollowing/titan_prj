//
// Created by vans on 30/8/17.
//

#ifndef PROJECT_USB_CAM_MANAGER_H
#define PROJECT_USB_CAM_MANAGER_H
#include <vector>

class usb_cam;
class ins_i2c;
class usb_cam_manager
{
public:
    explicit usb_cam_manager();
    ~usb_cam_manager();

private:
    void init();
    void deinit();
    void start_monitor_thread();
    void stop_monitor_thread();
    void set_usb_hub(u32 val);
    void set_usb_power(bool on);

    std::thread th_monitor_;//monitor usb attach/dettach
    bool b_monitor_exit_;
    std::vector<std::shared_ptr<usb_cam>> vec_usb_cams;

    std::shared_ptr mUsbPtrl;
};
#endif //PROJECT_USB_CAM_MANAGER_H
