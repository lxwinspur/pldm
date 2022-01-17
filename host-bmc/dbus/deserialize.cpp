#include "deserialize.hpp"

#include "custom_dbus.hpp"
#include "serialize.hpp"

#include <nlohmann/json.hpp>

namespace pldm
{
namespace deserialize
{
namespace fs = std::filesystem;

using Json = nlohmann::json;
using Properties = std::map<std::string, dbus::PropertyValue>;

using callback =
    std::function<void(const std::string& path,
                       std::map<std::string, dbus::PropertyValue> values)>;

std::unordered_map<std::string, callback> ibmDbusHandler{
    {"LocationCode",
     [](const std::string& path, Properties values) {
         if (values.contains("locationCode"))
         {
             pldm::dbus::CustomDBus::getCustomDBus().setLocationCode(
                 path, std::get<std::string>(values.at("locationCode")));
         }
     }},
    {"Associations",
     [](const std::string& path, Properties values) {
         if (values.contains("associations"))
         {
             pldm::dbus::CustomDBus::getCustomDBus().setAssociations(
                 path,
                 std::get<dbus::AssociationsObj>(values.at("associations")));
         }
     }},
    {"Available",
     [](const std::string& path, Properties values) {
         if (values.contains("available"))
         {
             pldm::dbus::CustomDBus::getCustomDBus().setAvailabilityState(
                 path, std::get<bool>(values.at("available")));
         }
     }},
    {"OperationalStatus",
     [](const std::string& path, Properties values) {
         if (values.contains("functional"))
         {
             pldm::dbus::CustomDBus::getCustomDBus().setOperationalStatus(
                 path, std::get<bool>(values.at("functional")));
         }
     }},
    {"InventoryItem",
     [](const std::string& path, Properties values) {
         if (values.contains("present"))
         {
             pldm::dbus::CustomDBus::getCustomDBus().updateItemPresentStatus(
                 path, std::get<bool>(values.at("present")));
         }
     }},
    {"Enable",
     [](const std::string& path, Properties values) {
         if (values.contains("enabled"))
         {
             pldm::dbus::CustomDBus::getCustomDBus().implementCpuCoreInterface(
                 path, std::get<bool>(values.at("enabled")));
         }
     }},
    {"ItemChassis",
     [](const std::string& path, Properties /* values */) {
         pldm::dbus::CustomDBus::getCustomDBus().implementChassisInterface(
             path);
     }},
    {"PCIeSlot",
     [](const std::string& path, Properties /* values */) {
         pldm::dbus::CustomDBus::getCustomDBus().implementPCIeSlotInterface(
             path);
     }},
    {"Motherboard",
     [](const std::string& path, Properties /* values */) {
         pldm::dbus::CustomDBus::getCustomDBus().implementMotherboardInterface(
             path);
     }},
    {"PowerSupply",
     [](const std::string& path, Properties /* values */) {
         pldm::dbus::CustomDBus::getCustomDBus().implementPowerSupplyInterface(
             path);
     }},
    {"Fan",
     [](const std::string& path, Properties /* values */) {
         pldm::dbus::CustomDBus::getCustomDBus().implementFanInterface(path);
     }},
    {"Connector",
     [](const std::string& path, Properties /* values */) {
         pldm::dbus::CustomDBus::getCustomDBus().implementConnecterInterface(
             path);
     }},
    {"VRM",
     [](const std::string& path, Properties /* values */) {
         pldm::dbus::CustomDBus::getCustomDBus().implementVRMInterface(path);
     }},
    {"FabricAdapter",
     [](const std::string& path, Properties /* values */) {
         pldm::dbus::CustomDBus::getCustomDBus().implementFabricAdapter(path);
     }},
    {"Board",
     [](const std::string& path, Properties /* values */) {
         pldm::dbus::CustomDBus::getCustomDBus().implementBoard(path);
     }},
    {"Global",
     [](const std::string& path, Properties /* values */) {
         pldm::dbus::CustomDBus::getCustomDBus().implementGlobalInterface(path);
     }},
    {"LicenseEntry", [](const std::string& path, Properties values) {
         std::string name{};
         std::string serialno{};
         dbus::LicenseEntryType type;
         dbus::LicenseEntryAuthorizationType authtype;
         uint32_t authdevno{};
         uint64_t exptime{};

         if (values.contains("name"))
         {
             name = std::get<std::string>(values.at("name"));
         }
         if (values.contains("serialNumber"))
         {
             serialno = std::get<std::string>(values.at("serialNumber"));
         }
         if (values.contains("type"))
         {
             type = std::get<dbus::LicenseEntryType>(values.at("type"));
         }
         if (values.contains("authorizationType"))
         {
             authtype = std::get<dbus::LicenseEntryAuthorizationType>(
                 values.at("authorizationType"));
         }
         if (values.contains("expirationTime"))
         {
             exptime = std::get<uint64_t>(values.at("expirationTime"));
         }
         if (values.contains("authDeviceNumber"))
         {
             authdevno = std::get<uint32_t>(values.at("authDeviceNumber"));
         }

         pldm::dbus::CustomDBus::getCustomDBus().implementLicInterfaces(
             path, authdevno, name, serialno, exptime, type, authtype);
     }}};

std::set<std::string> getDbusPaths(const fs::path& path)
{
    std::set<std::string> paths{};

    if (!fs::exists(path) || fs::is_empty(path))
    {
        std::cerr << "The file does not exist or is empty, FILE_PATH = " << path
                  << std::endl;
        return paths;
    }

    std::string root = "/xyz/openbmc_project/inventory/";
    try
    {
        std::ifstream jsonFile(path);
        auto json = Json::parse(jsonFile);

        // define the default JSON as empty
        const std::set<std::string> empty{};
        auto objectPaths = json.value("objectPaths", empty);
        std::ranges::transform(objectPaths, std::inserter(paths, paths.begin()),
                               [root](const auto& i) { return root + i; });
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to parse config file, FILE_PATH = " << path
                  << ", ERROR = " << e.what() << std::endl;
    }

    return paths;
}

void restoreDbusObj()
{
    if (!pldm::serialize::Serialize::getSerialize().deserialize())
    {
        return;
    }

    auto paths = getDbusPaths(DBUS_JSON_FILE);

    auto savedObjs = pldm::serialize::Serialize::getSerialize().getSavedObjs();
    for (auto& objs : savedObjs)
    {
        if (!paths.contains(objs.first))
        {
            continue;
        }

        for (auto& interfaces : objs.second)
        {
            ibmDbusHandler.at(objs.first)(interfaces.first, interfaces.second);
        }
    }
}

} // namespace deserialize
} // namespace pldm
