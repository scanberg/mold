cmake_minimum_required(VERSION 3.0)

project(libmold VERSION 0.1.1 LANGUAGES C)

option(MOLD_USE_RELATIVE_PATHS OFF)

file(GLOB SRC_FILES *)
file(GLOB CORE_FILES core/*)
file(GLOB SHADER_FILES shaders/*)
file(GLOB EXT_FILES ext/*)

source_group("src" FILES ${SRC_FILES})
source_group("core" FILES ${CORE_FILES})
source_group("ext" FILES ${EXT_FILES})
source_group("shaders" FILES ${SHADER_FILES})

if (MSVC)
    message(STATUS "MSVC configuration was used")
    add_compile_options(/MT /W4 /MP /GR- /fp:fast /MP /arch:AVX)# /arch:AVX2
endif()

add_library(mold ${CORE_FILES} ${SRC_FILES} ${EXT_FILES})

target_include_directories(mold
	PUBLIC
		$<INSTALL_INTERFACE:
			${CMAKE_CURRENT_SOURCE_DIR}/mold_draw.h>
			${CMAKE_CURRENT_SOURCE_DIR}/mold_filter.h>
			${CMAKE_CURRENT_SOURCE_DIR}/mold_util.h>
		$<BUILD_INTERFACE:
			${CMAKE_CURRENT_SOURCE_DIR}/mold_draw.h>
			${CMAKE_CURRENT_SOURCE_DIR}/mold_filter.h>
			${CMAKE_CURRENT_SOURCE_DIR}/mold_util.h>
)

if (MOLD_USE_RELATIVE_PATHS)
    target_compile_definitions(mold PRIVATE MOLD_SHADER_DIR=\"shaders\")

	# COPY SHADER FILES TO EXECUTABLE DIRECTORY TO KEEP RELATIVE PATH
    # CANNOT COPY ENTIRE DIRECTORY -> WILL REMOVE ANY EXISTING DIRECTORY
    file(GLOB_RECURSE COPY_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} shaders/*)

    foreach(FILENAME ${COPY_FILES})
    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
    set(DST "${CMAKE_BINARY_DIR}/$<CONFIG>/${FILENAME}")

    add_custom_command(
        TARGET mold POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
        )
    endforeach(FILENAME)

	#add_custom_command(
	#		TARGET mold POST_BUILD
	#		COMMAND ${CMAKE_COMMAND} -E copy_directory
	#				${CMAKE_CURRENT_SOURCE_DIR}/shaders
	#				${CMAKE_BINARY_DIR}/$<CONFIG>/shaders)
else()
	target_compile_definitions(mold PRIVATE MOLD_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/shaders\")
endif()