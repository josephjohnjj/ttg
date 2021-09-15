# Install script for directory: /home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world

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

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xworldx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/joseph/TTG/ttg/build/_deps/madness-build/src/madness/world/libMADworld.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so"
         OLD_RPATH "/usr/local/lib:/home/joseph/TTG/parsec_ttg/build/install/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xworldx" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xworldx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/madness/world" TYPE FILE FILES
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/info.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/print.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldam.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/future.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldmpi.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/world_task_queue.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/array_addons.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/stack.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/vector.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldgop.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/world_object.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/buffer_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/nodefaults.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/dependency_interface.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldhash.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldref.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldtypes.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/dqueue.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/parallel_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/vector_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/madness_exception.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldmem.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/thread.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldrmi.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/safempi.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldpapi.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldmutex.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/print_seq.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldhashmap.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/range.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/atomicint.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/posixmem.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldptr.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/deferred_cleanup.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/MADworld.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/world.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/uniqueid.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldprofile.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/timers.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/binary_fstream_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/mpi_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/text_fstream_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worlddc.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/mem_func_wrapper.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/taskfn.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/group.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/dist_cache.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/distributed_id.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/type_traits.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/function_traits.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/stubmpi.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/bgq_atomics.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/binsorter.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/parsec.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/meta.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldinit.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xworldx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/joseph/TTG/ttg/build/_deps/madness-build/src/madness/world/libMADworld.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so"
         OLD_RPATH "/usr/local/lib:/home/joseph/TTG/parsec_ttg/build/install/lib:"
         NEW_RPATH "")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libMADworld.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xworldx" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xworldx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/madness/world" TYPE FILE FILES
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/info.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/print.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldam.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/future.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldmpi.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/world_task_queue.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/array_addons.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/stack.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/vector.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldgop.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/world_object.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/buffer_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/nodefaults.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/dependency_interface.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldhash.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldref.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldtypes.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/dqueue.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/parallel_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/vector_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/madness_exception.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldmem.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/thread.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldrmi.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/safempi.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldpapi.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldmutex.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/print_seq.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldhashmap.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/range.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/atomicint.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/posixmem.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldptr.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/deferred_cleanup.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/MADworld.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/world.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/uniqueid.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldprofile.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/timers.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/binary_fstream_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/mpi_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/text_fstream_archive.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worlddc.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/mem_func_wrapper.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/taskfn.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/group.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/dist_cache.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/distributed_id.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/type_traits.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/function_traits.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/stubmpi.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/bgq_atomics.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/binsorter.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/parsec.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/meta.h"
    "/home/joseph/TTG/ttg/build/_deps/madness-src/src/madness/world/worldinit.h"
    )
endif()

