deps = [
  phosphor_dbus_interfaces,
  nlohmann_json,
  sdbusplus,
  sdeventplus,
  libpldm_dep,
  libpldmutils
]

sources = [
  'base.cpp',
  'bios.cpp',
  'bios_table.cpp',
  'bios_attribute.cpp',
  'bios_string_attribute.cpp',
  'bios_integer_attribute.cpp',
  'bios_enum_attribute.cpp',
  'bios_config.cpp',
  'pdr_utils.cpp',
  'pdr.cpp',
  'platform.cpp',
  'fru_parser.cpp',
  'fru.cpp',
  '../host-bmc/host_pdr_handler.cpp',
  '../host-bmc/dbus_to_event_handler.cpp',
  '../host-bmc/dbus_to_host_effecters.cpp',
  '../host-bmc/host_associations_parser.cpp',
  '../host-bmc/host_condition.cpp',
  '../host-bmc/utils.cpp',
  '../host-bmc/dbus/custom_dbus.cpp',
  '../host-bmc/dbus/associations.cpp',
  '../host-bmc/dbus/availability.cpp',
  '../host-bmc/dbus/chassis.cpp',
  '../host-bmc/dbus/cpu_core.cpp',
  '../host-bmc/dbus/enable.cpp',
  '../host-bmc/dbus/inventory_item.cpp',
  '../host-bmc/dbus/led_group.cpp',
  '../host-bmc/dbus/license_entry.cpp',
  '../host-bmc/dbus/operational_status.cpp',
  '../host-bmc/dbus/pcie_slot.cpp',
  '../host-bmc/dbus/serialize.cpp',
  '../host-bmc/dbus/location_code.cpp',
  '../host-bmc/dbus/deserialize.cpp',
  'event_parser.cpp'
]

if get_option('oem-ibm').enabled()
  sources += [
    '../oem/ibm/libpldmresponder/utils.cpp',
    '../oem/ibm/libpldmresponder/file_io.cpp',
    '../oem/ibm/libpldmresponder/file_table.cpp',
    '../oem/ibm/libpldmresponder/file_io_by_type.cpp',
    '../oem/ibm/libpldmresponder/file_io_type_pel.cpp',
    '../oem/ibm/libpldmresponder/file_io_type_dump.cpp',
    '../oem/ibm/libpldmresponder/file_io_type_cert.cpp',
    '../oem/ibm/libpldmresponder/platform_oem_ibm.cpp',
    '../oem/ibm/libpldmresponder/fru_oem_ibm.cpp',
    '../oem/ibm/libpldmresponder/oem_ibm_handler.cpp',
    '../oem/ibm/libpldmresponder/inband_code_update.cpp',
    '../oem/ibm/libpldmresponder/collect_slot_vpd.cpp',
    '../oem/ibm/requester/dbus_to_file_handler.cpp',
    '../oem/ibm/libpldmresponder/file_io_type_progress_src.cpp',
    '../oem/ibm/libpldmresponder/file_io_type_lic.cpp',
    '../oem/ibm/host-bmc/host_lamp_test.cpp',
  ]
endif

libpldmresponder = library(
  'pldmresponder',
  sources,
  version: meson.project_version(),
  dependencies: deps,
  install: true)

libpldmresponder = declare_dependency(
  link_with: libpldmresponder)

if get_option('tests').enabled()
  subdir('test')
endif
