#include <iostream> // Add missing include for std::cerr and std::endl
#include <cstring> // Add missing include for std::strerror
#include <unistd.h> // Add missing include for send and recv functions
#include <chrono> // Add missing include for std::chrono::milliseconds
#include <thread> // Add missing include for std::this_thread::sleep_for
#include <zlib.h> // Add missing include for crc32 function
#include <string> // Add missing include for std::string
#include <cerrno> // Add missing include for errno
#include <sys/socket.h> // Add missing include for socket functions
#include "compress.h" // Add missing include for CompressionUtility
#include "sender.h" //  Add Payload util

using Bytef = unsigned char; // Add missing typedef for Bytef

// Rest of the code remains unchanged

struct DataFrame {
    uint32_t magic; // Magic number to identify the payload
    uint32_t size; // Size of the payload
    uint32_t checksum; // Checksum of the payload
    char payload[0]; // Payload data

    // Constructor to initialize the struct
    DataFrame(uint32_t payloadSize, const char* payloadData) {
        magic = 0x12344321; // Set the magic number
        size = payloadSize;
        checksum = calculateChecksum(payloadData, payloadSize);
        // memcpy(payload, payloadData, payloadSize);
    }

    static uint32_t calculateChecksum(const char* data, uint32_t dataSize) {
        return crc32(0L, reinterpret_cast<const Bytef*>(data), dataSize);
    }
};

void SenderUtility::sendRawPayload(int fd, const std::string& source) {
    std::string raw = CompressionUtility::compress(source);
    // Create a DataFrame object with the payload data
    DataFrame dataFrame(raw.size(), raw.c_str());

    // Send the magic
    if (send(fd, reinterpret_cast<char*>(&dataFrame.magic), sizeof(dataFrame.magic), 0) != sizeof(dataFrame.magic)) {
        std::cerr << "Error sending magic: " << strerror(errno) << std::endl;
        return;
    }

    // Send the payload size
    if (send(fd, reinterpret_cast<char*>(&dataFrame.size), sizeof(dataFrame.size), 0) != sizeof(dataFrame.size)) {
        std::cerr << "Error sending payload size: " << strerror(errno) << std::endl;
        return;
    }

    // Send the checksum
    if (send(fd, reinterpret_cast<char*>(&dataFrame.checksum), sizeof(dataFrame.checksum), 0) != sizeof(dataFrame.checksum)) {
        std::cerr << "Error sending checksum: " << strerror(errno) << std::endl;
        return;
    }

    // Send the payload data
    int totalBytesSent = 0;
    while (totalBytesSent < dataFrame.size) {
        int bytesSent = send(fd, raw.c_str() + totalBytesSent, dataFrame.size - totalBytesSent, 0);
        if (bytesSent <= 0) {
            std::cerr << "Error sending payload data: " << strerror(errno) << std::endl;
            return;
        }
        totalBytesSent += bytesSent;
    }

    std::cout << "Payload sent successfully" << std::endl;
}

void SenderUtility::sendRawPayloadAndBroadcast(int fd, const std::string& source, MulticastHandler* multicastHandler) {
    sendRawPayload(fd, source); // Send the payload to the specified file descriptor

    // Broadcast the payload to the multicast group
    multicastHandler->sendMessage(source);
}

std::string SenderUtility::recvRawPayload(int fd) {
    // Receive the magic
    uint32_t magic;
    if (recv(fd, reinterpret_cast<char*>(&magic), sizeof(magic), 0) != sizeof(magic)) {
        std::cerr << "Error receiving magic: " << strerror(errno) << std::endl;
        return "";
    }

    // print magic as 0x hex
    std::cout << "Received Magic: 0x" << std::hex << magic << std::endl;

    // Receive the payload size
    uint32_t payloadSize;
    if (recv(fd, reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize), 0) != sizeof(payloadSize)) {
        std::cerr << "Error receiving payload size: " << strerror(errno) << std::endl;
        return "";
    }

    // Receive the checksum
    uint32_t checksum;
    if (recv(fd, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0) != sizeof(checksum)) {
        std::cerr << "Error receiving checksum: " << strerror(errno) << std::endl;
        return "";
    }

    // Check if the received magic is valid
    if (magic != 0x12344321) {
        std::cerr << "Invalid magic" << std::endl;
        return "";
    }

    // Allocate memory for the payload
    char* payload = new char[payloadSize];

    // Receive the payload data
    int totalBytesRead = 0;
    int eagainCount = 0;
    while (totalBytesRead < payloadSize) {
        int bytesRead = recv(fd, payload + totalBytesRead, payloadSize - totalBytesRead, 0);
        if (bytesRead <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available (non-blocking mode), try again after 500 ms
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                eagainCount++;
                if (eagainCount > 3) {
                    std::cerr << "Error receiving payload data: EAGAIN exceeded" << std::endl;
                    delete[] payload;
                    return "";
                }
                continue;
            } else {
                std::cerr << "Error receiving payload data: " << strerror(errno) << std::endl;
                delete[] payload;
                return "";
            }
        }
        totalBytesRead += bytesRead;
    }

    // Calculate the checksum of the received payload
    uint32_t calculatedChecksum = DataFrame::calculateChecksum(payload, payloadSize);
    // Check if the received checksum is valid
    if (checksum != calculatedChecksum) {
        std::cerr << "Invalid checksum" << std::endl;
        delete[] payload;
        return "";
    }

#ifdef _DEBUG
    for (int i = 0; i < payloadSize; i++) {
        std::cout << static_cast<int>(payload[i]);
        if (i != payloadSize - 1) {
            std::cout << "";
        }
    }
    std::cout << std::endl;
#endif

    // print size of payloadSize
    std::cout << "Received Payload Size: " << payloadSize << std::endl;

    // Process the received payload
    std::string receivedData = CompressionUtility::decompress(std::string(payload, payloadSize));
    std::cout << "Received Payload: " << receivedData << std::endl;

    // Clean up
    delete[] payload;
    payload = nullptr;

    return receivedData;
}

