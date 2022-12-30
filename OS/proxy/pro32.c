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
#include <pthread.h>
#include "cache.h"

#define SERVER_PORT 9605

#define END_FILE -2
#define EMPTY -1
#define WORKING 0
#define DOWNLOADING 1
#define DOWNLOADED 2
#define CLEAN 3

#define MAX_CLIENTS_AMOUNT 16
#define DEBUG 1
#define ADDRESS_BUF_SIZE 1024
#define CACHE_SIZE 1000
#define BUFFER_SIZE 1024

#define END_SOCKET_INPUT -3
#define CONTINUE_SOCKET_INPUT -4

#define PORT_LEN 5
#define NON_BLOCKING 1


typedef struct ClientAndCacheProperties {
    cache_t* cache;
    int cacheProperty[CACHE_SIZE];
    int clients[MAX_CLIENTS_AMOUNT];
    int clientsHttpSockets[MAX_CLIENTS_AMOUNT];
    int clientsLengthMes[MAX_CLIENTS_AMOUNT];
    int clientsHttpSocketsEnds[MAX_CLIENTS_AMOUNT];
    int cacheToClient[MAX_CLIENTS_AMOUNT];
    int sentBytes[MAX_CLIENTS_AMOUNT];
    struct sockaddr_in* clientSockaddrs[MAX_CLIENTS_AMOUNT];
} ClientAndCacheProperties_t;


typedef struct ClientIndexAndProperties {
    ClientAndCacheProperties_t ClientACP;
    int index;
} ClientIndexAndProperties_t;



int socketConnect(char* host, in_port_t port, struct sockaddr_in** clientSockaddr) {
    if (DEBUG) printf("[DEBUG]: getting host...\n");

    printf("host in function = %s\n", host);

    struct hostent* hp = gethostbyname(host);
    if (hp == NULL) {
        perror("gethostbyname");
        return EMPTY;
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

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket");
        return EMPTY;
    }
    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    int s = poll(fds, 1, 0/*�������*/);

    if (s > 0) {
        if (DEBUG)printf("[DEBUG]: Can read from socket\n");
        fds[0].revents = 0;
    }
    else {
        fds[0].revents = 0;
        return EMPTY;
    }

    int opt = fcntl(sock, F_GETFL, 0);
    /*if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
        printf("failed to set non-blocking socket\n");
        return EMPTY;
    }*/
    int error = connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr));
    if (error < -1 && error != EINPROGRESS) {
        perror("connect");
        return EMPTY;
    }
    if (DEBUG) printf("Connected!\n");
    if (fcntl(sock, F_SETFL, opt | O_NONBLOCK) == -1) {
        printf("failed to set non-blocking socket\n");
        return EMPTY;
    }

    if (DEBUG) printf("host openned!\n");

    *clientSockaddr = &addr;
    return sock;
}


typedef struct url {
    char* method;
    char* host;
    char* path;
    int port;
} url_t;

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

void acceptNewClient(int listenfd, int* clients) {
    int freeClientIndex = findFreeClientIndex(clients);
    clients[freeClientIndex] = accept(listenfd, (struct sockaddr*)NULL, NULL);
    if (DEBUG) printf("%i - fileDescrNum\n", clients[freeClientIndex]);
}

int tryAcceptNewClient(
    int listenfd,
    ClientAndCacheProperties_t clientAndCacheProperties,
    int alreadyConnectedClientsNumber,
    struct timeval* timeout) {

    if (alreadyConnectedClientsNumber < MAX_CLIENTS_AMOUNT) {

        struct pollfd fds[1];
        fds[0].fd = listenfd;
        fds[0].events = POLLIN;
        int s = poll(fds, 1, 0/*�������*/);

        //if (ret < -1) { perror("poll"); }
        //else if (ret == 0) {}
        //else if (ret > 0) {
        //    if (fds[0].revents & POLLIN)
        //        fds[0].revents = 0;
        //int s = select(listenfd + 1, &lfds, NULL, NULL, timeout);
        //printf("fuck me %i\n", s);

        if (s > 0) {
            if (DEBUG)printf("[DEBUG]: Can read from listen\n");
            acceptNewClient(listenfd, clientAndCacheProperties.clients);
            alreadyConnectedClientsNumber++;
            fds[0].revents = 0;
        }
    }
    return alreadyConnectedClientsNumber;
}

