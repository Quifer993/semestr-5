#ifndef LAB31PROXY_CLIENTINTERFACE_H
#define LAB31PROXY_CLIENTINTERFACE_H

#include "../buffer/Buffer.h"
#include <sys/poll.h>
#include <memory>
#include <list>

namespace ProxyServer {
    typedef enum {
        USER,
        HTTP_SERVER
    } TypeClient;

    class Client {
    public:
        virtual ~Client() = default;

        virtual int sendBuf(std::string* buf) = 0;

        virtual int readBuf(std::string *buf) = 0;

        virtual Buffer *getBuffer() = 0;

        virtual void setEvents(int event) = 0;

        virtual void setBuffer(Buffer *buffer) = 0;

        virtual TypeClient getTypeClient() = 0;

        virtual struct pollfd getPollFd() = 0;

        virtual void setPollElement(struct pollfd pollfd) = 0;

        virtual void setReventsZero() = 0;

        virtual Client *getPair() = 0;

        virtual void setPair(Client *pair) = 0;

        virtual std::list<Client*> getListHandlingEvent() = 0;

        virtual void addClientToHandlingEvent(Client* client) = 0;

        virtual void eraseIt(Client* client) = 0;
    };
}
#endif //LAB31PROXY_CLIENTINTERFACE_H
