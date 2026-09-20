#pragma once

#include <glm/glm.hpp>

extern bool cableMoving;
extern float cableT;
extern float cableDirection;
extern float cableSpeed;
extern float cabinYaw;
extern float pulleyAngle;

void updateAnimations(float deltaTime);
glm::vec3 getCableCarPosition();
void drawCableCar(const glm::vec3& position, float yawDegrees);
