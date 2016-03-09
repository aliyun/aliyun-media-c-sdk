include_directories(${CMAKE_SOURCE_DIR})

configure_file (
  "${PROJECT_SOURCE_DIR}/sample/sample.h.in"
  "${PROJECT_SOURCE_DIR}/sample/sample.h"
)

find_library(OSS_C_SDK_LIBRARY NAMES oss_c_sdk)
find_library(APR_LIBRARY apr-1 PATHS /usr/local/apr/lib/)
find_library(APR_UTIL_LIBRARY aprutil-1 PATHS /usr/local/apr/lib/)
find_library(MINIXML_LIBRARY mxml)
find_library(CURL_LIBRARY NAMES curl)
find_library(PTHREAD_LIBRARY NAMES pthread)
find_library(RT_LIBRARY NAMES rt)


IF (NOT ONLY_BUILD_SERVER)
   set(CLIENT_SAMPLE_SRC config.c client_sample.c)
   set(TS_SAMPLE_SRC config.c hls_sample.c)
   set(TS_STREAM_SAMPLE_SRC config.c hls_stream_sample.c)

   set(CLIENT_SAMPLE_BIN client_sample)
   set(TS_SAMPLE_BIN hls_sample)
   set(TS_STREAM_SAMPLE_BIN hls_stream_sample)

   add_executable(${CLIENT_SAMPLE_BIN} ${CLIENT_SAMPLE_SRC})
   add_executable(${TS_SAMPLE_BIN} ${TS_SAMPLE_SRC})
   add_executable(${TS_STREAM_SAMPLE_BIN} ${TS_STREAM_SAMPLE_SRC})

   target_link_libraries(${CLIENT_SAMPLE_BIN} ${CMAKE_PROJECT_NAME}_client)
   target_link_libraries(${CLIENT_SAMPLE_BIN} ${OSS_C_SDK_LIBRARY})
   target_link_libraries(${CLIENT_SAMPLE_BIN} ${APR_UTIL_LIBRARY})
   target_link_libraries(${CLIENT_SAMPLE_BIN} ${APR_LIBRARY})
   target_link_libraries(${CLIENT_SAMPLE_BIN} ${MINIXML_LIBRARY})
   target_link_libraries(${CLIENT_SAMPLE_BIN} ${CURL_LIBRARY})
   target_link_libraries(${CLIENT_SAMPLE_BIN} ${PTHREAD_LIBRARY})
   target_link_libraries(${CLIENT_SAMPLE_BIN} ${RT_LIBRARY})

   target_link_libraries(${TS_SAMPLE_BIN} ${CMAKE_PROJECT_NAME}_client)
   target_link_libraries(${TS_SAMPLE_BIN} ${OSS_C_SDK_LIBRARY})
   target_link_libraries(${TS_SAMPLE_BIN} ${APR_UTIL_LIBRARY})
   target_link_libraries(${TS_SAMPLE_BIN} ${APR_LIBRARY})
   target_link_libraries(${TS_SAMPLE_BIN} ${MINIXML_LIBRARY})
   target_link_libraries(${TS_SAMPLE_BIN} ${CURL_LIBRARY})
   target_link_libraries(${TS_SAMPLE_BIN} ${PTHREAD_LIBRARY})
   target_link_libraries(${TS_SAMPLE_BIN} ${RT_LIBRARY})

   target_link_libraries(${TS_STREAM_SAMPLE_BIN} ${CMAKE_PROJECT_NAME}_client)
   target_link_libraries(${TS_STREAM_SAMPLE_BIN} ${OSS_C_SDK_LIBRARY})
   target_link_libraries(${TS_STREAM_SAMPLE_BIN} ${APR_UTIL_LIBRARY})
   target_link_libraries(${TS_STREAM_SAMPLE_BIN} ${APR_LIBRARY})
   target_link_libraries(${TS_STREAM_SAMPLE_BIN} ${MINIXML_LIBRARY})
   target_link_libraries(${TS_STREAM_SAMPLE_BIN} ${CURL_LIBRARY})
   target_link_libraries(${TS_STREAM_SAMPLE_BIN} ${PTHREAD_LIBRARY})
   target_link_libraries(${TS_STREAM_SAMPLE_BIN} ${RT_LIBRARY})
ENDIF()

IF (NOT ONLY_BUILD_CLIENT)
   set(SERVER_SAMPLE_SRC config.c server_sample.c)
   set(SERVER_SAMPLE_BIN server_sample)
   add_executable(${SERVER_SAMPLE_BIN} ${SERVER_SAMPLE_SRC})

   target_link_libraries(${SERVER_SAMPLE_BIN} ${CMAKE_PROJECT_NAME}_server)
   target_link_libraries(${SERVER_SAMPLE_BIN} ${OSS_C_SDK_LIBRARY})
   target_link_libraries(${SERVER_SAMPLE_BIN} ${APR_UTIL_LIBRARY})
   target_link_libraries(${SERVER_SAMPLE_BIN} ${APR_LIBRARY})
   target_link_libraries(${SERVER_SAMPLE_BIN} ${MINIXML_LIBRARY})
   target_link_libraries(${SERVER_SAMPLE_BIN} ${CURL_LIBRARY})
   target_link_libraries(${SERVER_SAMPLE_BIN} ${PTHREAD_LIBRARY})
   target_link_libraries(${SERVER_SAMPLE_BIN} ${RT_LIBRARY})
ENDIF()


