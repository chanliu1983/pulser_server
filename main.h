#ifndef MAIN_H
#define MAIN_H

#include "secure.h"
#include "server.h"
#include "setting.h"
#include "sender.h"
#include "channel.h"
#include "compress.h"
#include "conduits.h"
#include "multicast.h"

void recvAndParsePayload(int fd);
void readCallback(evutil_socket_t fd, short events, void* arg);
void sslReadCallback(evutil_socket_t fd, short events, void* arg);

struct SSLManagerEventBase {
    SSLManager* sslManager;
    event_base* base;
};

SSLManagerEventBase* createSSLManagerEventBase(SSLManager* sslManager, event_base* base) {
    SSLManagerEventBase* sslManagerEventBase = new SSLManagerEventBase;
    sslManagerEventBase->sslManager = sslManager;
    sslManagerEventBase->base = base;
    return sslManagerEventBase;
}

#endif // MAIN_H
