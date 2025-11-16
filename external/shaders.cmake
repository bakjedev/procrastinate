find_program(SLANGC_EXECUTABLE
	NAMES slangc
)

if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc compiler not found")
endif()

message(STATUS "Found slangc: ${SLANGC_EXECUTABLE}")

function(compile_shaders TARGET_NAME SHADER_DIR OUTPUT_DIR)
	if(NOT TARGET_NAME)
		message(FATAL_ERROR "no TARGET_NAME argument")
	endif()	

	if(NOT SHADER_DIR)
		message(FATAL_ERROR "no SHADER_DIR argument")
	endif()

	if(NOT OUTPUT_DIR)
		message(FATAL_ERROR "no OUTPUT_DIR argument")
	endif()

	file(MAKE_DIRECTORY ${OUTPUT_DIR})

	file(GLOB_RECURSE SHADER_FILES
		"${SHADER_DIR}/*.slang"
	)

	set(SPIRV_FILES "")

	foreach(SHADER_FILE ${SHADER_FILES})
		file(RELATIVE_PATH REL_SHADER_PATH ${SHADER_DIR} ${SHADER_FILE})
		get_filename_component(SHADER_NAME ${REL_SHADER_PATH} NAME_WLE)
		get_filename_component(SHADER_SUBDIR ${REL_SHADER_PATH} DIRECTORY)

		if(SHADER_SUBDIR)
			file(MAKE_DIRECTORY "${OUTPUT_DIR}/${SHADER_SUBDIR}")
			set(SPIRV_FILE "${OUTPUT_DIR}/${SHADER_SUBDIR}/${SHADER_NAME}.spv")
		else()
			set(SPIRV_FILE "${OUTPUT_DIR}/${SHADER_NAME}.spv")
		endif()

		add_custom_command(
			OUTPUT ${SPIRV_FILE}
			COMMAND ${SLANGC_EXECUTABLE}
			-target spirv
			-o ${SPIRV_FILE}
			${SHADER_FILE}
			DEPENDS ${SHADER_FILE}
			COMMENT "Compiling ${REL_SHADER_PATH}"
			VERBATIM
		)

		list(APPEND SPIRV_FILES ${SPIRV_FILE})
	endforeach()

	add_custom_target(${TARGET_NAME} ALL
		DEPENDS ${SPIRV_FILES}
		COMMENT "Compiling all shaders"
	)
endfunction()