cmake_minimum_required(VERSION 3.3)
set(CMAKE_CXX_STANDARD 11)
set(LIB_NAME blobcast)
project(${LIB_NAME})

include_directories(include)
if (MSVC)
	set(MSVC_DIR msvc14)
	link_directories(${MSVC_DIR}/lib)
endif()

file(GLOB GLB_SRC_FILES "*.cpp" "*.h")
add_library(${LIB_NAME} STATIC ${GLB_SRC_FILES})
target_link_libraries(${LIB_NAME}
	glew32
	glu32
	opengl32
	glfw3
	${OPENGL_glu_LIBRARY}
	legacy_stdio_definitions
	freetype
	freetype-gl
	optimized BulletCollision debug BulletCollision_Debug
	optimized BulletDynamics  debug BulletDynamics_Debug
	optimized BulletSoftBody  debug BulletSoftBody_Debug
	optimized LinearMath      debug LinearMath_Debug
	)

add_subdirectory(server)
