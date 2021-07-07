if ( NOT CONTAINERS_PATH )
    message( STATUS "Path to Luos containers not set!" )
    return()
endif ()

set( CONTAINERS_COMMON_PATH "${CONTAINERS_PATH}/common" )

add_subdirectory("rtb_builder")

add_subdirectory("mesh_provisioner")
add_subdirectory("mesh_bridge")
