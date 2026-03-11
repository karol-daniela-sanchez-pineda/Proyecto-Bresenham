#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include "Origin.h"


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
 struct VertexTriangle {
	GLfloat pos[3];
	GLfloat color[4]; // RGBA
	GLfloat angle;
};

 using Vertex3angle = VertexTriangle;



int main() {
	// Initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	

	Vertex3angle triangles [] = {

		// Triangle 0 (blue, opaque) // 0 deg in radians
		{ { -0.3f, -0.3f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, 0.0f },
		{ {  0.3f, -0.3f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, 0.0f },
		{ {  0.0f,  0.3f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, 0.0f },

		// Triangle 1 (red, rotated 30 degrees) // 30 deg in radians
		{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.4f },  0.0f },
		{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.4f }, 0.0f },
		{ {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.4f }, 0.0f },

		 // Triangle 2 (green, semi-transparent) // 60 deg in radians
		{ { -0.8f, -0.8f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.6f }, 0.0f },
		{ {  0.8f, -0.8f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.6f }, 0.0f },
		{ {  0.0f,  0.8f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.6f }, 0.0f }

		
	};	



	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);
	if (window == nullptr) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	gladLoadGL();

	glViewport(0, 0, 800, 600);

	std::string vertexShaderSource = readShaderFile("shaders/vertex.vert");
	std::string fragmentShaderSource = readShaderFile("shaders/fragment.frag");



	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* sourceVertex = vertexShaderSource.c_str();
	glShaderSource(vertexShader, 1, &sourceVertex, nullptr);
	glCompileShader(vertexShader);
	printShaderLog(vertexShader, "VERTEX");

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* sourceFragment = fragmentShaderSource.c_str();
	glShaderSource(fragmentShader, 1, &sourceFragment, nullptr);
	glCompileShader(fragmentShader);
	printShaderLog(fragmentShader, "FRAGMENT");

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	printProgramLog(shaderProgram);
	
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	Origin origin;

	// Enable blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Get location of the alpha uniform
	//GLint alphaLoc = glGetUniformLocation(shaderProgram, "uAlpha");

	GLuint  VAO_tri, VBO_tri;
	GLuint  VAO_lines = 0, VBO_lines = 0;

	glGenVertexArrays(1, &VAO_tri);
	glGenBuffers(1, &VBO_tri);
	glGenVertexArrays(1, &VAO_lines);
	glGenBuffers(1, &VBO_lines);

	glBindVertexArray(VAO_tri);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_tri);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

	// use sizeof(Vertex) and offsetof to avoid manual stride/offset math
	GLsizei stride3angles = static_cast<GLsizei>(sizeof(Vertex3angle));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride3angles, (GLvoid*)offsetof(Vertex3angle, pos));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride3angles, (GLvoid*)offsetof(Vertex3angle, color));
	glEnableVertexAttribArray(1);

	// angle attribute (location = 2)
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride3angles, (GLvoid*)offsetof(Vertex3angle, angle));
	glEnableVertexAttribArray(2);

	// unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	
	//glEnable(GL_PROGRAM_POINT_SIZE);
	//glPointSize(95);


	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers(window);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);      
	glFrontFace(GL_CCW);

	while(!glfwWindowShouldClose(window)) {
		
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(shaderProgram);

		origin.draw();
		
		// draw triangles
		glBindVertexArray(VAO_tri);
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(sizeof(triangles) / sizeof(triangles[0])));


		//glDrawArrays(GL_POINTS, 0, 3);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO_tri);
	glDeleteBuffers(1, &VBO_tri);
	glDeleteProgram(shaderProgram);
	
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}	