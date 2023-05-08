#pragma once

#include <GLFW/glfw3.h>

#define KEY_ESCAPE GLFW_KEY_ESCAPE 
#define KEY_SPACE GLFW_KEY_SPACE

#define KEY_W GLFW_KEY_W
#define KEY_A GLFW_KEY_A
#define KEY_S GLFW_KEY_S
#define KEY_D GLFW_KEY_D
#define KEY_P GLFW_KEY_P

#define MOUSE_BUTTON_LEFT GLFW_MOUSE_BUTTON_LEFT
#define MOUSE_BUTTON_RIGHT GLFW_MOUSE_BUTTON_RIGHT
#define MOUSE_BUTTON_MIDDLE GLFW_MOUSE_BUTTON_MIDDLE

inline bool checkKeyPress(GLFWwindow* window, int key) {
	int status = glfwGetKey(window, key);
	return status == GLFW_PRESS || status == GLFW_REPEAT;
}

inline bool checkKeyRelease(GLFWwindow* window, int key) {
	return glfwGetKey(window, key) == GLFW_RELEASE;
}

inline bool checkMouseButtonPress(GLFWwindow* window, int key) {
	int status = glfwGetMouseButton(window, key);
	return status == GLFW_PRESS || status == GLFW_REPEAT;
}

inline bool checkMouseButtonRelease(GLFWwindow* window, int key) {
	return glfwGetMouseButton(window, key) == GLFW_RELEASE;
}