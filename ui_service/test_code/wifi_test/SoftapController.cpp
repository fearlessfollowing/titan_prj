/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License\n");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/wireless.h>

#define LOG_TAG "SoftapController"
#include "ifc.h"
#include "stringprintf.h"
#include "file.h"
//#include "wifi.h"
#include "SoftapController.h"
#define WIFI_ENTROPY_FILE	"/data/misc/wifi/entropy.bin"
#define SHA256_DIGEST_LENGTH 32
using namespace std;
static const char HOSTAPD_CONF_FILE[]    = "/data/misc/wifi/hostapd.conf";
static const char HOSTAPD_BIN_FILE[]    = "/system/bin/hostapd";

#define WIFI_DRIVER_OP_MODE_PARAM	"/sys/module/bcmdhd/parameters/op_mode"
int wifi_change_op_mode(int op_mode)
{
    char mode[4];
    int ret = 0;
    int len;
    int fd;

    fd = TEMP_FAILURE_RETRY(open(WIFI_DRIVER_OP_MODE_PARAM, O_WRONLY));
    if (fd < 0) {
        printf("Failed to open %s error (%s)\n", WIFI_DRIVER_OP_MODE_PARAM,
               strerror(errno));
        return -1;
    }

    sprintf(mode, "%d", op_mode);

    len = strlen(mode) + 1;

    if (TEMP_FAILURE_RETRY(write(fd, mode, len) != len)) {
        printf("Failed to write wlan operation mode param (%s)", strerror(errno));
        ret = -1;
    }
    printf("wifi_change_op_mode (%s %d %d)\n",
           WIFI_DRIVER_OP_MODE_PARAM,
           op_mode,len);
    close(fd);
    return ret;
}

//static char *set_ap_argv[] =
//{
//      "wlan0",
//      "AndroidAPw",
//      "broadcast",
//      "6",
//      "wpa2-psk",
//      "8888888ap",
//};


SoftapController::SoftapController()
    : mPid(0) {}

SoftapController::~SoftapController() {
    printf("force stop softap\n");
    stopSoftap();
}

int SoftapController::startSoftap() {
    pid_t pid = 1;

    if (mPid) {
        printf("SoftAP is already running\n");
        return 0;
    }
    // change wifi mode to 2
    wifi_change_op_mode(2);
//    if (ensure_entropy_file_exists() < 0) {
//        printf("Wi-Fi entropy file was not created\n");
//    }

    if ((pid = fork()) < 0) {
        printf("fork failed (%s)", strerror(errno));
        return -1;
    }

    if (!pid) {
//        ensure_entropy_file_exists();
        if (execl(HOSTAPD_BIN_FILE, HOSTAPD_BIN_FILE,
                  "-e", WIFI_ENTROPY_FILE,
                  HOSTAPD_CONF_FILE, (char *) NULL))
        {
            printf("execl failed (%s)", strerror(errno));
        }
        printf("SoftAP failed to start\n");
        return -1;
    } else {
        mPid = pid;
        printf("SoftAP started successfully mPid %d\n",mPid);
        usleep(AP_BSS_START_DELAY);
    }
    return 0;
}

int SoftapController::stopSoftap() {
    printf("stopSoftap mPid %d\n",mPid);
    if (mPid == 0) {
        printf("SoftAP is not running\n");
        return 0;
    }

    printf("Stopping the SoftAP service...\n");
    kill(mPid, SIGTERM);
    waitpid(mPid, NULL, 0);

    mPid = 0;
    printf("SoftAP stopped successfully\n");
    usleep(AP_BSS_STOP_DELAY);
    return 0;
}

bool SoftapController::isSoftapStarted() {
    return (mPid != 0);
}

int SoftapController::setAPInfo(char *pwd, char *ap_name, int open,int hidden)
{
    int channel = AP_CHANNEL_DEFAULT;

    string wbuf(StringPrintf("interface=%s\n"
                                     "driver=nl80211\n"
                                     "ctrl_interface=/data/misc/wifi/hostapd\n"
                                     "ssid=%s\n"
                                     "channel=%d\n"
                                     "ieee80211n=1\n"
                                     "hw_mode=%c\n"
                                     "ignore_broadcast_ssid=%d\n"
                                     "wowlan_triggers=any\n",
                             "wlan0", ap_name, channel, (channel <= 14) ? 'g' : 'a', hidden));

    string fbuf;
    if (open == 0) {
        char psk_str[2*SHA256_DIGEST_LENGTH+1];
        generatePsk(ap_name, pwd, psk_str);
        fbuf = StringPrintf("%swpa=2\nrsn_pairwise=CCMP\nwpa_psk=%s\n", wbuf.c_str(), psk_str);
    } else {
        fbuf = wbuf;
    }

    printf("set ap fbuf %s\n",fbuf.c_str());
    //force 0 -- AID_ROOT
    if (!WriteStringToFile(fbuf, HOSTAPD_CONF_FILE, 0660, 1000, 1010)) {
        printf("Cannot write to \"%s\": %s", HOSTAPD_CONF_FILE, strerror(errno));
        return -1;
    }
    return 0;
}

/*
 * Arguments:
 *  argv[2] - wlan interface
 *  argv[3] - SSID
 *  argv[4] - Broadcast/Hidden
 *  argv[5] - Channel
 *  argv[6] - Security
 *  argv[7] - Key
 */

