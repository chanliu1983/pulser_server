#include "server.h"
#include "conduits.h" // For ConduitsCollection, Conduit
#include <iostream>  // For std::cerr and std::endl
#include <stdexcept> // For std::runtime_error
#include <cstring>   // For strlen
#include <fstream>   // For std::ifstream
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
    // Load the index.html file from disk
    std::ifstream file("index.html");
    if (!file.is_open()) {
        std::cerr << "Failed to open index.html" << std::endl;
        evhttp_send_error(req, HTTP_NOTFOUND, "Not Found");
        return;
    }

    std::string html((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Call ConduitsCollection to get list of Conduits
    ConduitsCollection& conduitsCollection = ConduitsCollection::getInstance();
    std::vector<ConduitParser::Conduit> conduits = conduitsCollection.getConduits();

    // Generate the HTML for the list of Conduits
    std::string conduitListHtml = "<table>";
    conduitListHtml += "<tr><th>Conduit Name</th><th>File Descriptors</th></tr>";
    for (const ConduitParser::Conduit& conduit : conduits) {
        conduitListHtml += "<tr>";
        conduitListHtml += "<td>" + conduit.name + "</td>";
        conduitListHtml += "<td>";
        for (const int fd : conduit.fileDescriptors) {
            conduitListHtml += std::to_string(fd) + ", ";
        }
        conduitListHtml += "</td>";
        conduitListHtml += "</tr>";
    }
    conduitListHtml += "</table>";

    // Insert the conduitListHtml into the HTML
    size_t pos = html.find("</body>"); // Or some other suitable place
    if (pos != std::string::npos) {
        html.insert(pos, conduitListHtml);
    }

    // Add CSS style for the table
    std::string cssStyle = "<style>";
    cssStyle += "table {";
    cssStyle += "    border-collapse: collapse;";
    cssStyle += "    width: 100%;";
    cssStyle += "}";
    cssStyle += "th, td {";
    cssStyle += "    text-align: left;";
    cssStyle += "    padding: 8px;";
    cssStyle += "}";
    cssStyle += "th {";
    cssStyle += "    background-color: #4CAF50;";
    cssStyle += "    color: white;";
    cssStyle += "}";
    cssStyle += "tr:nth-child(even) {";
    cssStyle += "    background-color: #f2f2f2;";
    cssStyle += "}";
    cssStyle += "</style>";

    pos = html.find("<head>"); // Find the position to insert the CSS style
    if (pos != std::string::npos) {
        html.insert(pos + 6, cssStyle);
    }

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
