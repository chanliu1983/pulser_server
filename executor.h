#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>
#include "multicast.h"

class Executor {
public:
    Executor(MulticastHandler* multicastHandler);
    void processJsonCommand(const int& fd, const std::string& jsonCommand);

    void setServerName(const std::string &serverName);

private:
    void connect(const int& fd, const std::string& value);
    void disconnect(const int& fd, const std::string& value);

    // send to target group except self fd
    void send(const int &fd, const std::string &value, const std::string &target);
    // send to only self fd
    void sendToSelf(const int &fd, const std::string &value);

    std::string createPayloadJson(const std::string &value, const int &fd, const std::string &target);
    std::string addProcessedToJson(const std::string &value);
    bool isProcessed(const std::string &value);

private:
    MulticastHandler *multicastHandler_;
    std::string serverName_;
};

#endif // EXECUTOR_H