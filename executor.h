#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>
#include "multicast.h"

class Executor {
public:
    Executor(MulticastHandler* multicastHandler);
    void processJsonCommand(const int& fd, const std::string& jsonCommand);

private:
    void connect(const int& fd, const std::string& value);
    void disconnect(const int& fd, const std::string& value);
    void send(const int &fd, const std::string &value, const std::string &target);

    std::string createPayloadJson(const std::string &value, const int &fd);

private:
    MulticastHandler *multicastHandler_;
};

#endif // EXECUTOR_H