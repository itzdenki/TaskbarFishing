# Do not overwrite Discord configuration customized by the player.
get_filename_component(CONFIG_NAME "${SOURCE}" NAME)
if(NOT EXISTS "${DESTINATION}/${CONFIG_NAME}")
    file(COPY "${SOURCE}" DESTINATION "${DESTINATION}")
endif()
