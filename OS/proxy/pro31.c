#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/poll.h>

#define SERVER_PORT 9605

#define INFINITY -1

#define ERROR_MESSAGE -5
#define CONTINUE_SOCKET_INPUT -4
#define END_SOCKET_INPUT -3
#define END_FILE -2
#define EMPTY -1
#define WORKING 0
#define DOWNLOADING 1
#define DOWNLOADED 2
#define CLEAN 3

#define MAX_CLIENTS_AMOUNT 10
#define DEBUG 0
#define ADDRESS_BUF_SIZE 1024
#define CACHE_SIZE 1000
#define BUFFER_SIZE 1024


#define PORT_LEN 5
#define NON_BLOCKING 1

typedef struct cache {
    int page_size;
    char* title;
    char* page;
} cache_t;

typedef struct url {
    char* method;
    char* host;
    char* path;
    int port;
} url_t;

int connected = 1;


int socketConnect(struct pollfd* fds0, struct pollfd* fds, char* host, in_port_t port, struct sockaddr_in** clientSockaddr) {
    if (DEBUG) printf("[DEBUG]: getting host...\n");

    printf("host in function = %s\n", host);

    struct hostent* hp = gethostbyname(host);
    if (hp == NULL) {
        perror("gethostbyname");
        return 0;
    }

    if (DEBUG) printf("[DEBUG]: host got!\n");
    struct sockaddr_in addr;
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    char addressStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), addressStr, INET_ADDRSTRLEN);
    printf("Host address is %s\n", addressStr);

    if (DEBUG) printf("opening host...\n");

    int sock = socket(PF_INET, SOCK_STREAM /*| SOCK_NONBLOCK*/, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket");
        return 0;
    }

    fds[0].fd = sock;
    fds[0].events = POLLIN;
    int s = poll(fds, 1, -1/*�������*/);
    printf("fds[0].revents - %i, POLLHUB - %i\n", fds[0].revents, POLLHUP);
    //while (!(fds[0].revents & POLLIN));
    if (s > 0) {
        if (DEBUG)printf("[DEBUG]: Can read from socket\n");
        fds[0].revents = 0;
    }
    else {
        fds[0].revents = 0;
        fds[0].fd = 0;
        return 0;
    }

    int opt = fcntl(sock, F_GETFL, 0);
    /*if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
        printf("failed to set non-blocking socket\n");
        return EMPTY;
    }*/
    int error = connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr));
    if (error < -1 /*&& error != EINPROGRESS*/) {
        perror("connect");
        return 0;
    }
    if (DEBUG) printf("Connected!\n");
    if (fcntl(sock, F_SETFL, opt | O_NONBLOCK) == -1) {
        printf("failed to set non-blocking socket\n");
        return 0;
    }
    connected++;
    if (DEBUG) printf("host openned!\n");

    //*clientSockaddr = &addr;

    return sock;
}

void addr_init(struct sockaddr_in* addr, int port) {
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);
}

int findFreeClientIndex(const int* clients) {
    int freeClientIndex = 0;
    while (freeClientIndex < MAX_CLIENTS_AMOUNT && clients[freeClientIndex] >= 0) {
        freeClientIndex++;
    }
    return freeClientIndex;
}

//int acceptNewClient(int listenfd, int* clients) {
//    int freeClientIndex = findFreeClientIndex(clients);
//    clients[freeClientIndex] = accept(listenfd, (struct sockaddr*)NULL, NULL);
//    if (DEBUG) printf("%i - fileDescrNum\n", clients[freeClientIndex]);
//    return clients[freeClientIndex];
//}
//htons -- ����������� ������� ������� ������������ ������ �������������� ��������� ������ hostshort � ������� ������� ������������ ������.
//fd array  poll write

int checkEmptyFd(struct pollfd* fds, int start) {
    int emptyFd = EMPTY;
    for (int i = start + 1; i <= start + MAX_CLIENTS_AMOUNT; i++) {
        if (fds[i].fd == 0) {
            emptyFd = i;
            break;
        }
    }

    return emptyFd;
}

void tryAcceptNewClient(struct pollfd* fds) {
    int indexEmptyFd = checkEmptyFd(fds, 0);
    int inputSock = 0;
    if (indexEmptyFd != EMPTY) {
            if (DEBUG)printf("[DEBUG]: Can read from listen\n");
            fds[indexEmptyFd].fd = accept(fds[0].fd, (struct sockaddr*)NULL, NULL);
            fds[indexEmptyFd].events = POLLIN /*| POLLOUT*/;
            if (DEBUG) printf("%i - fileDescrNum\n", fds[indexEmptyFd].fd);
            connected++;
//            usleep(300000);
            //inputSock = acceptNewClient(listenfd, clients);
            //*alreadyConnectedClientsNumber++;

    }
}

