/**
 * @file DDSSubscriber.hpp
 *
 */

#ifndef FASTDDS_DDSSUBSCRIBER_HPP
#define FASTDDS_DDSSUBSCRIBER_HPP

#include <fastdds/dds/core/condition/GuardCondition.hpp>
#include <fastdds/dds/core/condition/WaitSet.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "EmergencyMSG.hpp"

using namespace eprosima::fastdds::dds;
using namespace std;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace emergencyMSG {

class DDSSubscriber: public DataReaderListener
{
public:

    DDSSubscriber(
            uint16_t samples,
            const string& topic_name);
    ~DDSSubscriber();

    void run();
    void stop();

private:
    bool is_stopped();
    EmergencyMSG emergency_msg_;
    DomainParticipant* participant_;
    Subscriber* subscriber_;
    Topic* topic_;
    DataReader* reader_;
    TypeSupport type_;
    WaitSet wait_set_;
    uint16_t samples_;
    uint16_t received_samples_;
    atomic<bool> stop_;
    GuardCondition terminate_condition_;
};

} // namespace emergencyMSG
} // namespace examples
} // namespace fastdds
} // namespace eprosima

#endif // FASTDDS_DDSSUBSCRIBER_HPP
