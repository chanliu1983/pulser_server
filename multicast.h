#ifndef MULTICAST_HANDLER_H
#define MULTICAST_HANDLER_H

#include <event2/event.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

class MulticastHandler {
public:
    MulticastHandler(const std::string& multicastAddress, int port);
    ~MulticastHandler();

    bool start();
    void stop();
    void sendMessage(const std::string& message);

    void setMulticastAddressAndPort(const std::string& multicastAddress, int port);

private:
    void setupSocket();
    void setupEvent();

    std::string multicastAddress_;
    int port_;
    int sockfd_;
    struct sockaddr_in addr_;
    struct event_base* eventBase_;
    struct event* recvEvent_;
    bool running_;
    
    // Event callback for receiving messages
    static void onMessageReceived(evutil_socket_t fd, short events, void* arg);
};

#endif // MULTICAST_HANDLER_H