void handleClientDisconnecting(
    int* connectedClientsNumber,
    struct pollfd* fds,
    int* clientsLengthMes,
    int* clientsHttpSocketsEnds,
    int* cacheToClient,
    int* sentBytes,
    int clientIndex,
    struct sockaddr_in** clientHostent) {
    printf("client %d disconnecting...\n", clientIndex);
    close(fds[clientIndex].fd);
    fds[clientIndex].fd = 0;
    fds[clientIndex].events = 0;
    fds[clientIndex].revents = 0;
    fds[clientIndex + MAX_CLIENTS_AMOUNT].fd = 0;
    fds[clientIndex + MAX_CLIENTS_AMOUNT].events = 0;
    fds[clientIndex + MAX_CLIENTS_AMOUNT].revents = 0;
    //printf("3fds[0].fd= %i\n", fds[0].fd);
    //usleep(1000000);
    clientsLengthMes[clientIndex] = EMPTY;
    cacheToClient[clientIndex] = EMPTY;
    clientsHttpSocketsEnds[clientIndex] = EMPTY;
    //clientHostent[clientIndex] = NULL;
    sentBytes[clientIndex] = 0;
    connectedClientsNumber--;
    connected--;
    //printf("2fds[0].fd= %i\n", fds[0].fd);
    //usleep(1000000);
}

void freeURL(url_t* pUrl) {
    if (pUrl != NULL) {
        if (pUrl->method != NULL) {
            free(pUrl->method);
        }
        if (pUrl->path != NULL) {
            free(pUrl->path);
        }
        if (pUrl->host != NULL) {
            free(pUrl->host);
        }

        free(pUrl);
    }
}

url_t* parseURL(char* urlBuffer, int* clientsLengthMes) {
    url_t* url = (url_t*)malloc(sizeof(url_t));
    url->method = NULL;
    url->path = NULL;
    url->host = NULL;
    url->port = 80;
    int urlBufferSize = strlen(urlBuffer);
    int startHostIndex = 0;
    int lengthHost = 0;

    int i;
    for (i = 0; urlBuffer[i] != ' ' && i < urlBufferSize; i++) {}
    url->method = (char*)malloc(sizeof(char) * (i + 1));
    strncpy(url->method, urlBuffer, i);
    url->method[i] = '\0';
    if (urlBuffer[i + 1] == 'h' &&
        urlBuffer[i + 2] == 't'&&
        urlBuffer[i + 3] == 't'&&
        urlBuffer[i + 4] == 'p'&&
        urlBuffer[i + 5] == 's') {
        free(url);
        return NULL;
    }
    if (DEBUG) printf("[DEBUG]: method Type = %s\n", url->method);

    if (urlBufferSize > 5) {
        for (int strIndex = 4; strIndex < urlBufferSize; strIndex++) {
            if (urlBuffer[strIndex] == ':' &&
                urlBuffer[strIndex - 1] == 't' &&
                urlBuffer[strIndex - 2] == 's' &&
                urlBuffer[strIndex - 3] == 'o' &&
                urlBuffer[strIndex - 4] == 'H') {

                strIndex += 2;
                startHostIndex = strIndex;
                int portCurrent = 0;
                bool isPort = false;
                while (urlBuffer[strIndex] != '\n' && urlBuffer[strIndex] != '\r' && strIndex < urlBufferSize && urlBuffer[strIndex] != ':') {
                    strIndex++;
                }
                    
                if (urlBuffer[strIndex] == ':') {
                    isPort = true;
                }
                lengthHost = strIndex - startHostIndex;
                strIndex++;
                while (urlBuffer[strIndex] >= '0' && urlBuffer[strIndex] <= '9' && strIndex < urlBufferSize) {
                    portCurrent = portCurrent * 10 + urlBuffer[strIndex] - '0';
                    strIndex++;
                }
                if (isPort) {
                    url->port = portCurrent;
                }
            }
        }
    }

    if (urlBufferSize > 9) {
        for (int strIndex = 5; strIndex < urlBufferSize; strIndex++) {
            if (urlBuffer[strIndex] == ':' &&
                urlBuffer[strIndex - 1] == 'e' &&
                urlBuffer[strIndex - 2] == 'g' &&
                urlBuffer[strIndex - 3] == 'n' &&
                urlBuffer[strIndex - 4] == 'a' &&
                urlBuffer[strIndex - 5] == 'R') {
                *clientsLengthMes = 0;
                strIndex += 2;
                bool isLength = false;
                while (strIndex < urlBufferSize && urlBuffer[strIndex] != '-') {
                    *clientsLengthMes = *clientsLengthMes * 10 + urlBuffer[strIndex] - '0';
                    strIndex++;
                }
            }
        }
        if (*clientsLengthMes == 0) {
            *clientsLengthMes = EMPTY;
        }
    }

    if (urlBufferSize > 9) {
        for (int strIndex = 5; strIndex < urlBufferSize; strIndex++) {
            if (urlBuffer[strIndex - 0] == '/' &&
                urlBuffer[strIndex - 1] == 'P' &&
                urlBuffer[strIndex - 2] == 'T' &&
                urlBuffer[strIndex - 3] == 'T' &&
                urlBuffer[strIndex - 4] == 'H') {
                urlBuffer[strIndex + 1] = '1';
                urlBuffer[strIndex + 2] = '.';
                urlBuffer[strIndex + 3] = '0';
            }
        }
    }

    for (int strIndex = 0; strIndex < urlBufferSize; strIndex++) {
        if (urlBuffer[strIndex] == '/') {
            if (strIndex + 1 == urlBufferSize) {
                break;
            }
            strIndex += 2;
            for (strIndex; urlBuffer[strIndex] != '/' && strIndex < urlBufferSize; strIndex++) {}

            url->host = (char*)malloc(sizeof(char) * (lengthHost + 2));
            strncpy(url->host, &(urlBuffer[startHostIndex]), lengthHost);
            url->host[lengthHost] = '.';
            url->host[lengthHost + 1] = '\0';
            printf("host = %s\n", url->host);
            //Range: bytes=4721-
            url->path = (char*)malloc(sizeof(char) * (urlBufferSize - strIndex + 2));
            strncpy(url->path, &(urlBuffer[strIndex + 1]), urlBufferSize - strIndex);
            url->path[urlBufferSize - strIndex + 1] = '\0';
//            printf("path = %s", url->path);

            break;
        }
    }
    return url;
}


