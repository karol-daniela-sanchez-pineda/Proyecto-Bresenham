#ifndef SHADERCLASS_H
#define SHADERCLASS_H
#include<glad/glad.h>
#include<string>
#include<fstream>
#include<sstream>
#include<iostream>
#include<cerrno>

static void printShaderLog(GLuint shader, const char* name);
static void printProgramLog(GLuint program);
std::string readShaderFile(const std::string& filePath);

class ShaderClass
{
public:

	GLuint ID;
	ShaderClass(const std::string& vertexPath, const std::string& fragmentPath);
	void Activate(GLuint ID);
	void Delete(GLuint ID);

};

#endif // SHADERCLASS_H
