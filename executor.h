#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>

class Executor {
public:
    void processJsonCommand(const int& fd, const std::string& jsonCommand);
private:
    void connect(const int& fd, const std::string& value);
    void disconnect(const int& fd, const std::string& value);
    void send(const int& fd, const std::string& value, const std::string& target);
};

#endif // EXECUTOR_H