include_directories( ${CMAKE_SOURCE_DIR}/libkdegames  )

set(kbreakout_SRCS 
   main.cpp
   mainwindow.cpp 
   canvasitems.cpp
   canvaswidget.cpp
   textitems.cpp
   item.cpp
   gift.cpp
   brick.cpp
   ball.cpp
   gameengine.cpp
   levelloader.cpp
   fontutils.cpp
   #generalsettings.cpp
)

kde4_add_ui_files(kbreakout_SRCS generalsettings.ui)

kde4_add_kcfg_files(kbreakout_SRCS settings.kcfgc )

kde4_add_app_icon(kbreakout_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../pics/hi*-app-kbreakout.png")

kde4_add_executable(kbreakout ${kbreakout_SRCS})

target_link_libraries(kbreakout kdegames kdegamesprivate ${KDE4_KDEUI_LIBS} ${QT_QTXML_LIBRARY})

install(TARGETS kbreakout  ${INSTALL_TARGETS_DEFAULT_ARGS} )
install(FILES kbreakoutui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kbreakout)
