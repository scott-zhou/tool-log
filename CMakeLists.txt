########################################################################
# CMake build script for UnitTest.
#

# BUILD_SHARED_LIBS is a standard CMake variable, but we declare it here to
# make it prominent in the GUI.
option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)." OFF)

# Defines the compiler/linker flags used to build Google Test and
# Google Mock.  You can tweak these definitions to suit your need.  A
# variable's value is empty before it's explicitly assigned to.
macro(config_compiler_and_linker)
  if (NOT disable_pthreads)
    # Defines CMAKE_USE_PTHREADS_INIT and CMAKE_THREAD_LIBS_INIT.
    find_package(Threads)
  endif()

  if (CMAKE_COMPILER_IS_GNUCXX)
    set(cxx_base_flags "-Wall -Wshadow")
    set(cxx_exception_flags "-fexceptions")
    set(cxx_no_exception_flags "-fno-exceptions")
    # Until version 4.3.2, GCC doesn't define a macro to indicate
    # whether RTTI is enabled.  
    set(cxx_no_rtti_flags "-fno-rtti")
    set(cxx_strict_flags
      "-Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "SunPro")
    set(cxx_exception_flags "-features=except")
    # Sun Pro doesn't provide macros to indicate whether exceptions and
    # RTTI are enabled.
    set(cxx_no_exception_flags "-features=no%except")
    set(cxx_no_rtti_flags "-features=no%rtti")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "VisualAge" OR
      CMAKE_CXX_COMPILER_ID STREQUAL "XL")
    # CMake 2.8 changes Visual Age's compiler ID to "XL".
    set(cxx_exception_flags "-qeh")
    set(cxx_no_exception_flags "-qnoeh")
    # Until version 9.0, Visual Age doesn't define a macro to indicate
    # whether RTTI is enabled.
    set(cxx_no_rtti_flags "-qnortti")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "HP")
    set(cxx_base_flags "-AA -mt")
    set(cxx_no_exception_flags "+noeh")
    # RTTI can not be disabled in HP aCC compiler.
    set(cxx_no_rtti_flags "")
  endif()

  set(cxx_base_flags "${cxx_base_flags}")

  set(cxx_exception "${CMAKE_CXX_FLAGS} ${cxx_base_flags} ${cxx_exception_flags}")
  set(cxx_no_exception
    "${CMAKE_CXX_FLAGS} ${cxx_base_flags} ${cxx_no_exception_flags}")
  set(cxx_default "${cxx_exception}")
  set(cxx_no_rtti "${cxx_default} ${cxx_no_rtti_flags}")
  set(cxx_use_own_tuple "${cxx_default}")

  set(cxx_strict "${cxx_default} ${cxx_strict_flags}")
endmacro()

function(cxx_library_with_type name type cxx_flags)
  # type can be either STATIC or SHARED to denote a static or shared library.
  # ARGN refers to additional arguments after 'cxx_flags'.
  add_library(${name} ${type} ${ARGN})
  set_target_properties(${name}
    PROPERTIES
    COMPILE_FLAGS "${cxx_flags}")
  if (BUILD_SHARED_LIBS OR type STREQUAL "SHARED")
    set_target_properties(${name}
      PROPERTIES)
  endif()
  if (CMAKE_USE_PTHREADS_INIT)
    target_link_libraries(${name} ${CMAKE_THREAD_LIBS_INIT})
  endif()
endfunction()

########################################################################
#
# Helper functions for creating build targets.

function(cxx_shared_library name cxx_flags)
  cxx_library_with_type(${name} SHARED "${cxx_flags}" ${ARGN})
endfunction()

function(cxx_library name cxx_flags)
  cxx_library_with_type(${name} "" "${cxx_flags}" ${ARGN})
endfunction()

# creates a named C++ executable that depends on the given libraries and
# is built from the given source files with the given compiler flags.
function(cxx_executable_with_flags name cxx_flags libs)
  add_executable(${name} ${ARGN})
  if (cxx_flags)
    set_target_properties(${name}
      PROPERTIES
      COMPILE_FLAGS "${cxx_flags}")
  endif()
  if (BUILD_SHARED_LIBS)
    set_target_properties(${name}
      PROPERTIES)
  endif()
  # To support mixing linking in static and dynamic libraries, link each
  # library in with an extra call to target_link_libraries.
  foreach (lib "${libs}")
    target_link_libraries(${name} ${lib})
  endforeach()
endfunction()

# cxx_executable(name dir lib srcs...)
#
# creates a named target that depends on the given libs and is built
# from the given source files.  dir/name.cxx is implicitly included in
# the source file list.
function(cxx_executable name dir libs)
  cxx_executable_with_flags(
    ${name} "${cxx_default}" "${libs}" "${dir}/${name}.cxx" ${ARGN})
endfunction()


########################################################################
#
# Project-wide settings
#
# Name of the project.
#
# CMake files in this project can refer to the root source directory
# as ${tool-log_SOURCE_DIR} and to the root binary directory as
# ${unittest_BINARY_DIR}.
# Language "C" is required for find_package(Threads).
project(tool-log CXX C)
cmake_minimum_required(VERSION 2.6.2)

config_compiler_and_linker()  # Defined in internal_utils.cmake.

# Where header files can be found.
include_directories(
  ${tool-log_SOURCE_DIR}/.
  )

# Where libraries can be found.
link_directories(
  )

########################################################################
#
# Defines the all libraries.  User tests should link with them
#
cxx_library(logfile "${cxx_strict}" logfile.cxx)
cxx_library(simplifylog "${cxx_strict}" simplifylog.cxx)

target_link_libraries(simplifylog logfile)

########################################################################
#
# link unit tests.
#
cxx_executable(test . simplifylog)
