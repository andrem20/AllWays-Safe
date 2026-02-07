#include "CloudInterface.hpp"
#include "../Messages/Components/Cloud/QueueReceiveCloudTypes.hpp"
#include "../Messages/EventsType.hpp"


#include <atomic>
#include <format>
#include <fstream>

#include <curl/curl.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <utility>

#define CONTROL_BOX_TABLE_ID "controlbox"
#define TMC_TABLE_ID "tmc"
#define PSEM_TABLE_ID "p_semaphore"
#define TSEM_TABLE_ID "t_semaphore"
#define PEDESTRIAN_TABLE_ID "pedestrian"

#define CONTROL_BOX_FK "controlboxid" // foreign key in table: this semaphore belongs to which control box?

/*---Constructor/Destructor---------------------------------------------------------------------------------------*/

CloudInterface::CloudInterface(std::string cloudURL, std::string controlBoxName, std::string tmcName, Mediator* mediator,
        std::atomic<bool>& _shutdown_request) :
    Component(mediator),
    cloudURL(std::move(cloudURL)),
    controlBoxName (std::move(controlBoxName )),
    tmcName(std::move(tmcName)),
    _shutdown_request(_shutdown_request),
    cloudThread(t_cloud)
{

}

/*---System Handling----------------------------------------------------------------------------------------------*/

/* GET JSON file from string path */
json load_json(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    json j;
    file >> j;  // parse JSON from file
    return j;
}

// INSTEAD OF CONNECT DO NOTIFY IN START FUNCTION   DEBUG
void CloudInterface::cloudNotify()
{
    std::string test_path = "/root/";
    json psem = load_json(test_path + "correct_PSEM.json");
    json tsem = load_json(test_path + "correct_TSEM.json");
    rx_cloud::TSEM_data ts_d={std::make_shared<json>(tsem)};
    rx_cloud::PSEM_data ps_d = {std::make_shared<json>(psem)};

    mediator->notify(this, Event{ CloudReceiveType{ ps_d } });
    mediator->notify(this, Event{ CloudReceiveType{ ts_d } });
}

/* ACTUAL API  */

void CloudInterface::cloudStart()
{
    cloudThread.run(this);
}

void CloudInterface::stop()
{
    cloudSendQueue.interrupt();
    cloudThread.join();
}

void CloudInterface::cloudConnect() const
{
    CURL* curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("CloudInterface::cloudConnect: curl_easy_init() failed");

    curl_easy_setopt(curl, CURLOPT_URL, cloudURL.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD request
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    const CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error("Cloud unavailable : curl_easy_perform() failed");

    std::cout << "Cloud connected successfully!\n";
}

nlohmann::json CloudInterface::query_database
            (const std::string& table,
            const std::string& field,
            const std::string& value) const
{
    std::string endpoint = "/data/" + table + "/" + field + "/" + value;
    std::string response = makeRequest(cloudURL, endpoint, "GET");
    return nlohmann::json::parse(response);
}

std::string CloudInterface::getTableID(const std::string& table, const std::string& identifier) const
{
    std::string endpoint = "/id/" + table + "/" + identifier;
    std::string response = makeRequest(cloudURL, endpoint, "GET");

    nlohmann::json result = nlohmann::json::parse(response);

    return result["id"].get<std::string>();
}

std::string CloudInterface::getTableID(const std::string& table, int identifier) const
{
    std::string endpoint = "/id/" + table + "/" + std::to_string(identifier);
    std::string response = makeRequest(cloudURL, endpoint, "GET");

    nlohmann::json result = nlohmann::json::parse(response);

    return result["id"].get<std::string>();
}

void CloudInterface::patch_database(
    const std::string& table,
    const std::string& identifierField,
    int identifierValue,
    const std::string& updateField,
    const int updateValue) const
{
    nlohmann::json body;
    body["identifierField"] = identifierField;
    body["identifierValue"] = identifierValue;
    body["updateField"] = updateField;
    body["updateValue"] = updateValue;

    std::string endpoint = "/data/" + table;
    std::string response = makeRequest(cloudURL, endpoint, "PATCH", body.dump()); // envia o JSON no corpo
}

void CloudInterface::post_psem_pedestrian(const std::string& psem_id,
                         const std::string& pedestrianCC_id) const
{
    nlohmann::json body;
    body["psem_id"] = psem_id;
    body["pedestrianCC_id"] = pedestrianCC_id;
    body["timestamp"] = get_iso8601_timestamp();

    makeRequest(
        cloudURL,
        "/data/p_semaphore_pedestrian",
        "POST",
        body.dump()
    );
}

void CloudInterface::post_emergency_vehicle
                            (const std::string& tmc_id,
                            const std::string& cb_id,
                            const std::string& licenseplate,
                            int origin,
                            int destination,
                            int priority) const
{
    nlohmann::json body;
    body["tmcid"] = tmc_id;
    body["controlbox_id"] = cb_id;
    body["licenseplate"] = licenseplate;
    body["origin"] = origin;
    body["destination"] = destination;
    body["priority_level"] = priority;
    body["timestamp"] = get_iso8601_timestamp();

    makeRequest(
        cloudURL,
        "/data/emergencyvehicle",
        "POST",
        body.dump()
    );
}

/*---Helper methods-----------------------------------------------------------------------------------------------*/

std::string CloudInterface::makeRequest(
    const std::string& cloudURL,
    const std::string& endpoint,
    const std::string& method,
    const std::string& body )
{
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to initialize CURL");

    std::string readBuffer;
    std::string url = cloudURL + endpoint;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    } else if (method == "PATCH") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    }

    const CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    }

    if (http_code >= 400) {
        throw std::runtime_error("HTTP error " + std::to_string(http_code) + ": " + readBuffer);
    }

    return readBuffer;
}

