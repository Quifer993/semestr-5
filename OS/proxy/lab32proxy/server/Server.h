#ifndef LAB31PROXY_SERVER_H
#define LAB31PROXY_SERVER_H

namespace ProxyServer {
    class Server {
    public:
        virtual void startServer() = 0;

        virtual ~Server() = default;
//    protected:
//        virtual ~server() = 0;
    };
}

#endif //LAB31PROXY_SERVER_H
