#include "server.h"
#include "conduits.h" // For ConduitsCollection, Conduit
#include <iostream>  // For std::cerr and std::endl
#include <stdexcept> // For std::runtime_error
#include <cstring>   // For strlen
#include <fstream>   // For std::ifstream
#include <event2/event.h> // For event_base_new, event_base_free, event_base_dispatch
#include <event2/http.h>  // For evhttp_new, evhttp_free, evhttp_bind_socket, evhttp_set_gencb, evhttp_request, evbuffer_new, evbuffer_free, evhttp_send_error, evhttp_send_reply, evhttp_add_header, evhttp_request_get_output_headers, evbuffer_add
#include "rapidjson/document.h" // For rapidjson::Document
#include "rapidjson/writer.h"   // For rapidjson::Writer
#include "rapidjson/stringbuffer.h" // For rapidjson::StringBuffer

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

void Server::handleRootRequest(struct evhttp_request* req) {
    // Load the index.html file from disk
    std::ifstream file("index.html");
    if (!file.is_open()) {
        std::cerr << "Failed to open index.html" << std::endl;
        evhttp_send_error(req, HTTP_NOTFOUND, "Not Found");
        return;
    }

    std::string html((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    struct evbuffer* responseBuffer = evbuffer_new();
    if (!responseBuffer) {
        std::cerr << "Failed to create response buffer" << std::endl;
        evhttp_send_error(req, HTTP_INTERNAL, "Internal Server Error");
        return;
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html");
    evbuffer_add(responseBuffer, html.c_str(), html.size());

    evhttp_send_reply(req, HTTP_OK, "OK", responseBuffer);
    evbuffer_free(responseBuffer);
}

void Server::handleDataRequest(struct evhttp_request* req) {
    struct evbuffer* responseBuffer = evbuffer_new();
    if (!responseBuffer) {
        std::cerr << "Failed to create response buffer" << std::endl;
        evhttp_send_error(req, HTTP_INTERNAL, "Internal Server Error");
        return;
    }

    // Call ConduitsCollection to get list of Conduits
    ConduitsCollection& conduitsCollection = ConduitsCollection::getInstance();
    std::vector<ConduitParser::Conduit> conduits = conduitsCollection.getConduits();

    // Generate the JSON for the list of Conduits
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();
    writer.Key("conduits");
    writer.StartArray();

    for (size_t i = 0; i < conduits.size(); i++) {
        const ConduitParser::Conduit& conduit = conduits[i];

        writer.StartObject();
        writer.Key("name");
        writer.String(conduit.name.c_str());

        writer.Key("fileDescriptors");
        writer.StartArray();
        for (size_t j = 0; j < conduit.fileDescriptors.size(); j++) {
            int fd = conduit.fileDescriptors[j];
            writer.Int(fd);
        }
        writer.EndArray();

        writer.EndObject();
    }

    writer.EndArray();
    writer.EndObject();

    std::string jsonData = buffer.GetString();

    // print json
    // std::cout << "JSON Data: " << jsonData << std::endl;

    evbuffer_add(responseBuffer, jsonData.c_str(), jsonData.size());

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evhttp_send_reply(req, HTTP_OK, "OK", responseBuffer);
    evbuffer_free(responseBuffer);
}

void Server::httpCallback(struct evhttp_request* req, void* arg) {
    // Extract the URI from the request
    const char* uri = evhttp_request_get_uri(req);
    // std::cout << "Requested URI: " << uri << std::endl;

    // Parse the URI
    struct evhttp_uri* parsed_uri = evhttp_uri_parse(uri);
    if (!parsed_uri) {
        std::cerr << "Failed to parse URI" << std::endl;
        evhttp_send_error(req, HTTP_BADREQUEST, 0);
        return;
    }

    // Extract components of the URI
    const char* path = evhttp_uri_get_path(parsed_uri);
    const char* query = evhttp_uri_get_query(parsed_uri);

    // std::cout << "Path: " << (path ? path : "") << std::endl;
    // std::cout << "Query: " << (query ? query : "") << std::endl;

    // Handle different paths
    if (path && strcmp(path, "/") == 0) {
        handleRootRequest(req);
    } else if (path && strcmp(path, "/data") == 0) {
        handleDataRequest(req);
    } else {
        evhttp_send_error(req, HTTP_NOTFOUND, 0);
    }
}
