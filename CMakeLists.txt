PROJECT(oss_c_media_sdk)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(CMAKE_VERSION 0.2.0)

option(ONLY_BUILD_CLIENT "only build client" OFF)
option(ONLY_BUILD_SERVER "only build server" OFF)

# default C / CXX flags
set(CMAKE_C_FLAGS " -g -ggdb -O0 -Wno-unused-parameter -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS " -g -ggdb -O0 -Wno-unused-parameter -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_DEBUG " -g -ggdb -O0 -Wno-unused-parameter -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_DEBUG " -g -ggdb -O0 -Wno-unused-parameter -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_RELEASE " -O3 -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_RELEASE " -O3 -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_MINSIZEREF " -Os -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_MINSIZEREF " -Os -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_RELWITHDEBINFO " -O2 -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O2 -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/bin)

FIND_PROGRAM(APR_CONFIG_BIN NAMES apr-config apr-1-config PATHS /usr/bin /usr/local/bin /usr/local/apr/bin/)

IF (APR_CONFIG_BIN)
    EXECUTE_PROCESS(
        COMMAND ${APR_CONFIG_BIN} --includedir
        OUTPUT_VARIABLE APR_INCLUDEDIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND ${APR_CONFIG_BIN} --cflags
        OUTPUT_VARIABLE APR_C_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND ${APR_CONFIG_BIN} --link-ld
        OUTPUT_VARIABLE APR_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ELSE()
    MESSAGE(FATAL_ERROR "Could not find apr-config/apr-1-config")
ENDIF()

FIND_PROGRAM(APU_CONFIG_BIN NAMES apu-config apu-1-config PATHS /usr/bin /usr/local/bin /usr/local/apr/bin/)

IF (APU_CONFIG_BIN)
    EXECUTE_PROCESS(
        COMMAND ${APU_CONFIG_BIN} --includedir
        OUTPUT_VARIABLE APU_INCLUDEDIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND ${APU_CONFIG_BIN} --cflags
        OUTPUT_VARIABLE APU_C_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND ${APU_CONFIG_BIN} --link-ld
        OUTPUT_VARIABLE APU_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ELSE()
    MESSAGE(FATAL_ERROR "Could not find apu-config/apu-1-config")
ENDIF()

#curl-config
FIND_PROGRAM(CURL_CONFIG_BIN NAMES curl-config)
  
IF (CURL_CONFIG_BIN)
    EXECUTE_PROCESS(
        COMMAND ${CURL_CONFIG_BIN} --libs
        OUTPUT_VARIABLE CURL_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
ELSE()
    MESSAGE(FATAL_ERROR "Could not find curl-config")
ENDIF()

#wolfssl-config
FIND_PROGRAM(WOLFSSL_CONFIG_BIN NAMES wolfssl-config)
  
IF (WOLFSSL_CONFIG_BIN)
    EXECUTE_PROCESS(
        COMMAND ${WOLFSSL_CONFIG_BIN} --includedir
        OUTPUT_VARIABLE WOLFSSL_INCLUDEDIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND ${WOLFSSL_CONFIG_BIN} --libs
        OUTPUT_VARIABLE WOLFSSL_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
ELSE()
    MESSAGE(FATAL_ERROR "Could not find wolfssl-config")
ENDIF()
  
# Compile and link lib_oss_c_sdk
include_directories(${APR_INCLUDEDIRS})
include_directories(${APU_INCLUDEDIRS})

find_path(OSS_C_SDK_INCLUDE_DIR NAMES oss_c_sdk/oss_api.h)
include_directories(${OSS_C_SDK_INCLUDE_DIR})

add_subdirectory(src)
add_subdirectory(sample)

IF (NOT ONLY_BUILD_CLIENT AND NOT ONLY_BUILD_SERVER)
    add_subdirectory(test)
ENDIF()