int SoftapController::setSoftap(int argc, char *argv[]) {
#if 1
    int hidden = 0;
    int channel = AP_CHANNEL_DEFAULT;

    if (argc < 5) {
        printf("Softap set is missing arguments. Please use:\n");
        printf("softap <wlan iface> <SSID> <hidden/broadcast> <channel> <wpa2?-psk|open> <passphrase>\n");
        return -2;
    }

    printf("setSoftap argc %d ",argc);

    for(int i = 0; i < argc;i++)
    {
        printf("argv[%d] %s",i,argv[i]);
    }

    if (!strcasecmp(argv[4], "hidden"))
        hidden = 1;

    if (argc >= 5) {
        channel = atoi(argv[5]);
        if (channel <= 0)
            channel = AP_CHANNEL_DEFAULT;
    }

    string wbuf(StringPrintf("interface=%s\n"
            "driver=nl80211\n"
            "ctrl_interface=/data/misc/wifi/hostapd\n"
            "ssid=%s\n"
            "channel=%d\n"
            "ieee80211n=1\n"
            "hw_mode=%c\n"
            "ignore_broadcast_ssid=%d\n"
            "wowlan_triggers=any\n",
            argv[2], argv[3], channel, (channel <= 14) ? 'g' : 'a', hidden));

    string fbuf;

    printf("Softap set argc %d\n wbuf %s\n",argc,wbuf.c_str());
    if (argc > 7) {
        char psk_str[2*SHA256_DIGEST_LENGTH+1];
        if (!strcmp(argv[6], "wpa-psk")) {
            generatePsk(argv[3], argv[7], psk_str);
            fbuf = StringPrintf("%swpa=3\nwpa_pairwise=TKIP CCMP\nwpa_psk=%s\n", wbuf.c_str(), psk_str);
        } else if (!strcmp(argv[6], "wpa2-psk")) {
            generatePsk(argv[3], argv[7], psk_str);
            fbuf = StringPrintf("%swpa=2\nrsn_pairwise=CCMP\nwpa_psk=%s\n", wbuf.c_str(), psk_str);
        } else if (!strcmp(argv[6], "open")) {
            fbuf = wbuf;
        }
    } else if (argc > 6) {
        if (!strcmp(argv[6], "open")) {
            fbuf = wbuf;
        }
    } else {
        fbuf = wbuf;
    }
    //force 0 -- AID_ROOT
    if (!WriteStringToFile(fbuf, HOSTAPD_CONF_FILE, 0660, 0, 0)) {
        printf("Cannot write to \"%s\": %s", HOSTAPD_CONF_FILE, strerror(errno));
        return -1;
    }
#endif
    return 0;
}

/*
 * Arguments:
 *	argv[2] - interface name
 *	argv[3] - AP or P2P or STA
 */
int SoftapController::fwReloadSoftap(int argc, char *argv[])
{
#if 0
    char *fwpath = NULL;
    int op_mode = WIFI_DRIVER_STA_MODE;

    if (argc < 4) {
        printf("SoftAP fwreload is missing arguments. Please use: softap <wlan iface> <AP|P2P|STA>\n");
        return -2;
    }

    if (strcmp(argv[3], "AP") == 0) {
        fwpath = (char *)wifi_get_fw_path(WIFI_GET_FW_PATH_AP);
	op_mode = WIFI_DRIVER_AP_MODE;
    } else if (strcmp(argv[3], "P2P") == 0) {
        fwpath = (char *)wifi_get_fw_path(WIFI_GET_FW_PATH_P2P);
	op_mode = WIFI_DRIVER_P2P_MODE;
    } else if (strcmp(argv[3], "STA") == 0) {
        fwpath = (char *)wifi_get_fw_path(WIFI_GET_FW_PATH_STA);
	op_mode = WIFI_DRIVER_STA_MODE;
    }

    printf("SoftAP fwreload arg[3] %s op_mode %d\n",argv[3],op_mode);
    if (wifi_change_op_mode(op_mode)) {
        printf("Softap wifi driver mode set failed\n");
    }
    if (!fwpath)
        return -5;
    if (wifi_change_fw_path((const char *)fwpath)) {
        printf("Softap fwReload failed\n");
        return -1;
    }
    else {
        printf("Softap fwReload - Ok\n");
    }
#endif
    return 0;
}
#ifdef __cplusplus
extern "C"
{
#endif
int PKCS5_PBKDF2_HMAC_SHA1(const char *pass, int passlen,
                           const unsigned char *salt, int saltlen, int iter,
                           int keylen, unsigned char *out);

#ifdef __cplusplus
}
#endif
void SoftapController::generatePsk(char *ssid, char *passphrase, char *psk_str) {
#if 1
    unsigned char psk[SHA256_DIGEST_LENGTH];
    int j;
    // Use the PKCS#5 PBKDF2 with 4096 iterations
    PKCS5_PBKDF2_HMAC_SHA1(passphrase, strlen(passphrase),
            reinterpret_cast<const unsigned char *>(ssid), strlen(ssid),
            4096, SHA256_DIGEST_LENGTH, psk);
    for (j=0; j < SHA256_DIGEST_LENGTH; j++) {
        sprintf(&psk_str[j*2], "%02x", psk[j]);
    }
#endif
}
