// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved

#include "mongoose.h"
#include <log/arlog.h>
#include <sys/types.h>
#include <log/stlog.h>
#include <system_properties.h>

#include <prop_cfg.h>

#undef TAG
#define TAG "FileHttp"
#define LIST_ROOT_PATH  "/mnt"



static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

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

static void ev_handler(struct mg_connection *nc, int ev, void *p) 
{
    if (ev == MG_EV_HTTP_REQUEST) {
        mg_serve_http(nc, (struct http_message *) p, s_http_server_opts);
    }
}



int main(void) 
{
    int iRet = -1;
    struct mg_mgr mgr;
    struct mg_connection *nc;
    const char* pListRootPath = NULL;

    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    arlog_configure(true, true, HTTP_APP_LOG_PATH, false);	/* 配置日志 */

    iRet = __system_properties_init();		/* 属性区域初始化 */
    if (iRet) {
        Log.e(TAG, "File Http Web server init properties failed %d", iRet);
        return -1;
    }

	  Log.d(TAG, "Service: update_check starting ^_^ !!");

    mg_mgr_init(&mgr, NULL);

    Log.d(TAG, "Starting web server on port %s MG_ENABLE_HTTP %d MG_ENABLE_TUN %d\n", s_http_port, MG_ENABLE_HTTP, MG_ENABLE_TUN);
    nc = mg_bind(&mgr, s_http_port, ev_handler);
    if (nc == NULL) {
        Log.e(TAG, "Failed to create listener");
        iRet = -1;
        goto EXIT;
    }

    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(nc);

    pListRootPath = property_get(PROP_SYS_FILE_LIST_ROOT);
    if (pListRootPath == NULL) {
        pListRootPath = LIST_ROOT_PATH;
    }

    Log.d(TAG, "[%s:%d] doc root [%s]", __FILE__, __LINE__, pListRootPath);
    s_http_server_opts.document_root = pListRootPath;   // Serve current directory
    s_http_server_opts.enable_directory_listing = "yes";

    while (true) {
        mg_mgr_poll(&mgr, 1000);
    }

EXIT:
    mg_mgr_free(&mgr);
    arlog_close();	
    return 0;
}
