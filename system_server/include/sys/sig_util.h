#ifndef _SIG_UTIL_H_
#define _SIG_UTIL_H_
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <log/log_wrapper.h>


#define __UNUSED (__attribute__((unused)))

#ifndef TAG
#define TAG "SigUtil"
#endif


static void pipe_signal_handler(int sig) 
{
    LOGDBG(TAG, "Ignore Pipe Signal....");
}

static void registerSig(__sighandler_t func) 
{
    signal(SIGTERM, func);
    signal(SIGHUP, func);
    signal(SIGUSR1, func);
    signal(SIGQUIT, func);
    signal(SIGINT, func);
    signal(SIGKILL, func);
    signal(SIGPIPE, func);    
}
#endif //_SIG_UTIL_H_
