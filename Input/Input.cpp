#include "Input.h"
#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../Core/Camera.h"
#include "../Scene/CableCar.h"
#include "../Scene/Ship.h"

static bool keyWasDown[GLFW_KEY_LAST + 1] = {};

static bool pressedOnce(GLFWwindow* window, int key)
{
	bool down = glfwGetKey(window, key) == GLFW_PRESS;
	bool result = down && !keyWasDown[key];
	keyWasDown[key] = down;
	return result;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void)scancode; (void)mods;
	if (action == GLFW_RELEASE)
	{
		keyWasDown[key] = false;
	}
	if (action != GLFW_PRESS)
		return;

	if (key == GLFW_KEY_SPACE)
		cableMoving = !cableMoving;
}

void mouseCallback(GLFWwindow*, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastMouseX = xpos;
		lastMouseY = ypos;
		firstMouse = false;
		return;
	}

	float xOffset = static_cast<float>(xpos - lastMouseX);
	float yOffset = static_cast<float>(lastMouseY - ypos);
	lastMouseX = xpos;
	lastMouseY = ypos;

	cameraYaw += xOffset * mouseSensitivity;
	cameraPitch += yOffset * mouseSensitivity;
	cameraPitch = std::clamp(cameraPitch, -89.0f, 89.0f);
}

void processContinuousInput(GLFWwindow* window, float deltaTime)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		cabinYaw += 80.0f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		cabinYaw -= 80.0f * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
		cableSpeed = std::min(cableSpeed + 0.20f * deltaTime, 1.0f);
	if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
		cableSpeed = std::max(cableSpeed - 0.20f * deltaTime, 0.02f);

	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
		tryMoveShip(shipMoveSpeed * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		tryMoveShip(-shipMoveSpeed * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		shipYaw += shipRotateSpeed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		shipYaw -= shipRotateSpeed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
		shipMoveSpeed = std::min(shipMoveSpeed + 0.4f * deltaTime, 5.0f);
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		shipMoveSpeed = std::max(shipMoveSpeed - 0.4f * deltaTime, 1.0f);

	glm::vec3 front = getCameraFront();
	glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
	glm::vec3 flatForward = front; flatForward.y = 0.0f;
	if (glm::length(flatForward) > 0.0001f)
		flatForward = glm::normalize(flatForward);

	float cameraStep = cameraMoveSpeed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPosition += flatForward * cameraStep;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPosition -= flatForward * cameraStep;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPosition -= right * cameraStep;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPosition += right * cameraStep;
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) cameraPosition += worldUp * cameraStep;
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) cameraPosition -= worldUp * cameraStep;

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) cameraYaw -= cameraLookSpeed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cameraYaw += cameraLookSpeed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) cameraPitch += cameraLookSpeed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) cameraPitch -= cameraLookSpeed * deltaTime;
	cameraPitch = std::clamp(cameraPitch, -89.0f, 89.0f);
}

void printControls()
{
	std::cout << "\n==============================================\n";
	std::cout << "3D CABLE CAR SIMULATION\n";
	std::cout << "==============================================\n";
	std::cout << "CABLE CAR:\n";
	std::cout << "  SPACE : Start / stop movement\n";
	std::cout << "  Q / E : Rotate cabin left / right around local Y\n";
	std::cout << "  + / - : Increase / decrease cable speed\n";
	std::cout << "SHIP:\n";
	std::cout << "  I / K : Move forward / backward\n";
	std::cout << "  J / L : Turn left / right\n";
	std::cout << "  U / O : Increase / decrease ship speed\n";
	std::cout << "CAMERA:\n";
	std::cout << "  W / S : Forward / backward\n";
	std::cout << "  A / D : Left / right\n";
	std::cout << "  R / F : Up / down\n";
	std::cout << "  Mouse or Arrow Keys : Look around\n";
	std::cout << "PROGRAM:\n";
	std::cout << "  ESC : Exit\n";
	std::cout << "==============================================\n\n";
}
