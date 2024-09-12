#ifndef CONDUITS_H
#define CONDUITS_H

#include <iostream>
#include <string>
#include <vector>

class ConduitParser {
public:
    struct Conduit {
        std::string name;
        std::string property1;
        std::string property2;

        std::vector<int> fileDescriptors;
    };

    std::vector<Conduit> parseConduits(const std::string& jsonData);
};

#include <mutex>
#include <fstream>

class ConduitsCollection {
public:
    static ConduitsCollection& getInstance();

    const std::vector<ConduitParser::Conduit>& getConduits();
    const ConduitParser::Conduit& getConduit(const std::string& name);

    void deleteFd(int fd);
    void deleteFd(const std::string& name, int fd);
    void addFd(const std::string& name, int fd);

    bool isFdInConduit(const std::string& name, int fd);

private:
    void loadConfig(const std::string& filename);

private:
    ConduitsCollection() = default;
    ConduitsCollection(const ConduitsCollection&) = delete;
    ConduitsCollection& operator=(const ConduitsCollection&) = delete;

    void parseConduits(const std::string& jsonData);

    std::vector<ConduitParser::Conduit> conduits_;
    std::mutex mutex_;
};

#endif // CONDUITS_H


