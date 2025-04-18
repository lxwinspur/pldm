#pragma once

#include "libpldm/base.h"
#include "libpldm/platform.h"
#include "libpldm/state_set.h"

#include "common/types.hpp"
#include "common/utils.hpp"
#include "dbus_to_host_effecters.hpp"
#include "host_associations_parser.hpp"
#include "libpldmresponder/event_parser.hpp"
#include "libpldmresponder/oem_handler.hpp"
#include "libpldmresponder/pdr_utils.hpp"
#include "requester/handler.hpp"
#include "utils.hpp"

#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>

#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <vector>

namespace pldm
{

using EntityType = uint16_t;
// vector which would hold the PDR record handle data returned by
// pldmPDRRepositoryChgEvent event data
using ChangeEntry = uint32_t;
using PDRRecordHandles = std::deque<ChangeEntry>;

/** @struct SensorEntry
 *
 *  SensorEntry is a unique key which maps a sensorEventType request in the
 *  PlatformEventMessage command to a host sensor PDR. This struct is a key
 *  in a std::map, so implemented operator==and operator<.
 */
struct SensorEntry
{
    pdr::TerminusID terminusID;
    pdr::SensorID sensorID;

    bool operator==(const SensorEntry& e) const
    {
        return ((terminusID == e.terminusID) && (sensorID == e.sensorID));
    }

    bool operator<(const SensorEntry& e) const
    {
        return ((terminusID < e.terminusID) ||
                ((terminusID == e.terminusID) && (sensorID < e.sensorID)));
    }
};

using HostStateSensorMap = std::map<SensorEntry, pdr::SensorInfo>;
using PDRList = std::vector<std::vector<uint8_t>>;

/** @class HostPDRHandler
 *  @brief This class can fetch and process PDRs from host firmware
 *  @details Provides an API to fetch PDRs from the host firmware. Upon
 *  receiving the PDRs, they are stored into the BMC's primary PDR repo.
 *  Adjustments are made to entity association PDRs received from the host,
 *  because they need to be assimilated into the BMC's entity association
 *  tree. A PLDM event containing the record handles of the updated entity
 *  association PDRs is sent to the host.
 */
class HostPDRHandler
{
  public:
    HostPDRHandler() = delete;
    HostPDRHandler(const HostPDRHandler&) = delete;
    HostPDRHandler(HostPDRHandler&&) = delete;
    HostPDRHandler& operator=(const HostPDRHandler&) = delete;
    HostPDRHandler& operator=(HostPDRHandler&&) = delete;
    ~HostPDRHandler() = default;

    using TerminusInfo =
        std::tuple<pdr::TerminusID, pdr::EID, pdr::TerminusValidity>;
    using TLPDRMap = std::map<pdr::TerminusHandle, TerminusInfo>;

    /** @brief Constructor
     *  @param[in] mctp_fd - fd of MCTP communications socket
     *  @param[in] mctp_eid - MCTP EID of host firmware
     *  @param[in] event - reference of main event loop of pldmd
     *  @param[in] repo - pointer to BMC's primary PDR repo
     *  @param[in] eventsJsonDir - directory path which has the config JSONs
     *  @param[in] entityTree - Pointer to BMC and Host entity association tree
     *  @param[in] bmcEntityTree - pointer to BMC's entity association tree
     *  @param[in] requester - reference to Requester object
     *  @param[in] handler - PLDM request handler
     */
    explicit HostPDRHandler(
        int mctp_fd, uint8_t mctp_eid, sdeventplus::Event& event,
        pldm_pdr* repo, const std::string& eventsJsonsDir,
        pldm_entity_association_tree* entityTree,
        pldm_entity_association_tree* bmcEntityTree,
        pldm::host_effecters::HostEffecterParser* hostEffecterParser,
        pldm::dbus_api::Requester& requester,
        pldm::requester::Handler<pldm::requester::Request>* handler,
        pldm::host_associations::HostAssociationsParser* asscoationsParser,
        pldm::responder::oem_platform::Handler* oemPlatformHandler);

    /** @brief fetch PDRs from host firmware. See @class.
     *  @param[in] recordHandles - list of record handles pointing to host's
     *             PDRs that need to be fetched.
     */

    void fetchPDR(PDRRecordHandles&& recordHandles);

    void deletePDRFromRepo(PDRRecordHandles&& recordHandles);

    /** @brief Send a PLDM event to host firmware containing a list of record
     *  handles of PDRs that the host firmware has to fetch.
     *  @param[in] pdrTypes - list of PDR types that need to be looked up in the
     *                        BMC repo
     *  @param[in] eventDataFormat - format for PDRRepositoryChgEvent in DSP0248
     */
    void sendPDRRepositoryChgEvent(std::vector<uint8_t>&& pdrTypes,
                                   uint8_t eventDataFormat);

