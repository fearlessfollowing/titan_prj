#include "UiListener.h"


UiListener::UiListener(int socket):SocketListener(socket, true)
{

}


bool UiListener::onDataAvailable(SocketClient *cli)
{
    return true;
}
