if ( NOT CONTAINERS_PATH )
    message( STATUS "Path to Luos containers not set!" )
    return()
endif ()

set( RTB_BUILDER_PATH   "${CONTAINERS_PATH}/rtb_builder" )

add_library( rtb_builder
    "${RTB_BUILDER_PATH}/rtb_builder.c"
)

add_compile_definitions(
    BSP_DEFINES_ONLY
    CONFIG_GPIO_AS_PINRESET
    DEBUG
)

target_link_libraries( rtb_builder PUBLIC
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
    # Drivers
    nrf5_nrfx_gpiote
    nrf5_nrfx_prs
    # Application
    nrf5_app_error
    nrf5_app_util_platform
    nrf5_app_timer
    nrf5_app_button
)

target_include_directories( rtb_builder PUBLIC
    "${RTB_BUILDER_PATH}"
)
