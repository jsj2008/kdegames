include_directories(${KDE4_INCLUDES} ${KDEGAMES_INCLUDE_DIR})

set(kigo_SRCS
    game/game.cpp
    game/move.cpp
    game/player.cpp
    game/score.cpp
    game/stone.cpp
    gui/config/generalconfig.cpp
    gui/mainwindow.cpp
    gui/graphicsview/gamescene.cpp
    gui/graphicsview/gameview.cpp
    gui/graphicsview/themerenderer.cpp
    gui/widgets/errorwidget.cpp
    gui/widgets/gamewidget.cpp
    gui/widgets/setupwidget.cpp
    main.cpp 
)

kde4_add_ui_files(kigo_SRCS 
    gui/config/generalconfig.ui
    gui/widgets/errorwidget.ui
    gui/widgets/gamewidget.ui
    gui/widgets/setupwidget.ui
)
kde4_add_kcfg_files(kigo_SRCS preferences.kcfgc)
kde4_add_app_icon(kigo_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../data/icons/hi*-app-kigo.png")
kde4_add_executable(kigo ${kigo_SRCS})

target_link_libraries(kigo ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} kdegames kdegamesprivate ${KDE4_KNEWSTUFF3_LIBS})# ${KGGZGAMES_LIBS})

install(TARGETS kigo ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES kigo.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(FILES kigo.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES kigo-games.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
install(PROGRAMS gui/kigo.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES gui/kigoui.rc DESTINATION ${DATA_INSTALL_DIR}/kigo)

#register_ggz_module(module.dsc)
