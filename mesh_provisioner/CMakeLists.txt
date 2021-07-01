if ( NOT CONTAINERS_PATH )
    message( STATUS "Path to Luos containers not set!" )
    return()
endif ()

set( MESH_PROVISIONER_PATH  "${CONTAINERS_PATH}/mesh_provisioner" )

add_library( mesh_provisioner
    "${MESH_PROVISIONER_PATH}/mesh_provisioner.c"
    "${MESH_PROVISIONER_PATH}/provisioning.c"
)

add_compile_definitions(
    BSP_DEFINES_ONLY
    CONFIG_GPIO_AS_PINRESET
    DEBUG
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
)

target_include_directories( mesh_provisioner PUBLIC
    "${MESH_PROVISIONER_PATH}"
)
