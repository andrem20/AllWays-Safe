#include <csignal>
#include <iostream>
#include <stdexcept>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/log/Log.hpp>

#include "DDSSubscriber.hpp"

using eprosima::fastdds::dds::Log;
using eprosima::fastdds::examples::emergencyMSG::DDSSubscriber;
using namespace std;

DDSSubscriber* msg = nullptr;

void signal_handler(int signum)
{
    std::cout << "\nSignal " << signum << " received, stopping Subscriber..." << std::endl;
    if (msg != nullptr)
    {
        msg->stop();
    }
}

int main(int argc, char** argv)
{

    uint16_t samples = 0;  // 0 = infinite
    const string topic_name = "EmergencyAlert";

    try
    {
        // Cria o subscriber
        DDSSubscriber subscriber(samples, topic_name);
        msg = &subscriber;

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
            cout << "Subscriber running. Press Ctrl+C to stop." << endl;
        }
        else
        {
            cout << "Subscriber will receive " << samples << " samples." << endl;
        }

        // Executa
        subscriber.run();

        cout << "Subscriber finished." << endl;
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