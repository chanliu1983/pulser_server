#ifndef SERVER_H
#define SERVER_H

#include <event2/http.h>
#include <event2/buffer.h>

class Server {
public:
    Server();
    ~Server();
    void start(int port);

private:
    static void httpCallback(struct evhttp_request* req, void* arg);

    struct event_base* base;
    struct evhttp* httpServer;
};

#endif // SERVER_H