void freeCache(cache_t* cache) {
    if (cache->page != NULL) { free(cache->page); }
    cache->page = NULL;
    if (cache->title != NULL) { free(cache->title); }
    cache->title = NULL;
    cache->page_size = 0;
}


int readToCache(
    struct pollfd* fds,
    int* cacheProperty,
    int* clientsLengthMes,
    cache_t* cache,
    int currentCacheIndex,
    int clientIndex,
    struct sockaddr_in* clientSockaddr) {

    int offset = cache[currentCacheIndex].page_size;
    int read_bytes = 0;

    int len = sizeof(clientSockaddr);
        if (fds[clientIndex + MAX_CLIENTS_AMOUNT].revents & POLLHUP) {
            cacheProperty[currentCacheIndex] = DOWNLOADED;
            printf("pollhub------------<<<<<<<<<<<\n");
            return END_SOCKET_INPUT;
        }
        //if (fds[clientIndex + MAX_CLIENTS_AMOUNT].revents & POLLIN)
            //fds[clientIndex + MAX_CLIENTS_AMOUNT].revents = 0;
        //read_bytes = recvfrom(fds[clientIndex + MAX_CLIENTS_AMOUNT].fd, &((cache[currentCacheIndex].page)[offset]), BUFFER_SIZE, 0,
        //    (struct sockaddr_in*)clientSockaddr,
        //    (socklen_t*)&len);
        read_bytes = read(fds[clientIndex + MAX_CLIENTS_AMOUNT].fd, &((cache[currentCacheIndex].page)[offset]), BUFFER_SIZE);

        if (read_bytes == -1) {
            //if (DEBUG)printf("[DEBUG]: read bytes error!! Now we check answer\n");
            if (errno != EAGAIN) {
                if (DEBUG)printf("[DEBUG]: not eagain -<< read_bytes = %i, clientsHttpSockets[clientIndex] = %i\n", read_bytes, fds[clientIndex + MAX_CLIENTS_AMOUNT].fd);
//                return ERROR_MESSAGE;
//                return CONTINUE_SOCKET_INPUT;
            }
            else {
                if (DEBUG)printf("[DEBUG]: eagain -<< clientsHttpSockets[clientIndex] = %i\n", fds[clientIndex + MAX_CLIENTS_AMOUNT].fd);
                errno = 0;
                //return END_SOCKET_INPUT;
                //break;
            }
            return ERROR_MESSAGE;

        }
        else if (read_bytes == 0) {
            printf("tut\n");
            cacheProperty[currentCacheIndex] = DOWNLOADED;
            //cacheProperty[currentCacheIndex] = DOWNLOADED;

            return END_FILE;
            //return END_SOCKET_INPUT;
        }
        else if (read_bytes > 0) {
            fds[clientIndex].events =POLLIN | POLLOUT;
//            printf("%s", &((cache[currentCacheIndex].page)[offset]));
            //if (!(cache[currentCacheIndex].page[9] == '2' &&
            //    cache[currentCacheIndex].page[10] == '0' &&
            //    cache[currentCacheIndex].page[11] == '0')){
            //    return ERROR_MESSAGE;
            //}
            //usleep(100000);
            offset += read_bytes;
            cache[currentCacheIndex].page_size += read_bytes;
            if (DEBUG)printf("[DEBUG]: read_bytes = %d. offset = %d\n", read_bytes, offset);

            char* resultAlloc = (char*)realloc(cache[currentCacheIndex].page, cache[currentCacheIndex].page_size + BUFFER_SIZE + 1);
            if(resultAlloc == NULL){
                return ERROR_MESSAGE;
            }
            cache[currentCacheIndex].page = resultAlloc;
            if (offset > 6 && DEBUG)printf("[DEBUG]: bytes ->>>> %i, %i, %i, %i, %i, %i\n", /*(int)((cache[currentCacheSize].page)[offset - 0]) ,*/
                (int)((cache[currentCacheIndex].page)[offset - 1])
                , (int)((cache[currentCacheIndex].page)[offset - 2])
                , (int)((cache[currentCacheIndex].page)[offset - 3])
                , (int)((cache[currentCacheIndex].page)[offset - 4])
                , (int)((cache[currentCacheIndex].page)[offset - 5])
                , (int)((cache[currentCacheIndex].page)[offset - 6]));

            cache[currentCacheIndex].page[cache[currentCacheIndex].page_size] = '\0';
            if (clientsLengthMes[clientIndex] == EMPTY) {
                if (/*(cache[currentCacheSize].page)[offset - 1] == '\0' || */((int)((cache[currentCacheIndex].page)[offset - 1]) == 10
                    && (int)((cache[currentCacheIndex].page)[offset - 2]) == 13
                    && (int)((cache[currentCacheIndex].page)[offset - 3]) == 10
                    && (int)((cache[currentCacheIndex].page)[offset - 4]) == 13
                    )) {
                    if (DEBUG) printf("[DEBUG]: End 10 13 10 13\n");
                    cacheProperty[currentCacheIndex] = DOWNLOADED;
                    return END_SOCKET_INPUT;
                }
            }
            else if (clientsLengthMes[clientIndex] == cache[currentCacheIndex].page_size) {
                cacheProperty[currentCacheIndex] = DOWNLOADED;
                return END_SOCKET_INPUT;
            }
        }
    //}

    return WORKING;
}

