if ( NOT CONTAINERS_PATH )
    message( STATUS "Path to Luos containers not set!" )
    return()
endif ()

set( CONTAINERS_COMMON_PATH "${CONTAINERS_PATH}/common" )
set( LUOS_MESH_MODELS_PATH  "${CONTAINERS_COMMON_PATH}/mesh_models" )

add_subdirectory("common/mesh_models")

add_subdirectory("led_toggler")
add_subdirectory("rtb_builder")
add_subdirectory("gate")

add_subdirectory("mesh_provisioner")
add_subdirectory("mesh_bridge")
