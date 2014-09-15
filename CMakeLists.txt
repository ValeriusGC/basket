cmake_minimum_required(VERSION 2.8.11)

project(basket)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Gpgme)
find_package(Gpgmepp)

include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

IF(GPGME_FOUND)
  SET(HAVE_LIBGPGME 1)
  SET(LARGEFILE_SOURCE_1)
  ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)
ELSE(GPGME_FOUND)
  MESSAGE("GPG not found, configuring without")
ENDIF(GPGME_FOUND)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

#add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(po)
add_subdirectory(tags)
add_subdirectory(welcome)
add_subdirectory(backgrounds)
