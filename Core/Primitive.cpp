#include "Primitive.h"
#include "../Config/Constants.h"
#include "../Config/Colors.h"

Mesh cubeMesh;
Mesh planeMesh;
Mesh cylinderMesh;
Mesh coneMesh;

Mesh createMesh(const std::vector<float>& vertices)
{
	Mesh mesh;
	mesh.vertexCount = static_cast<int>(vertices.size() / 3);

	glGenVertexArrays(1, &mesh.VAO);
	glGenBuffers(1, &mesh.VBO);

	glBindVertexArray(mesh.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return mesh;
}

Mesh createCubeMesh()
{
	std::vector<float> vertices =
	{
		-0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
		 0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
		-0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
		 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
		-0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
		-0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
		 0.5f, 0.5f, 0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
		 0.5f,-0.5f,-0.5f,  0.5f, 0.5f, 0.5f,  0.5f,-0.5f, 0.5f,
		-0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
		 0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
		-0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
		 0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f, 0.5f, 0.5f
	};
	return createMesh(vertices);
}

Mesh createPlaneMesh()
{
	return createMesh({ -0.5f,0.0f,-0.5f, 0.5f,0.0f,-0.5f, 0.5f,0.0f,0.5f, 0.5f,0.0f,0.5f, -0.5f,0.0f,0.5f, -0.5f,0.0f,-0.5f });
}

Mesh createCylinderMesh(int segments)
{
	std::vector<float> vertices;
	const float yBottom = -0.5f, yTop = 0.5f, radius = 0.5f;
	for (int i = 0; i < segments; ++i)
	{
		float a0 = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
		float a1 = 2.0f * PI * static_cast<float>(i + 1) / static_cast<float>(segments);
		float x0 = radius * std::cos(a0), z0 = radius * std::sin(a0);
		float x1 = radius * std::cos(a1), z1 = radius * std::sin(a1);
		vertices.insert(vertices.end(), { x0,yBottom,z0, x1,yBottom,z1, x1,yTop,z1, x1,yTop,z1, x0,yTop,z0, x0,yBottom,z0 });
		vertices.insert(vertices.end(), { 0.0f,yTop,0.0f, x1,yTop,z1, x0,yTop,z0 });
		vertices.insert(vertices.end(), { 0.0f,yBottom,0.0f, x0,yBottom,z0, x1,yBottom,z1 });
	}
	return createMesh(vertices);
}

Mesh createConeMesh(int segments)
{
	std::vector<float> vertices;
	const float baseY = -0.5f, apexY = 0.5f, radius = 0.5f;
	for (int i = 0; i < segments; ++i)
	{
		float a0 = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
		float a1 = 2.0f * PI * static_cast<float>(i + 1) / static_cast<float>(segments);
		float x0 = radius * std::cos(a0), z0 = radius * std::sin(a0);
		float x1 = radius * std::cos(a1), z1 = radius * std::sin(a1);
		vertices.insert(vertices.end(), { 0.0f,apexY,0.0f, x0,baseY,z0, x1,baseY,z1 });
		vertices.insert(vertices.end(), { 0.0f,baseY,0.0f, x1,baseY,z1, x0,baseY,z0 });
	}
	return createMesh(vertices);
}

void deleteMesh(Mesh& mesh)
{
	if (mesh.VAO != 0) glDeleteVertexArrays(1, &mesh.VAO);
	if (mesh.VBO != 0) glDeleteBuffers(1, &mesh.VBO);
	mesh = Mesh();
}

void drawMesh(const Mesh& mesh, const glm::mat4& model, const glm::vec3& color)
{
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniform3fv(colorLoc, 1, glm::value_ptr(color));
	glBindVertexArray(mesh.VAO);
	glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
	glBindVertexArray(0);
}

void drawCube(const glm::mat4& model, const glm::vec3& color) { drawMesh(cubeMesh, model, color); }
void drawPlane(const glm::mat4& model, const glm::vec3& color) { drawMesh(planeMesh, model, color); }
void drawCylinder(const glm::mat4& model, const glm::vec3& color) { drawMesh(cylinderMesh, model, color); }
void drawCone(const glm::mat4& model, const glm::vec3& color) { drawMesh(coneMesh, model, color); }
