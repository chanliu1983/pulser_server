#ifndef EVENTS_H
#define EVENTS_H

#include <event2/event.h>
#include <event2/util.h>
#include <mutex>
#include <map>

class EventsObjectCollection {
public:
    static EventsObjectCollection& getInstance();

    void addEventObject(evutil_socket_t fd, event* ev);
    void deleteEventObject(evutil_socket_t fd);
    event* getEventObject(evutil_socket_t fd);

    size_t size();
    
private:
    EventsObjectCollection(); // Private constructor to prevent instantiation
    EventsObjectCollection(const EventsObjectCollection&) = delete; // Delete copy constructor
    EventsObjectCollection& operator=(const EventsObjectCollection&) = delete; // Delete assignment operator

    std::map<evutil_socket_t, event*> eventMap; // Map to store event pointers
    std::mutex mutex;
};

#endif // EVENTS_H