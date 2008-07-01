project(ksame)

include_directories( ${CMAKE_SOURCE_DIR}/libkdegames/highscore  )


########### next target ###############

set(ksame_SRCS 
    main.cpp 
    mainwindow.cpp
    board.cpp
    view.cpp
    renderer.cpp
   )

kde4_add_app_icon(ksame_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app-ksame.png")
kde4_add_executable(ksame ${ksame_SRCS})

target_link_libraries(ksame  ${KDE4_KIO_LIBS} kdegames ${KDE4_KNOTIFYCONFIG_LIBS})

install(TARGETS ksame  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES ksame.desktop      DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES ksameui.rc         DESTINATION ${DATA_INSTALL_DIR}/ksame )
install( FILES default_theme.svgz DESTINATION ${DATA_INSTALL_DIR}/ksame/pics/ )
install( FILES old_theme.svgz	  DESTINATION ${DATA_INSTALL_DIR}/ksame/pics/ )

kde4_install_icons( ${ICON_INSTALL_DIR} )