int tryFindAtCache(
    const cache_t* cache,
    int cacheEntitiesAmount,
    int* cacheToClient,
    int* sentBytes,
    int clientIndex,
    const char* urlBuffer) {
    printf("\n\ncacheEntitiesAmount - %i\n\n", cacheEntitiesAmount);
    for (int i = 0; i < cacheEntitiesAmount; i++) {
        if (cache[i].title != NULL && strcmp(cache[i].title, urlBuffer) == 0) {
            cacheToClient[clientIndex] = i;
            sentBytes[clientIndex] = 0;
            printf("Page found in cache!\n");
            return 1;
        }
        else {
            //printf("%s\n%s\n", cache[i].title , urlBuffer);
        }
    }
    return 0;
}

char* createRequest(const url_t* url) {
    char* request = (char*)malloc(sizeof(char) * (ADDRESS_BUF_SIZE));
    if (request != NULL) {
        strcpy(request, url->method);
        strcat(request, " /");
        strcat(request, url->path);
//        strcat(request, "\r\n");
    }

    return request;
}

void sendRequest(
    struct pollfd* fds,
    int clientIndex,
    const url_t* url,
    char* urlBuf) {
    if (url->path == NULL) {
        if (DEBUG)printf("[DEBUG]:  url->path == NULL");
        int error = write(fds[clientIndex + MAX_CLIENTS_AMOUNT].fd, "GET /\r\n", strlen("GET /\r\n"));
        if (error < 0) {
            perror("write in sendRequest");
        }
    }
    else {
        char* request = createRequest(url);
//        if (DEBUG)printf("[DEBUG]: REQUEST: %s", request);
        int error = write(fds[clientIndex + MAX_CLIENTS_AMOUNT].fd, request, strlen(request));
        //if (DEBUG)printf("[DEBUG]: fdRem: %i, REQUEST: %s, len: %li, error - %i, client - %i\n", fds[clientIndex + MAX_CLIENTS_AMOUNT].fd, request, strlen(request), error, clientIndex);

        if (error < 0) {
            perror("write in sendRequest");
        }
        if (request != NULL) {
            free(request);
        }
    }
}

int resendFromCache(
    const cache_t* cache,
    struct pollfd* fds,
    int* cacheToClient,
    int* sent_bytes,
    int clientIndex) {
    int written_bytes = 0;

    if (cacheToClient[clientIndex] != EMPTY) {
        if (sent_bytes[clientIndex] < cache[cacheToClient[clientIndex]].page_size) {
            int bytes_left = cache[cacheToClient[clientIndex]].page_size - sent_bytes[clientIndex];
            if(fds[clientIndex].fd == 0){
                return END_FILE;
            }
            if ((written_bytes = write(fds[clientIndex].fd,
                &(cache[cacheToClient[clientIndex]].page[sent_bytes[clientIndex]]), 
                BUFFER_SIZE < bytes_left ? BUFFER_SIZE : bytes_left)) > 0) {
                sent_bytes[clientIndex] += written_bytes;
                if (DEBUG)printf("[DEBUG]: Cache puts in browser\n");                
            }
            else if (errno == EAGAIN) {
                //if (DEBUG)printf("[DEBUG]: Client disconnected! --- < 0\n");
                //cacheToClient[clientIndex] = EMPTY;
                //clients[clientIndex] = EMPTY;
                //*connectedClientsAmount--;
                printf("EAGAIN write in 'resendFromCache'\n");
//                usleep(5000000);
                return END_FILE;

            } else if (written_bytes <= EMPTY) {
                //if (DEBUG)printf("[DEBUG]: Client disconnected! --- < 0\n");
                //cacheToClient[clientIndex] = EMPTY;
                //clients[clientIndex] = EMPTY;
                //*connectedClientsAmount--;
                perror("written_bytes");
                usleep(10000000);
            }
            //printf("print difference %i\n", sent_bytes[clientIndex] - cache[cacheToClient[clientIndex]].page_size);
            if (sent_bytes[clientIndex] == cache[cacheToClient[clientIndex]].page_size) {
                //fds[clientIndex].events = POLLIN;
                //fds[clientIndex].revents = 0;
//                printf("%i\n\n\n", clientIndex);
                return END_FILE;
            }
        }
        else {
            return END_FILE;
        }
    }
    return 0;
}

