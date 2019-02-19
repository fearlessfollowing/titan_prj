//
// Created by vans on 17-3-24.
//
#include "../include_common.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../sp.h"
#include "../sig_util.h"
#include "SoftapController.h"
#include "TetherController.h"
#include "NatController.h"
#include "BandwidthController.h"
#include "RouteController.h"
#include "ifc.h"
#include "NetworkController.h"
#include "FirewallController.h"
#include "StrictController.h"
#include "IdletimerController.h"

#define WLAN_NAME "wlan0"

#define OEM_IPTABLES_FILTER_OUTPUT "oem_out"
#define OEM_IPTABLES_FILTER_FORWARD "oem_fwd"
#define OEM_IPTABLES_NAT_PREROUTING "oem_nat_pre"

static const char *ip_list[] =
{
        "192.168.42.2",
        "192.168.42.254",
        "192.168.43.2",
        "192.168.43.254",
        "192.168.44.2",
        "192.168.44.254",
        "192.168.45.2",
        "192.168.45.254",
        "192.168.46.2",
        "192.168.46.254",
        "192.168.47.2",
        "192.168.47.254",
        "192.168.48.2",
        "192.168.48.254",
        "192.168.49.2",
        "192.168.49.254",
};

static const char *cfg_on[] =
{
        WLAN_NAME,
        "192.168.43.1",
        "24",
        "up",
};
static const char *cfg_on1[] =
{
        WLAN_NAME,
        "0.0.0.0",
        "0",
        "on",
};

static const char *cfg_off[] =
{
        WLAN_NAME,
        "0.0.0.0",
        "0",
        "down",
};

#define AP_EXTERNAL

static const char *ip_forward = "tethering";

int set_wlan0_cfg(const char *argv[])
{
    int ret = -1;
    int iIndex = 3;
    struct in_addr addr;

    ifc_init();
    printf("set wlan0 %s %s %s %s\n",argv[0],argv[1],argv[2],argv[3]);
    if (!inet_aton(argv[1], &addr)) {
        // Handle flags only case
        printf("convert wlan0 addr error\n");
        goto EXIT;
    } else {
        if (ifc_set_addr(argv[0], addr.s_addr)) {
            goto EXIT;
        }
        printf("addr.s_addr 0x%x\n",addr.s_addr );
        // Set prefix length on a non zero address
        if (addr.s_addr != 0 && ifc_set_prefixLength(argv[0], atoi(argv[2]))) {
            goto EXIT;
        }
    }

    if (!strcmp(argv[iIndex], "up"))
    {
        printf("Trying to bring up %s\n", argv[0]);
        if (ifc_up(argv[0]))
        {
            printf("Error upping interface\n");
            goto EXIT;
        }
        else
        {
            ret = 0;
        }
    }
    else if (!strcmp(argv[iIndex], "down"))
    {
        printf("Trying to bring down %s\n", argv[0]);
        if (ifc_down(argv[0]))
        {
            printf("Error upping interface\n");
            goto EXIT;
        }
        else
        {
            ret = 0;
        }
    }
    else
    {
        printf("set_wlan0_cfg bad argv[%d] %s\n",
               iIndex,argv[iIndex]);
    }
EXIT:
    ifc_close();
    return ret;
}

static const char* FILTER_INPUT[] = {
        // Bandwidth should always be early in input chain, to make sure we
        // correctly count incoming traffic against data plan.
        BandwidthController::LOCAL_INPUT,
        FirewallController::LOCAL_INPUT,
        NULL,
};

static const char* FILTER_FORWARD[] = {
        OEM_IPTABLES_FILTER_FORWARD,
        FirewallController::LOCAL_FORWARD,
        BandwidthController::LOCAL_FORWARD,
        NatController::LOCAL_FORWARD,
        NULL,
};

static const char* FILTER_OUTPUT[] = {
        OEM_IPTABLES_FILTER_OUTPUT,
        FirewallController::LOCAL_OUTPUT,
        StrictController::LOCAL_OUTPUT,
        BandwidthController::LOCAL_OUTPUT,
        NULL,
};

static const char* RAW_PREROUTING[] = {
        BandwidthController::LOCAL_RAW_PREROUTING,
        IdletimerController::LOCAL_RAW_PREROUTING,
        NULL,
};

