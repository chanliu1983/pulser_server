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
#include "server.h"
#include "sender.h"
#include "events.h"
#include "compress.h"
#include "conduits.h"
#include "multicast.h"

void acceptCallback(evconnlistener* listener, evutil_socket_t fd, sockaddr* address, int socklen, void* arg) {
    std::cout << "Accepted connection" << std::endl;

    // Set the socket to non-blocking mode
    evutil_make_socket_nonblocking(fd);

    // Set up an event to monitor the socket for read events
    event_base* base = (event_base*)arg;
    event* readEvent = event_new(base, fd, EV_READ | EV_PERSIST, readCallback, nullptr);
    event_add(readEvent, nullptr);

    // Store the event pointer in the map
    EventsObjectCollection::getInstance().addEventObject(fd, readEvent);
}

void readCallback(evutil_socket_t fd, short events, void* arg) {
    static EventsObjectCollection& eventObjects = EventsObjectCollection::getInstance();

    if (events & EV_READ) {
        char buffer[1];
        // Check if the socket has been closed
        int result = recv(fd, buffer, 1, MSG_PEEK);
        if (result == 0) {
            // Socket has been closed
            std::cout << "Socket closed by the other side" << std::endl;

            // Close the socket
            evutil_closesocket(fd);
            ConduitsCollection::getInstance().deleteFd(fd);

            event* readEvent = eventObjects.getEventObject(fd);
            event_free(readEvent);
            eventObjects.deleteEventObject(fd);
        } else if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available (non-blocking mode)
                std::cout << "No data available, try again later" << std::endl;
                return;
            } else {
                // Other errors
                std::cerr << "Error checking socket status: " << strerror(errno) << std::endl;
                // Close the socket
                evutil_closesocket(fd);
                ConduitsCollection::getInstance().deleteFd(fd);

                event* readEvent = eventObjects.getEventObject(fd);
                event_free(readEvent);
                eventObjects.deleteEventObject(fd);
            }
        } else {
            // Socket is still open, continue reading
            recvAndParsePayload(fd);
        }
    }
}

#include "executor.h"

MulticastHandler multicastHandler("239.0.0.1", 16667);

void recvAndParsePayload(int fd)
{
    static Executor executor(&multicastHandler); // Create an instance of the Executor class

    std::string receivedData = SenderUtility::recvRawPayload(fd);
    executor.processJsonCommand(fd, receivedData);
}

#include <thread>

void runWebServer(Server& server) {
    server.start(8080);
}

int main() {
    // Ignore the SIGPIPE signal
    signal(SIGPIPE, SIG_IGN);

    ConduitsCollection& conduits = ConduitsCollection::getInstance();

    Server server;
    std::thread serverThread(runWebServer, std::ref(server));
    serverThread.detach();

    std::thread multicastThread(&MulticastHandler::start, &multicastHandler);

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
    sin.sin_port = htons(16666); // Replace with your desired port
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    listener = evconnlistener_new_bind(
        base,
        acceptCallback,
        base,
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
        // Check if the event map size has changed
        static size_t prevSize = 0;
        size_t currentSize = EventsObjectCollection::getInstance().size();
        if (currentSize != prevSize) {
            std::cout << "Event map count: " << currentSize << std::endl;
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