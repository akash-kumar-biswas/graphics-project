#pragma once

#include <vector>
#include <glm/glm.hpp>

const unsigned int INITIAL_WIDTH = 1280;
const unsigned int INITIAL_HEIGHT = 720;

const float PI = 3.14159265359f;

const float WATER_HALF_X = 22.0f;
const float WATER_HALF_Z = 11.5f;
const float SHIP_COLLISION_RADIUS = 1.70f;

struct RectObstacle
{
	float minX;
	float maxX;
	float minZ;
	float maxZ;
};

struct CircleObstacle
{
	glm::vec2 center;
	float radius;
};

const RectObstacle LEFT_ISLAND{ -20.0f, -8.4f, -5.3f, 5.3f };
const RectObstacle RIGHT_ISLAND{ 8.4f, 20.0f, -5.3f, 5.3f };
const RectObstacle STATION_A_OBSTACLE{ -12.3f, -6.2f, -2.2f, 2.2f };
const RectObstacle STATION_B_OBSTACLE{ 6.2f, 12.3f, -2.2f, 2.2f };

const std::vector<CircleObstacle> MOUNTAIN_OBSTACLES =
{
	{ glm::vec2(-16.0f, -0.5f), 4.30f },
	{ glm::vec2(-17.5f, -3.4f), 3.05f },
	{ glm::vec2(-16.8f,  3.5f), 2.80f },
	{ glm::vec2(16.0f,  0.0f), 4.40f },
	{ glm::vec2(17.5f, -3.5f), 3.10f },
	{ glm::vec2(16.8f,  3.5f), 2.80f }
};

const glm::vec3 stationAPosition(-9.40f, 4.55f, 0.0f);
const glm::vec3 stationBPosition(9.40f, 4.95f, 0.0f);
const glm::vec3 cableStart(-9.40f, 6.95f, 0.0f);
const glm::vec3 cableEnd(9.40f, 7.35f, 0.0f);

const float CABLE_DOCK_T_A = 0.145f;
const float CABLE_DOCK_T_B = 0.855f;
const float CABIN_DROP_FROM_CABLE = 1.65f;
const float PULLEY_RADIUS_WORLD = 0.62f;

const glm::vec3 shipPositionStart(0.0f, 0.26f, 6.4f);
