/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define LOG_TAG "TetherController"

#include "Fwmark.h"
#include "NetdConstants.h"
#include "Permission.h"
#include "TetherController.h"

using namespace std;

static const char BP_TOOLS_MODE[] = "bp-tools";
static const char IPV4_FORWARDING_PROC_FILE[] = "/proc/sys/net/ipv4/ip_forward";
static const char IPV6_FORWARDING_PROC_FILE[] = "/proc/sys/net/ipv6/conf/all/forwarding";

bool writeToFile(const char* filename, const char* value) {
    int fd = open(filename, O_WRONLY);
    if (fd < 0) {
        printf("Failed to open %s: %s\n", filename, strerror(errno));
        return false;
    }

    const ssize_t len = strlen(value);
    if (write(fd, value, len) != len) {
        printf("Failed to write %s to %s: %s\n", value, filename, strerror(errno));
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

//bool inBpToolsMode() {
//    // In BP tools mode, do not disable IP forwarding
//    char bootmode[PROPERTY_VALUE_MAX] = {0};
//    property_get("ro.bootmode", bootmode, "unknown\n");
//    return !strcmp(BP_TOOLS_MODE, bootmode);
//}

TetherController::TetherController() {
    mInterfaces = new InterfaceCollection();
    mDnsNetId = 0;
    mDnsForwarders = new NetAddressCollection();
    mDaemonFd = -1;
    mDaemonPid = 0;
//    if (inBpToolsMode()) {
//        enableForwarding(BP_TOOLS_MODE);
//    } else {
        setIpFwdEnabled();
//    }
}

TetherController::~TetherController() {
    InterfaceCollection::iterator it;
    printf("force stop Tethering\n");
    stopTethering();
    for (it = mInterfaces->begin(); it != mInterfaces->end(); ++it) {
        free(*it);
    }
    mInterfaces->clear();

    mDnsForwarders->clear();
    mForwardingRequests.clear();
}

bool TetherController::setIpFwdEnabled() {
    bool success = true;
    const char* value = mForwardingRequests.empty() ? "0" : "1";
    printf("Setting IP forward enable = %s\n", value);
    success &= writeToFile(IPV4_FORWARDING_PROC_FILE, value);
    success &= writeToFile(IPV6_FORWARDING_PROC_FILE, value);
    return success;
}

bool TetherController::enableForwarding(const char* requester) {
    // Don't return an error if this requester already requested forwarding. Only return errors for
    // things that the caller caller needs to care about, such as "couldn't write to the file to
    // enable forwarding".
    mForwardingRequests.insert(requester);
    return setIpFwdEnabled();
}

bool TetherController::disableForwarding(const char* requester) {
    mForwardingRequests.erase(requester);
    return setIpFwdEnabled();
}

size_t TetherController::forwardingRequestCount() {
    return mForwardingRequests.size();
}

#define TETHER_START_CONST_ARG        8

int TetherController::startTethering(int num_addrs, struct in_addr* addrs) {
    if (mDaemonPid != 0) {
        printf("Tethering already started\n");
        errno = EBUSY;
        return -1;
    }

    printf("Starting tethering services num_addrs %d\n",
           num_addrs);

    pid_t pid;
    int pipefd[2];

    if (pipe(pipefd) < 0) {
        printf("pipe failed (%s)\n", strerror(errno));
        return -1;
    }

    /*
     * TODO: Create a monitoring thread to handle and restart
     * the daemon if it exits prematurely
     */
    if ((pid = fork()) < 0) {
        printf("fork failed (%s)\n", strerror(errno));
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (!pid) {
        close(pipefd[1]);
        if (pipefd[0] != STDIN_FILENO) {
            if (dup2(pipefd[0], STDIN_FILENO) != STDIN_FILENO) {
                printf("dup2 failed (%s)\n", strerror(errno));
                return -1;
            }
            close(pipefd[0]);
        }

        int num_processed_args = TETHER_START_CONST_ARG + (num_addrs/2) + 1;
        char **args = (char **)malloc(sizeof(char *) * num_processed_args);
        args[num_processed_args - 1] = NULL;
        args[0] = (char *)"/system/bin/dnsmasq";
        args[1] = (char *)"--keep-in-foreground";
        args[2] = (char *)"--no-resolv";
        args[3] = (char *)"--no-poll";
        args[4] = (char *)"--dhcp-authoritative";
        // TODO: pipe through metered status from ConnService
        args[5] = (char *)"--dhcp-option-force=43,ANDROID_METERED";
        args[6] = (char *)"--pid-file";
        args[7] = (char *)"";

        int nextArg = TETHER_START_CONST_ARG;
        for (int addrIndex=0; addrIndex < num_addrs;) {
            char *start = strdup(inet_ntoa(addrs[addrIndex++]));
            char *end = strdup(inet_ntoa(addrs[addrIndex++]));
            asprintf(&(args[nextArg++]),"--dhcp-range=%s,%s,1h", start, end);
            free(start);
            free(end);
        }
//
        printf("nextArg %d\n",nextArg);
        for(int i = 0; i < nextArg; i++)
        {
            printf("args[%d] (%s)\n", i,args[i]);
        }
        if (execv(args[0], args)) {
            printf("execl failed (%s)\n", strerror(errno));
        }
        printf("Should never get here!\n");
        _exit(-1);
    } else {
        close(pipefd[0]);
        mDaemonPid = pid;
        mDaemonFd = pipefd[1];
        applyDnsInterfaces();
        printf("Tethering services running\n");
    }

    return 0;
}

int TetherController::stopTethering() {
    printf("Tethering stopTethering mDaemonPid %d\n",mDaemonPid);
    if (mDaemonPid == 0) {
        printf("Tethering already stopped\n");
        return 0;
    }

    printf("Stopping tethering services\n");

    kill(mDaemonPid, SIGTERM);
    waitpid(mDaemonPid, NULL, 0);
    mDaemonPid = 0;
    close(mDaemonFd);
    mDaemonFd = -1;
    printf("Tethering services stopped\n");
    return 0;
}

bool TetherController::isTetheringStarted() {
    return (mDaemonPid == 0 ? false : true);
}

#define MAX_CMD_SIZE 1024

int TetherController::setDnsForwarders(unsigned netId, char **servers, int numServers) {
    int i;
    char daemonCmd[MAX_CMD_SIZE];

    Fwmark fwmark;
    fwmark.netId = netId;
    fwmark.explicitlySelected = true;
    fwmark.protectedFromVpn = true;
    fwmark.permission = PERMISSION_SYSTEM;

    snprintf(daemonCmd, sizeof(daemonCmd), "update_dns:0x%x", fwmark.intValue);
    int cmdLen = strlen(daemonCmd);

    mDnsForwarders->clear();
    printf("setDnsForwarders(  %d %d )\n",
          numServers,netId);
    for (i = 0; i < numServers; i++) {
        printf("setDnsForwarders(0x%x %d = '%s')\n", fwmark.intValue, i, servers[i]);

        struct in_addr a;

        if (!inet_aton(servers[i], &a)) {
            printf("Failed to parse DNS server '%s'\n", servers[i]);
            mDnsForwarders->clear();
            return -1;
        }

        cmdLen += (strlen(servers[i]) + 1);
        if (cmdLen + 1 >= MAX_CMD_SIZE) {
            printf("Too many DNS servers listed\n");
            break;
        }

        strcat(daemonCmd, ":");
        strcat(daemonCmd, servers[i]);
        mDnsForwarders->push_back(a);
    }

    mDnsNetId = netId;
    if (mDaemonFd != -1) {
        printf("dns Sending update msg to dnsmasq [%s]", daemonCmd);
        if (write(mDaemonFd, daemonCmd, strlen(daemonCmd) +1) < 0) {
            printf("Failed to send update command to dnsmasq (%s)", strerror(errno));
            mDnsForwarders->clear();
            return -1;
        }
    }
    return 0;
}

unsigned TetherController::getDnsNetId() {
    return mDnsNetId;
}

NetAddressCollection *TetherController::getDnsForwarders() {
    return mDnsForwarders;
}

int TetherController::applyDnsInterfaces() {
    char daemonCmd[MAX_CMD_SIZE];

    strcpy(daemonCmd, "update_ifaces");
    int cmdLen = strlen(daemonCmd);
    InterfaceCollection::iterator it;
    bool haveInterfaces = false;

    for (it = mInterfaces->begin(); it != mInterfaces->end(); ++it) {
        cmdLen += (strlen(*it) + 1);
        if (cmdLen + 1 >= MAX_CMD_SIZE) {
            printf("Too many DNS ifaces listed\n");
            break;
        }

        strcat(daemonCmd, ":");
        strcat(daemonCmd, *it);
        haveInterfaces = true;
    }

    printf("applyDnsInterfaces mDaemonFd %d haveInterfaces %d\n",
          mDaemonFd,haveInterfaces);
    if ((mDaemonFd != -1) && haveInterfaces) {
        printf("apply dns Sending update msg to dnsmasq [%s]\n", daemonCmd);
        if (write(mDaemonFd, daemonCmd, strlen(daemonCmd) +1) < 0) {
            printf("Failed to send update command to dnsmasq (%s)\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}

int TetherController::tetherInterface(const char *interface) {
    printf("tetherInterface(%s)\n", interface);
    if (!isIfaceName(interface)) {
        errno = ENOENT;
        return -1;
    }
    mInterfaces->push_back(strdup(interface));

    if (applyDnsInterfaces()) {
        InterfaceCollection::iterator it;
        for (it = mInterfaces->begin(); it != mInterfaces->end(); ++it) {
            if (!strcmp(interface, *it)) {
                free(*it);
                mInterfaces->erase(it);
                break;
            }
        }
        return -1;
    } else {
        return 0;
    }
}

int TetherController::untetherInterface(const char *interface) {
    InterfaceCollection::iterator it;

    printf("untetherInterface(%s)\n", interface);

    for (it = mInterfaces->begin(); it != mInterfaces->end(); ++it) {
        if (!strcmp(interface, *it)) {
            free(*it);
            mInterfaces->erase(it);

            return applyDnsInterfaces();
        }
    }
    errno = ENOENT;
    return -1;
}

InterfaceCollection *TetherController::getTetheredInterfaceList() {
    return mInterfaces;
}
