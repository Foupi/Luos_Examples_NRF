if ( NOT CONTAINERS_PATH )
    message( STATUS "Path to Luos containers not set!" )
    return()
endif ()

set( MESH_BRIDGE_PATH    "${CONTAINERS_PATH}/mesh_bridge" )

add_library( mesh_bridge
    "${MESH_BRIDGE_PATH}/src/mesh_bridge.c"
    "${MESH_BRIDGE_PATH}/src/provisioning.c"
)

target_include_directories( mesh_bridge PUBLIC
    "${MESH_BRIDGE_PATH}/include"
)

add_compile_definitions(
    BSP_DEFINES_ONLY
    CONFIG_GPIO_AS_PINRESET
    DEBUG
    NRF52_SERIES
    NRF_MESH_LOG_ENABLE=NRF_LOG_USES_RTT
    CONFIG_APP_IN_CORE
)

target_link_libraries( mesh_bridge PUBLIC
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
