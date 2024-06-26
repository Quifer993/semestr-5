#ifndef LAB31PROXY_RESULTPARSEHEADING_H
#define LAB31PROXY_RESULTPARSEHEADING_H

#include <string>
#include "ParserTypeRequest.h"
#include "../Constants.h"

namespace ProxyServer {
    class ResultParseHeading {
    public:
        const std::string &getHostName() const {
            return hostName;
        }

        void setHostName(const std::string &hostName) {
            ResultParseHeading::hostName = hostName;
        }

        long long int getContentLength() const {
            return contentLength;
        }

        void setContentLength(long long int contentLength) {
            ResultParseHeading::contentLength = contentLength;
        }

        TypeRequestAndResponse getType() const {
            return type;
        }

        void setType(TypeRequestAndResponse type) {
            ResultParseHeading::type = type;
        }

        ~ResultParseHeading() = default;

        bool isResponseWithError() const {
            return responseWithError;
        }

        void setResponseWithError(bool responseWithError) {
            ResultParseHeading::responseWithError = responseWithError;
        }

        int getPort() {
            return port;
        }

        void setPort(int port) {
            ResultParseHeading::port = port;
        }

        bool isHaveContentLength() const {
            return haveContentLength;
        }

        void setHaveContentLength(bool haveContentLength) {
            ResultParseHeading::haveContentLength = haveContentLength;
        }

    private:
        std::string hostName;
        long long int contentLength = 0;
        TypeRequestAndResponse type;
        bool responseWithError = false;
        int port = DEFAULT_PORT;
        bool haveContentLength = false;

    };

}
#endif //LAB31PROXY_RESULTPARSEHEADING_H
