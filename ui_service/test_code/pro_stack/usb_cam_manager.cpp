//
// Created by vans on 30/8/17.
//

#include "usb_cam_manager.h"
#include "hotplug.h"
#include "ins_gpio.h"
#include "usb_cam_constant.h"
#include "../ins_i2c.h"

using namespace std;
usb_cam_manager::usb_cam_manager()
{
    init();
}

usb_cam_manager::~usb_cam_manager()
{
    deinit();
}

void usb_cam_manager::init()
{
    set_usb_hub(1);
    mUsbPtrl = sp<ins_i2c>(new ins_i2c(1,0x77));
    mUsbPtrl->i2c_write_byte(0x07,0x00);

    th_monitor_ = thread([this]
            {
                    start_monitor_thread();
            }
    );
}

void usb_cam_manager::deinit()
{
    stop_monitor_thread();
}

void usb_cam_manager::set_usb_power(bool on)
{
    if(on)
    {
        mUsbPtrl->i2c_write_byte(0x03,0xfd);
    }
    else
    {
        mUsbPtrl->i2c_write_byte(0x03,0);
    }
}

void usb_cam_manager::set_usb_hub(u32 val)
{
    for(u32 i = 0; i < sizeof(hub_gpio);i++)
    {
        if (gpio_is_requested(hub_gpio[i]) != 1)
        {
            gpio_request(hub_gpio[i]);
        }
        gpio_direction_output(hub_gpio[i], val);
    }

}

void usb_cam_manager::start_monitor_thread()
{
    b_monitor_exit_ = false;

    while(!b_monitor_exit_)
    {

    }
}

void usb_cam_manager::stop_monitor_thread()
{
    if(!b_monitor_exit_)
    {
        b_monitor_exit_ = true;
        if(th_monitor_.joinable())
        {
            th_monitor_.join();
        }
    }
}
