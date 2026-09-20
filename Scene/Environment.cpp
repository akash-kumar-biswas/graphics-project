#include "Environment.h"
#include "../Core/Primitive.h"
#include "../Config/Colors.h"
#include "../Config/Constants.h"

void drawWater()
{
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.55f, 0.0f));
	model = glm::scale(model, glm::vec3(44.0f, 1.0f, 30.0f));
	drawPlane(model, WATER_COLOR);
}

void drawMountain(const glm::vec3& basePosition, const glm::vec3& scale, float yRotation, const glm::vec3& color)
{
	glm::mat4 model(1.0f);
	model = glm::translate(model, basePosition + glm::vec3(0.0f, scale.y * 0.5f, 0.0f));
	model = glm::rotate(model, glm::radians(yRotation), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, scale);
	drawCone(model, color);
}

float mountainSurfaceY(const glm::vec3& mountainBase, const glm::vec3& mountainScale, float worldX, float worldZ)
{
	float dx = worldX - mountainBase.x;
	float dz = worldZ - mountainBase.z;
	float radialDistance = std::sqrt(dx * dx + dz * dz);
	float radius = 0.5f * std::max(mountainScale.x, mountainScale.z);
	float factor = std::clamp(1.0f - radialDistance / radius, 0.0f, 1.0f);
	return mountainBase.y + mountainScale.y * factor;
}

void drawTree(const glm::vec3& position, float scale, float yRotation)
{
	glm::mat4 parent(1.0f);
	parent = glm::translate(parent, position);
	parent = glm::rotate(parent, glm::radians(yRotation), glm::vec3(0.0f, 1.0f, 0.0f));
	parent = glm::scale(parent, glm::vec3(scale));

	glm::mat4 trunk = parent;
	trunk = glm::translate(trunk, glm::vec3(0.0f, 0.55f, 0.0f));
	trunk = glm::scale(trunk, glm::vec3(0.24f, 1.10f, 0.24f));
	drawCylinder(trunk, TREE_TRUNK);

	glm::mat4 leaves1 = parent;
	leaves1 = glm::translate(leaves1, glm::vec3(0.0f, 1.45f, 0.0f));
	leaves1 = glm::scale(leaves1, glm::vec3(1.10f, 1.65f, 1.10f));
	drawCone(leaves1, TREE_GREEN);

	glm::mat4 leaves2 = parent;
	leaves2 = glm::translate(leaves2, glm::vec3(0.0f, 2.05f, 0.0f));
	leaves2 = glm::scale(leaves2, glm::vec3(0.78f, 1.25f, 0.78f));
	drawCone(leaves2, TREE_GREEN_2);
}

void drawTrees()
{
	const glm::vec3 leftBase(-12.8f, 0.44f, -0.5f);
	const glm::vec3 leftScale(6.7f, 7.6f, 6.6f);
	const glm::vec3 rightBase(12.8f, 0.44f, 0.0f);
	const glm::vec3 rightScale(6.9f, 8.4f, 6.8f);

	struct TreePlacement { float x; float z; float scale; float yaw; bool leftMountain; };
	const TreePlacement trees[] =
	{
		{-14.0f,  1.0f, 0.72f,  18.0f, true },
		{-13.2f, -2.3f, 0.62f, -22.0f, true },
		{-11.0f,  1.2f, 0.68f,  30.0f, true },
		{-10.4f, -1.6f, 0.58f, -15.0f, true },
		{-12.0f,  2.0f, 0.57f,  42.0f, true },
		{-10.8f, -2.1f, 0.60f, -35.0f, true },
		{ -9.8f,  2.1f, 0.55f,  12.0f, true },
		{ 14.0f,  1.0f, 0.72f, -18.0f, false },
		{ 13.2f, -2.3f, 0.63f,  24.0f, false },
		{ 11.0f,  1.2f, 0.69f, -30.0f, false },
		{ 10.4f, -1.7f, 0.58f,  15.0f, false },
		{ 12.0f,  2.0f, 0.58f, -42.0f, false },
		{ 10.8f, -2.2f, 0.61f,  34.0f, false },
		{  9.8f,  2.1f, 0.56f, -10.0f, false }
	};

	for (const TreePlacement& tree : trees)
	{
		const glm::vec3& base = tree.leftMountain ? leftBase : rightBase;
		const glm::vec3& mountainScale = tree.leftMountain ? leftScale : rightScale;
		float y = mountainSurfaceY(base, mountainScale, tree.x, tree.z);
		drawTree(glm::vec3(tree.x, y, tree.z), tree.scale, tree.yaw);
	}

	drawTree(glm::vec3(-16.2f, 0.49f, 3.65f), 0.72f, 12.0f);
	drawTree(glm::vec3(-16.4f, 0.49f, -3.70f), 0.66f, -20.0f);
	drawTree(glm::vec3(16.2f, 0.49f, 3.65f), 0.72f, -12.0f);
	drawTree(glm::vec3(16.4f, 0.49f, -3.70f), 0.66f, 20.0f);
}
