/*!
 * @file EmergencyMSGCdrAux.hpp
 * This source file contains some definitions of CDR related functions.
 */

#ifndef FAST_DDS_EMERGENCYMSGCDRAUX_HPP
#define FAST_DDS_EMERGENCYMSGCDRAUX_HPP

#include "EmergencyMSG.hpp"
constexpr uint32_t EmergencyMSG_max_cdr_typesize {8UL};
constexpr uint32_t EmergencyMSG_max_key_cdr_typesize {0UL};


namespace eprosima {
namespace fastcdr {

class Cdr;
class CdrSizeCalculator;

eProsima_user_DllExport void serialize_key(
        eprosima::fastcdr::Cdr& scdr,
        const EmergencyMSG& data);


} // namespace fastcdr
} // namespace eprosima

#endif // FAST_DDS_EMERGENCYMSGCDRAUX_HPP

