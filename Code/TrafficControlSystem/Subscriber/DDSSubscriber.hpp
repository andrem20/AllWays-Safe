
#ifndef FASTDDS_DDSSUBSCRIBER_HPP
#define FASTDDS_DDSSUBSCRIBER_HPP

#include <fastdds/dds/core/condition/GuardCondition.hpp>
#include <fastdds/dds/core/condition/WaitSet.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "EmergencyMSG.hpp"
#include "../Mediator.hpp"
#include "../CppWrapper/CppWrapper.hpp"

namespace eprosima::fastdds::examples::emergencyMSG {

class DDSSubscriber: public dds::DataReaderListener, public Component
{
private:
    /*---DDS Attributes---------------------------------------------------------------------------------------------------*/
    EmergencyMSG emergency_msg_;
    dds::DomainParticipant* participant_;
    dds::Subscriber* subscriber_;
    dds::TopicDescription *topic_;
    dds::DataReader* reader_;
    dds::TypeSupport type_;
    dds::WaitSet wait_set_;
    uint16_t samples_;
    uint16_t received_samples_;
    std::atomic<bool>& _shutdown_requested;
    dds::GuardCondition terminate_condition_;

    /*---Helper methods-----------------------------------------------------------------------------------------------*/
    [[nodiscard]] bool is_stopped() const;
    void run();

    /*---Threading & Synchronization Resources------------------------------------------------------------------------*/
    std::unique_ptr<CppWrapper::Thread> ddsThread;
    static void* t_ddsCommunication(void* arg);

public:
    /*---Constructor/Destructor---------------------------------------------------------------------------------------*/
    DDSSubscriber(std::atomic<bool>& shutdownRequested, uint16_t samples, const std::string& topic_name, Mediator* mediator);
    ~DDSSubscriber() override;

    /*---Disable Copying (unique ownership)---------------------------------------------------------------------------*/
    DDSSubscriber(const DDSSubscriber&) = delete;
    DDSSubscriber& operator=(const DDSSubscriber&) = delete;

    /*---System Handling----------------------------------------------------------------------------------------------*/
    void start();
    void stop();
};

} // namespace eprosima::fastdds::examples::emergencyMSG

#endif // FASTDDS_DDSSUBSCRIBER_HPP