static const char* MANGLE_POSTROUTING[] = {
        BandwidthController::LOCAL_MANGLE_POSTROUTING,
        IdletimerController::LOCAL_MANGLE_POSTROUTING,
        NULL,
};

static const char* MANGLE_FORWARD[] = {
        NatController::LOCAL_MANGLE_FORWARD,
        NULL,
};

static const char* NAT_PREROUTING[] = {
        OEM_IPTABLES_NAT_PREROUTING,
        NULL,
};

static const char* NAT_POSTROUTING[] = {
        NatController::LOCAL_NAT_POSTROUTING,
        NULL,
};

static void createChildChains(IptablesTarget target, const char* table, const char* parentChain,
                              const char** childChains) {
    const char** childChain = childChains;
    do {
        // Order is important:
        // -D to delete any pre-existing jump rule (removes references
        //    that would prevent -X from working)
        // -F to flush any existing chain
        // -X to delete any existing chain
        // -N to create the chain
        // -A to append the chain to parent

        execIptablesSilently(target, "-t", table, "-D", parentChain, "-j", *childChain, NULL);
        execIptablesSilently(target, "-t", table, "-F", *childChain, NULL);
        execIptablesSilently(target, "-t", table, "-X", *childChain, NULL);
        execIptables(target, "-t", table, "-N", *childChain, NULL);
        execIptables(target, "-t", table, "-A", parentChain, "-j", *childChain, NULL);
    } while (*(++childChain) != NULL);
}

char *dns_array[] = {(char *)"192.168.2.1"};

/****
ndc softap fwreload wlan0 AP
ndc softap set wlan0 AndroidAPNew broadcast 1 wpa2-psk 8888888ap
ndc softap startap
ndc interface setcfg wlan0 192.168.43.1 24 up multicast running broadcast
ndc tether interface add wlan0
ndc network interface add local wlan0
ndc network route add local wlan0 192.168.43.0/24
ndc ipfwd enable tethering
ndc tether start 192.168.43.2 192.168.43.254
ndc tether dns set 100 192.168.2.1
ndc nat enable wlan0 eth0 1 192.168.43.0/24
ndc ipfwd add wlan0 eth0

 ***/
