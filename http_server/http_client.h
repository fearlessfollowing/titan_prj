#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include <string>
#include <functional>
#include "http_util.h"

// 此处必须用function类，typedef再后面函数指针赋值无效
using ReqCallback = std::function<void (std::string)>;

class HttpClient
{
public:
	                    HttpClient() {}
	                    ~HttpClient() {}

	static void         SendReq(const std::string &url, ReqCallback req_callback);
	static void         SendReq(const std::string &url, ReqCallback req_callback, const char* extra_headers, const char* post_data);


	static void         OnHttpEvent(mg_connection *connection, int event_type, void *event_data);
	static int          s_exit_flag;
	static ReqCallback  s_req_callback;
};

#endif /* _HTTP_CLIENT_H_ */