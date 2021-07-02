if ( NOT CONTAINERS_PATH )
    message( STATUS "Path to Luos containers not set!" )
    return()
endif ()

set( MESH_PROVISIONER_PATH  "${CONTAINERS_PATH}/mesh_provisioner" )

add_library( mesh_provisioner
    "${MESH_PROVISIONER_PATH}/src/mesh_provisioner.c"
    "${MESH_PROVISIONER_PATH}/src/provisioning.c"

    ${MESH_CORE_SOURCE_FILES}
    ${MESH_BEARER_SOURCE_FILES}
    ${PROV_COMMON_SOURCE_FILES}
    ${PROV_PROVISIONER_SOURCE_FILES}
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


target_include_directories( mesh_provisioner PUBLIC
    "${MESH_PROVISIONER_PATH}/include"

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
    DEBUG
    NRF52_SERIES
    NRF_MESH_LOG_ENABLE=NRF_LOG_USES_RTT
    CONFIG_APP_IN_CORE
)

target_link_libraries( mesh_provisioner PUBLIC
    #Luos
    luos

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
