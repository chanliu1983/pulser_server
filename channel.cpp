#include "channel.h"

FdChannelObjectCollection& FdChannelObjectCollection::getInstance() {
    static FdChannelObjectCollection instance;
    return instance;
}

void FdChannelObjectCollection::addChannelObject(evutil_socket_t fd, event* ev, SSL* ssl) {
    std::lock_guard<std::mutex> lock(mutex); // Lock the mutex
    ChannelObject channelObject;
    channelObject.ssl = ssl;
    channelObject.ev = ev;
    channelMap[fd] = channelObject;
}

void FdChannelObjectCollection::deleteChannelObject(evutil_socket_t fd) {
    std::lock_guard<std::mutex> lock(mutex); // Lock the mutex
    channelMap.erase(fd);
}

ChannelObject FdChannelObjectCollection::getChannelObject(evutil_socket_t fd) {
    std::lock_guard<std::mutex> lock(mutex); // Lock the mutex
    auto it = channelMap.find(fd);
    if (it != channelMap.end()) {
        return it->second;
    }
    return ChannelObject();
}

size_t FdChannelObjectCollection::size() {
    std::lock_guard<std::mutex> lock(mutex); // Lock the mutex
    return channelMap.size();
}

FdChannelObjectCollection::FdChannelObjectCollection() {
    // Private constructor to prevent instantiation
}

