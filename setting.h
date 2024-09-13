#ifndef SETTING_H
#define SETTING_H

#include <iostream>
#include <string>

class PulserConfig {
public:
    // Constructor
    PulserConfig(const std::string& filename);

    // Accessors
    std::string getClusterMultiCastIP() const;
    int getClusterMultiCastPort() const;
    int getManagementEndPointPort() const;
    int getConduitEndpointPort() const;
    std::string getServerName() const;

private:
    std::string clusterMultiCastIP;
    int clusterMultiCastPort;
    int managementEndPointPort;
    int conduitEndpointPort;

    std::string serverName;

    static int ini_handler(void* user, const char* section, const char* name, const char* value);

    bool loadConfig(const std::string& filename);
};

#endif // SETTING_H