#ifndef _HTT_SERVER_H_
#define _HTT_SERVER_H_

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <vector>

#include "web_http.h"


using ReqHandler = std::function<bool(mg_connection *conn, std::string body)>;

#define		METHOD_GET	(1 << 0)
#define		METHOD_POST	(1 << 1)

#define 	DEFAULT_WEB_PORT	"80"


struct HttpRequest {
	std::string 	mUrl;
	int 			mReqMethod;
	ReqHandler		mHandler;

	HttpRequest(std::string url, int method, ReqHandler handler) {
		mUrl = url;
		mReqMethod = method;
		mHandler = handler;
	}

	HttpRequest(std::string url, int method) {
		mUrl = url;
		mReqMethod = method;
		mHandler = nullptr;
	}
};


class HttpServer {
public:
						HttpServer();
	virtual				~HttpServer() {}

	void				setPort(const std::string dstPort);

	bool 				startHttpServer(); 
	bool 				stopHttpServer(); 

protected:
	virtual void		HandleEvent(mg_connection *connection, http_message *http_req);
	static HttpServer*	mInstance;

private:

	static void			OnHttpEvent(mg_connection *connection, int event_type, void *event_data);

	std::string					mPort;    
	mg_mgr						mMgr;    
};

#endif /* _HTT_SERVER_H_ */