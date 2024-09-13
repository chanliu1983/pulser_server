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
#include "db.h" // Include the db.h file

// Constructor for the Executor class
Executor::Executor(MulticastHandler* multicastHandler) : multicastHandler_(multicastHandler) {}

void Executor::setServerName(const std::string &serverName)
{
    serverName_ = serverName;
}

void Executor::processJsonCommand(const int& fd, const std::string& jsonCommand) {
    static Database db("data.db");

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

    if (!document.HasMember("message") || !document.HasMember("timestamp")) {
        std::cout << "Invalid JSON format. Missing 'message' or 'timestamp' field." << std::endl;
        return;
    }

    std::string action;    

    if (fd == -1) {
        action = "send";
    } else {
        if (!document.HasMember("action")) {
            std::cout << "Invalid JSON format. Missing 'action' field." << std::endl;
            return;
        }
        action = document["action"].GetString();
    }

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
    } else if (action == "store") {
        // Store the message in the database
        if (!document.HasMember("key")) {
            std::cout << "Invalid JSON format. Missing 'key' field." << std::endl;
            return;
        }

        std::string key = document["key"].GetString();
        db.Put(key, msg);
    } else if (action == "retrieve") {
        // Retrieve the message from the database
        if (!document.HasMember("key")) {
            std::cout << "Invalid JSON format. Missing 'key' field." << std::endl;
            return;
        }

        if (!document.HasMember("target")) {
            std::cout << "Invalid JSON format. Missing 'target' field." << std::endl;
            return;
        }

        std::string key = document["key"].GetString();
        std::string target = document["target"].GetString();
        bool isSingleRetrieval = document.HasMember("single") && document["single"].GetBool();
        std::string value = db.Get(key);
        std::cout << "Retrieved value: " << value << std::endl;

        // Create a JSON object with "serverName" and "value" fields
        rapidjson::Document response;
        response.SetObject();

        rapidjson::Value serverNameValue;
        serverNameValue.SetString(serverName_.c_str(), serverName_.size(), response.GetAllocator());
        response.AddMember("server", serverNameValue, response.GetAllocator());

        rapidjson::Value valueValue;
        valueValue.SetString(value.c_str(), value.size(), response.GetAllocator());
        response.AddMember("value", valueValue, response.GetAllocator());

        // Convert the JSON object to a string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        response.Accept(writer);
        std::string jsonStr = buffer.GetString();

        if (isSingleRetrieval) {
            sendToSelf(fd, jsonStr);
        } else {
            send(-1, jsonStr, target);
        }
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

void Executor::sendToSelf(const int& fd, const std::string& value) {
    // Send to self implementation
    std::string responseStr = createPayloadJson(value, fd, "");

    std::cout << "Sending to self: " << responseStr << std::endl;
    SenderUtility::sendRawPayload(fd, responseStr);
}

void Executor::send(const int& fd, const std::string& value, const std::string& target) {
    // Send implementation
    std::cout << "Sending: " << value << std::endl;

    if (fd != -1 && !ConduitsCollection::getInstance().isFdInConduit(target, fd)) {
        std::cerr << "Error: File descriptor not in conduit" << std::endl;
        return;
    }

    const ConduitParser::Conduit& conduit = ConduitsCollection::getInstance().getConduit(target);
    std::string responseStr = createPayloadJson(value, fd, target);

    for (int targetFd : conduit.fileDescriptors) {
        if (targetFd == fd) {
            continue; // Skip sending to oneself
        }
        // Send the message to each file descriptor
        std::cout << "Sending message to fd: " << targetFd << std::endl;
        SenderUtility::sendRawPayload(targetFd, responseStr);
    }

    if (fd == -1 || isProcessed(responseStr)) {
        return;
    }

    responseStr = addProcessedToJson(responseStr);
    SenderUtility::broadcastRawPayload(responseStr, multicastHandler_);
}

std::string Executor::createPayloadJson(const std::string &value, const int &fd, const std::string &target)
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

    rapidjson::Value targetValue;
    targetValue.SetString(target.c_str(), target.size(), response.GetAllocator());
    response.AddMember("target", targetValue, response.GetAllocator());

    // Convert the JSON object to a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    response.Accept(writer);
    return buffer.GetString();
}

std::string Executor::addProcessedToJson(const std::string &value)
{
    // Parse the JSON string
    rapidjson::Document document;
    document.Parse(value.c_str());

    // Check if the JSON is valid
    if (document.HasParseError()) {
        std::cerr << "Error parsing JSON: " << rapidjson::GetParseError_En(document.GetParseError()) << std::endl;
        return "";
    }

    // Check if the JSON is an object
    if (!document.IsObject()) {
        std::cerr << "Invalid JSON format. Expected an object." << std::endl;
        return "";
    }

    // Add a new field to the JSON object
    rapidjson::Value processedValue;
    processedValue.SetBool(true);
    document.AddMember("processed", processedValue, document.GetAllocator());

    // Convert the JSON object to a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return buffer.GetString();
}

bool Executor::isProcessed(const std::string &value)
{
    // Parse the JSON string
    rapidjson::Document document;
    document.Parse(value.c_str());

    // Check if the JSON is valid
    if (document.HasParseError()) {
        std::cerr << "Error parsing JSON: " << rapidjson::GetParseError_En(document.GetParseError()) << std::endl;
        return false;
    }

    // Check if the JSON is an object
    if (!document.IsObject()) {
        std::cerr << "Invalid JSON format. Expected an object." << std::endl;
        return false;
    }

    // Check if the "processed" field is present
    if (!document.HasMember("processed")) {
        return false;
    }

    // Check if the "processed" field is a boolean
    if (!document["processed"].IsBool()) {
        return false;
    }

    // Get the value of the "processed" field
    return document["processed"].GetBool();
}
