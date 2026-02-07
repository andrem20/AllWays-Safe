/*!
 * @file DDSSubscriber.cpp
 *
 */

#include "DDSSubscriber.hpp"

#include <condition_variable>
#include <stdexcept>
#include <ctime>
#include <iomanip>

#include <fastdds/dds/core/condition/GuardCondition.hpp>
#include <fastdds/dds/core/condition/WaitSet.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "EmergencyMSGPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;
using namespace std;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace emergencyMSG {

DDSSubscriber::DDSSubscriber(
        uint16_t samples,
        const string& topic_name)
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , type_(new EmergencyMSGPubSubType())
    , samples_(samples)
    , received_samples_(0)
    , stop_(false)
{
    // Create the participant
    auto factory = DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant_with_default_profile(nullptr, StatusMask::none());
    if (participant_ == nullptr)
    {
        throw runtime_error("Participant initialization failed");
    }

    // Register the type
    type_.register_type(participant_);

    // Create the subscriber
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    participant_->get_default_subscriber_qos(sub_qos);
    subscriber_ = participant_->create_subscriber(sub_qos, nullptr, StatusMask::none());
    if (subscriber_ == nullptr)
    {
        throw runtime_error("Subscriber initialization failed");
    }

    // Create the topic
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_ = participant_->create_topic(topic_name, type_.get_type_name(), topic_qos);
    if (topic_ == nullptr)
    {
        throw runtime_error("Topic initialization failed");
    }

    // Create the reader
    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    subscriber_->get_default_datareader_qos(reader_qos);
    reader_ = subscriber_->create_datareader(topic_, reader_qos, nullptr, StatusMask::all());
    if (reader_ == nullptr)
    {
        throw runtime_error("DataReader initialization failed");
    }

    // Prepare a wait-set
    wait_set_.attach_condition(reader_->get_statuscondition());
    wait_set_.attach_condition(terminate_condition_);
}

DDSSubscriber::~DDSSubscriber()
{
    if (nullptr != participant_)
    {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void DDSSubscriber::run()
{
    while (!is_stopped())
    {
        ConditionSeq triggered_conditions;
        ReturnCode_t ret_code = wait_set_.wait(triggered_conditions, eprosima::fastdds::dds::c_TimeInfinite);
        if (RETCODE_OK != ret_code)
        {
            EPROSIMA_LOG_ERROR(SUBSCRIBER_WAITSET, "Error waiting for conditions");
            continue;
        }
        for (Condition* cond : triggered_conditions)
        {
            StatusCondition* status_cond = dynamic_cast<StatusCondition*>(cond);
            if (nullptr != status_cond)
            {
                Entity* entity = status_cond->get_entity();
                StatusMask changed_statuses = entity->get_status_changes();
                if (changed_statuses.is_active(StatusMask::subscription_matched()))
                {
                    SubscriptionMatchedStatus status_;
                    reader_->get_subscription_matched_status(status_);
                    if (status_.current_count_change == 1)
                    {
                        cout << "Subscriber matched." << endl;
                    }
                    else if (status_.current_count_change == -1)
                    {
                        cout << "Subscriber unmatched." << endl;
                    }
                    else
                    {
                        cout << status_.current_count_change <<
                            " is not a valid value for SubscriptionMatchedStatus current count change" <<
                            endl;
                    }
                }
                if (changed_statuses.is_active(StatusMask::data_available()))
                {
                    SampleInfo info;
                    while ((!is_stopped()) &&
                            (RETCODE_OK == reader_->take_next_sample(&emergency_msg_, &info)))
                    {
                        if ((info.instance_state == ALIVE_INSTANCE_STATE) && info.valid_data)
                        {
                            received_samples_++;
                            int64_t timestamp_sec = info.source_timestamp.seconds();

                            time_t rawtime = static_cast<time_t>(timestamp_sec);
                            struct tm* timeinfo = localtime(&rawtime);

                            char time_buffer[80];
                            strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", timeinfo);


                            cout << "Message RECEIVED: ID=" << emergency_msg_.sender_id()
                              << ", Origin=" << static_cast<int>(emergency_msg_.origin())
                              << ", Destination=" << static_cast<int>(emergency_msg_.destination())
                              << ", Sent at:" << time_buffer << endl;

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

bool DDSSubscriber::is_stopped()
{
    return stop_.load();
}

void DDSSubscriber::stop()
{
    stop_.store(true);
    terminate_condition_.set_trigger_value(true);
}

} // namespace emergencyMSG
} // namespace examples
} // namespace fastdds
} // namespace eprosima
