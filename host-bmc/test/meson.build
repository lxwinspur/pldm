host_bmc_test_src = declare_dependency(
          sources: [
            '../dbus_to_host_effecters.cpp',
            '../../pldmd/dbus_impl_requester.cpp',
            '../../pldmd/instance_id.cpp'],
            include_directories: '../../requester')

test_sources = [
  '../utils.cpp',
  '../dbus/associations.cpp',
  '../dbus/availability.cpp',
  '../dbus/chassis.cpp',
  '../dbus/cpu_core.cpp',
  '../dbus/enable.cpp',
  '../dbus/inventory_item.cpp',
  '../dbus/operational_status.cpp',
  '../dbus/pcie_slot.cpp',
  '../dbus/license_entry.cpp',
  '../dbus/location_code.cpp',
  '../dbus/led_group.cpp',
  '../dbus/serialize.cpp',
  '../dbus/custom_dbus.cpp',
]

tests = [
  'dbus_to_host_effecter_test',
  'utils_test',
  'custom_dbus_test',
]

foreach t : tests
  test(t, executable(t.underscorify(), t + '.cpp',
                     test_sources,
                     implicit_include_directories: false,
                     link_args: dynamic_linker,
                     build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
                     dependencies: [
                         gtest,
                         gmock,
                         host_bmc_test_src,
                         libpldm_dep,
                         libpldmutils,
                         nlohmann_json,
                         phosphor_dbus_interfaces,
                         sdbusplus,
                         sdeventplus]),
       workdir: meson.current_source_dir())
endforeach
