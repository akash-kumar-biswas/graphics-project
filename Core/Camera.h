#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern glm::vec3 cameraPosition;
extern float cameraYaw;
extern float cameraPitch;
extern float cameraMoveSpeed;
extern float cameraLookSpeed;
extern float mouseSensitivity;
extern bool firstMouse;
extern double lastMouseX;
extern double lastMouseY;
extern bool cableCarCameraActive;

glm::vec3 getCameraFront();
glm::mat4 getGlobalCameraView();
glm::mat4 getCableCarParentMatrix();
glm::mat4 getCableCarCameraView();
