#include "events.h"

EventsObjectCollection& EventsObjectCollection::getInstance() {
    static EventsObjectCollection instance;
    return instance;
}

void EventsObjectCollection::addEventObject(evutil_socket_t fd, event* ev) {
    std::lock_guard<std::mutex> lock(mutex); // Lock the mutex
    eventMap[fd] = ev;
}

void EventsObjectCollection::deleteEventObject(evutil_socket_t fd) {
    std::lock_guard<std::mutex> lock(mutex); // Lock the mutex
    auto it = eventMap.find(fd);
    if (it != eventMap.end()) {
        eventMap.erase(it);
    }
}

event* EventsObjectCollection::getEventObject(evutil_socket_t fd) {
    std::lock_guard<std::mutex> lock(mutex); // Lock the mutex
    auto it = eventMap.find(fd);
    if (it != eventMap.end()) {
        return it->second;
    }
    return nullptr;
}

size_t EventsObjectCollection::size() {
    std::lock_guard<std::mutex> lock(mutex); // Lock the mutex
    return eventMap.size();
}

EventsObjectCollection::EventsObjectCollection() {
    // Private constructor to prevent instantiation
}

