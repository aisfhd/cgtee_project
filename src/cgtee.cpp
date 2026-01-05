#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

#include "world/cube.h"
#include "world/chunk.h"
#include "world/mesh.h"

static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;

int main(){
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
   	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(windowWidth, windowHeight, "cgtee", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to open a GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
    int version = gladLoadGL();
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

   	glClearColor(0.2f, 0.2f, 0.25f, 0.0f);

    glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	double rand_var;
	double r,g,b;

    do
    {
		rand_var = glfwGetTime();
		r = sin(rand_var + M_PI_4);
		g = sin(rand_var + M_PI_2);
		b = sin(rand_var + M_PI_4 + M_PI_2);
		r = abs(r);
		g = abs(g);
		b = abs(b);
		glClearColor(r,g,b,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glfwSwapBuffers(window);
		glfwPollEvents();
    } while (!glfwWindowShouldClose(window));
	glfwTerminate();


}