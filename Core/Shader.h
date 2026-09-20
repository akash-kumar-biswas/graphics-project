#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

extern unsigned int shaderProgram;
extern int modelLoc;
extern int viewLoc;
extern int projectionLoc;
extern int colorLoc;

unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);
void checkShaderCompile(unsigned int shader, const char* name);
void checkProgramLink(unsigned int program);
