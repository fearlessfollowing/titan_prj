#include <iostream>
#include <memory>
#include <sys/ins_types.h>
#include <util/SingleInstance.h>

#include "log_wrapper.h"

#include "EventServer.h"


int main(int argc, char *argv[]) 
{
    LogWrapper::init("/home/nvidia/insta360/log", "web_log", true);

	printf("----------------- Web Server(V1.0) Start ----------------\n");
    auto eventServer = Singleton<EventServer>::getInstance("10000", "event_server");
	eventServer->startServer();

	printf("----------------- Web Server normal exit ----------------\n");
	return 0;
}