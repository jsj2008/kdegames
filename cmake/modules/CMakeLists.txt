# install the cmake files

set(cmakeFiles
	FindLibKDEGames.cmake
)

install(FILES ${cmakeFiles} DESTINATION ${DATA_INSTALL_DIR}/cmake/modules )
