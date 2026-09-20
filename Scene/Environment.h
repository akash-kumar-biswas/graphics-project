#pragma once

#include <glm/glm.hpp>

void drawWater();
void drawMountain(const glm::vec3& basePosition, const glm::vec3& scale, float yRotation, const glm::vec3& color);
float mountainSurfaceY(const glm::vec3& mountainBase, const glm::vec3& mountainScale, float worldX, float worldZ);
void drawTree(const glm::vec3& position, float scale, float yRotation);
void drawTrees();
