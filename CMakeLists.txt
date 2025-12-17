#main cmake
cmake_minimum_required(VERSION 4.0)
project(cgtee)

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
    external/tinygltf-2.9.7
	src/
)

add_executable(cgtee
	src/cgtee.cpp
    src/render/shader.cpp
)
target_link_libraries(cgtee
	${OPENGL_LIBRARY}
	glfw
	glad
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    # Release w/o console
    message(STATUS "Configuring 'cgtee' for **WINDOWS** subsystem (No Console)")
        target_link_options(cgtee PRIVATE 
        LINKER:/ENTRY:mainCRTStartup 
        LINKER:/SUBSYSTEM:WINDOWS
    )

elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug + console
    message(STATUS "Configuring 'cgtee' for **CONSOLE** subsystem")
    target_link_options(cgtee PRIVATE 
        LINKER:/SUBSYSTEM:CONSOLE
    )

else()
    # Default
    message(STATUS "Configuring 'cgtee' for default subsystem (usually CONSOLE)")
    target_link_options(cgtee PRIVATE 
        LINKER:/SUBSYSTEM:CONSOLE
    )
endif()


#set_target_properties(cgtee PROPERTIES 
#    LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")