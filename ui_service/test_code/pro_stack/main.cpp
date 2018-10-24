//
// Created by vans on 17-3-24.
//
#include "../include_common.h"
#include "../sp.h"
#include "../sig_util.h"
#include "../msg_util.h"
#include "../arlog.h"


int main(int argc, char **argv)
{
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    arlog_configure(true,true,"/data/s_log",false);
    while(1)
    {
        msg_util::sleep_ms(3000);
    }
}