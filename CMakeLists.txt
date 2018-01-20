project(xmr-stak)

cmake_minimum_required(VERSION 3.1.0)

################################################################################
# Compile time options
################################################################################

if(DEFINED ENV{CONFIG_STATSD_ADDRESS})
    add_definitions("-DCONFIG_STATSD_ADDRESS=\"$ENV{CONFIG_STATSD_ADDRESS}\"")
else()
    add_definitions("-DCONFIG_STATSD_ADDRESS=\"127.0.0.1\"")
endif()

if(DEFINED ENV{CONFIG_STATSD_PORT})
    add_definitions("-DCONFIG_STATSD_PORT=$ENV{CONFIG_STATSD_PORT}")
else()
    add_definitions("-DCONFIG_STATSD_PORT=8080")
endif()

if(DEFINED ENV{CONFIG_STATSD_PREFIX})
    # Note the trailing period at the end of the prefix!
    add_definitions("-DCONFIG_STATSD_PREFIX=\"$ENV{CONFIG_STATSD_PREFIX}.\"")
else()
    add_definitions("-DCONFIG_STATSD_PREFIX=\"myPrefix.\"")
endif()

if(DEFINED ENV{CONFIG_STATSD_MACHINE_ID})
    add_definitions("-DCONFIG_STATSD_MACHINE_ID=\"$ENV{CONFIG_STATSD_MACHINE_ID}\"")
else()
    add_definitions("-DCONFIG_STATSD_MACHINE_ID=\"myMachine\"")
endif()



# pool_address    - Pool address should be in the form "pool.supportxmr.com:3333". Only stratum pools are supported.
# wallet_address  - Your wallet, or pool login.
# pool_password   - Can be empty in most cases or "x".
# use_tls         - This option will make us connect using Transport Layer Security.
# tls_fingerprint - Server's SHA256 fingerprint. If this string is non-empty then we will check the server's cert against it.
# pool_weight     - Pool weight is a number telling the miner how important the pool is. Miner will mine mostly at the pool
#                   with the highest weight, unless the pool fails. Weight must be an integer larger than 0.
#
add_definitions("-DCONFIG_POOL_POOL_ADDRESS=\"xmr-us-east1.nanopool.org:14444\"")
add_definitions("-DCONFIG_POOL_WALLET_ADDRESS=\"$ENV{POOL_WALLET_ADDRESS}\"")
add_definitions("-DCONFIG_POOL_POOL_PASSWORD=\"x\"")


# Network timeouts.
# Because of the way this client is written it doesn't need to constantly talk (keep-alive) to the server to make
# sure it is there. We detect a buggy / overloaded server by the call timeout. The default values will be ok for
# nearly all cases. If they aren't the pool has most likely overload issues. Low call timeout values are preferable -
# long timeouts mean that we waste hashes on potentially stale jobs. Connection report will tell you how long the
# server usually takes to process our calls.
#
# call_timeout - How long should we wait for a response from the server before we assume it is dead and drop the connection.
# retry_time	- How long should we wait before another connection attempt.
#                Both values are in seconds.
# giveup_limit - Limit how many times we try to reconnect to the pool. Zero means no limit. Note that stak miners
#                don't mine while the connection is lost, so your computer's power usage goes down to idle.
#
add_definitions("-DCONFIG_CALL_TIMEOUT=10")
add_definitions("-DCONFIG_RETRY_TIME=30")
add_definitions("-DCONFIG_GIVEUP_LIMIT=0")


# Output control.
# Since most people are used to miners printing all the time, that's what we do by default too. This is suboptimal
# really, since you cannot see errors under pages and pages of text and performance stats. Given that we have internal
# performance monitors, there is very little reason to spew out pages of text instead of concise reports.
# Press 'h' (hashrate), 'r' (results) or 'c' (connection) to print reports.
#
# verbose_level - 0 - Don't print anything.
#                 1 - Print intro, connection event, disconnect event
#                 2 - All of level 1, and new job (block) event if the difficulty is different from the last job
#                 3 - All of level 1, and new job (block) event in all cases, result submission event.
#                 4 - All of level 3, and automatic hashrate report printing
#
# print_motd    - Display messages from your pool operator in the hashrate result.
#
add_definitions("-DCONFIG_VERBOSE_LEVEL=3")  # Set the log verbosity
add_definitions("-DCONFIG_PRINT_MOTD=true")  # Enable or disable messages from the pool


# Automatic hashrate report
#
# h_print_time - How often, in seconds, should we print a hashrate report if verbose_level is set to 4.
#                This option has no effect if verbose_level is not 4.
#
add_definitions("-DCONFIG_H_PRINT_TIME=60")

# TLS Settings
# If you need real security, make sure tls_secure_algo is enabled (otherwise MITM attack can downgrade encryption
# to trivially breakable stuff like DES and MD5), and verify the server's fingerprint through a trusted channel.
#
# tls_secure_algo - Use only secure algorithms. This will make us quit with an error if we can't negotiate a secure algo.
#
add_definitions("-DCONFIG_TLS_SECURE_ALGO=true")


