
#include "DDSPublisher.hpp"
#include "EmergencyMSGPubSubTypes.hpp"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>

#include <stdexcept>
#include <string>

namespace eprosima::fastdds::examples::emergencyMSG {

/*---Constructor/Destructor---------------------------------------------------------------------------------------*/

DDSPublisher::DDSPublisher(
    std::atomic<bool>& shutdown_requested_,
    uint16_t samples,
    uint16_t expected_matches,
    const std::string& topic_name)
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new EmergencyMSGPubSubType())
    , matched_(0)
    , samples_(samples)
    , expected_matches_(expected_matches)
    , ready_to_publish_(false)
    , shutdown_requested_(shutdown_requested_)
{

    stateMutex = std::make_unique<CppWrapper::Mutex>();
    stateCV = std::make_unique<CppWrapper::CondVar>(*stateMutex);

    // Set up the data type
    set_emergency_message("4916OE", 8, 2, 1);

    auto factory = dds::DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant_with_default_profile(nullptr, dds::StatusMask::none());
    if (!participant_)
        throw std::runtime_error("Participant initialization failed");

    type_.register_type(participant_);

    dds::PublisherQos pub_qos = dds::PUBLISHER_QOS_DEFAULT;
    participant_->get_default_publisher_qos(pub_qos);
    publisher_ = participant_->create_publisher(pub_qos, nullptr, dds::StatusMask::none());
    if (!publisher_)
        throw std::runtime_error("Publisher initialization failed");

    dds::TopicQos topic_qos = dds::TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_ = participant_->create_topic(topic_name, type_.get_type_name(), topic_qos);
    if (!topic_)
        throw std::runtime_error("Topic initialization failed");

    dds::DataWriterQos writer_qos = dds::DATAWRITER_QOS_DEFAULT;
    publisher_->get_default_datawriter_qos(writer_qos);

    writer_qos.reliability().kind = dds::RELIABLE_RELIABILITY_QOS;
    writer_qos.durability().kind = dds::TRANSIENT_LOCAL_DURABILITY_QOS;
    writer_qos.history().kind = dds::KEEP_LAST_HISTORY_QOS;
    writer_qos.history().depth = 100;

    writer_ = publisher_->create_datawriter(topic_, writer_qos, this, dds::StatusMask::all());
    if (!writer_)
        throw std::runtime_error("DataWriter initialization failed");
}

DDSPublisher::~DDSPublisher()
{
    stop();

    if (participant_) {
        participant_->delete_contained_entities();
        dds::DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

/*---System Handling----------------------------------------------------------------------------------------------*/

void DDSPublisher::start()
{
    ddsThread = std::make_unique<CppWrapper::Thread>(t_ddsCommunication);
    ddsThread->setPriority(90);
    ddsThread->run(this);
}

void DDSPublisher::set_emergency_message(const std::string &sender_id, uint8_t origin, uint8_t destination, uint8_t priority_level)
    {
    stateMutex->LockMutex();
    emergency_msg_.sender_id(sender_id);
    emergency_msg_.origin(origin);
    emergency_msg_.destination(destination);
    emergency_msg_.priority_level(priority_level);
    stateMutex->UnlockMutex();
}

/*---Helper methods-----------------------------------------------------------------------------------------------*/

bool DDSPublisher::is_stopped() const
{
    return shutdown_requested_.load();
}

void DDSPublisher::on_publication_matched( dds::DataWriter* /*writer*/, const dds::PublicationMatchedStatus& info)
{
    stateMutex->LockMutex();

    if (info.current_count_change == 1)
    {
        matched_++;
        ready_to_publish_ = true;
        stateCV->condSignal();
    } else if (info.current_count_change == -1) {
        matched_ = static_cast<int16_t>(info.current_count);
        std::cout << "\n>>> Subscriber disconnected <<<\n";
    }
    stateMutex->UnlockMutex();
}

bool DDSPublisher::publish() const
{
    if (!is_stopped())
        return (dds::RETCODE_OK == writer_->write(&emergency_msg_));

    return false;
}

void DDSPublisher::run()
{
    std::cout << "Publisher initialized. Waiting for CB to warn..." << std::endl;

    while (!is_stopped())
    {
        stateMutex->LockMutex();
        while (!ready_to_publish_ && !is_stopped())
            stateCV->condWait();
        stateMutex->UnlockMutex();

        if (is_stopped()) break;

        if (ready_to_publish_)
        {
            uint16_t sent = 0;
            while (!is_stopped() && sent < samples_) {
                if (dds::RETCODE_OK == writer_->write(&emergency_msg_))
                {
                    sent++;
                    std::cout << "Warning message sent:  "
                         << " ID= " << emergency_msg_.sender_id()
                         << ", Origin= " << static_cast<int>(emergency_msg_.origin())
                         << ", Destination= " << static_cast<int>(emergency_msg_.destination())
                         << ", Priority level= " << static_cast<int>(emergency_msg_.priority_level())
                         << std::endl;

                    if (samples_ == 1u)
                    {
                        dds::ReturnCode_t acked = dds::RETCODE_ERROR;
                        do
                        {
                            dds::Duration_t acked_wait{1, 0};
                            acked = writer_->wait_for_acknowledgments(acked_wait);
                        } while (acked != dds::RETCODE_OK);
                    }
                }
                stateMutex->LockMutex();
                stateCV->condTimedWaitMs(period_ms_);
                stateMutex->UnlockMutex();
            }

            {
                stateMutex->LockMutex();
                published_subscribers_.insert(current_matched_subscriber_);
                ready_to_publish_ = false;
                stateMutex->UnlockMutex();
            }

        }
    }
    std::cout << "Publisher closed." << std::endl;
}

void DDSPublisher::stop() const
{
    stateMutex->LockMutex();
    stateCV->condBroadcast();
    stateMutex->UnlockMutex();
    ddsThread->join();
}

/*---Threading & Synchronization Resources------------------------------------------------------------------------*/

void * DDSPublisher::t_ddsCommunication(void *arg)
{
    static_cast<DDSPublisher*>(arg)->run();
    return nullptr;
}

} // namespace eprosima::fastdds::examples::emergencyMSG