//
// Created by vans on 17-3-24.
//
#include <include_common.h>
#include <hw/fan_control.h>
#include <hw/ins_gpio.h>

using namespace std;
#define TAG "fan_control"
#define NEW_FAN_CONTROL


static const int GPIO_F7 = 152;
static const int GPIO_C16 = 39;
fan_control::fan_control(int on , int off):on_ms(on),off_ms(off)
{
    Log.d(TAG,"on_ms is %d off_ms %d",on_ms,off_ms);
    init();
}

fan_control::~fan_control()
{
    deinit();
}

//void fan_control::set_delay_us(int us)
//{
//    delay_us = 0;
//}

void fan_control::control_thread()
{
    Log.d(TAG," start control_thread ");
#if 1
    //rst io
    if (gpio_is_requested(GPIO_F7) != 1)
    {
//        Log.d(TAG,"create gpio GPIO_F7 %d",GPIO_F7);
        gpio_request(GPIO_F7);
    }
    gpio_direction_output(GPIO_F7, 1);
#ifdef NEW_FAN_CONTROL
    if (gpio_is_requested(GPIO_C16) != 1)
    {
//        Log.d(TAG,"create gpio GPIO_39 %d",GPIO_C16);
        gpio_request(GPIO_C16);
    }
    gpio_direction_output(GPIO_C16, 1);
#else
    Log.d(TAG,"new fan control");
    system("test_gpio 39 1 0");
#endif

//    gpio_direction_output(GPIO_F7, 1);
//    msg_util::sleep_ms(2000);
//    while(!bExit)
//    {
//        gpio_direction_output(GPIO_F7, 0);
//        msg_util::sleep_ms(off_ms);
//        gpio_direction_output(GPIO_F7, 1);
//        msg_util::sleep_ms(on_ms);
//        gpio_direction_output(GPIO_F7, 0);
//        msg_util::sleep_ms(off_ms);
//        gpio_direction_output(GPIO_F7, 1);
//        msg_util::sleep_ms(on_ms);
//    }
#else
    while(!bExit)
    {
        msg_util::sleep_ms(1);
        system("test_gpio 39 1 0");
        msg_util::sleep_ms(1);
        system("test_gpio 39 1 1");
    }
#endif
}

void fan_control::init()
{

    th_control_ = thread([this]
                         {
                             control_thread();
                         });
}

void fan_control::deinit()
{
    Log.d(TAG,"deinit");
    if(!bExit)
    {
        bExit = true;
        if(th_control_.joinable())
        {
            th_control_.join();
        }
    }
    Log.d(TAG,"deinit 2 to keep fan on");
    system("test_gpio 39 1 1");
    //keep 152 down
    gpio_direction_output(GPIO_F7, 0);
    gpio_free(GPIO_F7);
    Log.d(TAG,"deinit 3");
}

