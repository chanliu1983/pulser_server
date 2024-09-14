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
    bool configureContext(const std::string& certFile, const std::string& keyFile);
    SSL* createSSL(int socket);
    void cleanup();

    SSL_CTX* getSSLContext() const;

private:
    SSL_CTX* ctx_;
};

#endif // SECURE_H