void handleClientDisconnecting(
    int* clients,
    int* clientsLengthMes,
    int* clientsHttpSocketsEnds,
    int* clientsHttpSockets,
    int* cacheToClient,
    int* sentBytes,
    int clientIndex,
    struct sockaddr_in** clientHostent) {
    printf("client %d disconnecting...\n", clientIndex);
    close(clients[clientIndex]);
    clients[clientIndex] = EMPTY;
    clientsLengthMes[clientIndex] = EMPTY;
    cacheToClient[clientIndex] = EMPTY;
    clientsHttpSocketsEnds[clientIndex] = EMPTY;
    clientHostent[clientIndex] = NULL;
    sentBytes[clientIndex] = 0;
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
                    printf("%i\n", strIndex);

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

    for (int strIndex = 0; strIndex < urlBufferSize; strIndex++) {
        if (urlBuffer[strIndex] == '/') {
            if (strIndex + 1 == urlBufferSize) {
                break;
            }
            strIndex += 2;
            for (strIndex; urlBuffer[strIndex] != '/'; strIndex++) {}

            url->host = (char*)malloc(sizeof(char) * (lengthHost + 2));
            strncpy(url->host, &(urlBuffer[startHostIndex]), lengthHost);
            url->host[lengthHost] = '.';
            url->host[lengthHost + 1] = '\0';
            printf("host = %s\n", url->host);
            //Range: bytes=4721-
            url->path = (char*)malloc(sizeof(char) * (urlBufferSize - strIndex + 2));
            strncpy(url->path, &(urlBuffer[strIndex + 1]), urlBufferSize - strIndex - 1);
            url->path[urlBufferSize - strIndex] = '\0';
            printf("path = %s\n", url->path);

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
}


int readToCache(
    int* cacheProperty,
    int* clientsLengthMes,
    cache_t* cache,
    int currentCacheIndex,
    const int* clientsHttpSockets,
    int clientIndex,
    struct sockaddr_in* clientSockaddr) {

    int offset = cache[currentCacheIndex].page_size;
    int read_bytes = 0;

    int len = sizeof(clientSockaddr);

    struct pollfd fds[1];
    fds[0].fd = clientsHttpSockets[clientIndex];
    fds[0].events = POLLIN | POLLHUP;
    int ret = poll(fds, 1, 0/*�������*/);
    if (ret < -1) { perror("poll"); }
    //else if (ret == 0) { printf("nothing!"); }
    else if (ret > 0) {
        if (fds[0].revents & POLLHUP) {
            fds[0].revents = 0;
            //cacheProperty[currentCacheIndex] = DOWNLOADED;
            printf("pollhub------------<<<<<<<<<<<\n");
            return END_SOCKET_INPUT;
        }
        if (fds[0].revents & POLLIN)
            fds[0].revents = 0;
        read_bytes = recvfrom(clientsHttpSockets[clientIndex], &((cache[currentCacheIndex].page)[offset]), BUFFER_SIZE, 0,
            (struct sockaddr_in*)clientSockaddr,
            (socklen_t*)&len);

        //read_bytes = read(clientsHttpSockets[clientIndex], &((cache[currentCacheIndex].page)[offset]), BUFFER_SIZE);

        if (read_bytes == -1) {
            //if (DEBUG)printf("[DEBUG]: read bytes error!! Now we check answer\n");
            if (errno != EAGAIN) {
                if (DEBUG)printf("[DEBUG]: not eagain -<< read_bytes = %i, clientsHttpSockets[clientIndex] = %i\n", read_bytes, clientsHttpSockets[clientIndex]);
                return CONTINUE_SOCKET_INPUT;
            }
            else {
                if (DEBUG)printf("[DEBUG]: eagain -<< clientsHttpSockets[clientIndex] = %i\n", clientsHttpSockets[clientIndex]);
                errno = 0;

                //return END_SOCKET_INPUT;
                //break;
            }
        }
        else if (read_bytes > 0) {
            //usleep(100000);
            offset += read_bytes;
            cache[currentCacheIndex].page_size += read_bytes;
            if (DEBUG)printf("[DEBUG]: read_bytes = %d. offset = %d\n", read_bytes, offset);

            cache[currentCacheIndex].page = (char*)realloc(cache[currentCacheIndex].page, cache[currentCacheIndex].page_size + BUFFER_SIZE + 1);
            if (DEBUG)printf("[DEBUG]: bytes ->>>> %i, %i, %i, %i, %i, %i\n", /*(int)((cache[currentCacheSize].page)[offset - 0]) ,*/
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
    }

    return WORKING;
}

int tryFindAtCache(
    const cache_t* cache,
    int cacheEntitiesAmount,
    int* cacheToClient,
    int* sentBytes,
    int clientIndex,
    const char* urlBuffer) {

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
        strcat(request, "\r\n");
    }

    return request;
}

void sendRequest(
    const int* clientsHttpSockets,
    int clientIndex,
    const url_t* url,
    char* urlBuf) {
    if (url->path == NULL) {
        if (DEBUG)printf("[DEBUG]:  url->path == NULL");
        write(clientsHttpSockets[clientIndex], "GET /\r\n", strlen("GET /\r\n"));
    }
    else {
        char* request = createRequest(url);
        if (DEBUG)printf("[DEBUG]: REQUEST: %s", request);
        write(clientsHttpSockets[clientIndex], request, strlen(request));
        if (request != NULL) {
            free(request);
        }
    }
}

int resendFromCache(
    const cache_t* cache,
    int* clients,
    int* cacheToClient,
    int* sent_bytes,
    int clientIndex) {
    int written_bytes = 0;

    if (cacheToClient[clientIndex] != EMPTY) {
        if (sent_bytes[clientIndex] < cache[cacheToClient[clientIndex]].page_size) {
            int bytes_left = cache[cacheToClient[clientIndex]].page_size - sent_bytes[clientIndex];
            if ((written_bytes = write(clients[clientIndex],
                &(cache[cacheToClient[clientIndex]].page[sent_bytes[clientIndex]]),
                BUFFER_SIZE < bytes_left ? BUFFER_SIZE : bytes_left)) > 0) {
                sent_bytes[clientIndex] += written_bytes;
                if (DEBUG)printf("[DEBUG]: Cache puts in browser\n");

            }
            else if (written_bytes <= EMPTY) {
                //if (DEBUG)printf("[DEBUG]: Client disconnected! --- < 0\n");
                //cacheToClient[clientIndex] = EMPTY;
                //clients[clientIndex] = EMPTY;
                //*connectedClientsAmount--;
                perror("written_bytes");
            }
        }
        else {
            return END_FILE;
        }
    }
    return 0;
}


void threadSocketHandler(void* buffer) {
    ClientAndCacheProperties_t clientAndCacheProperties = *(ClientAndCacheProperties_t*)(*buffer);


}


int main(int argc, char* argv[]) {
    ClientAndCacheProperties_t clientAndCacheProperties;

    int listenfd = 0;
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    clientAndCacheProperties.cache = (cache_t*)calloc(CACHE_SIZE, sizeof(cache_t));
    //clientAndCacheProperties.cache
    int cacheProperty[CACHE_SIZE];
    
    for (int i = 0; i < CACHE_SIZE; i++) {
        clientAndCacheProperties.cacheProperty[i] = EMPTY;
    }

    bool cleaning = false;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int result = 1;
    socklen_t socklen = sizeof(result);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &result, socklen);

    //int clients[MAX_CLIENTS_AMOUNT];
    //int clientsHttpSockets[MAX_CLIENTS_AMOUNT];
    //int clientsLengthMes[MAX_CLIENTS_AMOUNT];
    //int clientsHttpSocketsEnds[MAX_CLIENTS_AMOUNT];
    //int cacheToClient[MAX_CLIENTS_AMOUNT];
    //int sentBytes[MAX_CLIENTS_AMOUNT];
    //struct sockaddr_in* clientSockaddrs[MAX_CLIENTS_AMOUNT];
    int clientsIndexes[MAX_CLIENTS_AMOUNT];

    for (int k = 0; k < MAX_CLIENTS_AMOUNT; k++) {
        clientsIndexes[k] = EMPTY;
        clientAndCacheProperties.clients[k] = EMPTY;
        clientAndCacheProperties.clientsHttpSockets[k] = EMPTY;
        clientAndCacheProperties.cacheToClient[k] = EMPTY;
        clientAndCacheProperties.sentBytes[k] = EMPTY;
        clientAndCacheProperties.clientsHttpSocketsEnds[k] = EMPTY;
        clientAndCacheProperties.clientsLengthMes[k] = EMPTY;
    }


    addr_init(&serv_addr, SERVER_PORT);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listenfd, MAX_CLIENTS_AMOUNT);

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    int error_code = 0;
    if (DEBUG)printf("[DEBUG]: listenfd = %d\n", listenfd);

    fd_set cfds;
    int connectedClientsNumber = 0;
    int cacheIndex = 0;
    int cacheSize = 0;
    pthread_t threads[MAX_CLIENTS_AMOUNT];


    while (1) {
        connectedClientsNumber = tryAcceptNewClient(listenfd, clientAndCacheProperties, connectedClientsNumber, &timeout);
        ClientIndexAndProperties_t ciap;
        ciap.ClientACP = clientAndCacheProperties;

        ciap.index = ;
        error_code = pthread_create(&threads[i], NULL, threadSocketHandler, &clientAndCacheProperties);
        if (error_code != 0) {
            printf("Thread creation error: %s\n", strerror(error_code));
            count_created_threads = i;
            break;
        }
        //
        for (int clientIndex = 0; clientIndex < MAX_CLIENTS_AMOUNT; clientIndex++) {
            if (clients[clientIndex] == EMPTY) continue;

            struct pollfd fds[1];
            fds[0].fd = clients[clientIndex];
            fds[0].events = POLLIN;
            int ret = poll(fds, 1, 0/*�������*/);

            if (ret < -1) {
                perror("poll");
                handleClientDisconnecting(clients, clientsLengthMes, clientsHttpSocketsEnds, clientsHttpSockets, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
                connectedClientsNumber--;
            }
            else if (ret == 0) {}
            else if (ret > 0) {
                if (fds[0].revents & POLLIN)
                    fds[0].revents = 0;

                char urlBuffer[ADDRESS_BUF_SIZE + 1];
                int read_bytes = read(clients[clientIndex], urlBuffer, ADDRESS_BUF_SIZE);

                if (read_bytes <= 0) {
                    if (read_bytes == 0) {
                        bool isOwner = clientsHttpSockets[clientIndex] != EMPTY;
                        if (isOwner) {
                            close(clientsHttpSockets[clientIndex]);
                            clientsHttpSockets[clientIndex] = EMPTY;
                        }
                        if (isOwner && clientAndCacheProperties.cacheProperty[cacheToClient[clientIndex]] != DOWNLOADED && clientAndCacheProperties.cacheProperty[cacheToClient[clientIndex]] != EMPTY) {
                            clientAndCacheProperties.cacheProperty[cacheToClient[clientIndex]] = CLEAN;
                            cleaning = true;
                            for (int i = 0; i < MAX_CLIENTS_AMOUNT; i++) {
                                if (clientAndCacheProperties.cachePrope rty[cacheToClient[i]] == CLEAN && clientIndex != i) {
                                    handleClientDisconnecting(clients, clientsLengthMes, clientsHttpSocketsEnds, clientsHttpSockets, cacheToClient, sentBytes, i, clientSockaddrs);
                                    connectedClientsNumber--;
                                }
                            }
                        }

                        if (DEBUG)printf("[DEBUG]: read bytes == 0 !!!!\n");
                        handleClientDisconnecting(clients, clientsLengthMes, clientsHttpSocketsEnds, clientsHttpSockets, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
                        connectedClientsNumber--;
                    }
                    continue;
                }

                urlBuffer[read_bytes] = '\0';
                printf("\"%s\"\n", urlBuffer);

                url_t* url = parseURL(urlBuffer, &(clientsLengthMes[clientIndex]));
                if (url == NULL /*|| strcmp(url->method, "GET") != 0*/) {
                    if (DEBUG) printf("[DEBUG]: URL parsing fail\n");
                    freeURL(url);
                    continue;
                }
                printf("PORT=%d\n", url->port);
                printf("clientIndex = %d, MAX_CLIENTS_AMOUNT = %d\n", clientIndex, MAX_CLIENTS_AMOUNT);

                int findAtCache = tryFindAtCache(clientAndCacheProperties.cache, cacheSize, cacheToClient, sentBytes, clientIndex, urlBuffer);

                if (!findAtCache) {
                    if (DEBUG) printf("[DEBUG]: URL not find in cache\n");
                    if (DEBUG) printf("[DEBUG]: Connecting socket...\n");

                    clientsHttpSockets[clientIndex] = socketConnect(url->host, url->port, &clientSockaddrs[clientIndex]);

                    if (clientsHttpSockets[clientIndex] == EMPTY) {
                        write(clients[clientIndex], "Failed connect!\n", strlen("Failed connect!\n"));
                        if (DEBUG) printf("[DEBUG]: Socket not connect -------------------!!!!!!!!!!!!\n");
                        freeURL(url);
                        if (clientsHttpSockets[clientIndex] != EMPTY) {
                            close(clientsHttpSockets[clientIndex]);
                            clientsHttpSockets[clientIndex] = EMPTY;
                        }


                        handleClientDisconnecting(clients, clientsLengthMes, clientsHttpSocketsEnds, clientsHttpSockets, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
                        connectedClientsNumber--;
                        continue;
                    }
                    clientsHttpSocketsEnds[clientIndex] = WORKING;
                    if (DEBUG)printf("[DEBUG]: Socket connected.\n");

                    sendRequest(clientsHttpSockets, clientIndex, url, urlBuffer);

                    if (cacheIndex == CACHE_SIZE) {
                        cacheIndex = 0;
                    }

                    if (cacheSize == CACHE_SIZE && (clientAndCacheProperties.cache[cacheIndex].title) != NULL) {
                        if (true) {

                        }
                        if (DEBUG)printf("[DEBUG]: free cache...1\n");
                        freeCache(&clientAndCacheProperties.cache[cacheIndex]);
                    }

                    clientAndCacheProperties.cache[cacheIndex].title = (char*)malloc(sizeof(char) * strlen(urlBuffer) + 1);
                    strcpy(clientAndCacheProperties.cache[cacheIndex].title, urlBuffer);
                    clientAndCacheProperties.cache[cacheIndex].title[strlen(urlBuffer)] = '\0';

                    clientAndCacheProperties.cache[cacheIndex].page_size = 0;
                    clientAndCacheProperties.cache[cacheIndex].page = (char*)malloc(BUFFER_SIZE + 1);
                    clientAndCacheProperties.cache[cacheIndex].page[clientAndCacheProperties.cache[cacheIndex].page_size] = '\0';
                    cacheToClient[clientIndex] = cacheIndex;
                    sentBytes[clientIndex] = 0;

                    cacheIndex++;
                    if (cacheSize < CACHE_SIZE) {
                        cacheSize++;
                    }
                    clientAndCacheProperties.cacheProperty[cacheToClient[clientIndex]] = DOWNLOADING;

                }
                else {
                    clientsHttpSocketsEnds[clientIndex] = END_SOCKET_INPUT;
                }

                freeURL(url);
            }

            if (clientsHttpSockets[clientIndex] != EMPTY) {
                clientsHttpSocketsEnds[clientIndex] = readToCache(clientAndCacheProperties.cacheProperty, clientsLengthMes,
                    clientAndCacheProperties.cache, cacheToClient[clientIndex], clientsHttpSockets,
                    clientIndex, clientSockaddrs[clientIndex]);

                if (clientsHttpSocketsEnds[clientIndex] == END_SOCKET_INPUT) {
                    close(clientsHttpSockets[clientIndex]);
                    clientsHttpSockets[clientIndex] = EMPTY;
                }
            }

            if (resendFromCache(clientAndCacheProperties.cache, clients, cacheToClient, sentBytes, clientIndex) == END_FILE &&
                clientsHttpSocketsEnds[clientIndex] == END_SOCKET_INPUT &&
                clientAndCacheProperties.cacheProperty[cacheToClient[clientIndex]] == DOWNLOADED
                ) {

                handleClientDisconnecting(clients, clientsLengthMes, clientsHttpSocketsEnds, clientsHttpSockets, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
                connectedClientsNumber--;
            }
            if (clientAndCacheProperties.cacheProperty[cacheToClient[clientIndex]] == EMPTY) {
                handleClientDisconnecting(clients, clientsLengthMes, clientsHttpSocketsEnds, clientsHttpSockets, cacheToClient, sentBytes, clientIndex, clientSockaddrs);
                connectedClientsNumber--;
            }
            //printf("%i - cacheProperty[cacheToClient[clientIndex]], %i - cacheToClient[clientIndex] at socketfunc\n", cacheProperty[cacheToClient[clientIndex]], cacheToClient[clientIndex]);

        }
        for (int i = 0; i < CACHE_SIZE && cleaning; i++) {
            if (clientAndCacheProperties.cacheProperty[i] == CLEAN) {
                printf("[CLEANING  %i]: %s\n", i, clientAndCacheProperties.cache[i].title);

                freeCache(&clientAndCacheProperties.cache[i]);
                clientAndCacheProperties.cacheProperty[i] = EMPTY;
                printf("Next Cleaning %i...\n", i);
            }
        }
        cleaning = false;
    }
    free(clientAndCacheProperties.cache);
    return 0;
}
