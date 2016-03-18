cmake_minimum_required(VERSION 3.3)
set(CMAKE_CXX_STANDARD 11)
set(LIB_NAME blobcast)
project(${LIB_NAME})

option(BLOBCAST_RTMP
	"Stream to a local RTMP server instead oF UDP multicast"
	OFF)

include_directories(include)
if (CMAKE_COMPILER_IS_GNUCXX)
	link_directories(gcc/lib)
elseif (MSVC)
	set(MSVC_DIR msvc14)
	link_directories(${MSVC_DIR}/lib)
endif()

if(BLOBCAST_RTMP)
	add_definitions(-DRTMP_STREAM)
endif(BLOBCAST_RTMP)

file(GLOB GLB_SRC_FILES "*.c" "*.cpp" "*.h")
add_library(${LIB_NAME} STATIC ${GLB_SRC_FILES})
if (WIN32)
target_link_libraries(${LIB_NAME}
	glew32
	opengl32
	glfw3
	SOIL
	${OPENGL_glu_LIBRARY}
	legacy_stdio_definitions
	freetype
	freetype-gl
	avcodec
	avformat
	avutil
	swscale
	ws2_32
	optimized RakNetLibStatic debug RakNetLibStatic_Debug
	optimized BulletCollision debug BulletCollision_Debug
	optimized BulletDynamics  debug BulletDynamics_Debug
	optimized BulletSoftBody  debug BulletSoftBody_Debug
	optimized LinearMath      debug LinearMath_Debug
	optimized SPARK	          debug SPARK_Debug
	optimized SPARK_GL	  debug SPARK_GL_Debug
	)
else()
set(GLFW_LIBRARIES X11 Xrandr Xinerama Xcursor dl z bz2 Xxf86vm GL pthread)
set(FFMPEG_LIBRARIES
	va lzma
	freetype
	mp3lame
	opus
	theora theoraenc theoradec
	vorbis vorbisenc
	vpx x264 x265)
target_link_libraries(${LIB_NAME}
	GLEW
	OpenGL
	glfw3
	${GLFW_LIBRARIES}
	${OPENGL_glu_LIBRARY}
	SOIL
	freetype
	freetype-gl
	${FFMPEG_LIBRARIES}
	avcodec
	avformat
	avutil
	swresample
	swscale
	RakNetLibStatic
	BulletSoftBody
	BulletDynamics
	BulletCollision
	LinearMath
	)
endif()

add_subdirectory(server)
add_subdirectory(client)
