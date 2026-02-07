
#ifndef FASTDDS_DDSPUBLISHER_HPP
#define FASTDDS_DDSPUBLISHER_HPP

#include "EmergencyMSG.hpp"
#include "CppWrapper_pthreads/CppWrapper.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

namespace eprosima::fastdds::examples::emergencyMSG {

class DDSPublisher : public dds::DataWriterListener
{
private:
    /*---DDS Attributes---------------------------------------------------------------------------------------------------*/
    EmergencyMSG emergency_msg_;
    dds::DomainParticipant *participant_;
    dds::Publisher* publisher_;
    dds::Topic *topic_;
    dds::DataWriter* writer_;
    dds::TypeSupport type_;
    std::set<rtps::GUID_t> published_subscribers_;
    rtps::GUID_t current_matched_subscriber_;

    int16_t matched_;
    uint16_t samples_;
    uint16_t expected_matches_;

    bool ready_to_publish_;
    const uint32_t period_ms_ = 100;
    std::atomic<bool>& shutdown_requested_;

    /*---Helper methods-----------------------------------------------------------------------------------------------*/
    [[nodiscard]] bool is_stopped() const;
    void on_publication_matched(dds::DataWriter* writer, const dds::PublicationMatchedStatus& info) override;
    [[nodiscard]] bool publish() const;
    void run();
    void stop() const;

    /*---Threading & Synchronization Resources------------------------------------------------------------------------*/
    std::unique_ptr<CppWrapper::Thread> ddsThread;
    std::unique_ptr<CppWrapper::Mutex> stateMutex;
    std::unique_ptr<CppWrapper::CondVar> stateCV;
    static void* t_ddsCommunication(void* arg);

public:
    /*---Constructor/Destructor---------------------------------------------------------------------------------------*/
    DDSPublisher(std::atomic<bool> &shutdown_requested_, uint16_t samples, uint16_t expected_matches,
                 const std::string &topic_name);
    ~DDSPublisher() override;

    /*---Disable Copying (unique ownership)---------------------------------------------------------------------------*/
    DDSPublisher(const DDSPublisher&) = delete;
    DDSPublisher& operator=(const DDSPublisher&) = delete;

    /*---System Handling----------------------------------------------------------------------------------------------*/
    void start();
    void set_emergency_message(const std::string& sender_id, uint8_t origin, uint8_t destination, uint8_t priority_level);

};

} // namespace eprosima::fastdds::examples::emergencyMSG

#endif // FASTDDS_DDSPUBLISHER_HPP