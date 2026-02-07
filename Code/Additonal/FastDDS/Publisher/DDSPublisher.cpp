/**
 * @file PublisherApp.cpp
 *
 */

#include "DDSPublisher.hpp"

#include <condition_variable>
#include <stdexcept>
#include <string>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>

#include "EmergencyMSGPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;
using namespace std;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace emergencyMSG {

DDSPublisher::DDSPublisher(
        uint16_t samples,
        uint16_t expected_matches,
        const string& topic_name)
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new EmergencyMSGPubSubType())
    , matched_(0)
    , samples_(samples)
    , expected_matches_(expected_matches)
    , stop_(false)
{
    // Set up the data type with initial values
    emergency_msg_.sender_id("4916OE");
    emergency_msg_.origin(1);
    emergency_msg_.destination(2);

    // Create the participant
    auto factory = DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant_with_default_profile(nullptr, StatusMask::none());
    if (participant_ == nullptr)
    {
        throw runtime_error("Participant initialization failed");
    }

    // Register the type
    type_.register_type(participant_);

    // Create the publisher
    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    participant_->get_default_publisher_qos(pub_qos);
    publisher_ = participant_->create_publisher(pub_qos, nullptr, StatusMask::none());
    if (publisher_ == nullptr)
    {
        throw runtime_error("Publisher initialization failed");
    }

    // Create the topic
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_ = participant_->create_topic(topic_name, type_.get_type_name(), topic_qos);
    if (topic_ == nullptr)
    {
        throw runtime_error("Topic initialization failed");
    }

    // Create the data writer
    DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
    writer_qos.history().depth = 5;
    publisher_->get_default_datawriter_qos(writer_qos);
    writer_ = publisher_->create_datawriter(topic_, writer_qos, this, StatusMask::all());
    if (writer_ == nullptr)
    {
        throw runtime_error("DataWriter initialization failed");
    }
}

DDSPublisher::~DDSPublisher()
{
    if (nullptr != participant_)
    {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void DDSPublisher::on_publication_matched(
        DataWriter* /*writer*/,
        const PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = static_cast<int16_t>(info.current_count);
        cout << "Publisher matched." << endl;
        cv_.notify_one();
    }
    else if (info.current_count_change == -1)
    {
        matched_ = static_cast<int16_t>(info.current_count);
        cout << "Publisher unmatched." << endl;
    }
    else
    {
        cout << info.current_count_change
                  << " is not a valid value for PublicationMatchedStatus current count change" << endl;
    }
}

void DDSPublisher::run()
{
    uint16_t sent = 0;
    while (!is_stopped() && (sent < samples_) )
    {
        if (publish())
        {
            sent++;
            cout << "Message: ID=" << emergency_msg_.sender_id()
               << ", Origin=" << static_cast<int>(emergency_msg_.origin())
               << ", Destination=" << static_cast<int>(emergency_msg_.destination())
               << " SENT" << endl;


            if (samples_ == 1u)
            {
                ReturnCode_t acked = RETCODE_ERROR;
                do
                {
                    dds::Duration_t acked_wait{1, 0};
                    acked = writer_->wait_for_acknowledgments(acked_wait);
                }
                while (acked != RETCODE_OK);
            }
        }
        if (samples_ != 0 && sent >= samples_)
        {
            break;
        }

        // Wait for period or stop event
        unique_lock<mutex> period_lock(mutex_);
        cv_.wait_for(period_lock, chrono::milliseconds(period_ms_), [&]()
                {
                    return is_stopped();
                });
    }
}

bool DDSPublisher::publish()
{
    bool ret = false;
    // Wait for the data endpoints discovery
    unique_lock<mutex> matched_lock(mutex_);
    cv_.wait(matched_lock, [&]()
            {
                // at least one has been discovered
                return ((matched_ >= expected_matches_) || is_stopped());
            });

    if (!is_stopped())
    {
        ret = (RETCODE_OK == writer_->write(&emergency_msg_));
    }
    return ret;
}

bool DDSPublisher::is_stopped()
{
    return stop_.load();
}

void DDSPublisher::stop()
{
    stop_.store(true);
    cv_.notify_one();
}

} // namespace emergencyMSG
} // namespace examples
} // namespace fastdds
} // namespace eprosima
