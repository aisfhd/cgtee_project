#include "controls.h"
#include "../other/globals.h"
#include "../world/physics.h"
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    float speed = 100.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 2.5f;
    glm::vec3 frontXZ = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 rightXZ = glm::normalize(glm::cross(cameraFront, cameraUp));
    glm::vec3 movement = glm::vec3(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement += frontXZ * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement -= frontXZ * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement -= rightXZ * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement += rightXZ * speed;
    cameraPos += movement;
    verticalVelocity += gravity * deltaTime;
    cameraPos.y += verticalVelocity * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded)
    {
        verticalVelocity = jumpForce;
        isGrounded = false;
        cameraPos.y += 1.0f;
    }
    checkCollisions(cameraPos, verticalVelocity);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    float sensitivity = 0.1f;
    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
}