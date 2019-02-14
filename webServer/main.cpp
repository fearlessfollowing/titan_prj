#include <iostream>
#include <memory>
#include <sys/ins_types.h>
#include "log_wrapper.h"

#include "osc_server.h"


int main(int argc, char *argv[]) 
{
    LogWrapper::init("/home/nvidia/insta360/log", "web_log", true);
    
	printf("----------------- Web Server(V1.0) Start ----------------\n");
	OscServer* oscServer = OscServer::Instance();
	oscServer->startOscServer();

	printf("----------------- Web Server normal exit ----------------\n");
	return 0;
}