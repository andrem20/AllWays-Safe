#include <csignal>
#include <iostream>
#include <stdexcept>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/log/Log.hpp>

#include "DDSPublisher.hpp"

using eprosima::fastdds::dds::Log;
using namespace eprosima::fastdds::examples::emergencyMSG;

using namespace std;

DDSPublisher* msg = nullptr;

void signal_handler(int signum)
{
    cout << "\nSignal " << signum << " received, stopping Publisher..." << endl;
    if (msg != nullptr)
    {
        msg->stop();
    }
}

int main(int argc, char** argv)
{
    uint16_t samples = 3;  // 0 = infinite
    uint16_t matched = 1;  // waits for 1 subscriber
    const string topic_name = "EmergencyAlert";

   try
    {
        // creates publisher
        DDSPublisher publisher(samples, matched, topic_name);
        msg = &publisher;

        // Configura signal handlers
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
#ifndef _WIN32
        signal(SIGQUIT, signal_handler);
        signal(SIGHUP, signal_handler);
#endif

        // Informação
        if (samples == 0)
        {
            cout << "Publisher running. Press Ctrl+C to stop." << endl;
        }
        else
        {
            cout << "Publisher will send " << samples << " samples." << endl;
        }

        // Executa
        publisher.run();

        cout << "Publisher finished." << endl;
    }
    catch (const runtime_error& e)
    {
        cerr << "Error: " << e.what() << endl;
        Log::Reset();
        return EXIT_FAILURE;
    }

    Log::Reset();
    return EXIT_SUCCESS;
}