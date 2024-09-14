#ifndef SENDER_H
#define SENDER_H

#include <cstdint>
#include <string>
#include <openssl/ssl.h>
#include "multicast.h"

class SenderUtility {
public:
    static std::string recvRawPayloadSSL(SSL* ssl);
    static void sendRawPayloadSSL(SSL* ssl, const std::string& source);

    static void broadcastRawPayload(const std::string& source, MulticastHandler* multicastHandler);
};

#endif // SENDER_H
