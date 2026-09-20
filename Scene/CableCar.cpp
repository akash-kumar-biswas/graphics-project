#include "CableCar.h"
#include "../Core/Primitive.h"
#include "../Config/Colors.h"
#include "../Config/Constants.h"

bool cableMoving = false;
float cableT = 0.0f;
float cableDirection = 1.0f;
float cableSpeed = 0.10f;
float cabinYaw = 0.0f;
float pulleyAngle = 0.0f;

void updateAnimations(float deltaTime)
{
	if (!cableMoving)
	{
		return;
	}

	cableT += cableDirection * cableSpeed * deltaTime;
	if (cableT >= 1.0f)
	{
		cableT = 1.0f;
		cableDirection = -1.0f;
	}
	else if (cableT <= 0.0f)
	{
		cableT = 0.0f;
		cableDirection = 1.0f;
	}

	pulleyAngle += 220.0f * cableDirection * deltaTime;
}

glm::vec3 getCableCarPosition()
{
	glm::vec3 cablePoint = cableStart + cableT * (cableEnd - cableStart);
	return cablePoint + glm::vec3(0.0f, -CABIN_DROP_FROM_CABLE, 0.0f);
}

void drawCableCar(const glm::vec3& position, float yawDegrees)
{
	glm::mat4 cabinParent(1.0f);
	cabinParent = glm::translate(cabinParent, position);
	cabinParent = glm::rotate(cabinParent, glm::radians(yawDegrees), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 lowerBody = cabinParent;
	lowerBody = glm::translate(lowerBody, glm::vec3(0.0f, -0.30f, 0.0f));
	lowerBody = glm::scale(lowerBody, glm::vec3(1.60f, 0.50f, 1.05f));
	drawCube(lowerBody, CABIN_RED);

	glm::mat4 upperBody = cabinParent;
	upperBody = glm::translate(upperBody, glm::vec3(0.0f, 0.18f, 0.0f));
	upperBody = glm::scale(upperBody, glm::vec3(1.60f, 0.44f, 1.05f));
	drawCube(upperBody, CABIN_WHITE);

	glm::mat4 roof = cabinParent;
	roof = glm::translate(roof, glm::vec3(0.0f, 0.62f, 0.0f));
	roof = glm::scale(roof, glm::vec3(1.74f, 0.15f, 1.12f));
	drawCube(roof, CABIN_DARK_RED);

	glm::mat4 frontWindow = cabinParent;
	frontWindow = glm::translate(frontWindow, glm::vec3(0.0f, 0.12f, 0.69f));
	frontWindow = glm::scale(frontWindow, glm::vec3(1.12f, 0.34f, 0.035f));
	drawCube(frontWindow, CABIN_WINDOW);

	glm::mat4 rearWindow = cabinParent;
	rearWindow = glm::translate(rearWindow, glm::vec3(0.0f, 0.12f, -0.69f));
	rearWindow = glm::scale(rearWindow, glm::vec3(1.12f, 0.34f, 0.035f));
	drawCube(rearWindow, CABIN_WINDOW);

	glm::mat4 sideWindow = cabinParent;
	sideWindow = glm::translate(sideWindow, glm::vec3(-0.82f, 0.12f, 0.0f));
	sideWindow = glm::scale(sideWindow, glm::vec3(0.035f, 0.34f, 0.70f));
	drawCube(sideWindow, CABIN_WINDOW);

	glm::mat4 hanger = cabinParent;
	hanger = glm::translate(hanger, glm::vec3(0.0f, 1.02f, 0.0f));
	hanger = glm::scale(hanger, glm::vec3(0.13f, 0.66f, 0.13f));
	drawCube(hanger, CABIN_DARK);

	glm::mat4 grip = cabinParent;
	grip = glm::translate(grip, glm::vec3(0.0f, 1.38f, 0.0f));
	grip = glm::scale(grip, glm::vec3(0.48f, 0.09f, 0.15f));
	drawCube(grip, CABIN_DARK);
}
