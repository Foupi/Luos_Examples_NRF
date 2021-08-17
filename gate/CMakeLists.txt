if ( NOT CONTAINERS_PATH )
    message( STATUS "Path to Luos containers not set!" )
    return()
endif ()

if ( NOT CONTAINERS_COMMON_PATH )
    message( STATUS "Path to containers common codebase not set!" )
    return()
endif ()

set ( GATE_PATH "${CONTAINERS_PATH}/gate" )

add_library( gate
    "${GATE_PATH}/gate.c"

    "${GATE_PATH}/cmd.c"
    "${GATE_PATH}/convert.c"
    "${GATE_PATH}/json_mnger.c"

    "${GATE_PATH}/cJSON/cJSON.c"

    "${CONTAINERS_COMMON_PATH}/src/uart_helpers.c"
)

target_include_directories( gate PUBLIC
    "${GATE_PATH}"

    "${GATE_PATH}/cJSON"

    "${CONTAINERS_COMMON_PATH}/include"
)

add_compile_definitions(
    BSP_DEFINES_ONLY
    CONFIG_GPIO_AS_PINRESET
    DEBUG
)

target_link_libraries( gate PUBLIC
    # Luos
    luos

    # NRF
    # Common
    nrf5_mdk
    nrf5_section
    nrf5_strerror
    nrf5_atomic
    nrf5_atfifo
    nrf5_atflags
    nrf5_memobj
    nrf5_balloc
    nrf5_ringbuf
    nrf5_pwr_mgmt
    # Drivers
    nrf5_nrfx_prs
    nrf5_nrfx_uarte
    nrf5_nrfx_uart
    nrf5_nrfx_systick
    nrf5_drv_uart
    # External
    nrf5_ext_fprintf
    nrf5_ext_segger_rtt
    # Logger
    nrf5_log
    nrf5_log_backend_serial
    nrf5_log_backend_rtt
    nrf5_log_default_backends
    # Application
    nrf5_app_error
    nrf5_app_util_platform
    nrf5_app_timer
    nrf5_app_fifo
    nrf5_app_uart_fifo
    # Flash storage
    nrf5_fstorage
    # BSP
    nrf5_boards
    nrf5_bsp_defs
    nrf5_sdh
    nrf5_fstorage_sd
)
