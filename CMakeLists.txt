project(xmr-stak)

cmake_minimum_required(VERSION 3.1.0)

# enforce C++11
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "install prefix" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# allow user to extent CMAKE_PREFIX_PATH via environment variable
list(APPEND CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")

################################################################################
# CMake user options
################################################################################

# gcc 5.1 is the first GNU version without CoW strings
# https://github.com/fireice-uk/xmr-stak-nvidia/pull/10#issuecomment-290821792
# If you remove this guard to compile with older gcc versions the miner will produce
# a high rate of wrong shares.
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.1)
        message(FATAL_ERROR "g++ version must be at least 5.1!")
    endif()
endif()

set(BUILD_TYPE "Release;Debug")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${BUILD_TYPE}")

# Set the CPU type to native architecture
set(CMAKE_CXX_FLAGS "-march=native -mtune=native ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "-march=native -mtune=native ${CMAKE_C_FLAGS}")

# allow user to extent CMAKE_PREFIX_PATH via environment variable
list(APPEND CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")

################################################################################
# Find PThreads
################################################################################

find_package(Threads REQUIRED)
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
    
###############################################################################
# Find OpenSSL
###############################################################################

option(OpenSSL_ENABLE "Enable or disable the requirement of OpenSSL" ON)
if(OpenSSL_ENABLE)
    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        include_directories(${OPENSSL_INCLUDE_DIR})
        set(LIBS ${LIBS} ${OPENSSL_LIBRARIES})
    else()
        message(FATAL_ERROR "OpenSSL NOT found: use `-DOpenSSL_ENABLE=OFF` to build without SSL support")
    endif()
else()
    add_definitions("-DCONF_NO_TLS")
endif()

################################################################################
# Find hwloc
################################################################################

find_path(HWLOC_INCLUDE_DIR
    NAMES
        hwloc.h
    PATHS
        /opt/local
        /usr/local
        /usr
        ENV "PROGRAMFILES(X86)"
        ENV "HWLOC_ROOT"
    PATH_SUFFIXES
        include)

find_library(HWLOC
    NAMES
        libhwloc.lib
        hwloc
    PATHS
        ENV "HWLOC_ROOT"
    PATH_SUFFIXES
        lib)

if(HWLOC STREQUAL "HWLOC-NOTFOUND" OR ${HWLOC_INCLUDE_DIR} STREQUAL "HWLOC_INCLUDE_DIR-NOTFOUND")
    message(FATAL_ERROR "hwloc NOT found: use `-DHWLOC_ENABLE=OFF` to build without hwloc support")
else()
    set(LIBS ${LIBS} ${HWLOC})
    include_directories(AFTER ${HWLOC_INCLUDE_DIR})
endif()


################################################################################
# Compile & Link
################################################################################

include_directories(BEFORE .)

set (CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# activate sse2 and aes-ni
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2 -maes")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2 -maes")

# activate static libgcc and libstdc++ linking
set(BUILD_SHARED_LIBRARIES OFF)
set(DL_LIB ${CMAKE_DL_LIBS})
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    set(LIBS "-static-libgcc -static-libstdc++ ${LIBS}")
endif()

## compile C files
file(GLOB SRCFILES_C "xmrstak/backend/cpu/crypto/*.h" "xmrstak/backend/cpu/crypto/*.hpp" "xmrstak/backend/cpu/crypto/*.c")
set_source_files_properties(${SRCFILES_C} PROPERTY C_STANDARD 99)
#
#add_library(xmr-stak-c STATIC ${SRCFILES_C})
#set_property(TARGET xmr-stak-c PROPERTY C_STANDARD 99)
#target_link_libraries(xmr-stak-c ${LIBS})

# compile generic backend files
file(GLOB BACKEND_CPP
    "c_blake/*.hpp"
    "c_blake/*.cpp"
    "c_skein/*.hpp"
    "c_skein/*.cpp"
    "c_groestl/*.hpp"
    "c_groestl/*.cpp"
    "c_jh/*.hpp"
    "c_jh/*.cpp"
    "c_keccak/*.hpp"
    "c_keccak/*.cpp"
    "c_hwlock/*.hpp"
    "c_hwlock/*.cpp"
    "c_cryptonight/*.hpp"
    "c_cryptonight/*.cpp"
    "xmrstak/backend/cpu/crypto/*.h"
    "xmrstak/backend/cpu/crypto/*.hpp"
    "xmrstak/backend/cpu/crypto/*.c"
    "xmrstak/*.hpp"
    "xmrstak/*.cpp"
    "xmrstak/backend/cpu/*.hpp"
    "xmrstak/backend/cpu/*.cpp"
    "xmrstak/backend/*.hpp"
    "xmrstak/backend/*.cpp"
    "xmrstak/backend/cpu/crypto/*.cpp"
    "xmrstak/misc/*.hpp"
    "xmrstak/misc/*.cpp"
    "xmrstak/rapidjson/*/*"
    "xmrstak/rapidjson/*"
    "xmrstak/net/*.hpp"
    "xmrstak/net/*.cpp"
)

#add_library(xmr-stak-backend STATIC ${BACKEND_CPP})
#target_link_libraries(xmr-stak-backend xmr-stak-c ${CMAKE_DL_LIBS})

# compile final binary
file(GLOB SRCFILES_CPP "xmrstak/cli/*.cpp")
set_source_files_properties(${SRCFILES_CPP} PROPERTIES LANGUAGE CXX)

add_executable(xmr-stak ${SRCFILES_CPP} ${BACKEND_CPP} ${SRCFILES_C} c_hwlock/do_hwlock.cpp c_hwlock/do_hwlock.hpp)

set(EXECUTABLE_OUTPUT_PATH "bin")
set(LIBRARY_OUTPUT_PATH "bin")

target_link_libraries(xmr-stak ${LIBS})

################################################################################
# Install
################################################################################

# do not install the binary if the project and install are equal
if( NOT CMAKE_INSTALL_PREFIX STREQUAL PROJECT_BINARY_DIR )
    install(TARGETS xmr-stak RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
else()
    # this rule is used if the install prefix is the build directory
    install(CODE "MESSAGE(\"xmr-stak installed to folder 'bin'\")")
endif()
