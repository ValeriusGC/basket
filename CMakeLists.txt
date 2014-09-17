cmake_minimum_required(VERSION 2.8.11)

project(basket)

# Debug
set(CMAKE_BUILD_TYPE Debug)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc, uic automatically when needed.
set(CMAKE_AUTOMOC ON)

set(BASKET_INSTALL_DIR ${CMAKE_BINARY_DIR}/basket)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Gpgme)
find_package(Gpgmepp)

# Use system zlib on unix and Qt ZLIB on Windows
IF(UNIX)
        find_package(ZLIB REQUIRED)
ELSE(UNIX)
        SET(ZLIB_INCLUDE_DIRS "${QT_ROOT}/src/3rdparty/zlib" CACHE STRING "Path to ZLIB headers of Qt")
        SET(ZLIB_LIBRARIES "")
        IF(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
                MESSAGE("Please specify a valid zlib include dir")
        ENDIF(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
ENDIF(UNIX)

# if there is no QT_ROOT, try to deduce it from Qt QtCore include
if ("${QT_ROOT}" STREQUAL "")
    set(QT_ROOT ${QT_QTCORE_INCLUDE_DIR}/../..)
endif()

IF(GPGME_FOUND)
  SET(HAVE_LIBGPGME 1)
  SET(LARGEFILE_SOURCE_1)
  ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)
ELSE(GPGME_FOUND)
  MESSAGE("GPG not found, configuring without")
ENDIF(GPGME_FOUND)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

#add_subdirectory(doc)
add_subdirectory(po)
add_subdirectory(quazip)
add_subdirectory(src)
add_subdirectory(welcome)
add_subdirectory(backgrounds)
