#include "server.h"
#include <iostream>  // For std::cerr and std::endl
#include <stdexcept> // For std::runtime_error
#include <cstring>   // For strlen
#include <event2/event.h> // For event_base_new, event_base_free, event_base_dispatch
#include <event2/http.h>  // For evhttp_new, evhttp_free, evhttp_bind_socket, evhttp_set_gencb, evhttp_request, evbuffer_new, evbuffer_free, evhttp_send_error, evhttp_send_reply, evhttp_add_header, evhttp_request_get_output_headers, evbuffer_add

Server::Server() : base(event_base_new()), httpServer(evhttp_new(base)) {
    if (!base) {
        std::cerr << "Could not initialize libevent!" << std::endl;
        throw std::runtime_error("Failed to initialize libevent");
    }

    if (!httpServer) {
        std::cerr << "Failed to create HTTP server" << std::endl;
        throw std::runtime_error("Failed to create HTTP server");
    }
}

Server::~Server() {
    if (httpServer) {
        evhttp_free(httpServer);
    }

    if (base) {
        event_base_free(base);
    }
}

void Server::start(int port) {
    if (evhttp_bind_socket(httpServer, "0.0.0.0", port) != 0) {
        std::cerr << "Failed to bind HTTP server to address and port" << std::endl;
        throw std::runtime_error("Failed to bind HTTP server");
    }

    evhttp_set_gencb(httpServer, httpCallback, nullptr);

    event_base_dispatch(base);
}

void Server::httpCallback(struct evhttp_request* req, void* arg) {
    struct evbuffer* responseBuffer = evbuffer_new();
    if (!responseBuffer) {
        std::cerr << "Failed to create response buffer" << std::endl;
        evhttp_send_error(req, HTTP_INTERNAL, "Internal Server Error");
        return;
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain");

    const char* responseText = "Hello, World!";
    evbuffer_add(responseBuffer, responseText, strlen(responseText));

    evhttp_send_reply(req, HTTP_OK, "OK", responseBuffer);

    evbuffer_free(responseBuffer);
}