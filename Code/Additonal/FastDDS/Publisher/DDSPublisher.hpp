/**
 * @file PublisherApp.hpp
 *
 */

#ifndef FASTDDS_DDSPUBLISHER_HPP
#define FASTDDS_DDSPUBLISHER_HPP

#include <condition_variable>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>


#include "EmergencyMSG.hpp"

using namespace eprosima::fastdds::dds;
using namespace std;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace emergencyMSG {

class DDSPublisher : public DataWriterListener
{
public:

    DDSPublisher(
            uint16_t samples,
            uint16_t expected_matches,
            const string& topic_name);

    ~DDSPublisher();

    //! Publisher matched method
    void on_publication_matched(
            DataWriter* writer,
            const PublicationMatchedStatus& info) override;

    void run();
    void stop();

private:
    bool is_stopped();
    //! Publish a sample
    bool publish();

    EmergencyMSG emergency_msg_;
    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    TypeSupport type_;
    int16_t matched_;
    uint16_t samples_;
    uint16_t expected_matches_;
    mutex mutex_;
    condition_variable cv_;
    atomic<bool> stop_;
    const uint32_t period_ms_ = 100; // in ms
};

} // namespace emergencyMSG
} // namespace examples
} // namespace fastdds
} // namespace eprosima

#endif // FASTDDS_DDSPUBLISHER_HPP
