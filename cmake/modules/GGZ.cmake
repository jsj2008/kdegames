# Installation of GGZ game modules and GGZ library checks
# Josef Spillner <josef@ggzgamingzone.org> 2006, 2007
# see install_pam_service for some comments on DESTDIR etc.

###########################################################
# Documentation
# - call register_ggz_module(module.dsc) in your game's CMakeLists.txt
# - set -DGGZ_NOREGISTRY=/usr/share/ggz/modules/kdegames when building binary packages
# - set -DGGZ_FORCEREGISTRY=1 to force registration in out-of-prefix installations

###########################################################
# Check for existence of 'ggz-config' tool

FIND_PROGRAM(GGZCONFIG_EXECUTABLE NAMES ggz-config)

if(GGZCONFIG_EXECUTABLE)
    set(GGZCONFIG_FOUND 1)
    MESSAGE(STATUS "GGZ: Success: ggz-config was found as ${GGZCONFIG_EXECUTABLE}.")
else(GGZCONFIG_EXECUTABLE)
    MESSAGE(STATUS "GGZ: Warning: ggz-config not found, game modules will not be registered!")
    MESSAGE(STATUS " Source package containing ggz-config: 'ggz-client-libs'")
    MESSAGE(STATUS " Binary package containing ggz-config: 'ggzcore-bin' (or similar)")
endif(GGZCONFIG_EXECUTABLE)

###########################################################
# Installation hook for module.dsc files
# Documentation is in GGZ SVN under docs/ggz-project/packagers

# FIXME: improve cmake to allow typed options
#        option(GGZ_NOREGISTRY PATH "Directory where to install
#        the module.dsc files to")
# FIXME: Bug in cmake? REG needs to be declared whenever
#        -DGGZ_NOREGISTRY is given. However, why is that, especially since
#        the error occurs in the 'else' tree which isn't used in that case.

if(GGZCONFIG_FOUND)
    if(GGZ_NOREGISTRY)
        set(REG "" STRING)
        set(NOREG "--noregistry=${GGZ_NOREGISTRY}")
        MESSAGE(STATUS "GGZ: Installing game module information to ${GGZ_NOREGISTRY}.")
        MESSAGE(STATUS "GGZ: Post-registration hooks need to be called by distributors.")
    else(GGZ_NOREGISTRY)
        EXECUTE_PROCESS(COMMAND ${GGZCONFIG_EXECUTABLE} -c OUTPUT_VARIABLE REG)
        STRING(REPLACE "\n" "" REG "${REG}")
        if(SYSCONF_INSTALL_DIR STREQUAL "${REG}")
            MESSAGE(STATUS "GGZ: Registering game module information into ${REG}.")
        else(SYSCONF_INSTALL_DIR STREQUAL "${REG}")
            if(GGZ_FORCEREGISTRY)
              MESSAGE(STATUS "GGZ: Forced registration into ${REG}.")
            else(GGZ_FORCEREGISTRY)
              set(SKIP_GGZCONFIG TRUE BOOL)
              MESSAGE(STATUS "GGZ: Warning: Out-of-prefix installation, no registration unless GGZ_FORCEREGISTRY is given.")
              MESSAGE(STATUS " KDE Games configuration gets installed into: ${SYSCONF_INSTALL_DIR}")
              MESSAGE(STATUS " GGZ configuration is expected in: ${REG}")
            endif(GGZ_FORCEREGISTRY)
        endif(SYSCONF_INSTALL_DIR STREQUAL "${REG}")
    endif(GGZ_NOREGISTRY)
else(GGZCONFIG_FOUND)
    set(SKIP_GGZCONFIG TRUE BOOL)
endif(GGZCONFIG_FOUND)

if(SKIP_GGZCONFIG)
    macro(register_ggz_module MODFILE)
    endmacro(register_ggz_module)
    MESSAGE(STATUS "GGZ: Warning: KDE games will not be found by GGZ clients.")
else(SKIP_GGZCONFIG)
    macro(register_ggz_module MODFILE)
        install(CODE "
            exec_program(${GGZCONFIG_EXECUTABLE} ARGS --install -D --force ${NOREG} --modfile=${CMAKE_CURRENT_SOURCE_DIR}/${MODFILE})
        ")
    endmacro(register_ggz_module)
endif(SKIP_GGZCONFIG)

###########################################################
# Specify the location and dependencies of the GGZ libraries within kdegames

set(KGGZGAMES_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libkdegames)
set(KGGZGAMES_LIBS kggzgames kggzmod kggznet)
set(KGGZMOD_LIBS kggzmod kggznet)
set(KGGZNET_LIBS kggznet)

