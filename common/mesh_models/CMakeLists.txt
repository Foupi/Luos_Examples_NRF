if ( NOT LUOS_MESH_MODELS_PATH )
    message( STATUS "Path to Luos Mesh models not set!" )
    return()
endif ()

add_subdirectory("luos_rtb_model")
