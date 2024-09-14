
#include "setting.h"

#include "inih/ini.h" // Include the INI parser header

// Constructor
PulserConfig::PulserConfig(const std::string& filename) {
    if (!loadConfig(filename)) {
        std::cerr << "Failed to load configuration from " << filename << std::endl;
    }
}

// Accessors
std::string PulserConfig::getClusterMultiCastIP() const {
    return clusterMultiCastIP;
}

int PulserConfig::getClusterMultiCastPort() const {
    return clusterMultiCastPort;
}

int PulserConfig::getManagementEndPointPort() const {
    return managementEndPointPort;
}

int PulserConfig::getConduitEndpointPort() const {
    return conduitEndpointPort;
}

std::string PulserConfig::getServerName() const {
    return serverName;
}

std::string PulserConfig::getCertFile() const {
    return certFile;
}

std::string PulserConfig::getKeyFile() const {
    return keyFile;
}

// INI handler function
int PulserConfig::ini_handler(void* user, const char* section, const char* name, const char* value) {
    PulserConfig* config = static_cast<PulserConfig*>(user);
    if (config == nullptr) {
        return 0;
    }

    if (section == nullptr) {
        return 0;
    }

    if (std::string(section) == "Pulser") {
        if (std::string(name) == "ClusterMultiCastIP") {
            config->clusterMultiCastIP = value;
        } else if (std::string(name) == "ClusterMultiCastPort") {
            config->clusterMultiCastPort = std::stoi(value);
        } else if (std::string(name) == "ManagementEndPointPort") {
            config->managementEndPointPort = std::stoi(value);
        } else if (std::string(name) == "ConduitEndpointPort") {
            config->conduitEndpointPort = std::stoi(value);
        } else if (std::string(name) == "PulserServerName") {
            config->serverName = value;
        } else if (std::string(name) == "CertFile") {
            config->certFile = value;
        } else if (std::string(name) == "KeyFile") {
            config->keyFile = value;
        }
    }

    return 1;
}

bool PulserConfig::loadConfig(const std::string& filename) {
    if (ini_parse(filename.c_str(), ini_handler, this) < 0) {
        std::cerr << "Failed to parse INI file: " << filename << std::endl;
        return false;
    }

    return true;
}