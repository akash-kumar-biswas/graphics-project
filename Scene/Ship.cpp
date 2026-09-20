#include "Ship.h"
#include "../Core/Primitive.h"
#include "../Config/Colors.h"
#include "../Config/Constants.h"

glm::vec3 shipPosition = shipPositionStart;
float shipYaw = 180.0f;
float shipMoveSpeed = 3.2f;
float shipRotateSpeed = 70.0f;

static bool circleIntersectsRectangle(const glm::vec2& center, float radius, const RectObstacle& rect)
{
	float closestX = std::clamp(center.x, rect.minX, rect.maxX);
	float closestZ = std::clamp(center.y, rect.minZ, rect.maxZ);
	float dx = center.x - closestX;
	float dz = center.y - closestZ;
	return dx * dx + dz * dz < radius * radius;
}

bool shipPositionIsValid(const glm::vec3& candidate)
{
	if (candidate.x < -WATER_HALF_X + SHIP_COLLISION_RADIUS || candidate.x > WATER_HALF_X - SHIP_COLLISION_RADIUS || candidate.z < -WATER_HALF_Z + SHIP_COLLISION_RADIUS || candidate.z > WATER_HALF_Z - SHIP_COLLISION_RADIUS)
		return false;

	glm::vec2 shipXZ(candidate.x, candidate.z);
	if (circleIntersectsRectangle(shipXZ, SHIP_COLLISION_RADIUS, LEFT_ISLAND)) return false;
	if (circleIntersectsRectangle(shipXZ, SHIP_COLLISION_RADIUS, RIGHT_ISLAND)) return false;
	if (circleIntersectsRectangle(shipXZ, SHIP_COLLISION_RADIUS, STATION_A_OBSTACLE)) return false;
	if (circleIntersectsRectangle(shipXZ, SHIP_COLLISION_RADIUS, STATION_B_OBSTACLE)) return false;

	for (const CircleObstacle& mountain : MOUNTAIN_OBSTACLES)
	{
		float safeDistance = mountain.radius + SHIP_COLLISION_RADIUS;
		float actualDistance = glm::length(shipXZ - mountain.center);
		if (actualDistance < safeDistance)
			return false;
	}

	return true;
}

void tryMoveShip(float signedDistance)
{
	float yawRadians = glm::radians(shipYaw);
	glm::vec3 direction(std::sin(yawRadians), 0.0f, std::cos(yawRadians));
	glm::vec3 candidate = shipPosition + direction * signedDistance;
	if (shipPositionIsValid(candidate))
	{
		shipPosition = candidate;
	}
}

void drawShip()
{
	glm::mat4 shipParent(1.0f);
	shipParent = glm::translate(shipParent, shipPosition);
	shipParent = glm::rotate(shipParent, glm::radians(shipYaw), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 hull = shipParent;
	hull = glm::scale(hull, glm::vec3(1.35f, 0.42f, 3.10f));
	drawCube(hull, SHIP_HULL);

	glm::mat4 bow = shipParent;
	bow = glm::translate(bow, glm::vec3(0.0f, 0.0f, 1.55f));
	bow = glm::scale(bow, glm::vec3(0.96f, 0.30f, 0.86f));
	drawCube(bow, SHIP_HULL);

	glm::mat4 deck = shipParent;
	deck = glm::translate(deck, glm::vec3(0.0f, 0.32f, 0.0f));
	deck = glm::scale(deck, glm::vec3(1.10f, 0.18f, 1.55f));
	drawCube(deck, SHIP_WHITE);

	glm::mat4 cabin = shipParent;
	cabin = glm::translate(cabin, glm::vec3(-0.10f, 0.62f, -0.28f));
	cabin = glm::scale(cabin, glm::vec3(0.80f, 0.44f, 1.05f));
	drawCube(cabin, SHIP_WHITE);

	glm::mat4 roof = shipParent;
	roof = glm::translate(roof, glm::vec3(-0.10f, 0.92f, -0.28f));
	roof = glm::scale(roof, glm::vec3(0.95f, 0.16f, 1.10f));
	drawCube(roof, SHIP_RED);

	glm::mat4 chimney = shipParent;
	chimney = glm::translate(chimney, glm::vec3(-0.52f, 1.04f, -0.35f));
	chimney = glm::scale(chimney, glm::vec3(0.16f, 0.42f, 0.16f));
	drawCube(chimney, SHIP_DARK);
}