void readFromClient(int* connectedClientsNumber, struct pollfd* fds, int* clientsLengthMes, int* clientsHttpSocketsEnds,
                    int* cacheToClient, int* sentBytes, int clientIndex, struct sockaddr_in** clientSockaddrs,
                    cache_t* cache, int* cacheProperty, int* cacheIndex, int* cacheSize) {
    char urlBuffer[ADDRESS_BUF_SIZE + 1];
    int read_bytes = read(fds[clientIndex].fd, urlBuffer, ADDRESS_BUF_SIZE);//while with infinity read maybe
    

    if (read_bytes <= 0) {
        if (read_bytes == 0) {
            //if (DEBUG)printf("%s\n", cache[cacheToClient[clientIndex]].title);
            //printf("[CLEANINGG  %i]: %s %i\n", clientIndex, cache[cacheToClient[clientIndex]].title, cache[clientIndex].page_size);
            //return;
            if (DEBUG)printf("[DEBUG]: read bytes == 0 !!!!\n");
            bool isOwner = fds[clientIndex + MAX_CLIENTS_AMOUNT].fd != 0 && fds[clientIndex + MAX_CLIENTS_AMOUNT].fd != 0;
            bool havingClonesCache = false;
            if(isOwner){
                for (int i = 1; i <= MAX_CLIENTS_AMOUNT; i++) {
                    if (cacheToClient[i] == cacheToClient[clientIndex] && clientIndex != i) {
                        havingClonesCache = true;
                        cacheProperty[i] = EMPTY;
                        struct pollfd fd1 = fds[i];
                        struct pollfd fd2 = fds[clientIndex];
                        fds[i] = fd2;
                        fds[clientIndex] = fd1;
                        //                    cacheProperty[cacheToClient[i]] = EMPTY;
                        handleClientDisconnecting(connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds, cacheToClient, sentBytes, i, clientSockaddrs);
                        break;
                    }
                }
            }

            if(!havingClonesCache){
                if (isOwner && fds[clientIndex + MAX_CLIENTS_AMOUNT].fd != 0) {
                    close(fds[clientIndex + MAX_CLIENTS_AMOUNT].fd);
                    fds[clientIndex + MAX_CLIENTS_AMOUNT].fd = 0;
                    connected--;
                }
                if(isOwner && cache[cacheToClient[clientIndex]].title != NULL){
                    freeCache(&(cache[cacheToClient[clientIndex]]));
                }
                cacheProperty[cacheToClient[clientIndex]] = EMPTY;
                handleClientDisconnecting(connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
            }
            //find clones
            //if нет -> clean
            //else -> передача контроля


//            bool isOwner = fds[clientIndex + MAX_CLIENTS_AMOUNT].fd != 0 && fds[clientIndex + MAX_CLIENTS_AMOUNT].fd != 0;
//            if (havingClonesCache && isOwner && fds[clientIndex + MAX_CLIENTS_AMOUNT].fd != 0) {
//                close(fds[clientIndex + MAX_CLIENTS_AMOUNT].fd);
//                fds[clientIndex + MAX_CLIENTS_AMOUNT].fd = 0;
//                connected--;
//            }
            /*
            if (havingClonesCache && isOwner && cacheProperty[cacheToClient[clientIndex]] != DOWNLOADED && cacheProperty[cacheToClient[clientIndex]] != EMPTY) {
                cacheProperty[cacheToClient[clientIndex]] = CLEAN;
                for (int i = 1; i <= MAX_CLIENTS_AMOUNT; i++) {
                    if (cacheProperty[cacheToClient[i]] == CLEAN && clientIndex != i) {
                        cacheProperty[i] = EMPTY;

                        handleClientDisconnecting(connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds, cacheToClient, sentBytes, i, clientSockaddrs);
                    }
                }
                printf("[CLEANING  %i]: %s\n", clientIndex, cache[cacheToClient[clientIndex]].title);

                if(cache[cacheToClient[clientIndex]].title != NULL){
                    freeCache(&(cache[cacheToClient[clientIndex]]));

                }
                cacheProperty[cacheToClient[clientIndex]] = EMPTY;
                printf("Next Cleaning %i %i...\n", clientIndex, cacheToClient[clientIndex]);
            }*/
//            CLEAN1

//            handleClientDisconnecting(connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
        }
        else {
            printf("<=0 read bytes !!!!\n");
        }
    }
    else {

        urlBuffer[read_bytes] = '\0';
        printf("\"%s\"\n", urlBuffer);

        url_t* url = parseURL(urlBuffer, &(clientsLengthMes[clientIndex]));
        if (url == NULL) {
            if (DEBUG) printf("[DEBUG]: URL parsing fail\n");
            return;
        }
        if (strcmp(url->method, "GET") != 0) {
            if (DEBUG) printf("[DEBUG]: URL parsing fail -- method not 'GET'\n");
            handleClientDisconnecting(connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
            freeURL(url);
            return;
        }

        printf("PORT=%d\n", url->port);
        printf("clientIndex = %d, MAX_CLIENTS_AMOUNT = %d\n", clientIndex, MAX_CLIENTS_AMOUNT);

        int findAtCache = tryFindAtCache(cache, *cacheSize, cacheToClient, sentBytes, clientIndex, urlBuffer);

        if (!findAtCache) {
            if (DEBUG) printf("[DEBUG]: URL not find in cache\n");
            if (DEBUG) printf("[DEBUG]: Connecting socket...\n");
            //int emptyFd = checkEmptyFd(fds, MAX_CLIENTS_AMOUNT);
            //if (emptyFd == EMPTY) {
            //}
            fds[clientIndex + MAX_CLIENTS_AMOUNT].fd = socketConnect(fds, &(fds[clientIndex + MAX_CLIENTS_AMOUNT]), url->host, url->port, &(clientSockaddrs[clientIndex]));
            fds[clientIndex + MAX_CLIENTS_AMOUNT].events = POLLIN /*| POLLOUT ����� ������� ���*/;
            //clientsHttpSockets[clientIndex] = socketConnect(url->host, url->port, &clientSockaddrs[clientIndex]);
            if (fds[clientIndex + MAX_CLIENTS_AMOUNT].fd == 0) {
                write(fds[clientIndex].fd, "Failed connect!\n", strlen("Failed connect!\n"));/////////////////////poll write
                if (DEBUG) printf("[DEBUG]: Socket not connect -------------------!!!!!!!!!!!!\n");
                freeURL(url);
                if (fds[clientIndex + MAX_CLIENTS_AMOUNT].fd != 0) {
                    close(fds[clientIndex + MAX_CLIENTS_AMOUNT].fd);
                    fds[clientIndex + MAX_CLIENTS_AMOUNT].fd = 0;
                    connected--;
                }

                handleClientDisconnecting(connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
                return;
            }
            clientsHttpSocketsEnds[clientIndex] = WORKING;
            if (DEBUG)printf("[DEBUG]: Socket connected.\n");

            sendRequest(fds, clientIndex, url, urlBuffer);

            if (*cacheIndex == CACHE_SIZE) {
                (*cacheIndex) = 0;
            }

            if (*cacheSize == CACHE_SIZE && (cache[*cacheIndex].title) != NULL) {
                if (DEBUG)printf("[DEBUG]: free cache...1\n");
                freeCache(&(cache[*cacheIndex]));
            }

            cache[*cacheIndex].title = (char*)malloc(sizeof(char) * strlen(urlBuffer) + 1);
            strcpy(cache[*cacheIndex].title, urlBuffer);
            cache[*cacheIndex].title[strlen(urlBuffer)] = '\0';
//            printf("cache start %i : %s - cache.title\n", clientIndex, cache[*cacheIndex].title);
            cache[*cacheIndex].page_size = 0;
            cache[*cacheIndex].page = (char*)malloc(BUFFER_SIZE + 1);
            cache[*cacheIndex].page[0] = '\0';
//            cache[*cacheIndex].page[cache[*cacheIndex].page_size] = '\0';
            cacheToClient[clientIndex] = *cacheIndex;
            sentBytes[clientIndex] = 0;

            (*cacheIndex)++;
            if (*cacheSize < CACHE_SIZE) {
                (*cacheSize)++;
            }
            cacheProperty[cacheToClient[clientIndex]] = DOWNLOADING;
        }
        else {
            //убрал clean подсосов

            clientsHttpSocketsEnds[clientIndex] = END_SOCKET_INPUT;
            fds[clientIndex].events = POLLOUT;
        }

        freeURL(url);  
    }
}

int main(int argc, char* argv[]) {
    int listenfd = 0;
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    cache_t* cache = (cache_t*)calloc(CACHE_SIZE, sizeof(cache_t));
    int cacheProperty[CACHE_SIZE];
    for (int i = 0; i < CACHE_SIZE; i++) {
        cacheProperty[i] = EMPTY;
    }

    bool cleaning = false;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int result = 1;
    socklen_t socklen = sizeof(result);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &result, socklen);

    //int clients[MAX_CLIENTS_AMOUNT];
    //int clientsHttpSockets[MAX_CLIENTS_AMOUNT];
    int clientsLengthMes[MAX_CLIENTS_AMOUNT];
    int clientsHttpSocketsEnds[MAX_CLIENTS_AMOUNT];
    int cacheToClient[MAX_CLIENTS_AMOUNT];
    int sentBytes[MAX_CLIENTS_AMOUNT];
    struct sockaddr_in* clientSockaddrs[MAX_CLIENTS_AMOUNT];

    for (int k = 1; k <= MAX_CLIENTS_AMOUNT; k++) {
        //clients[k] = EMPTY;
        //clientsHttpSockets[k] = EMPTY;
        cacheToClient[k] = EMPTY;
        sentBytes[k] = EMPTY;
        clientsHttpSocketsEnds[k] = EMPTY;
        clientsLengthMes[k] = EMPTY;
    }

    addr_init(&serv_addr, SERVER_PORT);
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listenfd, 5);

    if (DEBUG)printf("[DEBUG]: listenfd = %d\n", listenfd);

    int connectedClientsNumber = 0;
    int cacheIndex = 0;
    int cacheSize = 0;

    struct pollfd fds[MAX_CLIENTS_AMOUNT * 2 + 1] = { 0 };
    fds[0].fd = listenfd;
    fds[0].events = POLLIN;
    while (1) {
//        printf("%i - fds[0].fd\n", fds[0].fd);
        int resPoll = poll(fds, 1 + MAX_CLIENTS_AMOUNT * 2, INFINITY);
//        for(int i = 1; i < 2 * MAX_CLIENTS_AMOUNT + 1; i++){
//            printf("%i ", fds[i].fd);
//            if(i == MAX_CLIENTS_AMOUNT){
//                printf("\t");
//            }
//        }
//        printf("\n");
//        for(int i = 1; i < 2 * MAX_CLIENTS_AMOUNT + 1; i++){
//            printf("%i ", fds[i].revents);
//            if(i == MAX_CLIENTS_AMOUNT){
//                printf("\t");
//            }
//        }
//        printf("\n\n");
        //int resPoll = poll(fds, connected, INFINITY);
//        printf("%i - fds ready to work. connected  = %i\n", resPoll, connected);
        //usleep(1000000);
//        usleep(100000);
        if (fds[0].revents & POLLIN) {
            fds[0].revents = 0;
            printf("Try accept\n");
            tryAcceptNewClient(fds);
            if (poll(fds, 1, 0) < 0) continue;
            printf("fds[0].revents - %i\n", fds[0].revents);
            //if (fds[0].revents & POLL) {

            //}
        }
        for (int clientIndex = 1; clientIndex <= 2 * MAX_CLIENTS_AMOUNT; clientIndex++) {
                //printf("%i - revents clientI - %i\n", fds[clientIndex].revents, clientIndex);
                //printf("%i - fds[0].fd\n", fds[0].fd);
                if(fds[clientIndex].revents & POLLHUP){
                    if (clientIndex <= MAX_CLIENTS_AMOUNT) {
                        perror("wtf");
//                        usleep(10000000);
                        handleClientDisconnecting(&connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds,
                                                  cacheToClient, sentBytes, clientIndex, clientSockaddrs);

                    }
                    else {
                        int clientInShift = clientIndex - MAX_CLIENTS_AMOUNT;
                        bool isOwner = fds[clientIndex].fd != 0;
                        if (isOwner) {
                            close(fds[clientIndex].fd);
                            connected--;
                            fds[clientIndex].fd = 0;
                        }
                        if (isOwner && cacheProperty[cacheToClient[clientInShift]] != DOWNLOADED &&
                        cacheProperty[cacheToClient[clientInShift]] != EMPTY) {
                            cacheProperty[cacheToClient[clientInShift]] = CLEAN;
                            for (int i = 1; i <= MAX_CLIENTS_AMOUNT; i++) {
                                if (cacheProperty[cacheToClient[i]] == CLEAN && clientInShift != i) {
                                    cacheProperty[cacheToClient[i]] = EMPTY;

                                    handleClientDisconnecting(&connectedClientsNumber, fds, clientsLengthMes,
                                                              clientsHttpSocketsEnds, cacheToClient, sentBytes,
                                                              clientInShift, clientSockaddrs);
                                }
                            }
                            printf("[CLEANING  %i]: %s\n", clientInShift, cache[cacheToClient[clientInShift]].title);

                            freeCache(&(cache[cacheToClient[clientInShift]]));
                            cacheProperty[cacheToClient[clientInShift]] = EMPTY;
                            printf("Next Cleaning %i...\n", clientInShift);
                        }

                        handleClientDisconnecting(&connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds,
                                                  cacheToClient, sentBytes, clientInShift, clientSockaddrs);
                    }
                }
                else if (fds[clientIndex].revents & POLLIN) {
                    if (clientIndex <= MAX_CLIENTS_AMOUNT) {
                        readFromClient(&connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds,//473 - EVENTS = 0
                            cacheToClient, sentBytes, clientIndex, clientSockaddrs, cache,
                            cacheProperty, &cacheIndex, &cacheSize);
//                        printf("%i - sockFD\n", fds[clientIndex + MAX_CLIENTS_AMOUNT].fd);
                    }
                    else {
                        int clientInShift = clientIndex - MAX_CLIENTS_AMOUNT;
                        if (fds[clientIndex].fd != 0) {
                            clientsHttpSocketsEnds[clientInShift] = readToCache(fds, cacheProperty, clientsLengthMes,
                                cache, cacheToClient[clientInShift], clientInShift, clientSockaddrs[clientInShift]);
                            //fds[clientInShift].events = POLLIN | POLLOUT;
                            if (clientsHttpSocketsEnds[clientInShift] == END_SOCKET_INPUT) {
                                printf("disc %i\n", clientInShift);
                                close(fds[clientIndex].fd);
                                fds[clientIndex].fd = 0;
                                connected--;
                            }

                            else if (clientsHttpSocketsEnds[clientInShift] == END_FILE) {
                                printf("disc %i\n", clientInShift);
                                if (fds[clientIndex].fd != 0) {
                                    close(fds[clientIndex].fd);
                                }

                                connected--;
                                handleClientDisconnecting(&connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds,
                                    cacheToClient, sentBytes, clientInShift, clientSockaddrs);
                            }
                            else if (clientsHttpSocketsEnds[clientInShift] == ERROR_MESSAGE) {
                                printf("disc %i\n", clientInShift);
                                //usleep(10000000);
                                bool isOwner = fds[clientIndex].fd != 0;
                                if (isOwner) {
                                    close(fds[clientIndex].fd);
                                    connected--;
                                    fds[clientIndex].fd = 0;
                                }
                                if (isOwner && cacheProperty[cacheToClient[clientInShift]] != DOWNLOADED &&
                                cacheProperty[cacheToClient[clientInShift]] != EMPTY) {
                                    cacheProperty[cacheToClient[clientInShift]] = CLEAN;
                                    for (int i = 1; i <= MAX_CLIENTS_AMOUNT; i++) {
                                        if (cacheProperty[cacheToClient[i]] == CLEAN && clientInShift != i) {
                                            cacheProperty[cacheToClient[i]] = EMPTY;

                                            handleClientDisconnecting(&connectedClientsNumber, fds, clientsLengthMes,
                                                                      clientsHttpSocketsEnds, cacheToClient, sentBytes,
                                                                      clientInShift, clientSockaddrs);
                                        }
                                    }
                                    printf("[CLEANING  %i]: %s\n", clientInShift, cache[cacheToClient[clientInShift]].title);

                                    freeCache(&(cache[cacheToClient[clientInShift]]));
                                    cacheProperty[cacheToClient[clientInShift]] = EMPTY;
                                    printf("Next Cleaning %i...\n", clientInShift);
                                }

                                handleClientDisconnecting(&connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds,
                                    cacheToClient, sentBytes, clientInShift, clientSockaddrs);
                            }
                        }
                    }
                }
                else if(fds[clientIndex].revents & POLLOUT){
                    if (clientIndex <= MAX_CLIENTS_AMOUNT) {
                        int result = resendFromCache(cache, fds, cacheToClient, sentBytes, clientIndex);
//                        printf("%i %i %i %i\n", clientIndex, result, clientsHttpSocketsEnds[clientIndex] == END_SOCKET_INPUT, cacheProperty[cacheToClient[clientIndex]] == DOWNLOADED);
                        if (result == END_FILE &&
                            clientsHttpSocketsEnds[clientIndex] == END_SOCKET_INPUT &&
                            cacheProperty[cacheToClient[clientIndex]] == DOWNLOADED
                            ) {
                            
                            handleClientDisconnecting(&connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds,
                                cacheToClient, sentBytes, clientIndex, clientSockaddrs);
                        }
                        else{
//                            if(result == END_FILE &&
//                                clientsHttpSocketsEnds[clientIndex] == END_SOCKET_INPUT){
//                                    handleClientDisconnecting(&connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
//                            }
                        }
                    }
                    else {

                    }
                }
                else{
//                    printf("fds[clientIndex].revents = %i, clientIndex - %i, fds[clientIndex].fd - %i\n", fds[clientIndex].revents, clientIndex, fds[clientIndex].fd);
                }
                fds[clientIndex].revents = 0;
                if(cacheToClient[cacheIndex] == EMPTY &&
                cache[cacheToClient[cacheIndex]].title == NULL &&
                clientIndex > 0 && clientIndex <= MAX_CLIENTS_AMOUNT &&
                fds[clientIndex].revents == POLLOUT){
                    handleClientDisconnecting(&connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds,
                                              cacheToClient, sentBytes, clientIndex, clientSockaddrs);
                }
            //if (cacheProperty[cacheToClient[clientIndex]] == EMPTY) {
            //    handleClientDisconnecting(&connectedClientsNumber, fds, clientsLengthMes, clientsHttpSocketsEnds, clientsHttpSockets, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
            //}
//            printf("%i - cacheProperty[cacheToClient[clientIndex]], %i - cacheToClient[clientIndex] at socketfunc\n", cacheProperty[cacheToClient[clientIndex]], cacheToClient[clientIndex]);

        }

//        printf("%i - connected\n", connected);
    }
    close(listenfd);
    free(cache);
    return 0;
}
