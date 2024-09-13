
#include "compress.h"

#include <cstring>
#include <lz4.h>

std::string CompressionUtility::compressZlib(const char* data, const size_t& data_length) {
    // Estimate the maximum size of the compressed data
    uLong compressedSize = compressBound(data_length);
    
    // Allocate memory for the compressed data
    std::string compressedData(compressedSize, '\0');

    // Compress the data
    int result = compress2(
        reinterpret_cast<Bytef*>(&compressedData[0]),     // Pointer to the destination buffer
        &compressedSize,                                  // Size of the destination buffer
        reinterpret_cast<const Bytef*>(data),             // Pointer to the source buffer
        data_length,                                      // Size of the source buffer
        Z_BEST_COMPRESSION                                // Compression level
    );

    if (result != Z_OK) {
        // Compression failed
        return "";
    }

    // Resize the compressed data string to the actual compressed size
    compressedData.resize(compressedSize);

    return compressedData;
}


std::string CompressionUtility::compress(const std::string& input) {
    return CompressionUtility::compressLZ4(input);
}

std::string CompressionUtility::decompress(const std::string& input) {
    return CompressionUtility::decompressLZ4(input);
}

std::string CompressionUtility::decompressZlib(const char* buffer, const size_t& buffer_length) {
    // Initial guess for decompressed data size (you might need to adjust this size)
    size_t initial_size = 1024;  // Starting size, can be adjusted based on typical use cases
    std::string decompressedData(initial_size, '\0');

    uLongf decompressed_size = initial_size;
    uLongf used_compressed_size = buffer_length;
    int result;

    // Attempt decompression
    while ((result = uncompress2(
        reinterpret_cast<Bytef*>(&decompressedData[0]),  // Destination buffer
        &decompressed_size,                              // Size of the decompressed buffer (output size)
        reinterpret_cast<const Bytef*>(buffer),          // Source buffer (compressed data)
        &used_compressed_size                           // Size of the compressed buffer (input size)
    )) == Z_BUF_ERROR) {
        // If buffer is not sufficient, resize and try again
        decompressedData.resize(decompressed_size);
    }

    // Check for errors
    if (result != Z_OK) {
        return "";
    }

    // Resize to the actual decompressed size
    decompressedData.resize(decompressed_size);

    return decompressedData;
}

#include <iostream>
#include <vector>

std::string CompressionUtility::compressLZ4(const std::string& input) {
// Estimate the maximum size of the compressed data
    int maxCompressedSize = LZ4_compressBound(input.size());

    // Allocate memory for the compressed data
    std::vector<char> compressed(maxCompressedSize);

    // Compress the data
    int compressedSize = LZ4_compress_default(input.c_str(), compressed.data(), input.size(), maxCompressedSize);

    if (compressedSize <= 0) {
        // Compression failed
        std::cerr << "Compression failed" << std::endl;
        return "";
    }

    // Resize the vector to the actual compressed size
    compressed.resize(compressedSize);

    return std::string(compressed.begin(), compressed.end());
}

std::string CompressionUtility::decompressLZ4(const std::string& input) {
    // Allocate memory for the decompressed data
    std::string decompressedData(input.size() * 10, '\0');

    // Decompress the data
    int decompressedSize = LZ4_decompress_safe(input.c_str(), &decompressedData[0], input.size(), decompressedData.size());

    if (decompressedSize <= 0) {
        // Decompression failed, try with a larger buffer
        decompressedData.resize(input.size() * 20, '\0');
        decompressedSize = LZ4_decompress_safe(input.c_str(), &decompressedData[0], input.size(), decompressedData.size());

        if (decompressedSize <= 0) {
            // Decompression still failed
            return "";
        }
    }

    // Resize the decompressed data string to the actual decompressed size
    decompressedData.resize(decompressedSize);

    return decompressedData;
}
