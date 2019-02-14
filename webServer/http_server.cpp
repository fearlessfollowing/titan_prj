#include <utility>
#include <sys/ins_types.h>
#include "http_server.h"

HttpServer* HttpServer::mInstance;

HttpServer::HttpServer()
{
	mPort = DEFAULT_WEB_PORT;
	mInstance = this;
}

void HttpServer::setPort(const std::string dstPort)
{
	mPort = dstPort;
}


bool HttpServer::startHttpServer()
{
	mg_mgr_init(&mMgr, NULL);

	mg_connection *connection = mg_bind(&mMgr, mPort.c_str(), OnHttpEvent);
	if (connection == NULL)
		return false;

	mg_set_protocol_http_websocket(connection);

	printf("starting http server at port: %s\n", mPort.c_str());

	while (true) {
		// printf("------------------> mg_mgr_poll\n");
		mg_mgr_poll(&mMgr, 5); 	// ms
	}

	return true;
}


void HttpServer::OnHttpEvent(mg_connection *connection, int event_type, void *event_data)
{
	http_message *httpReq = (http_message *)event_data;
	
	switch (event_type) {
		case MG_EV_HTTP_REQUEST:
			mInstance->HandleEvent(connection, httpReq);
			break;
	
		case MG_EV_CLOSE: {
			printf("---> close connection socket[%d] now\n", connection->sock);
			break;
		}

		case MG_EV_CONNECT: {
			printf("---> startup connection socket[%d] now\n", connection->sock);			
			break;
		}

        case MG_EV_SEND: {
            // printf("---> send over on socket[%d], send [%d]bytes data\n", connection->sock, *((int*)event_data) );
            break;
        }

		default:
			break;
	}
}


void HttpServer::HandleEvent(mg_connection *connection, http_message *httpReq)
{
	printf("-----------> HttpServer::HandleEvent called...\n");
}


bool HttpServer::stopHttpServer()
{
	mg_mgr_free(&mMgr);
	return true;
}