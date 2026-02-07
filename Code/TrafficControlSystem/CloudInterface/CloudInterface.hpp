#ifndef TRAFFICCONTROLSYSTEM_CLOUDINTERFACE_HPP
#define TRAFFICCONTROLSYSTEM_CLOUDINTERFACE_HPP

#include <string>
#include <nlohmann/json.hpp>

#include "../Mediator.hpp"
#include "../CppWrapper/CppWrapper.hpp"
#include "../Messages/Components/Cloud/QueueSendCloudTypes.hpp"

class CloudInterface: public Component
{
private:
    /*---Attributes---------------------------------------------------------------------------------------------------*/
    /* Identifiers specified on Constructor */
    std::string cloudURL;
    std::string controlBoxName; // raspMari.local
    std::string tmcName;

    /* Identifiers specified Internally */
    std::string controlboxID;
    std::string tmcID;

    std::atomic<bool>& _shutdown_request;

    /*---Helper methods-----------------------------------------------------------------------------------------------*/
    static std::string makeRequest(const std::string& cloudURL, const std::string& endpoint, const std::string& method,
        const std::string& body = "");
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static std::string get_iso8601_timestamp();

public:
    /*---Constructor/Destructor---------------------------------------------------------------------------------------*/
    CloudInterface(std::string cloudURL, std::string controlBoxName, std::string tmcName, Mediator* mediator,
        std::atomic<bool>& _shutdown_request);
    ~CloudInterface()override = default;

    /*---System Handling----------------------------------------------------------------------------------------------*/

    void cloudStart();
    void cloudSetUp();
    void stop();

    void cloudNotify(); // TEST METHOD

    void cloudConnect() const;
    /*GET*/
    [[nodiscard]] nlohmann::json query_database(const std::string& table, const std::string& field, const std::string& value) const;
    [[nodiscard]] std::string getTableID(const std::string& table, const std::string& identifier) const;
    [[nodiscard]] std::string getTableID(const std::string& table, int identifier) const;
    /*PATCH*/
    void patch_database(const std::string& table, const std::string& identifierField, int identifierValue,
        const std::string& updateField, int updateValue) const;
    /*POST*/
    void post_psem_pedestrian(const std::string& psem_id, const std::string& pedestrianCC_id) const;
    void post_emergency_vehicle(const std::string& tmc_id, const std::string& cb_id, const std::string& licenseplate,
        int origin, int destination, int priority) const;

    /*---Threading & Synchronization Resources------------------------------------------------------------------------*/
    CppWrapper::Thread cloudThread;
    static void* t_cloud(void* arg);

    CppWrapper::Mutex cloudMutex;
    CppWrapper::Queue<CloudSendType> cloudSendQueue;

};

#endif //TRAFFICCONTROLSYSTEM_CLOUDINTERFACE_HPP