    /** @brief Lookup host sensor info corresponding to requested SensorEntry
     *
     *  @param[in] entry - TerminusID and SensorID
     *
     *  @return SensorInfo corresponding to the input paramter SensorEntry
     *          throw std::out_of_range exception if not found
     */
    const pdr::SensorInfo& lookupSensorInfo(const SensorEntry& entry) const
    {
        return sensorMap.at(entry);
    }

    /** @brief Handles state sensor event
     *
     *  @param[in] stateSetId - state set Id
     *  @param[in] entry - state sensor entry
     *  @param[in] state - event state
     *
     *  @return PLDM completion code
     */
    int handleStateSensorEvent(
        const std::vector<pldm::pdr::StateSetId>& stateSetId,
        const pldm::responder::events::StateSensorEntry& entry,
        pdr::EventState state);

    /** @brief Parse state sensor PDRs and populate the sensorMap lookup data
     *         structure
     *
     *  @param[in] stateSensorPDRs - host state sensor PDRs

     *
     */
    void parseStateSensorPDRs();

    /** @brief this function sends a GetPDR request to Host firmware.
     *  And processes the PDRs based on type
     *
     *  @param[in] - nextRecordHandle - the next record handle to ask for
     */
    void getHostPDR(uint32_t nextRecordHandle = 0);

    /** @brief set the Host firmware condition when pldmd starts
     */
    void setHostFirmwareCondition();

    /** @brief set HostSensorStates when pldmd starts or restarts
     *  and updates the D-Bus property
     *  @param[in] stateSensorPDRs - host state sensor PDRs

     */
    void setHostSensorState();
    void _setHostSensorState();

    /** @brief check whether Host is running when pldmd starts
     */
    bool isHostUp();

    /** @brief whether we received PLDM_RECORDS_MODIFIED event data operation
     *  from host
     */
    bool isHostPdrModified = false;

    /** @brief map that captures various terminus information **/
    TLPDRMap tlPDRInfo;

  private:
    /** @brief set the FRU presence based on the host off signal
     */
    void setPresenceFrus();

    /* @brief get the Parent chassis object path for a fru
     */
    std::string getParentChassis(const std::string& fruPath);

    /** @brief set the presence of the fru from record handle
     *  @param[in] recorHandle - record handle of the PDR
     */
    void setRecordPresent(uint32_t recorHandle);

    /** @brief deferred function to fetch PDR from Host, scheduled to work on
     *  the event loop. The PDR exchg with the host is async.
     *  @param[in] source - sdeventplus event source
     */
    void _fetchPDR(sdeventplus::source::EventBase& source);

    /** @brief Merge host firmware's entity association PDRs into BMC's
     *  @details A merge operation involves adding a pldm_entity under the
     *  appropriate parent, and updating container ids.
     *  @param[in] pdr - entity association pdr
     */
    void mergeEntityAssociations(const std::vector<uint8_t>& pdr);

    /** @brief process the Host's PDR and add to BMC's PDR repo
     *  @param[in] eid - MCTP id of Host
     *  @param[in] response - response from Host for GetPDR
     *  @param[in] respMsgLen - response message length
     */
    void processHostPDRs(mctp_eid_t eid, const pldm_msg* response,
                         size_t respMsgLen);

    /** @brief send PDR Repo change after merging Host's PDR to BMC PDR repo
     *  @param[in] source - sdeventplus event source
     */
    void _processPDRRepoChgEvent(sdeventplus::source::EventBase& source);

    /** @brief fetch the next PDR based on the record handle sent by Host
     *  @param[in] nextRecordHandle - next record handle
     *  @param[in] source - sdeventplus event source
     */
    void _processFetchPDREvent(uint32_t nextRecordHandle,
                               sdeventplus::source::EventBase& source);

    /** @brief Get FRU record table metadata by host
     *
     *  @param[out] uint16_t    - total table records
     */
    void getFRURecordTableMetadataByHost(const PDRList& fruRecordSetPDRs);

    /** @brief Set Location Code in the dbus objects
     *
     *  @param[in] fruRecordSetPDRs - the Fru Record set PDR's
     *  @param[in] fruRecordData - the Fru Record Data
     */

    void setLocationCode(
        const PDRList& fruRecordSetPDRs,
        const std::vector<responder::pdr_utils::FruRecordDataFormat>&
            fruRecordData);
    void setFRUDynamicAssociations();

    /** @brief Get FRU record table by host
     *
     *  @return
     */
    void getFRURecordTableByHost(uint16_t& total,
                                 const PDRList& fruRecordSetPDRs);

    /** @brief Create DBUS objects
     *
     * @ return
     */
    void createDbusObjects(const PDRList& fruRecordSetPDRs);

    /** @brief Get FRU Record Set Identifier from FRU Record data Format
     *  @param[in] fruRecordSetPDRs - fru record set pdr
     *  @param[in] entity           - PLDM entity information
     *  @return
     */
    uint16_t getRSI(const PDRList& fruRecordSetPDRs, const pldm_entity& entity);