# LARGE PAGE SUPPORT
# Large pages need a properly set up OS. It can be difficult if you are not used to systems administration,
# but the performance results are worth the trouble - you will get around 20% boost. Slow memory mode is
# meant as a backup, you won't get stellar results there. If you are running into trouble, especially
# on Windows, please read the common issues in the README.
#
# By default we will try to allocate large pages. This means you need to "Run As Administrator" on Windows.
# You need to edit your system's group policies to enable locking large pages. Here are the steps from MSDN
#
# 1. On the Start menu, click Run. In the Open box, type gpedit.msc.
# 2. On the Local Group Policy Editor console, expand Computer Configuration, and then expand Windows Settings.
# 3. Expand Security Settings, and then expand Local Policies.
# 4. Select the User Rights Assignment folder.
# 5. The policies will be displayed in the details pane.
# 6. In the pane, double-click Lock pages in memory.
# 7. In the Local Security Setting – Lock pages in memory dialog box, click Add User or Group.
# 8. In the Select Users, Service Accounts, or Groups dialog box, add an account that you will run the miner on
# 9. Reboot for change to take effect.
#
# Windows also tends to fragment memory a lot. If you are running on a system with 4-8GB of RAM you might need
# to switch off all the auto-start applications and reboot to have a large enough chunk of contiguous memory.
#
# On Linux you will need to configure large page support "sudo sysctl -w vm.nr_hugepages=128" and increase your
# ulimit -l. To do do this you need to add following lines to /etc/security/limits.conf - "* soft memlock 262144"
# and "* hard memlock 262144". You can also do it Windows-style and simply run-as-root, but this is NOT
# recommended for security reasons.
#
# Memory locking means that the kernel can't swap out the page to disk - something that is unlikely to happen on a
# command line system that isn't starved of memory. I haven't observed any difference on a CLI Linux system between
# locked and unlocked memory. If that is your setup see option "no_mlck".
#
#
#
# use_slow_memory defines our behaviour with regards to large pages. There are three possible options here:
# always_use    - Don't even try to use large pages. Always use slow memory.
# print_warning - We will try to use large pages, but fall back to slow memory if that fails.
# no_mlck       - This option is only relevant on Linux, where we can use large pages without locking memory.
#                 It will never use slow memory, but it won't attempt to mlock
# never_use     - If we fail to allocate large pages we will print an error and exit.
#
add_definitions("-DCONFIG_USE_SLOW_MEMORY=print_warning")


# Manual hardware AES override
#
# Some VMs don't report AES capability correctly. You can set this value to true to enforce hardware AES or
# to false to force disable AES or null to let the miner decide if AES is used.
#
# WARNING: setting this to true on a CPU that doesn't support hardware AES will crash the miner.
#
add_definitions("-DCONFIG_AES_OVERRIDE=true")

# Buffered output control.
# When running the miner through a pipe, standard output is buffered. This means that the pipe won't read
# each output line immediately. This can cause delays when running in background.
# Set this option to true to flush stdout after each line, so it can be read immediately.
#
add_definitions("-DCONFIG_FLUSH_STDOUT=false")


# Set the number of threads in the system
execute_process(
        COMMAND ./scripts/count-nproc
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE SYSTEM_NPROC
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND ./scripts/count-cache-l2
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE SYSTEM_CACHE_L2
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND ./scripts/count-cache-l3
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE SYSTEM_CACHE_L3
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT SYSTEM_NPROC STREQUAL "")
    add_definitions("-DCONFIG_SYSTEM_NPROC=${SYSTEM_NPROC}")
elseif(NOT "$ENV{SYSTEM_NPROC}" STREQUAL "")
    add_definitions("-DCONFIG_SYSTEM_NPROC=$ENV{SYSTEM_NPROC}")
else()
    message(FATAL_ERROR "Failed to detect number of cores")
endif()

if(NOT SYSTEM_CACHE_L2 STREQUAL "")
    add_definitions("-DCONFIG_SYSTEM_CACHE_L2=${SYSTEM_CACHE_L2}")
elseif(NOT "$ENV{SYSTEM_CACHE_L2}" STREQUAL "")
    add_definitions("-DCONFIG_SYSTEM_CACHE_L2=$ENV{SYSTEM_CACHE_L2}")
else()
    message(FATAL_ERROR "Failed to detect amount of L2 cache")
endif()

if(NOT SYSTEM_CACHE_L3 STREQUAL "")
    add_definitions("-DCONFIG_SYSTEM_CACHE_L3=${SYSTEM_CACHE_L3}")
elseif(NOT "$ENV{SYSTEM_CACHE_L3}" STREQUAL "")
    add_definitions("-DCONFIG_SYSTEM_CACHE_L3=$ENV{SYSTEM_CACHE_L3}")
else()
    message(FATAL_ERROR "Failed to detect amount of L3 cache")
endif()

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
        BACKEND_CPP
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
        "xmrstak/*.hpp"
        "xmrstak/*.cpp"
        "xmrstak/cli/cli-miner.cpp"
        "xmrstak/cli/statsd.cpp"
        "xmrstak/backend/cpu/*.hpp"
        "xmrstak/backend/cpu/*.cpp"
        "xmrstak/backend/*.hpp"
        "xmrstak/backend/*.cpp"
        "xmrstak/misc/*.hpp"
        "xmrstak/misc/*.cpp"
        "xmrstak/net/*.hpp"
        "xmrstak/net/*.cpp"
        "includes/StatsdClient.cpp"
        "includes/UDPSender.cpp"
)
set_source_files_properties(${SRCFILES_CPP} PROPERTIES LANGUAGE CXX)

add_executable(xmr-stak ${SRCFILES_CPP} ${BACKEND_CPP})

set(EXECUTABLE_OUTPUT_PATH "bin")
set(LIBRARY_OUTPUT_PATH "bin")

target_link_libraries(xmr-stak ${LIBS})



# compile final binary
file(GLOB STATSD_TEST_CPP "includes/StatsdClient.cpp" "includes/UDPSender.cpp" "includes/statsd_test.cpp")
set_source_files_properties(${STATSD_TEST_CPP} PROPERTIES LANGUAGE CXX)
add_executable(statsd-test ${STATSD_TEST_CPP})
target_link_libraries(statsd-test ${LIBS})


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
