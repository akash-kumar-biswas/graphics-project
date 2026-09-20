#pragma once

#include <GLFW/glfw3.h>

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void processContinuousInput(GLFWwindow* window, float deltaTime);
void printControls();
