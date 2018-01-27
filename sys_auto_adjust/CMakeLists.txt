project(sys-auto-adjust)
cmake_minimum_required(VERSION 3.1.0)

option(CONFIG_SYSTEM_ENABLE "Enable CONFIG_SYSTEM" ON)
if(CONFIG_SYSTEM_ENABLE)
    set(CONFIG_SYSTEM_NPROC $ENV{SYSTEM_NPROC})
    set(CONFIG_SYSTEM_CACHE_L2 $ENV{SYSTEM_CACHE_L2})
    set(CONFIG_SYSTEM_CACHE_L3 $ENV{SYSTEM_CACHE_L3})
endif()
configure_file(sys_auto_adjust.hpp.in sys_auto_adjust.hpp @ONLY)


################################################################################
# Compiler settings
################################################################################

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++lastest")
# enforce C++11
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

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

# compile generic backend files
file(
        GLOB
        SYS_AUTO_ADJUST_CPP
        "autoAdjust.cpp"
        "sys_auto_adjust_main.cpp"
)
set_source_files_properties(${SYS_AUTO_ADJUST_CPP} PROPERTIES LANGUAGE CXX)
add_executable(sys-auto-adjust ${SYS_AUTO_ADJUST_CPP})

set(EXECUTABLE_OUTPUT_PATH "./bin")
set(LIBRARY_OUTPUT_PATH "./bin")
target_link_libraries(sys-auto-adjust ${LIBS})

################################################################################
# Install
################################################################################

# do not install the binary if the project and install are equal
if( NOT CMAKE_INSTALL_PREFIX STREQUAL PROJECT_BINARY_DIR )
    install(TARGETS sys-auto-adjust RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
else()
    # this rule is used if the install prefix is the build directory
    install(CODE "MESSAGE(\"sys-auto-adjust installed to folder 'bin'\")")
endif()
