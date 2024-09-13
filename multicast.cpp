#include "multicast.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

MulticastHandler::MulticastHandler(const std::string& multicastAddress, int port)
    : multicastAddress_(multicastAddress), port_(port), sockfd_(-1), eventBase_(nullptr), recvEvent_(nullptr), running_(false) {}

MulticastHandler::~MulticastHandler() {
    stop();
}

bool MulticastHandler::start() {
    if (running_) return true;  // Already running

    setupSocket();
    setupEvent();

    running_ = true;
    return true;
}

void MulticastHandler::stop() {
    if (!running_) return;

    running_ = false;
    if (recvEvent_) {
        event_del(recvEvent_);
        event_free(recvEvent_);
    }
    if (eventBase_) {
        event_base_free(eventBase_);
    }
    if (sockfd_ != -1) {
        close(sockfd_);
    }
}

void MulticastHandler::sendMessage(const std::string& message) {
    if (!running_) return;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(multicastAddress_.c_str());
    addr.sin_port = htons(port_);

    if (sendto(sockfd_, message.c_str(), message.length(), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("sendto");
    }
}

void MulticastHandler::setupSocket() {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(port_);

    if (bind(sockfd_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(multicastAddress_.c_str());
    mreq.imr_interface.s_addr = INADDR_ANY;

    if (setsockopt(sockfd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
}

void MulticastHandler::setupEvent() {
    eventBase_ = event_base_new();
    if (!eventBase_) {
        std::cerr << "Could not create event base" << std::endl;
        exit(EXIT_FAILURE);
    }

    recvEvent_ = event_new(eventBase_, sockfd_, EV_READ | EV_PERSIST, onMessageReceived, this);
    if (!recvEvent_) {
        std::cerr << "Could not create event" << std::endl;
        exit(EXIT_FAILURE);
    }

    event_add(recvEvent_, nullptr);
    event_base_dispatch(eventBase_);
}

void MulticastHandler::onMessageReceived(evutil_socket_t fd, short events, void* arg) {
    MulticastHandler* handler = static_cast<MulticastHandler*>(arg);
    
    char buffer[1024];
    struct sockaddr_in senderAddr;
    socklen_t addrLen = sizeof(senderAddr);

    int recvLen = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&senderAddr, &addrLen);
    if (recvLen < 0) {
        perror("recvfrom");
        return;
    }
    buffer[recvLen] = '\0';

    std::cout << "Received message: " << buffer << std::endl;
}
