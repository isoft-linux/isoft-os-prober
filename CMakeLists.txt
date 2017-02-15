project(isoftosprober)
cmake_minimum_required(VERSION 2.8.12)

set(PROJECT_NAME "isoftosprober")
set(PROJECT_VERSION "1.0.0")
set(GETTEXT_PACKAGE "isoftosprober")
set(PROJECT_LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)

add_definitions(-Werror)
add_definitions("-DPROJECT_NAME=\"${PROJECT_NAME}\"")
add_definitions("-DPROJECT_VERSION=\"${PROJECT_VERSION}\"")
add_definitions("-DGETTEXT_PACKAGE=\"${GETTEXT_PACKAGE}\"")
add_definitions("-DPROJECT_LOCALEDIR=\"${PROJECT_LOCALEDIR}\"")

option(ENABLE_DEBUG "Enable debug" OFF)
if(ENABLE_DEBUG)
    add_definitions(-g -DDEBUG)
endif()

if(NOT DEFINED CMAKE_INSTALL_SYSCONFDIR)
    set(CMAKE_INSTALL_SYSCONFDIR "/etc")
endif()

find_package(PkgConfig REQUIRED)

if(NOT NO_SYSTEMD)
    pkg_check_modules(SYSTEMD "systemd")
endif()

if(SYSTEMD_FOUND)
    add_definitions(-DHAVE_SYSTEMD)
    set(CMAKE_AUTOMOC_MOC_OPTIONS -DHAVE_SYSTEMD)

    if (NOT DEFINED SYSTEMD_SYSTEM_UNIT_DIR)
        execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=systemdsystemunitdir systemd OUTPUT_VARIABLE SYSTEMD_SYSTEM_UNIT_DIR)
        string(REGEX REPLACE "[ \t\n]+" "" SYSTEMD_SYSTEM_UNIT_DIR ${SYSTEMD_SYSTEM_UNIT_DIR})
    endif()
endif()

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GIO2 REQUIRED gio-2.0)
pkg_check_modules(GIOUNIX REQUIRED gio-unix-2.0)

find_program(GDBUS_CODEGEN_EXECUTABLE NAMES gdbus-codegen DOC "gdbus-codegen executable")
if(NOT GDBUS_CODEGEN_EXECUTABLE)
    message(FATAL_ERROR "Executable gdbus-codegen not found")
endif()

include(GNUInstallDirs)
include(FeatureSummary)

add_subdirectory(daemon)
add_subdirectory(data)
add_subdirectory(po)
if(ENABLE_DEBUG)
    add_subdirectory(test)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
