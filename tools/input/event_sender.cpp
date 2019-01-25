/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2、Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: event_sender.cpp
** 功能描述: 用于模拟硬件按键发送按键事件
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2019年1月25日
** 修改记录:
** V1.0			Skymixos		2019年1月25日		创建文件，添加注释
******************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/input.h> 
#include <errno.h>
#include <unistd.h>
#include <string>
#include <thread>

#include <prop_cfg.h>

#include <system_properties.h>


/*
 * 一次按键的过程: hexdump /dev/input/event4
 * UP: (按下+同步 + 松开 + 同步)
 * 0000000 b6e9 56bc 0000 0000 8652 0007 0000 0000
 * 0000010 0001 0101 0001 0000 b6e9 56bc 0000 0000
 * 0000020 8652 0007 0000 0000 0000 0000 0000 0000
 * 0000030 b6e9 56bc 0000 0000 879a 000a 0000 0000
 * 0000040 0001 0101 0000 0000 b6e9 56bc 0000 0000
 * 0000050 879a 000a 0000 0000 0000 0000 0000 0000
 * DOWN:
 * 0000060 b711 56bc 0000 0000 9956 000e 0000 0000
 * 0000070 0001 0102 0001 0000 b711 56bc 0000 0000
 * 0000080 9956 000e 0000 0000 0000 0000 0000 0000
 * 0000090 b712 56bc 0000 0000 df9f 0001 0000 0000
 * 00000a0 0001 0102 0000 0000 b712 56bc 0000 0000
 * 00000b0 df9f 0001 0000 0000 0000 0000 0000 0000
 * BACK:
 * 00000c0 b734 56bc 0000 0000 d670 000c 0000 0000
 * 00000d0 0001 0104 0001 0000 b734 56bc 0000 0000
 * 00000e0 d670 000c 0000 0000 0000 0000 0000 0000
 * 00000f0 b735 56bc 0000 0000 5385 0000 0000 0000
 * 0000100 0001 0104 0000 0000 b735 56bc 0000 0000
 * 0000110 5385 0000 0000 0000 0000 0000 0000 0000
 * SETTING:
 * 0000120 b750 56bc 0000 0000 164f 0000 0000 0000
 * 0000130 0001 0103 0001 0000 b750 56bc 0000 0000
 * 0000140 164f 0000 0000 0000 0000 0000 0000 0000
 * 0000150 b750 56bc 0000 0000 56be 0002 0000 0000
 * 0000160 0001 0103 0000 0000 b750 56bc 0000 0000
 * 0000170 56be 0002 0000 0000 0000 0000 0000 0000
 * POWER:
 * 0000300 b76f 56bc 0000 0000 225b 0007 0000 0000
 * 0000310 0001 0100 0001 0000 b76f 56bc 0000 0000
 * 0000320 225b 0007 0000 0000 0000 0000 0000 0000
 * 0000330 b76f 56bc 0000 0000 3acb 0009 0000 0000
 * 0000340 0001 0100 0000 0000 b76f 56bc 0000 0000
 * 0000350 3acb 0009 0000 0000 0000 0000 0000 0000
 * 
 */

typedef unsigned short u16;
typedef int s32;


#define ACTION_UP           "up"
#define ACTION_DOWN         "down"
#define ACTION_BACK         "back"
#define ACTION_SETTING      "setting"
#define ACTION_POWRER       "power"
#define ACTION_SHUTDOWN     "shutdown"


#define EVT_TYPE_KEY        0x1
#define EVT_TYPE_SYNC       0x0
#define KEY_CODE_UP         0x101
#define KEY_CODE_DOWN       0x102
#define KEY_CODE_BACK       0x104
#define KEY_CODE_SETTING    0x103
#define KEY_CODE_POWER      0x100

#define _KEY_DOWN            0x1
#define _KEY_UP              0x0

#define DEFAULT_INTERVAL    300

#define EVT_PATH    "/dev/input/event4"


static void print_usage()
{
    fprintf(stdout, "event_sender <\"up\"|\"down\"|\"power\"|\"back\"|\"setting\"|\"shutdown\">\n");
    fprintf(stdout, "event_sender /dev/input/eventX <\"up\"|\"down\"|\"power\"|\"back\"|\"setting\"|\"shutdown\">\n");

    fprintf(stdout, "up   - UP key\n");
    fprintf(stdout, "down - Down key\n");
    fprintf(stdout, "back - Back key\n");
    fprintf(stdout, "setting - Setting key\n");
    fprintf(stdout, "power - Confirm key\n");
    fprintf(stdout, "shutdown - Means Long press Power key\n");
}

static void sleep_ms(uint32_t uiMS)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(uiMS));
}



static void sendKeyEvt(std::string evt_path, std::string evt)
{
    s32 code = KEY_CODE_UP;
    struct input_event event[4];
    const char* path = NULL;
    const char* pInterval = NULL;
    int iSleepInterval = DEFAULT_INTERVAL;
    
    if (evt_path == "none")
        path = EVT_PATH;
    else 
        path = evt_path.c_str();

    pInterval = property_get(PROP_PRESS_INTERVAL);
    if (pInterval) {
        iSleepInterval = atoi(pInterval);
    }

    if (evt == ACTION_UP) code = KEY_CODE_UP;
    else if (evt == ACTION_DOWN) code = KEY_CODE_DOWN;
    else if (evt == ACTION_BACK) code = KEY_CODE_BACK;
    else if (evt == ACTION_SETTING) code = KEY_CODE_SETTING;
    else code = KEY_CODE_POWER;

    event[0].type = EVT_TYPE_KEY;
    event[0].code = code;
    event[0].value = _KEY_DOWN;
    
    event[1].type = EVT_TYPE_SYNC;
    event[1].code = 0;
    event[1].value = 0;

    event[2].type = EVT_TYPE_KEY;
    event[2].code = code;
    event[2].value = _KEY_UP;

    event[3].type = EVT_TYPE_SYNC;
    event[3].code = 0;
    event[3].value = 0;

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "could not open %s, %s\n", path, strerror(errno));
        return;
    }

    write(fd, &event[0], 2 * sizeof(struct input_event));
    sleep_ms(iSleepInterval);
    write(fd, &event[2], 2 * sizeof(struct input_event));
    close(fd);
}


int main(int argc, char* argv[])
{
    bool bSendEvt = false;
    std::string action_str;
    std::string evt_path = "none";

    if (argc != 2 && argc != 3) {
        print_usage();
        return -1;
    }

    if (__system_properties_init()) {
        fprintf(stderr, "event_sender exit: __system_properties_init() failed\n");
        return -1;
    }

    if (argc == 2)
        action_str = argv[1];
    else {
        evt_path = argv[1];
        action_str = argv[2];        
    }

    if (action_str == ACTION_UP 
        || action_str == ACTION_DOWN 
        || action_str == ACTION_BACK
        || action_str == ACTION_SETTING
        || action_str == ACTION_POWRER
        || action_str == ACTION_SHUTDOWN) {
        bSendEvt = true;
    } else {
        fprintf(stderr, "Not Support action[%s]\n", action_str.c_str());
    }

    if (bSendEvt) {
        sendKeyEvt(evt_path, action_str);
    }
    return 0;
}