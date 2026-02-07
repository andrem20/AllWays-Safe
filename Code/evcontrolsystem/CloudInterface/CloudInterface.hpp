#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct WifiNetwork {
    std::string ssid;
    std::string password;
};

class CloudInterface {

private:
    /*---Attributes---------------------------------------------------------------------------------------------------*/
    std::string cloudURL;
    std::string wpaConfigFile_;

    /*---Helper methods-----------------------------------------------------------------------------------------------*/
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    [[nodiscard]] std::string makeRequest(const std::string& endpoint) const;
    [[nodiscard]] nlohmann::json querySSID(const std::string& table, const std::string& field, const std::string& value) const;
    [[nodiscard]] std::string getTableID(const std::string& table, const std::string& identifier) const;

public:
    /*---Constructor/Destructor---------------------------------------------------------------------------------------*/
    explicit CloudInterface(const std::string& url);
    ~CloudInterface();

    /*---System Handling----------------------------------------------------------------------------------------------*/
    void cloudConnect() const;
    [[nodiscard]] std::vector<WifiNetwork> getWifiNetworksByTMC(const std::string& tmcName) const;
    void clearNetworks() const;
    void addNetwork(const WifiNetwork& net) const;
    void reinitializeWifiNetworks() const;

};
