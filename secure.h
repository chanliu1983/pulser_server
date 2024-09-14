#ifndef SECURE_H
#define SECURE_H

#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>

class SSLManager {
public:
    SSLManager();
    ~SSLManager();

    bool initialize();
    SSL_CTX* createContext();
    bool configureContext(SSL_CTX* ctx, const std::string& certFile, const std::string& keyFile);
    SSL* createSSL(SSL_CTX* ctx, int socket);
    void cleanup();

private:
    SSL_CTX* ctx_;
};

#endif // SECURE_H