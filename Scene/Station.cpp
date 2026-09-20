#include "Station.h"
#include "../Core/Primitive.h"
#include "../Config/Colors.h"
#include "../Config/Constants.h"

void drawStation(const glm::vec3& position, bool stationA)
{
	const glm::vec3 accent = stationA ? STATION_ACCENT_A : STATION_ACCENT_B;

	glm::mat4 floor(1.0f);
	floor = glm::translate(floor, position + glm::vec3(0.0f, -0.10f, 0.0f));
	floor = glm::scale(floor, glm::vec3(4.75f, 0.28f, 3.45f));
	drawCube(floor, STATION_FLOOR);

	const float postX = 2.05f;
	const float postZ = 1.35f;
	for (const glm::vec3& offset : { glm::vec3(-postX, 0.0f, -postZ), glm::vec3(postX, 0.0f, -postZ), glm::vec3(-postX, 0.0f, postZ), glm::vec3(postX, 0.0f, postZ) })
	{
		glm::mat4 support(1.0f);
		support = glm::translate(support, position + offset + glm::vec3(0.0f, 1.25f, 0.0f));
		support = glm::scale(support, glm::vec3(0.24f, 2.50f, 0.24f));
		drawCube(support, STATION_DARK);
	}

	glm::mat4 topFrame(1.0f);
	topFrame = glm::translate(topFrame, position + glm::vec3(0.0f, 2.15f, 0.0f));
	topFrame = glm::scale(topFrame, glm::vec3(4.20f, 0.16f, 2.75f));
	drawCube(topFrame, STATION_WALL_2);

	glm::mat4 roof(1.0f);
	roof = glm::translate(roof, position + glm::vec3(0.0f, 3.05f, 0.0f));
	roof = glm::scale(roof, glm::vec3(4.95f, 0.20f, 3.55f));
	drawCube(roof, STATION_DARK);

	glm::mat4 sign(1.0f);
	sign = glm::translate(sign, position + glm::vec3(0.0f, 1.55f, 1.44f));
	sign = glm::scale(sign, glm::vec3(0.95f, 0.34f, 0.08f));
	drawCube(sign, accent);

	glm::mat4 cableGuide(1.0f);
	cableGuide = glm::translate(cableGuide, position + glm::vec3(0.0f, 2.55f, 1.22f));
	cableGuide = glm::scale(cableGuide, glm::vec3(1.90f, 0.10f, 0.10f));
	drawCube(cableGuide, STATION_DARK);
}

void drawCable()
{
	glm::vec3 direction = cableEnd - cableStart;
	glm::vec3 midpoint = (cableStart + cableEnd) * 0.5f;
	float length = glm::length(direction);
	float angleZ = std::atan2(direction.y, direction.x);

	glm::mat4 model(1.0f);
	model = glm::translate(model, midpoint);
	model = glm::rotate(model, angleZ, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(length, 0.055f, 0.055f));
	drawCube(model, CABLE_COLOR);
}

void drawPulley(const glm::vec3& position, float angleDegrees)
{
	glm::mat4 roofPlate(1.0f);
	roofPlate = glm::translate(roofPlate, position + glm::vec3(0.0f, 0.92f, 0.0f));
	roofPlate = glm::scale(roofPlate, glm::vec3(2.00f, 0.16f, 0.40f));
	drawCube(roofPlate, STATION_DARK);

	const float hangerX = 0.62f;
	for (float x : { -hangerX, hangerX })
	{
		glm::mat4 hanger(1.0f);
		hanger = glm::translate(hanger, position + glm::vec3(x, 0.46f, 0.16f));
		hanger = glm::scale(hanger, glm::vec3(0.16f, 1.00f, 0.18f));
		drawCube(hanger, STATION_DARK);
	}

	glm::mat4 axle(1.0f);
	axle = glm::translate(axle, position + glm::vec3(0.0f, 0.02f, -0.03f));
	axle = glm::rotate(axle, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	axle = glm::scale(axle, glm::vec3(0.22f, 1.00f, 0.22f));
	drawCylinder(axle, PULLEY_SPOKE);

	glm::mat4 wheelParent(1.0f);
	wheelParent = glm::translate(wheelParent, position);
	wheelParent = glm::rotate(wheelParent, glm::radians(angleDegrees), glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 wheel = wheelParent;
	wheel = glm::rotate(wheel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	wheel = glm::scale(wheel, glm::vec3(1.10f, 0.26f, 1.10f));
	drawCylinder(wheel, PULLEY_COLOR);

	glm::mat4 spoke1 = wheelParent;
	spoke1 = glm::scale(spoke1, glm::vec3(1.00f, 0.08f, 0.30f));
	drawCube(spoke1, PULLEY_SPOKE);

	glm::mat4 spoke2 = wheelParent;
	spoke2 = glm::rotate(spoke2, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	spoke2 = glm::scale(spoke2, glm::vec3(1.00f, 0.08f, 0.30f));
	drawCube(spoke2, PULLEY_SPOKE);

	glm::mat4 hub(1.0f);
	hub = glm::translate(hub, position + glm::vec3(0.0f, 0.0f, 0.15f));
	hub = glm::rotate(hub, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	hub = glm::scale(hub, glm::vec3(0.22f, 0.14f, 0.22f));
	drawCylinder(hub, PULLEY_SPOKE);
}
