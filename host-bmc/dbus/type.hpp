#pragma once

#include "license_entry.hpp"

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace pldm
{
namespace dbus
{

using LicenseEntryType =
    sdbusplus::com::ibm::License::Entry::server::LicenseEntry::Type;
using LicenseEntryAuthorizationType = sdbusplus::com::ibm::License::Entry::
    server::LicenseEntry::AuthorizationType;
using AssociationsObj =
    std::vector<std::tuple<std::string, std::string, std::string>>;

using PropertyValue =
    std::variant<bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
                 uint64_t, double, std::string, AssociationsObj,
                 LicenseEntryType, LicenseEntryAuthorizationType>;

// eg: {{object path, {interfaces, {property name, value}}}
using SavedObjs =
    std::map<std::string,
             std::map<std::string, std::map<std::string, PropertyValue>>>;
} // namespace dbus
} // namespace pldm
