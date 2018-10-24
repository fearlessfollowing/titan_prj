//
// Created by vans on 16-12-2.
//

#ifndef INC_360PRO_SERVICE_SIG_UTIL_H
#define INC_360PRO_SERVICE_SIG_UTIL_H
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static void default_signal_handler(int sig)
{
    //rec 15 when pkill
    if(sig != 15)
    {
        printf("other handler sig error sig %d\n",sig);
    }
    exit(0);
}

static void pipe_signal_handler(int sig)
{
    printf("ignore pipe signal handler \n");
}

static void registerSig(__sighandler_t func)
{
    signal(SIGTERM, func);
    signal(SIGHUP, func);
    signal(SIGUSR1, func);
    signal(SIGQUIT, func);
    signal(SIGINT, func);
    signal(SIGKILL, func);
}
#endif //INC_360PRO_SERVICE_SIG_UTIL_H
