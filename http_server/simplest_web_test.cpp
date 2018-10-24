// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved

#include "http_util.h"
#include <log/arlog.h>
#include <sys/types.h>
#include <log/stlog.h>
#include <system_properties.h>

#include <iostream>
#include "http_client.h"

#include <json/value.h>
#include <json/json.h>

#include <prop_cfg.h>

#undef  TAG
#define TAG "HttpServer"

#define LIST_ROOT_PATH  "/mnt"

static const char *s_http_port = "10000";


static void default_signal_handler(int sig)
{
    Log.d(TAG, "sig is %d", sig);
    if (sig != 15) {
        Log.d(TAG, "other handler sig error sig %d", sig);
    }

    exit(0);
}

static void pipe_signal_handler(int sig)
{
    Log.d(TAG, "ignore pipe signal handler");
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


static void printHttpReq(struct http_message* p)
{
    char tmpBuf[1024] = {0};
    char tmpName[512] = {0};
    char tmpVal[512] = {0};

    Log.d(TAG, "============== Request Line ===================");
    
    strncpy(tmpBuf, p->method.p, p->method.len);
    Log.d(TAG, "Request Method: %s", tmpBuf);
    
    memset(tmpBuf, 0, sizeof(tmpBuf));
    strncpy(tmpBuf, p->uri.p, p->uri.len);
    Log.d(TAG, "URI: %s", tmpBuf);
    
    memset(tmpBuf, 0, sizeof(tmpBuf));
    strncpy(tmpBuf, p->proto.p, p->proto.len);
    Log.d(TAG, "Proto Version: %s", tmpBuf);

    Log.d(TAG, "============== Head Line ===================");
    for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
        memset(tmpName, 0, sizeof(tmpName));
        memset(tmpVal, 0, sizeof(tmpVal));

        if (p->header_names[i].p) {
            strncpy(tmpName, p->header_names[i].p, p->header_names[i].len);
            strncpy(tmpVal, p->header_values[i].p, p->header_values[i].len);

            Log.d(TAG, "Name: %s, Value:%s", tmpName, tmpVal);
        } else {
            break;
        }
    }

    Log.d(TAG, "============== Request Body ===================");
    Log.d(TAG, "Body: %s", p->body.p);

}


static void ev_handler(struct mg_connection *nc, int ev, void *p) 
{
    if (ev == MG_EV_HTTP_REQUEST) {
        // mg_serve_http(nc, (struct http_message *) p, s_http_server_opts);
        Log.d(TAG, "[%s: %d] Get Http Request now ...", __FILE__, __LINE__);
        printHttpReq((struct http_message *)p);
    }
}



void handle_func(std::string rsp)
{
	std::cout << "http rsp1: " << rsp << std::endl;
}


int main()
{

    Json::Value root;
    Json::Value param;
    Json::FastWriter writer;
    std::string content;

    param["method"] = "get";
    root["name"] = "camera._getSetCamState";
    root["parameters"] = param;
    content = writer.write(root);

    std::cout << "print message: " << content.c_str() << std::endl;

	std::string url1 = "http://127.0.0.1:20000/ui/commands/execute";
	HttpClient::SendReq(url1, handle_func, "Content-Type: application/json\r\n", content.c_str());
	
    return 0;
}
