#include <algorithm>
#include "conduits.h"
#include "rapidjson/document.h"

std::vector<ConduitParser::Conduit> ConduitParser::parseConduits(const std::string& jsonData) {
    std::vector<ConduitParser::Conduit> conduits;

    rapidjson::Document root;
    root.Parse(jsonData.c_str());

    if (!root.HasParseError()) {
        const rapidjson::Value& conduitArray = root["conduits"];
        for (const auto& conduit : conduitArray.GetArray()) {
            ConduitParser::Conduit parsedConduit;
            parsedConduit.name = conduit["name"].GetString();
            parsedConduit.property1 = conduit["config"]["property1"].GetString();
            parsedConduit.property2 = conduit["config"]["property2"].GetString();
            conduits.push_back(parsedConduit);
        }
    } else {
        std::cerr << "Failed to parse JSON data" << std::endl;
    }

    return conduits;
}

void ConduitsCollection::deleteFd(int fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& conduit : conduits_) {
        conduit.fileDescriptors.erase(std::remove(conduit.fileDescriptors.begin(), conduit.fileDescriptors.end(), fd), conduit.fileDescriptors.end());
    }
}

void ConduitsCollection::addFd(const std::string& name, int fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto conduit = std::find_if(conduits_.begin(), conduits_.end(), [&name](const ConduitParser::Conduit& conduit) {
        return conduit.name == name;
    });

    if (conduit != conduits_.end()) {
        // Check if the file descriptor already exists in the conduit
        if (std::find(conduit->fileDescriptors.begin(), conduit->fileDescriptors.end(), fd) == conduit->fileDescriptors.end()) {
            conduit->fileDescriptors.push_back(fd);
        }
    }
}

bool ConduitsCollection::isFdInConduit(const std::string& name, int fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto conduit = std::find_if(conduits_.begin(), conduits_.end(), [&name](const ConduitParser::Conduit& conduit) {
        return conduit.name == name;
    });

    if (conduit != conduits_.end()) {
        return std::find(conduit->fileDescriptors.begin(), conduit->fileDescriptors.end(), fd) != conduit->fileDescriptors.end();
    }

    return false;
}

void ConduitsCollection::deleteFd(const std::string& name, int fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto conduit = std::find_if(conduits_.begin(), conduits_.end(), [&name](const ConduitParser::Conduit& conduit) {
        return conduit.name == name;
    });

    if (conduit != conduits_.end()) {
        conduit->fileDescriptors.erase(std::remove(conduit->fileDescriptors.begin(), conduit->fileDescriptors.end(), fd), conduit->fileDescriptors.end());
    }
}

ConduitsCollection& ConduitsCollection::getInstance() {
    static ConduitsCollection instance;
    static bool initialized = false;
    if (!initialized) {
        instance.loadConfig("config.json"); // Call loadConfig with "config.json" as argument
        initialized = true;
    }
    return instance;
}

const std::vector<ConduitParser::Conduit>& ConduitsCollection::getConduits() {
    std::lock_guard<std::mutex> lock(mutex_);
    return conduits_;
}

const ConduitParser::Conduit& ConduitsCollection::getConduit(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto conduit = std::find_if(conduits_.begin(), conduits_.end(), [&name](const ConduitParser::Conduit& conduit) {
        return conduit.name == name;
    });

    if (conduit != conduits_.end()) {
        return *conduit;
    } else {
        throw std::runtime_error("Conduit not found: " + name);
    }
}

void ConduitsCollection::loadConfig(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream file(filename);
    if (file.is_open()) {
        std::string jsonData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        parseConduits(jsonData);
    } else {
        throw std::runtime_error("Failed to open file: " + filename);
    }
}

void ConduitsCollection::parseConduits(const std::string& jsonData) {
    ConduitParser parser;
    conduits_ = parser.parseConduits(jsonData);
}