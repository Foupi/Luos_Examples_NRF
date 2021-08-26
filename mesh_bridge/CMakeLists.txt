if ( NOT CONTAINERS_PATH )
    message( STATUS "Path to Luos containers not set!" )
    return()
endif ()

if ( NOT CONTAINERS_COMMON_PATH )
    message( STATUS "Path to containers common codebase not set!" )
    return()
endif ()

set( MESH_BRIDGE_PATH    "${CONTAINERS_PATH}/mesh_bridge" )

add_library( mesh_bridge
    "${MESH_BRIDGE_PATH}/src/mesh_bridge.c"

    "${MESH_BRIDGE_PATH}/src/mesh_bridge_utils.c"

    "${MESH_BRIDGE_PATH}/src/data_struct/local_container_table.c"
    "${MESH_BRIDGE_PATH}/src/data_struct/luos_mesh_msg_queue.c"
    "${MESH_BRIDGE_PATH}/src/data_struct/remote_container_table.c"

    "${MESH_BRIDGE_PATH}/src/management/app_luos_msg_model.c"
    "${MESH_BRIDGE_PATH}/src/management/app_luos_rtb_model.c"
    "${MESH_BRIDGE_PATH}/src/management/mesh_msg_queue_manager.c"

    "${MESH_BRIDGE_PATH}/src/mesh/mesh_init.c"
    "${MESH_BRIDGE_PATH}/src/mesh/provisioning.c"

    "${CONTAINERS_COMMON_PATH}/src/luos_mesh_common.c"

    ${MESH_CORE_SOURCE_FILES}
    ${MESH_BEARER_SOURCE_FILES}
    ${PROV_COMMON_SOURCE_FILES}
    ${PROV_PROVISIONEE_SOURCE_FILES}
    ${PROV_PROVISIONER_SOURCE_FILES}    # prov_provisioner_auth_data
    ${PROV_BEARER_ADV_SOURCE_FILES}
    ${ACCESS_SOURCE_FILES}
    ${DFU_SOURCE_FILES}
    ${MESH_STACK_SOURCE_FILES}
    ${CONFIG_SERVER_SOURCE_FILES}
    ${HEALTH_SERVER_SOURCE_FILES}
    ${MESH_APP_TIMER_SOURCE_FILES}
    ${${PLATFORM}_SOURCE_FILES}
    ${${nRF5_SDK_VERSION}_SOURCE_FILES}
)

target_include_directories( mesh_bridge PUBLIC
    "${MESH_BRIDGE_PATH}/include"

    "${MESH_BRIDGE_PATH}/include/data_struct"
    "${MESH_BRIDGE_PATH}/include/management"
    "${MESH_BRIDGE_PATH}/include/mesh"

    "${CONTAINERS_COMMON_PATH}/include"

    ${MESH_INCLUDE_DIRS}
    ${CONFIG_SERVER_INCLUDE_DIRS}
    ${HEALTH_SERVER_INCLUDE_DIRS}
    ${${SOFTDEVICE}_INCLUDE_DIRS}
    ${${PLATFORM}_INCLUDE_DIRS}
    ${${BOARD}_INCLUDE_DIRS}
    ${${nRF5_SDK_VERSION}_INCLUDE_DIRS}
)

add_compile_definitions(
    BSP_DEFINES_ONLY
    CONFIG_GPIO_AS_PINRESET
    #DEBUG
    NRF52_SERIES
    NRF_MESH_LOG_ENABLE=NRF_LOG_USES_RTT
    CONFIG_APP_IN_CORE
)

target_link_libraries( mesh_bridge PUBLIC
    #Luos
    luos
    luos_config

    #Models
    luos_msg_model
    luos_rtb_model

    # NRF
    # External
    nrf5_ext_fprintf
    nrf5_ext_segger_rtt
    # Logger
    nrf5_log
    nrf5_log_backend_serial
    nrf5_log_backend_rtt
    nrf5_log_default_backends

    # Mesh
    uECC_${PLATFORM}
)
