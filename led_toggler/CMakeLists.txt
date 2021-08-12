if ( NOT CONTAINERS_PATH )
    message( STATUS "Path to Luos containers not set!" )
    return()
endif ()

set( LED_TOGGLER_PATH   "${CONTAINERS_PATH}/led_toggler" )

add_library( led_toggler
    "${LED_TOGGLER_PATH}/led_toggler.c"
)

target_include_directories( led_toggler PUBLIC
    "${LED_TOGGLER_PATH}"
)

add_compile_definitions(
    BSP_DEFINES_ONLY
    CONFIG_GPIO_AS_PINRESET
    DEBUG
)

target_link_libraries( led_toggler PUBLIC
    # Luos
    luos
)
