if ( NOT LUOS_MESH_MODELS_PATH )
    message( STATUS "Path to Luos Mesh models not set!" )
    return()
endif ()

if ( NOT CONTAINERS_COMMON_PATH )
    message( STATUS "Path to containers common codebase not set!" )
    return()
endif ()

set( LUOS_MSG_MODEL_PATH "${LUOS_MESH_MODELS_PATH}/luos_msg_model" )

add_library( luos_msg_model
    "${LUOS_MSG_MODEL_PATH}/src/luos_msg_model.c"

    ${MESH_CORE_SOURCE_FILES}
    ${ACCESS_SOURCE_FILES}
    ${CONFIG_SERVER_SOURCE_FILES}
    ${HEALTH_SERVER_SOURCE_FILES}
    ${${PLATFORM}_SOURCE_FILES}
    ${${nRF5_SDK_VERSION}_SOURCE_FILES}
)

target_include_directories( luos_msg_model PUBLIC
    "${LUOS_MSG_MODEL_PATH}/include"

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
    DEBUG
    NRF52_SERIES
    NRF_MESH_LOG_ENABLE=NRF_LOG_USES_RTT
    CONFIG_APP_IN_CORE
)

target_link_libraries( luos_msg_model PUBLIC
    # Luos
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
