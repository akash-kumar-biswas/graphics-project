#include "Shader.h"

unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource)
{
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, nullptr);
	glCompileShader(vertexShader);
	checkShaderCompile(vertexShader, "VERTEX SHADER");

	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
	glCompileShader(fragmentShader);
	checkShaderCompile(fragmentShader, "FRAGMENT SHADER");

	unsigned int program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	checkProgramLink(program);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}

void checkShaderCompile(unsigned int shader, const char* name)
{
	int success = 0;
	char infoLog[1024];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
		std::cout << name << " compilation failed:\n" << infoLog << '\n';
	}
}

void checkProgramLink(unsigned int program)
{
	int success = 0;
	char infoLog[1024];

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(program, 1024, nullptr, infoLog);
		std::cout << "Program link failed:\n" << infoLog << '\n';
	}
}
