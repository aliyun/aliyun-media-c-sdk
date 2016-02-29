include_directories(${CMAKE_SOURCE_DIR})

configure_file (
  "${PROJECT_SOURCE_DIR}/test/test.h.in"
  "${PROJECT_SOURCE_DIR}/test/test.h"
)

set(TEST_BIN_NAME ${CMAKE_PROJECT_NAME}_test)

set(TEST_SOURCE_FILES CuTest.c 
		      config.c 
		      test_client.c
		      test_server.c
		      test_sts.c
		      test_ts.c
		      test_all.c)

include_directories (${APR_INCLUDE_DIR})
include_directories (${APR_UTIL_INCLUDE_DIR})
include_directories (${MINIXML_INCLUDE_DIR})
include_directories (${CURL_INCLUDE_DIR})
include_directories (${WOLFSSL_INCLUDE_DIR})
include_directories ("${CMAKE_SOURCE_DIR}/src")

find_library(APR_LIBRARY apr-1 PATHS /usr/local/apr/lib/)
find_library(APR_UTIL_LIBRARY aprutil-1 PATHS /usr/local/apr/lib/)
find_library(MINIXML_LIBRARY mxml)
find_library(CURL_LIBRARY curl)
find_library(PTHREAD_LIBRARY pthread)
find_library(RT_LIBRARY rt)
find_library(OSS_C_SDK_LIBRARY oss_c_sdk)

add_executable(${TEST_BIN_NAME} ${TEST_SOURCE_FILES})

target_link_libraries(${TEST_BIN_NAME} ${OSS_C_SDK_LIBRARY})
target_link_libraries(${TEST_BIN_NAME} ${APR_UTIL_LIBRARY})
target_link_libraries(${TEST_BIN_NAME} ${APR_LIBRARY})
target_link_libraries(${TEST_BIN_NAME} ${MINIXML_LIBRARY})
target_link_libraries(${TEST_BIN_NAME} ${CURL_LIBRARY})
target_link_libraries(${TEST_BIN_NAME} ${PTHREAD_LIBRARY})
target_link_libraries(${TEST_BIN_NAME} ${WOLFSSL_LIBRARY})
target_link_libraries(${TEST_BIN_NAME} ${RT_LIBRARY})
target_link_libraries(${TEST_BIN_NAME} ${CMAKE_PROJECT_NAME}_client)
target_link_libraries(${TEST_BIN_NAME} ${CMAKE_PROJECT_NAME}_server)
