project( kshisen )

if(NOT COMPILING_KDEGAMES)
	find_package(KDE4 REQUIRED)
	include(KDE4Defaults)
	include(MacroLibrary)
	find_package(KDEGames REQUIRED)

	set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
	find_package(LibKMahjongg REQUIRED)

	add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
	add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
	include_directories(${QDBUS_INCLUDE_DIRS} ${KDE4_INCLUDES} ${KDEGAMES_INCLUDE_DIRS} ${LIBKMAHJONGG_INCLUDE_DIR})
else(NOT COMPILING_KDEGAMES)
	set(LIBKMAHJONGG_LIBRARY kmahjongglib)
endif(NOT COMPILING_KDEGAMES)

add_subdirectory( src )
add_subdirectory( sounds )

install( PROGRAMS kshisen.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR} )
