#include "CloudInterface.hpp"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

/*---Constructor/Destructor---------------------------------------------------------------------------------------*/
CloudInterface::CloudInterface(const std::string& url)
    : cloudURL(url),
      wpaConfigFile_("/etc/wpa_supplicant.conf")
{}

CloudInterface::~CloudInterface() = default;

/*---System Handling----------------------------------------------------------------------------------------------*/
void CloudInterface::cloudConnect() const
{
    CURL* curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("curl_easy_init() failed");

    curl_easy_setopt(curl, CURLOPT_URL, cloudURL.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error(std::string("Cloud unavailable: ") + curl_easy_strerror(res));

    std::cout << "Cloud connected successfully!\n";
}

std::vector<WifiNetwork> CloudInterface::getWifiNetworksByTMC(const std::string &tmcName) const
{
    std::vector<WifiNetwork> nets;

    std::string tmcid = getTableID("tmc", tmcName);
    if (tmcid.empty())
        return nets;

    nlohmann::json result = querySSID("controlbox", "tmcid", tmcid);
    if (!result.is_array())
        return nets;

    for (const auto& item : result) {
        if (!item.is_object()) continue;
        if (!item.contains("ssid") || !item.contains("ssid_password")) continue;

        try {
            std::string ssidStr = item.at("ssid").get<std::string>();
            std::string passwordStr = item.at("ssid_password").get<std::string>();
            if (!ssidStr.empty())
                nets.push_back({ssidStr, passwordStr});
        } catch (const nlohmann::json::type_error&) {
            continue;
        }
    }

    return nets;
}

void CloudInterface::clearNetworks() const
{
    std::ifstream inFile(wpaConfigFile_);
    if (!inFile.is_open())
        throw std::runtime_error("failed to open wifi configuration file for reading");

    std::vector<std::string> outputLines;
    std::string line;
    bool inNetworkBlock = false;

    while (std::getline(inFile, line)) {
        if (line.find("network={") != std::string::npos) {
            inNetworkBlock = true;
            continue;
        }
        if (inNetworkBlock) {
            if (line.find('}') != std::string::npos) inNetworkBlock = false;
            continue;
        }
        if (!line.empty()) outputLines.push_back(line);
    }

    inFile.close();

    std::ofstream outFile(wpaConfigFile_, std::ios::trunc);
    if (!outFile.is_open())
        throw std::runtime_error("failed to open wifi configuration file for writing");

    for (const auto& l : outputLines)
        outFile << l << "\n";
}

void CloudInterface::addNetwork(const WifiNetwork& net) const
{
    if (net.ssid.empty() || net.password.empty()) return;

    std::ofstream outFile(wpaConfigFile_, std::ios::app);
    if (!outFile.is_open())
        throw std::runtime_error("failed to open wifi configuration file for writing");

    outFile << "\nnetwork={\n";
    outFile << "  ssid=\"" << net.ssid << "\"\n";
    outFile << "  psk=\"" << net.password << "\"\n";
    outFile << "  key_mgmt=WPA-PSK\n";
    outFile << "}\n";
}

void CloudInterface::reinitializeWifiNetworks() const
{
    int ret = system("wpa_cli -i wlan0 reconfigure");
    if (ret != 0)
        std::cerr << "Warning: wpa_cli reconfigure failed\n";
}

/*---Helper methods-----------------------------------------------------------------------------------------------*/
size_t CloudInterface::writeCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

std::string CloudInterface::makeRequest(const std::string& endpoint) const
{
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init() failed");

    std::string response;
    std::string fullUrl = cloudURL + endpoint;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error(std::string("curl failed: ") + curl_easy_strerror(res));
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code >= 400)
        throw std::runtime_error("HTTP error " + std::to_string(http_code));

    return response;
}

nlohmann::json CloudInterface::querySSID(const std::string& table, const std::string& field, const std::string& value) const
{
    std::string response = makeRequest("/data/" + table + "/" + field + "/" + value);
    try {
        return nlohmann::json::parse(response);
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return {};
    }
}

std::string CloudInterface::getTableID(const std::string& table, const std::string& identifier) const
{
    std::string response = makeRequest("/id/" + table + "/" + identifier);
    if (response.empty()) {
        std::cerr << "getTableID: Empty response\n";
        return "";
    }

    try {
        nlohmann::json r = nlohmann::json::parse(response);
        if (r.contains("error")) {
            std::cerr << "getTableID: Server error - " << r["detail"].get<std::string>() << "\n";
            return "";
        }
        if (!r.contains("id")) {
            std::cerr << "getTableID: Response missing 'id' field\n";
            return "";
        }
        return r["id"].get<std::string>();
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return "";
    }
}
