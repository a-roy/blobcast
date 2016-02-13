set(EXEC_NAME blobserver)
project(${EXEC_NAME})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
set(GLB_PATH ..)

include_directories(
	${GLB_PATH}
	${GLB_PATH}/include
	)
set(EXT_LIBS )
if (MSVC)
	set(MSVC_DIR ${GLB_PATH}/msvc14)
	link_directories(${MSVC_DIR}/lib)
	file(GLOB EXT_LIBS
		"${MSVC_DIR}/bin/*.dll"
		)
endif()

file(GLOB SRC_FILES "*.cpp" "*.h")
add_executable(${EXEC_NAME} ${SRC_FILES})

target_link_libraries(${EXEC_NAME}
	blobcast
	glew32
	glu32
	opengl32
	glfw3
	${OPENGL_glu_LIBRARY}
	avcodec
	avformat
	avutil
	swscale
	)

foreach(lib ${EXT_LIBS})
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${lib}"
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)
endforeach(lib)
