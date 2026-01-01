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
    src/render/texture.cpp
    src/render/model.cpp

    src/world/mesh.cpp
    src/world/chunk.cpp
    src/world/worldgen.cpp
    src/world/physics.cpp

    src/other/globals.cpp
    src/other/pathglobals.cpp
    src/other/controls.cpp
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

install(TARGETS cgtee DESTINATION bin)
install(DIRECTORY src/assets/ DESTINATION assets)
install(DIRECTORY src/shaders/ DESTINATION shaders)