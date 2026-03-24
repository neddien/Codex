project("CodexEditor")

if (NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_custom_target(
		CodexEditor ALL 
		COMMAND dotnet build -c Debug
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
	#add_custom_command(
    #    TARGET CodexEditor
    #    POST_BUILD
    #    COMMAND ${CMAKE_COMMAND} -E copy_directory "${}/ProjectTemplates/" "${ALVM_BINARY_OUTPUT_DIR}"
    #)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
	add_custom_target(
		CodexEditor ALL 
		COMMAND dotnet build -c Release
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
endif()
