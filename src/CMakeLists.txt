IF (NOT ONLY_BUILD_SERVER)
   set(CLIENT_SRC oss_media_define.c 
   	       oss_media_log.c
	       oss_media_file.c
	       )

   add_library(${CMAKE_PROJECT_NAME}_client SHARED ${CLIENT_SRC})
   add_library(${CMAKE_PROJECT_NAME}_client_static STATIC ${CLIENT_SRC})

   set_target_properties(${CMAKE_PROJECT_NAME}_client PROPERTIES VERSION ${CMAKE_VERSION}  SOVERSION ${CMAKE_VERSION})

   INSTALL(TARGETS ${CMAKE_PROJECT_NAME}_client LIBRARY DESTINATION lib)
   INSTALL(TARGETS ${CMAKE_PROJECT_NAME}_client_static ARCHIVE DESTINATION lib)

   INSTALL(FILES oss_media_define.h
   		 oss_media_file.h
		 DESTINATION include/oss_c_media_sdk)
ENDIF()

IF (NOT ONLY_BUILD_CLIENT)
   set(SERVER_SRC oss_media_define.c
	       oss_media_log.c
	       oss_media.c
	       sts/token.c
	       sts/general.c
	       sts/util.c
	       sts/request.c
	       sts/time.c
	       sts/jsmn.c
	       )

   add_library(${CMAKE_PROJECT_NAME}_server SHARED ${SERVER_SRC})
   add_library(${CMAKE_PROJECT_NAME}_server_static STATIC ${SERVER_SRC})

   set_target_properties(${CMAKE_PROJECT_NAME}_server PROPERTIES VERSION ${CMAKE_VERSION}  SOVERSION ${CMAKE_VERSION})

   INSTALL(TARGETS ${CMAKE_PROJECT_NAME}_server LIBRARY DESTINATION lib)
   INSTALL(TARGETS ${CMAKE_PROJECT_NAME}_server_static ARCHIVE DESTINATION lib)

   INSTALL(FILES
     oss_media_define.h
     oss_media.h
     DESTINATION include/oss_c_media_sdk)

   INSTALL(FILES
     sts/libsts.h
     DESTINATION include/oss_c_media_sdk/sts/)

ENDIF()