    /** @brief Get present state from state sensor readings
     *  @param[in] sensorId     - state sensor Id
     *  @param[in] type         - entity type
     *  @param[in] instance     - entity instance num
     *  @param[in] containerId  - entity container id
     *
     *  @param[out] state     - pldm operational fault status
     *  @param[in] path       - object path
     *  @param[in] stateSetId - state set Id
     */
    void getPresentStateBySensorReadigs(uint16_t sensorId, uint16_t type,
                                        uint16_t instance, uint16_t containerId,
                                        uint8_t state, const std::string& path,
                                        pldm::pdr::StateSetId stateSetId);

    /** @brief Set the OperationalStatus interface
     *  @return
     */
    void setOperationStatus();
    /** @brief Get the Validity of a Terminus ID
     *
     *  @param[out] bool - true if valid, false otherwise
     */
    bool getValidity(const pldm::pdr::TerminusID& tid);

    /** @brief Set the Present dbus Property
     *  @param[in] path     - object path
     *  @return
     */
    void setPresentPropertyStatus(const std::string& path);

    /** @brief Update the Led Group path
     *  @param[in] path     - object path
     *  @return
     */
    std::string updateLedGroupPath(const std::string& path);

    /** @brief fd of MCTP communications socket */
    int mctp_fd;
    /** @brief MCTP EID of host firmware */
    uint8_t mctp_eid;
    /** @brief reference of main event loop of pldmd, primarily used to schedule
     *  work.
     */
    sdeventplus::Event& event;

    /** @brief iterator to track the entries in the objPathMap */
    ObjectPathMaps::iterator objMapIndex;

    HostStateSensorMap::iterator sensorMapIndex;

    /** @brief pointer to BMC's primary PDR repo, host PDRs are added here */
    pldm_pdr* repo;

    pldm::responder::events::StateSensorHandler stateSensorHandler;
    /** @brief Pointer to BMC's and Host's entity association tree */
    pldm_entity_association_tree* entityTree;

    /** @brief Pointer to BMC's entity association tree */
    pldm_entity_association_tree* bmcEntityTree;

    /** @brief Pointer to host effecter parser */
    pldm::host_effecters::HostEffecterParser* hostEffecterParser;

    /** @brief reference to Requester object, primarily used to access API
     * to obtain PLDM instance id.
     */
    pldm::dbus_api::Requester& requester;

    /** @brief PLDM request handler */
    pldm::requester::Handler<pldm::requester::Request>* handler;
    pldm::host_associations::HostAssociationsParser* associationsParser;

    /** @brief sdeventplus event source */
    std::unique_ptr<sdeventplus::source::Defer> pdrFetchEvent;
    std::unique_ptr<sdeventplus::source::Defer> deferredFetchPDREvent;
    std::unique_ptr<sdeventplus::source::Defer> deferredPDRRepoChgEvent;

    /** @brief list of PDR record handles pointing to host's PDRs */
    PDRRecordHandles pdrRecordHandles;

    /** @brief list of PDR record handles modified pointing to host's PDRs */
    PDRRecordHandles modifiedPDRRecordHandles;

    /** @brief maps an entity type to parent pldm_entity from the BMC's entity
     *  association tree
     */
    std::map<EntityType, pldm_entity> parents;
    /** @brief D-Bus property changed signal match */
    std::unique_ptr<sdbusplus::bus::match::match> hostOffMatch;

    /** @brief D-Bus property changed signal match */
    std::unique_ptr<sdbusplus::bus::match::match> chassisOffMatch;

    /** @brief sensorMap is a lookup data structure that is build from the
     *         hostPDR that speeds up the lookup of <TerminusID, SensorID> in
     *         PlatformEventMessage command request.
     */
    HostStateSensorMap sensorMap;

    PDRList::const_iterator sensorIndex;
    PDRList stateSensorPDRs;
    /** @brief whether response received from Host */
    bool responseReceived;

    /** @brief veriable that captures if the first entity association PDR
     *         from host is merged into the BMC tree
     */
    bool mergedHostParents;

    /** @brief whether timed out waiting for a response from Host */
    bool timeOut;
    /** @brief request message instance id */
    uint8_t insId;

    /** @brief maps an object path to pldm_entity from the BMC's entity
     *         association tree
     */
    ObjectPathMaps objPathMap;

    /** @brief maps an entity name to map, maps to entity name to pldm_entity
     */
    EntityAssociations entityAssociations;

    /** @brief the vector of FRU Record Data Format
     */
    std::vector<responder::pdr_utils::FruRecordDataFormat> fruRecordData;

    /** @OEM platform handler */
    pldm::responder::oem_platform::Handler* oemPlatformHandler;

    /** @brief Object path and entity association and is only loaded once
     */
    bool objPathEntityAssociation;
};

} // namespace pldm
