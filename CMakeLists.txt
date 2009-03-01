project(kdegames)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)

# Integration with the GGZ Gaming Zone
include(GGZ)
set(GGZ_REGISTRYNAME "kdegames")

macro_log_feature(GGZCONFIG_FOUND "ggz-client-libs" "GGZ configuration program"	"http://www.ggzgamingzone.org/" FALSE "" "Needed to support registering KDE games with the GGZ Gaming Zone for online gaming.")

macro_optional_find_package(QCA2)
macro_log_feature(QCA2_FOUND "QCA2" "Qt Cryptographic Architecture" "http://delta.affinix.com/qca" FALSE "2.0.0" "Needed to build Ksirk Jabber support's GroupWise and Jabberprotocols.")

macro_optional_find_package(OpenGL)
macro_log_feature(OPENGL_FOUND "OpenGL" "API for developing portable, interactive 2D and 3Dgraphics applications" "http://mesa3d.sourceforge.net" FALSE "" "Kubrick will not be built and KSudoku will not have Roxdoku support without OpenGL.")
macro_log_feature(QT_QTOPENGL_FOUND "QtOpenGL" "Qt module that make it easy to use OpenGL in Qt apps" "http://doc.trolltech.com/latest/qtopengl.html" FALSE "" "Kubrick will not be built and KSudoku will not have Roxdoku support without QtOpenGL.")

if(OPENGL_FOUND AND QT_QTOPENGL_FOUND)
    set(OPENGL_SUPPORT true)
endif(OPENGL_FOUND AND QT_QTOPENGL_FOUND)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories ( ${QDBUS_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})
# add the includes for libkdegames and the fwd headers, available for all the applications
include_directories(${CMAKE_SOURCE_DIR}/libkdegames ${CMAKE_SOURCE_DIR}/libkdegames/includes)
set(HIGHSCORE_DIRECTORY "" CACHE STRING "Where to install system-wide highscores e.g. /var/games")

if(WIN32)
    # detect oxygen icon dir at configure time based on KDEDIRS - there may be different package installation locations
    execute_process(COMMAND "${KDE4_KDECONFIG_EXECUTABLE}" --path icon OUTPUT_VARIABLE _dir ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(TO_CMAKE_PATH "${_dir}" __dir)
    find_path(KDE4_ICON_DIR oxygen PATHS
        ${__dir}
    )
    message(STATUS "using oxygen application icons from ${KDE4_ICON_DIR}")
else(WIN32)
    set (KDE4_ICON_DIR  ${CMAKE_INSTALL_PREFIX}/share/icons)
endif(WIN32)

macro_optional_add_subdirectory(doc)
add_subdirectory(libkdegames)
add_subdirectory(libkmahjongg)

macro_optional_add_subdirectory(bomber)
macro_optional_add_subdirectory(bovo)
macro_optional_add_subdirectory(lskat)
macro_optional_add_subdirectory(kapman)
macro_optional_add_subdirectory(katomic)
macro_optional_add_subdirectory(kbattleship)
macro_optional_add_subdirectory(kblackbox)
macro_optional_add_subdirectory(kblocks)
macro_optional_add_subdirectory(kbounce)
macro_optional_add_subdirectory(kbreakout)
macro_optional_add_subdirectory(kdiamond)
macro_optional_add_subdirectory(kfourinline)
macro_optional_add_subdirectory(kgoldrunner)
macro_optional_add_subdirectory(killbots)
macro_optional_add_subdirectory(kiriki)
macro_optional_add_subdirectory(kjumpingcube)
macro_optional_add_subdirectory(klines)
macro_optional_add_subdirectory(kmahjongg)
macro_optional_add_subdirectory(kmines)
macro_optional_add_subdirectory(knetwalk)
macro_optional_add_subdirectory(kolf)
macro_optional_add_subdirectory(kollision)
macro_optional_add_subdirectory(konquest)
macro_optional_add_subdirectory(kpat)
macro_optional_add_subdirectory(kreversi)
macro_optional_add_subdirectory(ksame)
macro_optional_add_subdirectory(kshisen)
if(QCA2_FOUND)
    macro_optional_add_subdirectory(ksirk)
endif(QCA2_FOUND)
macro_optional_add_subdirectory(kspaceduel)
macro_optional_add_subdirectory(ksquares)
macro_optional_add_subdirectory(ksudoku)
macro_optional_add_subdirectory(ktuberling)
macro_optional_add_subdirectory(ktron)
if(OPENGL_SUPPORT)
    macro_optional_add_subdirectory(kubrick)
endif(OPENGL_SUPPORT)

add_subdirectory(cmake)

macro_display_feature_log()
