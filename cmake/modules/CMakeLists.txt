# install the cmake files

file(GLOB cmakeFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.cmake")

install(FILES ${cmakeFiles} DESTINATION ${DATA_INSTALL_DIR}/cmake/modules )

