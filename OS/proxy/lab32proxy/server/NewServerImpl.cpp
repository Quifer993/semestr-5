#include "NewServerImpl.h"
#include <pthread.h>

using namespace ProxyServer;

void NewServerImpl::startServer() {
    std::list<ArgsForThread> listFd;
    while (true) {
        try {
            int sock = _serverSocket->acceptNewClientSock();
            listFd.push_front(ArgsForThread(sock, *_cash));
            pthread_t pthread;
            errno = pthread_create(&pthread, NULL, &NewServerImpl::startingMethodForThread,
                                   (void *) &(*listFd.begin()));
            if (errno != SUCCESS) {
                perror("pthread_create error");
                continue;
                errno = SUCCESS;
            }
//            pthread_join(pthread, NULL);
            errno = pthread_detach(pthread);
            if (errno != SUCCESS) {
                perror("pthread_create error");
                errno = SUCCESS;
                continue;
            }
        } catch (std::exception *exception) {
            std::cerr << exception->what() << std::endl;
            LOG_ERROR("exception in connect");
        }
    }
}

void *NewServerImpl::startingMethodForThread(void *args) {
    ArgsForThread *argsForThread = (ArgsForThread *) args;
    auto pBuffer = std::make_shared<Buffer *>(new BufferImpl(argsForThread->getCash()));
    auto pClient = std::make_shared<Client *>(new ClientImpl(argsForThread->getSock(), TypeClient::USER, *pBuffer));
    (*pClient)->getBuffer()->setIsClientConnect(true);
    HandlerOneClientImpl handlerOneClient = HandlerOneClientImpl(*pClient);
    handlerOneClient.startHandler();
    pthread_exit(NULL);
}

NewServerImpl::NewServerImpl() {
    _serverSocket = std::unique_ptr<ServerSocket>(new ServerSocketImpl());
    _serverSocket->connectSocket();
    _cash = std::make_shared<Cash *>(new CashImpl());
}

NewServerImpl::~NewServerImpl() {
    _serverSocket->closeSocket();
}

