#ifndef SENDER_H
#define SENDER_H

#include <cstdint>
#include <string>
#include "multicast.h"

class SenderUtility {
public:
    static void sendRawPayload(int fd, const std::string& source);
    static void sendRawPayloadAndBroadcast(int fd, const std::string& source, MulticastHandler* multicastHandler);
    static std::string recvRawPayload(int fd);
};

#endif // SENDER_H
