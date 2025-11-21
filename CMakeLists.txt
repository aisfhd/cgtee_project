#main cmake
cmake_minimum_required(VERSION 4.0)
project(cvgtee)

find_package(OpenGL REQUIRED)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_subdirectory(external)

include_directories(
	external/glad-4.6/include
	external/glfw-3.4/include
	external/glm-1.0.2/
	external/
	lab3/
)

add_executable(cvgtee
	src/cvgtee.cpp
)
target_link_libraries(cvgtee
	${OPENGL_LIBRARY}
	glfw
	glad
)
#set_target_properties(cvgtee PROPERTIES 
#    LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")