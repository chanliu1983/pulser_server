#include <iostream>
#include <string>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include "executor.h" // Include the executor.h file
#include "conduits.h" // Include the conduits.h file

// Constructor for the Executor class
Executor::Executor(MulticastHandler* multicastHandler) : multicastHandler_(multicastHandler) {}

void Executor::processJsonCommand(const int& fd, const std::string& jsonCommand) {
    rapidjson::Document document;
    document.Parse(jsonCommand.c_str());

    if (document.HasParseError()) {
        std::cout << "Error parsing JSON: " << rapidjson::GetParseError_En(document.GetParseError()) << std::endl;
        return;
    }

    if (!document.IsObject()) {
        std::cout << "Invalid JSON format. Expected an object." << std::endl;
        return;
    }

    if (!document.HasMember("action") || !document.HasMember("message") || !document.HasMember("timestamp")) {
        std::cout << "Invalid JSON format. Missing 'action', 'message', or 'timestamp' field." << std::endl;
        return;
    }

    std::string action = document["action"].GetString();
    std::string msg = document["message"].GetString();
    std::string timestamp = document["timestamp"].GetString();

    // print time stamp
    std::cout << "Received timestamp: " << timestamp << std::endl;

    if (action == "connect") {
        connect(fd, msg);
    } else if (action == "disconnect") {
        disconnect(fd, msg);
    } else if (action == "send") {
        // check if target field is present
        if (!document.HasMember("target")) {
            std::cout << "Invalid JSON format. Missing 'target' field." << std::endl;
            return;
        }
        
        std::string target = document["target"].GetString();
        send(fd, msg, target);
    } else {
        std::cout << "Invalid action: " << action << std::endl;
    }
}

void Executor::connect(const int& fd, const std::string& value) {
    // Connect implementation
    std::cout << "Connecting: " << value << std::endl;
    ConduitsCollection::getInstance().addFd(value, fd);
}

void Executor::disconnect(const int& fd, const std::string& value) {
    // Disconnect implementation
    std::cout << "Disconnecting: " << value << std::endl;
    ConduitsCollection::getInstance().deleteFd(value, fd);
}

#include "sender.h"

void Executor::send(const int& fd, const std::string& value, const std::string& target) {
    // Send implementation
    std::cout << "Sending: " << value << std::endl;

    if (!ConduitsCollection::getInstance().isFdInConduit(target, fd)) {
        std::cerr << "Error: File descriptor not in conduit" << std::endl;
        return;
    }

    const ConduitParser::Conduit& conduit = ConduitsCollection::getInstance().getConduit(target);
    std::string responseStr = createPayloadJson(value, fd);

    for (int targetFd : conduit.fileDescriptors) {
        if (targetFd == fd) {
            continue; // Skip sending to oneself
        }
        // Send the message to each file descriptor
        std::cout << "Sending message to fd: " << targetFd << std::endl;
        SenderUtility::sendRawPayload(targetFd, responseStr);
    }

    SenderUtility::broadcastRawPayload(responseStr, multicastHandler_);
}

std::string Executor::createPayloadJson(const std::string &value, const int &fd)
{
    // Create a JSON object with "message", "source", and "timestamp" fields
    rapidjson::Document response;
    response.SetObject();

    rapidjson::Value messageValue;
    messageValue.SetString(value.c_str(), value.size(), response.GetAllocator());
    response.AddMember("message", messageValue, response.GetAllocator());

    rapidjson::Value sourceValue;
    sourceValue.SetInt(fd);
    response.AddMember("source", sourceValue, response.GetAllocator());

    rapidjson::Value timestampValue;
    std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string timestamp = std::ctime(&currentTime);

    timestampValue.SetString(timestamp.c_str(), timestamp.size() - 1, response.GetAllocator());
    response.AddMember("timestamp", timestampValue, response.GetAllocator());

    // Convert the JSON object to a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    response.Accept(writer);
    return buffer.GetString();
}
