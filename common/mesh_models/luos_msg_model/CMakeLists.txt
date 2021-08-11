if ( NOT LUOS_MESH_MODELS_PATH )
    message( STATUS "Path to Luos Mesh models not set!" )
    return()
endif ()

if ( NOT LUOS_MESH_MODELS_COMMON_PATH )
    message( STATUS "Path to Luos Mesh models common codebase not set!" )
    return()
endif ()

if ( NOT CONTAINERS_COMMON_PATH )
    message( STATUS "Path to containers common codebase not set!" )
    return()
endif ()

set( LUOS_MSG_MODEL_PATH "${LUOS_MESH_MODELS_PATH}/luos_msg_model" )

add_library( luos_msg_model
    "${LUOS_MSG_MODEL_PATH}/src/luos_msg_model.c"

    # FIXME Mesh sources
)

target_include_directories( luos_msg_model PUBLIC
    "${LUOS_MSG_MODEL_PATH}/include"
    "${LUOS_MESH_MODELS_COMMON_PATH}/include"

    # FIXME Mesh includes
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
    #Luos
    luos

    # FIXME Log libraries

    # FIXME uECC
)
