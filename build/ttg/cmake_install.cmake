# Install script for directory: /home/joseph/TTG/ttg/ttg

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xttgx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/home/joseph/TTG/ttg/ttg/ttg.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xttgx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ttg" TYPE FILE FILES
    "/home/joseph/TTG/ttg/ttg/ttg/impl_selector.h"
    "/home/joseph/TTG/ttg/ttg/ttg/broadcast.h"
    "/home/joseph/TTG/ttg/ttg/ttg/edge.h"
    "/home/joseph/TTG/ttg/ttg/ttg/execution.h"
    "/home/joseph/TTG/ttg/ttg/ttg/func.h"
    "/home/joseph/TTG/ttg/ttg/ttg/op.h"
    "/home/joseph/TTG/ttg/ttg/ttg/reduce.h"
    "/home/joseph/TTG/ttg/ttg/ttg/runtimes.h"
    "/home/joseph/TTG/ttg/ttg/ttg/terminal.h"
    "/home/joseph/TTG/ttg/ttg/ttg/traverse.h"
    "/home/joseph/TTG/ttg/ttg/ttg/world.h"
    "/home/joseph/TTG/ttg/ttg/ttg/wrap.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xttgx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ttg/util" TYPE FILE FILES
    "/home/joseph/TTG/ttg/ttg/ttg/util/backtrace.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/bug.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/demangle.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/dot.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/future.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/hash.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/macro.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/meta.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/print.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/trace.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/tree.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/version.h"
    "/home/joseph/TTG/ttg/ttg/ttg/util/void.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xttgx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ttg/base" TYPE FILE FILES
    "/home/joseph/TTG/ttg/ttg/ttg/base/keymap.h"
    "/home/joseph/TTG/ttg/ttg/ttg/base/op.h"
    "/home/joseph/TTG/ttg/ttg/ttg/base/terminal.h"
    "/home/joseph/TTG/ttg/ttg/ttg/base/world.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xttg-madx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ttg/madness" TYPE FILE FILES
    "/home/joseph/TTG/ttg/ttg/ttg/madness/ttg.h"
    "/home/joseph/TTG/ttg/ttg/ttg/madness/watch.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xttg-parsecx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ttg/parsec" TYPE FILE FILES "/home/joseph/TTG/ttg/ttg/ttg/parsec/ttg.h")
endif()

