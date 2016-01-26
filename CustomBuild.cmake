if(MSVC OR MSVC_IDE)
	set_target_properties(unionclientlibrary PROPERTIES COMPILE_FLAGS /EHsc)
endif()
if(TARGET_OS STREQUAL windows)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 iphlpapi psapi)
elseif(TARGET_OS STREQUAL linux)
    target_link_libraries(${PROJECT_NAME} PRIVATE rt)
endif()