size_t CloudInterface::writeCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

std::string CloudInterface::get_iso8601_timestamp() {
    auto t = std::time(nullptr);
    auto tm = *std::gmtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

void CloudInterface::cloudSetUp()
{
    controlboxID  = getTableID(CONTROL_BOX_TABLE_ID, controlBoxName);
    tmcID = getTableID(TMC_TABLE_ID, "tmc1");
}

/*---Threading & Synchronization Resources------------------------------------------------------------------------*/
void* CloudInterface::t_cloud(void* arg)
{
    auto self = static_cast<CloudInterface*>(arg);
    
    self->cloudConnect();
    self->cloudSetUp();

    while (!self->_shutdown_request.load())
    {
        auto data = self->cloudSendQueue.receive();

        auto visitor = [&] (auto&& obj)
        {
            using T = std::decay_t<decltype(obj)>;

            if  constexpr (std::is_same_v<T, tx_cloud::Configure>)
            {
                json psem = self->query_database(obj.psem_table, CONTROL_BOX_FK, self->controlboxID);
                json tsem = self->query_database(obj.tsem_table, CONTROL_BOX_FK, self->controlboxID);

                rx_cloud::TSEM_data ts_d={std::make_shared<json>(tsem)};
                rx_cloud::PSEM_data ps_d = {std::make_shared<json>(psem)};

                self->mediator->notify(self, Event{ CloudReceiveType{ ps_d } });
                self->mediator->notify(self, Event{ CloudReceiveType{ ts_d } });
            }
            else if constexpr (std::is_same_v<T, tx_cloud::EmergencyContext>)
            {
                self->post_emergency_vehicle (self->tmcID, self->controlboxID, obj.EmVehicleID,
                    obj.Origin, obj.Destination, obj.priority);
            }
            else if constexpr (std::is_same_v<T, tx_cloud::ValidateRFID>)
            {
                std::cout<<"UUID: 0x" << std::hex << obj.uuid << "\n" << std::dec;

                std::string hex_str_prefix = std::format("{:#X}", obj.uuid);
                std::cout << hex_str_prefix <<std::endl;
                json result = self->query_database(PEDESTRIAN_TABLE_ID, "physicaltag_id", hex_str_prefix);
                if (result.value("found", false))
                {
                    std::cout << "Pedestrian Exists"<< std::endl;

                    // Register Pedestrian Passed in that Location
                    std::string psemID = self->getTableID(PSEM_TABLE_ID, obj.location);
                    std::string cc_id = self->getTableID(PEDESTRIAN_TABLE_ID,hex_str_prefix);
                    self->post_psem_pedestrian(psemID, cc_id);

                    // Notify Answer
                    self->mediator->notify(self, Event{rx_cloud::RFID_Validation{true, obj.location}});
                }
            }
            else if constexpr (std::is_same_v<T, tx_cloud::TrafficSemaphoreUpdate> ||
                std::is_same_v<T, tx_cloud::PedestrianSemaphoreUpdate>)
            {
                self->patch_database(obj.table, "location", obj.location,
                    "status", obj.status);
            }
        };
        std::visit(visitor, data);
    }
    return arg;
}