int main(int argc, char **argv)
{
    int ret;

    int cmd;
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    (void)ip_forward;
    (void)ip_list;

    sp<NetworkController> sNetCtrl = sp<NetworkController>(new NetworkController());
    sp<SoftapController> mSoftControl = sp<SoftapController>(new SoftapController());
    sp<TetherController> sTetherCtrl = sp<TetherController>(new TetherController());
#ifdef AP_EXTERNAL
    sp<NatController> mNatCtrl = sp<NatController>(new NatController());


    sp<BandwidthController> sBandwidthCtrl = sp<BandwidthController>(new BandwidthController());

    sp<IdletimerController> sIdletimerCtrl = sp<IdletimerController>(new IdletimerController());
    sp<FirewallController> sFirewallCtrl =  sp<FirewallController>(new FirewallController());
    sp<StrictController> sStrictCtrl = sp<StrictController>(new StrictController());

    // Create chains for children modules
    createChildChains(V4V6, "filter", "INPUT", FILTER_INPUT);
    createChildChains(V4V6, "filter", "FORWARD", FILTER_FORWARD);
    createChildChains(V4V6, "filter", "OUTPUT", FILTER_OUTPUT);
    createChildChains(V4V6, "raw", "PREROUTING", RAW_PREROUTING);
    createChildChains(V4V6, "mangle", "POSTROUTING", MANGLE_POSTROUTING);
    createChildChains(V4, "mangle", "FORWARD", MANGLE_FORWARD);
    createChildChains(V4, "nat", "PREROUTING", NAT_PREROUTING);
    createChildChains(V4, "nat", "POSTROUTING", NAT_POSTROUTING);

    sFirewallCtrl->setupIptablesHooks();

    mNatCtrl->setupIptablesHooks();
    sBandwidthCtrl->setupIptablesHooks();
    sIdletimerCtrl->setupIptablesHooks();
    sBandwidthCtrl->enableBandwidthControl(false);
#endif
    while(1)
    {
        ret = scanf("%d",&cmd);
        printf("scanf (%d %d)\n",ret,cmd);
        switch(cmd)
        {
            case 0:
            {
                mSoftControl->setAPInfo((char *) "8888888c", (char *) "Insta360-Pro-888888");
                mSoftControl->startSoftap();
#ifdef AP_EXTERNAL
#if 0
                ret = RouteController::disableTethering("wlan0","eth0");
                printf("0RouteController ret %d\n",ret);

                ret = sBandwidthCtrl->removeGlobalAlertInForwardChain();
                printf("0sBandwidthCtrl ret %d\n",ret);
                ret |= mNatCtrl->disableNat("wlan0","eth0");
                printf("0sNatCtrl ret %d\n",ret);

                ret= sTetherCtrl->untetherInterface(WLAN_NAME);
                printf("0 untetherInterface ret %d\n",ret);
                ret = sTetherCtrl->stopTethering();
                printf("0 stopTethering ret %d\n",ret);
#endif
#endif
                msg_util::sleep_ms(1000);
                set_wlan0_cfg(cfg_on);
                ret = sTetherCtrl->tetherInterface(WLAN_NAME);
                printf("0 tetherInterface ret %d\n",ret);

#ifdef AP_EXTERNAL
                ret = sNetCtrl->addInterfaceToNetwork(NetworkController::LOCAL_NET_ID, WLAN_NAME);
                printf("0 addInterfaceToNetwork ret %d\n",ret);

                ret = sNetCtrl->addRoute(NetworkController::LOCAL_NET_ID, WLAN_NAME, "192.168.43.0/24", nullptr, false, 0);
                printf("0 untetherInterface ret %d\n",ret);

                sTetherCtrl->enableForwarding(ip_forward);
#endif
                int num_addrs = 16;
                int arg_index = 0;
                int array_index = 0;
                in_addr *addrs = (in_addr *) malloc(sizeof(in_addr) * num_addrs);
                while (array_index < num_addrs)
                {
                    if (!inet_aton(ip_list[arg_index++], &(addrs[array_index++])))
                    {
                        printf("error [%d]\n", arg_index);
                        free(addrs);
                        return 0;
                    }
                }
                ret = sTetherCtrl->startTethering(num_addrs, addrs);
                free(addrs);
                printf("start tether ret %d\n", ret);

#ifdef AP_EXTERNAL
                ret = sTetherCtrl->setDnsForwarders(100, dns_array, 1);
                printf("set dns forward ret %d\n", ret);

                ret = mNatCtrl->enableNat("wlan0", "eth0");
                printf("mNatCtrl ret %d\n", ret);
                if (!ret)
                {
                    /* Ignore ifaces for now. */
                    ret = sBandwidthCtrl->setGlobalAlertInForwardChain();
                    printf("sBandwidthCtrl ret %d\n", ret);
                }
                ret = RouteController::enableTethering("wlan0", "eth0");
                printf("RouteController ret %d\n",ret);
#endif
            }
                break;
            case 1:
                set_wlan0_cfg(cfg_on1);
#ifdef AP_EXTERNAL
                ret = RouteController::disableTethering("wlan0","eth0");
                printf("2RouteController ret %d\n",ret);

                ret = sBandwidthCtrl->removeGlobalAlertInForwardChain();
                printf("2sBandwidthCtrl ret %d\n",ret);
                ret |= mNatCtrl->disableNat("wlan0","eth0");
                printf("2sNatCtrl ret %d\n",ret);
                ret = sTetherCtrl->untetherInterface(WLAN_NAME);
                printf("2 untetherInterface ret %d\n",ret);
                ret = sNetCtrl->removeInterfaceFromNetwork(NetworkController::LOCAL_NET_ID, WLAN_NAME);
                printf("2 removeInterfaceFromNetwork ret %d\n",ret);
#endif
                ret = sTetherCtrl->stopTethering();
                printf("2 stopTethering ret %d\n",ret);
#ifdef AP_EXTERNAL
                ret = sTetherCtrl->disableForwarding(ip_forward);
                printf("2 disableForwarding ret %d\n",ret);
#endif
                mSoftControl->stopSoftap();
                set_wlan0_cfg(cfg_off);
                break;
            case 10:
                ret = 0;
                goto EXIT;
            default:
                break;
        }
    }

EXIT:
    mSoftControl->stopSoftap();
    sTetherCtrl->stopTethering();
    return ret;
}