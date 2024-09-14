#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "secure.h"

SSLManager::SSLManager() : ctx_(nullptr) {}

SSLManager::~SSLManager() {
    if (ctx_) {
        SSL_CTX_free(ctx_);
    }
    cleanup();
}

bool SSLManager::initialize() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    return true;
}

SSL_CTX* SSLManager::createContext() {
    const SSL_METHOD* method = SSLv23_client_method();
    ctx_ = SSL_CTX_new(method);
    if (!ctx_) {
        std::cerr << "Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        return nullptr;
    }
    return ctx_;
}

bool SSLManager::configureContext(SSL_CTX* ctx, const std::string& certFile, const std::string& keyFile) {
    SSL_CTX_set_ecdh_auto(ctx, 1);

    if (SSL_CTX_use_certificate_file(ctx, certFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, keyFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    return true;
}

SSL* SSLManager::createSSL(SSL_CTX* ctx, int socket) {
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, socket);
    return ssl;
}

void SSLManager::cleanup() {
    EVP_cleanup();
}
