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
#include "conduits.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

std::vector<evutil_socket_t> connections; // Store accepted connections
std::map<evutil_socket_t, event*> eventMap; // Map to store event pointers

void acceptCallback(evconnlistener* listener, evutil_socket_t fd, sockaddr* address, int socklen, void* arg) {
    std::cout << "Accepted connection" << std::endl;

    // Add the new connection to the collection
    connections.push_back(fd);

    // Set the socket to non-blocking mode
    evutil_make_socket_nonblocking(fd);

    // Set up an event to monitor the socket for read events
    event_base* base = (event_base*)arg;
    event* readEvent = event_new(base, fd, EV_READ | EV_PERSIST, readCallback, nullptr);
    event_add(readEvent, nullptr);

    // Store the event pointer in the map
    eventMap[fd] = readEvent;
}

void readCallback(evutil_socket_t fd, short events, void* arg) {
    if (events & EV_READ) {
        char buffer[1];
        // Check if the socket has been closed
        int result = recv(fd, buffer, 1, MSG_PEEK);
        if (result == 0) {
            // Socket has been closed
            std::cout << "Socket closed by the other side" << std::endl;
            // Remove the closed socket from the connections vector
            auto it = std::find(connections.begin(), connections.end(), fd);
            if (it != connections.end()) {
                connections.erase(it);
            }
            // Close the socket
            evutil_closesocket(fd);
            ConduitsCollection::getInstance().deleteFd(fd);

            // Remove the closed socket from the connections vector
            connections.erase(std::remove(connections.begin(), connections.end(), fd), connections.end());

            // Retrieve the event pointer from the map
            auto it0 = eventMap.find(fd);
            if (it0 == eventMap.end()) {
                std::cerr << "Event not found for socket: " << fd << std::endl;
                return;
            }
            event* readEvent = it0->second;
            event_free(readEvent);
            eventMap.erase(fd);
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

                // Remove the closed socket from the connections vector
                connections.erase(std::remove(connections.begin(), connections.end(), fd), connections.end());

                // Retrieve the event pointer from the map
                auto it = eventMap.find(fd);
                if (it == eventMap.end()) {
                    std::cerr << "Event not found for socket: " << fd << std::endl;
                    return;
                }
                event* readEvent = it->second;
                event_free(readEvent);
                eventMap.erase(fd);
            }
        } else {
            // Socket is still open, continue reading
            recvAndParsePayload(fd);
        }
    }
}

struct DataFrame {
    uint32_t magic; // Magic number to identify the payload
    uint32_t size; // Size of the payload
    uint32_t checksum; // Checksum of the payload
    char payload[0]; // Payload data

    // Constructor to initialize the struct
    DataFrame(uint32_t payloadSize, const char* payloadData) {
        magic = 0x12344321; // Set the magic number
        size = payloadSize;
        checksum = calculateChecksum(payloadData, payloadSize);
        memcpy(payload, payloadData, payloadSize);
    }

    static uint32_t calculateChecksum(const char* data, uint32_t dataSize) {
        return crc32(0L, reinterpret_cast<const Bytef*>(data), dataSize);
    }
};

#include "executor.h"

void recvAndParsePayload(int fd)
{
    static Executor executor; // Create an instance of the Executor class

    // Receive the magic
    uint32_t magic;
    if (recv(fd, reinterpret_cast<char*>(&magic), sizeof(magic), 0) != sizeof(magic))
    {
        std::cerr << "Error receiving magic: " << strerror(errno) << std::endl;
        return;
    }

    // print magic as 0x hex
    std::cout << "Received Magic: 0x" << std::hex << magic << std::endl;

    // Receive the payload size
    uint32_t payloadSize;
    if (recv(fd, reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize), 0) != sizeof(payloadSize))
    {
        std::cerr << "Error receiving payload size: " << strerror(errno) << std::endl;
        return;
    }

    // Receive the checksum
    uint32_t checksum;
    if (recv(fd, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0) != sizeof(checksum))
    {
        std::cerr << "Error receiving checksum: " << strerror(errno) << std::endl;
        return;
    }

    // Check if the received magic is valid
    if (magic != 0x12344321)
    {
        std::cerr << "Invalid magic" << std::endl;
        return;
    }

    // Allocate memory for the payload
    char* payload = new char[payloadSize];

    // Receive the payload data
    int totalBytesRead = 0;
    int eagainCount = 0;
    while (totalBytesRead < payloadSize)
    {
        int bytesRead = recv(fd, payload + totalBytesRead, payloadSize - totalBytesRead, 0);
        if (bytesRead <= 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available (non-blocking mode), try again after 500 ms
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                eagainCount++;
                if (eagainCount > 3) {
                    std::cerr << "Error receiving payload data: EAGAIN exceeded" << std::endl;
                    delete[] payload;
                    return;
                }
                continue;
            } else {
                std::cerr << "Error receiving payload data: " << strerror(errno) << std::endl;
                delete[] payload;
                return;
            }
        }
        totalBytesRead += bytesRead;
    }

    // Calculate the checksum of the received payload
    uint32_t calculatedChecksum = DataFrame::calculateChecksum(payload, payloadSize);
    // Check if the received checksum is valid
    if (checksum != calculatedChecksum)
    {
        std::cerr << "Invalid checksum" << std::endl;
        delete[] payload;
        return;
    }

    // Process the received payload
    std::string receivedData(payload, payloadSize);
    std::cout << "Received Payload: " << receivedData << std::endl;

    // Clean up
    delete[] payload;
    payload = nullptr;

    // Process the received payload
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
        // Print out the number of connections
        std::cout << "Number of connections: " << connections.size() << std::endl;

        // Print out all the file descriptor (fd) IDs
        std::cout << "File Descriptor IDs: ";
        for (const auto& fd : connections) {
            std::cout << fd << " ";
        }
        std::cout << std::endl;
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