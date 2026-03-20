#include "ShaderClass.h"

std::string readShaderFile(const std::string& filePath) {
	std::ifstream fileStream(filePath, std::ios::in);
	if (!fileStream.is_open()) {
		std::cerr << "ERROR: Failed to open shader file: " << filePath << std::endl;
		return "";
	}

	std::stringstream sstr;
	sstr << fileStream.rdbuf();
	fileStream.close();

	return sstr.str();
}

static void printShaderLog(GLuint shader, const char* name) {
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar infoLog[1024];
		glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
		std::cerr << "ERROR: Shader compile failed (" << name << ")\n" << infoLog << std::endl;
	}
}

static void printProgramLog(GLuint program) {
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		GLchar infoLog[1024];
		glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
		std::cerr << "ERROR: Program link failed\n" << infoLog << std::endl;
	}
}


void ShaderClass::Delete(GLuint ID) {
	glDeleteProgram(ID);
}

void ShaderClass::Activate(GLuint ID) {
	glUseProgram(ID);
}

 ShaderClass::ShaderClass(const std::string& vertexPath, const std::string& fragmentPath) {
	std::string vertexCode = readShaderFile(vertexPath);
	std::string fragmentCode = readShaderFile(fragmentPath);
	
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* sourceVertex = vertexCode.c_str();
	glShaderSource(vertexShader, 1, &sourceVertex, nullptr);
	glCompileShader(vertexShader);
	printShaderLog(vertexShader, "VERTEX");

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* sourceFragment = fragmentCode.c_str();
	glShaderSource(fragmentShader, 1, &sourceFragment, nullptr);
	glCompileShader(fragmentShader);
	printShaderLog(fragmentShader, "FRAGMENT");

	ID = glCreateProgram();
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	glLinkProgram(ID);
	
	printProgramLog(ID);
	
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
}	


