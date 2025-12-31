#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);