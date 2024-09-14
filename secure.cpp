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
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    return true;
}

SSL_CTX* SSLManager::createContext() {
    ctx_ = SSL_CTX_new(TLS_server_method());
    if (!ctx_) {
        std::cerr << "Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        return nullptr;
    }

    // Set the minimum and maximum TLS versions
    SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);  // Set minimum version to TLS 1.2
    SSL_CTX_set_max_proto_version(ctx_, TLS1_3_VERSION);  // Set maximum version to TLS 1.3

    return ctx_;
}

bool SSLManager::configureContext(const std::string& certFile, const std::string& keyFile) {
    SSL_CTX_set_ecdh_auto(ctx_, 1);

    if (SSL_CTX_use_certificate_file(ctx_, certFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx_, keyFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    return true;
}

SSL* SSLManager::createSSL(int socket) {
    SSL* ssl = SSL_new(ctx_);
    SSL_set_fd(ssl, socket);
    return ssl;
}

void SSLManager::cleanup() {
    EVP_cleanup();
}

SSL_CTX* SSLManager::getSSLContext() const {
    return ctx_;
}
