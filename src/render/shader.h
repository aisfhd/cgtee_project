#pragma once
#include <glad/glad.h>
#include <string>

GLuint LoadShadersFromFile(std::string vertex_file_path, std::string fragment_file_path);

GLuint LoadShadersFromString(std::string VertexShaderCode, std::string FragmentShaderCode);
