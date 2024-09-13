#ifndef COMPRESS_H
#define COMPRESS_H

#include <string>
#include <cstring>
#include <zlib.h>

class CompressionUtility {
public:
    static std::string compress(const std::string& input);
    static std::string decompress(const std::string& input);

    static std::string compressZlib(const char* data, const size_t& data_length);
    static std::string decompressZlib(const char* buffer, const size_t& buffer_length);

    static std::string compressLZ4(const std::string& input);
    static std::string decompressLZ4(const std::string& input);
};

#endif // COMPRESS_H