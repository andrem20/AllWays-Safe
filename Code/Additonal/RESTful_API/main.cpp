#include <iostream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <string>
#include <stdexcept>

using json = nlohmann::json;
using namespace std;

std::string get_iso8601_timestamp() {
    auto t = std::time(nullptr);
    auto tm = *std::gmtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// Callback para armazenar a resposta HTTP
size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// Função genérica para fazer requests HTTP
std::string makeRequest(
    const std::string& endpoint,
    const std::string& method,
    const std::string& body = ""
) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to initialize CURL");

    std::string readBuffer;
    std::string url = "http://192.168.0.138:3000" + endpoint;

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

    // Verificar código de resposta HTTP
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    }
//notificação para a msg queue
    if (http_code >= 400) {
        throw std::runtime_error("HTTP error " + std::to_string(http_code) + ": " + readBuffer);
    }

    return readBuffer;
}

void cloudConnect(const std::string& url)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("CloudInterface::cloudConnect: curl_easy_init() failed");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD request
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    const CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error("Cloud unavailable : curl_easy_perform() failed");

    std::cout << "Cloud connected successfully!\n";
}

// Função para obter dados com filtro
json query_database(const std::string& table, const std::string& field, const std::string& value) {
    std::string endpoint = "/data/" + table + "/" + field + "/" + value;
    std::string response = makeRequest(endpoint, "GET");
    return json::parse(response);
}

string getTableID(const std::string& table,
                          const std::string& identifier){
    std::string endpoint = "/id/" + table + "/" + identifier;
    std::string response = makeRequest(endpoint, "GET");

    json result = json::parse(response);

    return result["id"].get<string>();
}

string getTableID(const std::string& table,
                          int identifier){
    std::string endpoint = "/id/" + table + "/" + std::to_string(identifier);
    std::string response = makeRequest(endpoint, "GET");

    json result = json::parse(response);

    return result["id"].get<string>();
}


// Função PATCH modular para atualizar qualquer campo
void patch_database(
    const std::string& table,
    const std::string& identifierField,
    int identifierValue,
    const std::string& updateField,
    const json& updateValue
) {
    json body;
    body["identifierField"] = identifierField;
    body["identifierValue"] = identifierValue;
    body["updateField"] = updateField;
    body["updateValue"] = updateValue;

    std::string endpoint = "/data/" + table;
    std::string response = makeRequest(endpoint, "PATCH", body.dump()); // envia o JSON no corpo
}

void post_psem_pedestrian(const std::string& psem_id,
                         const std::string& pedestrianCC_id)
{
    json body;
    body["psem_id"] = psem_id;
    body["pedestrianCC_id"] = pedestrianCC_id;
    body["timestamp"] = get_iso8601_timestamp();

    makeRequest(
        "/data/p_semaphore_pedestrian",
        "POST",
        body.dump()
    );
}

void post_emergency_vehicle(const std::string& tmc_id,
                            const std::string& cb_id,
                            const std::string& licenseplate,
                            int origin,
                            int destination,
                            int priority)
{
    json body;
    body["tmcid"] = tmc_id;
    body["controlbox_id"] = cb_id;
    body["licenseplate"] = licenseplate;
    body["origin"] = origin;
    body["destination"] = destination;
    body["priority_level"] = priority;
    body["timestamp"] = get_iso8601_timestamp();

    makeRequest(
        "/data/emergencyvehicle",
        "POST",
        body.dump()
    );
}


int main() {

    /*string id_controlbox = getTableID("controlbox", "raspMari.local");
    json result_tsem = query_database("t_semaphore", "controlboxid",id_controlbox);
    // result_tsem.size() == 0 → não tem semáforos
    json result_psem = query_database("p_semaphore", "controlboxid", id_controlbox);

    std::cout << result_tsem.dump() << std::endl;
    std::cout << result_psem.dump() << std::endl;*/

    /*json result_pedestrians = query_database("pedestrian", "physicaltag_id", "34356543");
    bool found = result_pedestrians.value("found", false);

    if (found) {
        cout << "existe"<< endl;// existe
    } else {
        cout << "não existe"<< endl;// não existe
    }*/


    /*patch_database("t_semaphore", "location", 3, "status", "GREEN");
    patch_database("p_semaphore", "location", 3, "status", "GREEN");*/

    /*string id_controlbox = getTableID("controlbox", "raspMari.local");
    string id_tmc = getTableID("tmc", "tmc1");
    post_emergency_vehicle(id_tmc, id_controlbox, "58-58-69", 2, 4, 5);

    string id_psem = getTableID("p_semaphore", 3);
    string cc_id = getTableID("pedestrian","34343");
    post_psem_pedestrian(id_psem,cc_id);*/
    std::string cbName = "raspMari.local";

    std::string cbid = getTableID( "controlbox", cbName);

    nlohmann::json result = query_database( "controlbox", "id", cbid);
    std::cout << result.dump() << std::endl;

    return 0;
};
