find_program(GLSLC_EXECUTABLE
	NAMES glslc
	HINTS
		$ENV{VULKAN_SDK}/bin
		$ENV{VULKAN_SDK}/Bin
)

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc compiler not found")
endif()

message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")

function(compile_shaders SHADER_DIR OUTPUT_DIR)
	if(NOT SHADER_DIR)
		message(FATAL_ERROR "no SHADER_DIR argument")
	endif()

	if(NOT OUTPUT_DIR)
		message(FATAL_ERROR "no OUTPUT_DIR argument")
	endif()

	file(MAKE_DIRECTORY ${OUTPUT_DIR})

	file(GLOB_RECURSE SHADER_FILES
		"${SHADER_DIR}/*.vert"
		"${SHADER_DIR}/*.frag"
		"${SHADER_DIR}/*.comp"
	)

	set(SPIRV_FILES "")

	foreach(SHADER_FILE ${SHADER_FILES})
		file(RELATIVE_PATH REL_SHADER_PATH ${SHADER_DIR} ${SHADER_FILE})
		get_filename_component(SHADER_NAME ${REL_SHADER_PATH} NAME)
		get_filename_component(SHADER_SUBDIR ${REL_SHADER_PATH} DIRECTORY)

		if(SHADER_SUBDIR)
			file(MAKE_DIRECTORY "${OUTPUT_DIR}/${SHADER_SUBDIR}")
			set(SPIRV_FILE "${OUTPUT_DIR}/${SHADER_SUBDIR}/${SHADER_NAME}.spv")
		else()
			set(SPIRV_FILE "${OUTPUT_DIR}/${SHADER_NAME}.spv")
		endif()

		add_custom_command(
			OUTPUT ${SPIRV_FILE}
			COMMAND ${GLSLC_EXECUTABLE} -o ${SPIRV_FILE} ${SHADER_FILE}
			DEPENDS ${SHADER_FILE}
			COMMENT "Compiling ${REL_SHADER_PATH}"
			VERBATIM
		)

		list(APPEND SPIRV_FILES ${SPIRV_FILE})
	endforeach()

	add_custom_target(shaders ALL DEPENDS ${SPIRV_FILES})
endfunction()