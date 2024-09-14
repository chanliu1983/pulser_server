#ifndef CHANNEL_H
#define CHANNEL_H

#include <event2/event.h>
#include <event2/util.h>
#include <openssl/ssl.h>
#include <mutex>
#include <map>

struct ChannelObject {
    SSL* ssl;
    event* ev;
};

class FdChannelObjectCollection {
public:
    static FdChannelObjectCollection& getInstance();

    void addChannelObject(evutil_socket_t fd, event* ev, SSL* ssl);
    void deleteChannelObject(evutil_socket_t fd);
    ChannelObject getChannelObject(evutil_socket_t fd);

    size_t size();
    
private:
    FdChannelObjectCollection(); // Private constructor to prevent instantiation
    FdChannelObjectCollection(const FdChannelObjectCollection&) = delete; // Delete copy constructor
    FdChannelObjectCollection& operator=(const FdChannelObjectCollection&) = delete; // Delete assignment operator

    std::map<evutil_socket_t, ChannelObject> channelMap; // Map of file descriptors to channel objects
    std::mutex mutex;
};

#endif // CHANNEL_H