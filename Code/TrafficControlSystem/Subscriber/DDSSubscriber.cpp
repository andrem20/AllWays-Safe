#include "DDSSubscriber.hpp"
#include "EmergencyMSGPubSubTypes.hpp"

#include <condition_variable>
#include <ctime>
#include <iomanip>

#include <fastdds/dds/core/condition/GuardCondition.hpp>
#include <fastdds/dds/core/condition/WaitSet.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "../Messages/Components/DDSEvent.hpp"

namespace eprosima::fastdds::examples::emergencyMSG {

/*---Constructor/Destructor---------------------------------------------------------------------------------------*/

DDSSubscriber::DDSSubscriber(
        std::atomic<bool>& shutdownRequested,
        uint16_t samples,
        const std::string& topic_name,
        Mediator* mediator)
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , type_(new EmergencyMSGPubSubType())
    , samples_(samples)
    , received_samples_(0)
    , _shutdown_requested(shutdownRequested), Component(mediator)
{

    auto factory = dds::DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant_with_default_profile(nullptr, dds::StatusMask::none());
    if (participant_ == nullptr)
        throw std::runtime_error("Participant initialization failed");

    type_.register_type(participant_);

    dds::SubscriberQos sub_qos = dds::SUBSCRIBER_QOS_DEFAULT;
    participant_->get_default_subscriber_qos(sub_qos);
    subscriber_ = participant_->create_subscriber(sub_qos, nullptr, dds::StatusMask::none());
    if (subscriber_ == nullptr)
        throw std::runtime_error("Subscriber initialization failed");

    dds::TopicQos topic_qos = dds::TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_ = participant_->create_topic(topic_name, type_.get_type_name(), topic_qos);
    if (topic_ == nullptr)
        throw std::runtime_error("Topic initialization failed");

    dds::DataReaderQos reader_qos = dds::DATAREADER_QOS_DEFAULT;
    subscriber_->get_default_datareader_qos(reader_qos);

    reader_qos.reliability().kind = dds::RELIABLE_RELIABILITY_QOS;
    reader_qos.durability().kind = dds::TRANSIENT_LOCAL_DURABILITY_QOS;
    reader_qos.history().kind = dds::KEEP_LAST_HISTORY_QOS;
    reader_qos.history().depth = 100;

    reader_ = subscriber_->create_datareader(topic_, reader_qos, nullptr, dds::StatusMask::all());
    if (reader_ == nullptr)
        throw std::runtime_error("DataReader initialization failed");

    wait_set_.attach_condition(reader_->get_statuscondition());
    wait_set_.attach_condition(terminate_condition_);

    std::cout << "Subscriber Created" << std::endl;
}

DDSSubscriber::~DDSSubscriber()
{
    if (nullptr != participant_)
    {
        participant_->delete_contained_entities();
        dds::DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

/*---System Handling----------------------------------------------------------------------------------------------*/

void DDSSubscriber::start()
{
    ddsThread = std::make_unique<CppWrapper::Thread>(t_ddsCommunication);
    ddsThread->run(this);
}

/*---Helper methods-----------------------------------------------------------------------------------------------*/

bool DDSSubscriber::is_stopped() const
{
    return _shutdown_requested.load();
}

void DDSSubscriber::run()
{
    while (!is_stopped())
    {
      //  std::cout << "Subscriber Running" << std::endl;

        dds::ConditionSeq triggered_conditions;
        dds::Duration_t timeout {0, 100000000};
        dds::ReturnCode_t ret_code = wait_set_.wait(triggered_conditions, timeout); // eprosima::fastdds::dds::c_TimeInfinite
        if (dds::RETCODE_TIMEOUT == ret_code)
        {
            continue;
        }
        if (dds::RETCODE_OK != ret_code)
        {
            EPROSIMA_LOG_ERROR(SUBSCRIBER_WAITSET, "Error waiting for conditions");
            continue;
        }
        for (dds::Condition* cond : triggered_conditions)
        {
            dds::StatusCondition* status_cond = dynamic_cast<dds::StatusCondition*>(cond);
            if (nullptr != status_cond)
            {
                dds::Entity* entity = status_cond->get_entity();
                dds::StatusMask changed_statuses = entity->get_status_changes();
                if (changed_statuses.is_active(dds::StatusMask::subscription_matched()))
                {
                    dds::SubscriptionMatchedStatus status_;
                    reader_->get_subscription_matched_status(status_);
                    if (status_.current_count_change == 1)
                    {
                        std::cout << "Subscriber matched." << std::endl;
                    }
                    else if (status_.current_count_change == -1)
                    {
                        std::cout << "Subscriber unmatched." << std::endl;
                        DDSEvent event_EM_Stop
                        {
                           .qualifier = DDS_Event_Qualifier::EMERGENCY_FINISH
                        };

                        mediator->notify(this, event_EM_Stop);

                    }
                    else
                    {
                        std::cout << status_.current_count_change <<
                            " is not a valid value for SubscriptionMatchedStatus current count change" <<
                            std::endl;
                    }
                }
                if (changed_statuses.is_active(dds::StatusMask::data_available()))
                {
                    dds::SampleInfo info;
                    while ((!is_stopped()) &&
                            (dds::RETCODE_OK == reader_->take_next_sample(&emergency_msg_, &info)))
                    {
                        if ((info.instance_state == dds::ALIVE_INSTANCE_STATE) && info.valid_data)
                        {
                            received_samples_++;

                            std::cout << "Warning message received: " << emergency_msg_.sender_id()
                              << " Origin= " << static_cast<int>(emergency_msg_.origin())
                              << ", Destination= " << static_cast<int>(emergency_msg_.destination())
                              << ", Priority Level= " << static_cast<int>(emergency_msg_.priority_level())
                              << std::endl;

                            DDSEvent event_EM_Start
                            {
                                .qualifier = DDS_Event_Qualifier::EMERGENCY_START,
                                .license_plate =  emergency_msg_.sender_id(),
                                .location = emergency_msg_.origin(),
                                .direction = emergency_msg_.destination(),
                                .priority = emergency_msg_.priority_level(),
                            };

                            mediator->notify(this, event_EM_Start);

                            if (samples_ > 0 && (received_samples_ >= samples_))
                            {
                                stop();
                            }
                        }
                    }
                }
            }
        }
    }
}

void DDSSubscriber::stop()
{
    std::cerr << "Subscriber Stopped" << std::endl;
    terminate_condition_.set_trigger_value(true);
    ddsThread->join();
}

/*---Threading & Synchronization Resources------------------------------------------------------------------------*/

void * DDSSubscriber::t_ddsCommunication(void *arg)
{
    static_cast<DDSSubscriber*>(arg)->run();
    return nullptr;
}

} // namespace eprosima::fastdds::examples::emergencyMSG



