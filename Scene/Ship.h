#pragma once

#include <glm/glm.hpp>

extern glm::vec3 shipPosition;
extern float shipYaw;
extern float shipMoveSpeed;
extern float shipRotateSpeed;

bool shipPositionIsValid(const glm::vec3& candidate);
void tryMoveShip(float signedDistance);
void drawShip();
