if ( NOT CONTAINERS_PATH )
    message( STATUS "Path to Luos containers not set!" )
    return()
endif ()

set( RTB_BUILDER_PATH   "${CONTAINERS_PATH}/rtb_builder" )

add_library( rtb_builder
    "${RTB_BUILDER_PATH}/rtb_builder.c"
)

target_link_libraries( rtb_builder PUBLIC
    luos
)

target_include_directories( rtb_builder PUBLIC
    "${RTB_BUILDER_PATH}"
)
