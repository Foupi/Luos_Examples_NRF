if ( NOT LUOS_MESH_MODELS_PATH )
    message( STATUS "Path to Luos Mesh models not set!" )
    return()
endif ()

set( LUOS_MESH_MODELS_COMMON_PATH   "${LUOS_MESH_MODELS_PATH}/common" )

add_subdirectory("luos_rtb_model")
