set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories(  ${CMAKE_SOURCE_DIR}/libkdegames )

MACRO(LIBKDEGAMES_EXECUTABLE_TESTS)
       FOREACH(_testname ${ARGN})
               set(${_testname}_SRCS ${_testname}.cpp)
               #if test has an ui file, use it
               IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_testname}.ui)
                  kde4_add_ui_files(${_testname}_SRCS ${_testname}.ui)
               ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_testname}.ui)

               kde4_add_executable(${_testname} TEST ${${_testname}_SRCS})
               target_link_libraries(${_testname} ${KDE4_KDECORE_LIBS} ${KDE4_KDEGAMES_LIBS} ${QT_QTTEST_LIBRARY} kdegames)
       ENDFOREACH(_testname)
ENDMACRO(LIBKDEGAMES_EXECUTABLE_TESTS)

if (Q_WS_X11)
LIBKDEGAMES_EXECUTABLE_TESTS(
#    kxerrorhandlertest # File missing from svn
#     kgamepopupitemtest
)
endif (Q_WS_X11)
