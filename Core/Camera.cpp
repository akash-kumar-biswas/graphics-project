#include "Camera.h"
#include "../Config/Constants.h"

glm::vec3 cameraPosition(0.0f, 10.5f, 32.0f);
float cameraYaw = -90.0f;
float cameraPitch = -17.0f;
float cameraMoveSpeed = 8.0f;
float cameraLookSpeed = 70.0f;
float mouseSensitivity = 0.10f;
bool firstMouse = true;
double lastMouseX = INITIAL_WIDTH / 2.0;
double lastMouseY = INITIAL_HEIGHT / 2.0;
bool cableCarCameraActive = false;

glm::vec3 getCameraFront()
{
	glm::vec3 front;
	float yawRad = glm::radians(cameraYaw);
	float pitchRad = glm::radians(cameraPitch);
	front.x = std::cos(yawRad) * std::cos(pitchRad);
	front.y = std::sin(pitchRad);
	front.z = std::sin(yawRad) * std::cos(pitchRad);
	return glm::normalize(front);
}

glm::mat4 getGlobalCameraView()
{
	return glm::lookAt(cameraPosition, cameraPosition + getCameraFront(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 getCableCarParentMatrix()
{
	return glm::mat4(1.0f);
}

glm::mat4 getCableCarCameraView()
{
	return getGlobalCameraView();
}
