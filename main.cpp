#include <event2/event.h>
#include <event2/listener.h>
#include <algorithm>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <zlib.h>
#include <signal.h>
#include <thread>
#include <map>
#include "main.h"

void cleanupFd(evutil_socket_t fd) {
    static FdChannelObjectCollection& channelMap = FdChannelObjectCollection::getInstance();
    static ConduitsCollection& conduits = ConduitsCollection::getInstance();

    ChannelObject obj = channelMap.getChannelObject(fd);
    if (obj.ssl) {
        SSL_shutdown(obj.ssl);
        SSL_free(obj.ssl);
    }

    event* ev = obj.ev;
    if (ev) {
        event_del(ev);
        event_free(ev);
    }

    channelMap.deleteChannelObject(fd);
    conduits.deleteFd(fd);

    evutil_closesocket(fd);
    std::cout << "Connection closed" << std::endl;
}

void sslReadCallback(evutil_socket_t fd, short events, void* arg) {
    SSL* ssl = static_cast<SSL*>(arg);

    if (events & EV_READ) {
        char buffer[1];
        int bytes_read = SSL_peek(ssl, buffer, sizeof(buffer));

        if (bytes_read > 0) {
            // Process the received data
            recvAndParsePayload(fd);
        } else if (bytes_read == 0) {
            // Connection closed
            cleanupFd(fd);
        } else {
            int error = SSL_get_error(ssl, bytes_read);
            if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
                // Handle non-blocking mode
                return;
            } else {
                std::cerr << "Could not read data from SSL connection: ";
                ERR_print_errors_fp(stderr);
                cleanupFd(fd);
            }
        }
    }
}

void acceptCallback(evconnlistener* listener, evutil_socket_t fd, sockaddr* address, int socklen, void* arg) {
    SSLManagerEventBase* sslManagerEventBase = static_cast<SSLManagerEventBase*>(arg);
    SSLManager* sslManager = sslManagerEventBase->sslManager;
    event_base* base = sslManagerEventBase->base;

    SSL* ssl = sslManager->createSSL(fd);
    if (!ssl) {
        std::cerr << "Could not create SSL object" << std::endl;
        return;
    }

    int acceptResult = SSL_accept(ssl);
    if (acceptResult <= 0) {
        int error = SSL_get_error(ssl, acceptResult);
        if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
            static FdChannelObjectCollection& fdChannelObjectCollection = FdChannelObjectCollection::getInstance();

            // Set the socket to non-blocking mode
            evutil_make_socket_nonblocking(fd);

            // Create a new event for reading
            event* readEvent = event_new(base, fd, EV_READ | EV_PERSIST, sslReadCallback, ssl);
            event_add(readEvent, nullptr);
            
            fdChannelObjectCollection.addChannelObject(fd, readEvent, ssl);
            return;
        } else {
            std::cerr << "Could not accept SSL connection: ";
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            return;
        }
    }
}

#include "executor.h"

MulticastHandler multicastHandler("239.0.0.1", 16667);
Executor executor(&multicastHandler);
SSLManager sslManager;
int managementPort = 8080;

void recvAndParsePayload(int fd) {
    static FdChannelObjectCollection& channelObject = FdChannelObjectCollection::getInstance();
    ChannelObject obj = channelObject.getChannelObject(fd);
    SSL* ssl = obj.ssl;

    std::string receivedData = SenderUtility::recvRawPayloadSSL(ssl);
    executor.processJsonCommand(fd, receivedData);
}

#include <thread>

void runWebServer(Server& server) {
    server.start(managementPort);
}

int main() {
    // load ini setting file
    PulserConfig config("setting.ini");

    // Initialize the SSL manager
    if (!sslManager.initialize()) {
        std::cerr << "Could not initialize the SSL manager!" << std::endl;
        return 1;
    }

    // configuration context for ssl
    sslManager.createContext(); 

    if (!sslManager.configureContext(config.getCertFile(), config.getKeyFile())) {
        std::cerr << "Could not configure the SSL context!" << std::endl;
        return 1;
    }

    managementPort = config.getManagementEndPointPort();

    multicastHandler.setMulticastAddressAndPort(config.getClusterMultiCastIP(), config.getClusterMultiCastPort());
    multicastHandler.setOnMessageReceivedCallback([](const std::string& message) {
        std::cout << "Received message from multicast: " << message << std::endl;
        executor.processJsonCommand(-1, message);
    });

    executor.setServerName(config.getServerName());

    // Ignore the SIGPIPE signal
    signal(SIGPIPE, SIG_IGN);

    ConduitsCollection& conduits = ConduitsCollection::getInstance();

    Server server;
    std::thread serverThread(runWebServer, std::ref(server));
    serverThread.detach();

    std::thread multicastThread(&MulticastHandler::start, &multicastHandler);
    multicastThread.detach();

    struct event_base* base;
    struct evconnlistener* listener;
    struct sockaddr_in sin;

    base = event_base_new();
    if (!base) {
        std::cerr << "Could not initialize libevent!" << std::endl;
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(config.getConduitEndpointPort()); // Replace with your desired port
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    listener = evconnlistener_new_bind(
        base,
        acceptCallback,
        createSSLManagerEventBase(&sslManager, base),
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
        -1,
        (struct sockaddr*)&sin,
        sizeof(sin)
    );

    if (!listener) {
        std::cerr << "Could not create a listener!" << std::endl;
        return 1;
    }

    // Create a timeout event
    event* timeoutEvent = event_new(base, -1, EV_TIMEOUT | EV_PERSIST, [](evutil_socket_t fd, short events, void* arg) {
        static FdChannelObjectCollection& channelObject = FdChannelObjectCollection::getInstance();
        // Check if the event map size has changed
        static size_t prevSize = 0;
        size_t currentSize = channelObject.size();
        if (currentSize != prevSize) {
            std::cout << "Channel map count: " << currentSize << std::endl;
            prevSize = currentSize;
        }
    }, nullptr);

    // Set the timeout event to trigger every 1 second
    timeval tv;
    evutil_timerclear(&tv);
    tv.tv_sec = 5;
    event_add(timeoutEvent, &tv);

    // Start the event loop
    event_base_dispatch(base);

    // Clean up
    event_free(timeoutEvent);
    evconnlistener_free(listener);
    event_base_free(base);

    return 0;
}