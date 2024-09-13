#ifndef SENDER_H
#define SENDER_H

#include <cstdint>
#include <string>

class SenderUtility {
public:
    static void sendRawPayload(int fd, const std::string& source);
    static std::string recvRawPayload(int fd);
};

#endif // SENDER_H
