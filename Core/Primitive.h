#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

#include "Shader.h"

struct Mesh
{
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	int vertexCount = 0;
};

extern Mesh cubeMesh;
extern Mesh planeMesh;
extern Mesh cylinderMesh;
extern Mesh coneMesh;

Mesh createMesh(const std::vector<float>& vertices);
Mesh createCubeMesh();
Mesh createPlaneMesh();
Mesh createCylinderMesh(int segments = 16);
Mesh createConeMesh(int segments = 8);
void deleteMesh(Mesh& mesh);

void drawMesh(const Mesh& mesh, const glm::mat4& model, const glm::vec3& color);
void drawCube(const glm::mat4& model, const glm::vec3& color);
void drawPlane(const glm::mat4& model, const glm::vec3& color);
void drawCylinder(const glm::mat4& model, const glm::vec3& color);
void drawCone(const glm::mat4& model, const glm::vec3& color);
