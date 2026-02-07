#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "../../TrafficControlSystem.hpp"

using json = nlohmann::json;

/* TEST SET
 *  - INPUT Data Validation w/ JSON files
 *  - Item Creation
 *  - Traffic Configurations
 *
 *  Files under /correct_config and /wrong_config must be inside the RPi, under /root
 */

#define CHECK_CORRECT_CONFIG

#ifndef CHECK_CORRECT_CONFIG
#define CHECK_WRONG_CONFIG
#endif

std::string test_path = "/root/";

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

int main ()
{
    try
    {
        TrafficControlSystem& tcs = TrafficControlSystem::getInstance();

        // Load respective JSON
#ifdef CHECK_CORRECT_CONFIG
        json psem = load_json(test_path + "correct_PSEM.json");
        json tsem = load_json(test_path + "correct_TSEM.json");
#endif
#ifdef CHECK_WRONG_CONFIG
        json psem = load_json(test_path + "wrong_PSEM.json");
        json tsem = load_json(test_path + "wrong_TSEM.json");
#endif

        if (const int ret = tcs.createComponents(psem); ret < 0) // Check pedestrian sem vector + crosswalks
            throw std::runtime_error("Error creating PSEM: " + std::to_string(ret));
        if (const int ret = tcs.createComponents(tsem); ret < 0) // Check traffic sem vector
            throw std::runtime_error("Error creating TSEM: " + std::to_string(ret));

        tcs.findConfigurations(); // Check configurations vector

    }  catch (const std::runtime_error& e) {
        std::cerr << "Runtime error: " << e.what() << '\n';
    }

    return 0;
}