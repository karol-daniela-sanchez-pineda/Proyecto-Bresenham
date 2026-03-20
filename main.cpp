#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstddef>
#include "ShaderClass.h"
#include "Origin.h"


 struct VertexSquare {
	 GLfloat pos[3];
	 GLfloat color[4]; // RGBA
 };

 // --- add near the top of main.cpp (after CreateGridSquaresForViewport/PaintLineBresenham) ---
 static GLuint g_VAO_sq = 0, g_VBO_sq = 0, g_EBO_sq = 0;
 static std::vector<VertexSquare> g_gridVerts;
 static std::vector<GLuint> g_gridIndices;
 static int g_gridCols = 80;
 static int g_gridRows = 80;
 static GLfloat g_baseColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
 static GLfloat g_lineColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
 static int g_line_x0 = 25, g_line_y0 = 12, g_line_x1 = 70, g_line_y1 = 25;
 static GLsizei g_squareIndexCount = 0;
 static float g_startX = 0.0f, g_startY = 0.0f, g_cellW = 0.0f, g_cellH = 0.0f;

 // Creates VAO/VBO/EBO for a square (or any vertex/index arrays matching VertexSquare)
 static void CreateSquareVAO(GLuint& VAO, GLuint& VBO, GLuint& EBO,
	 const VertexSquare* verts, size_t vertCount,
	 const GLuint* indices, size_t indexCount)
 {
	 glGenVertexArrays(1, &VAO);
	 glGenBuffers(1, &VBO);
	 glGenBuffers(1, &EBO);

	 glBindVertexArray(VAO);

	 glBindBuffer(GL_ARRAY_BUFFER, VBO);
	 glBufferData(GL_ARRAY_BUFFER, vertCount * sizeof(VertexSquare), verts, GL_STATIC_DRAW);

	 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	 glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), indices, GL_STATIC_DRAW);

	 GLsizei strideSquare = static_cast<GLsizei>(sizeof(VertexSquare));
	 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideSquare, (GLvoid*)offsetof(VertexSquare, pos));
	 glEnableVertexAttribArray(0);

	 glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, strideSquare, (GLvoid*)offsetof(VertexSquare, color));
	 glEnableVertexAttribArray(1);

	 // unbind array buffer and VAO (element array binding is stored in VAO)
	 glBindBuffer(GL_ARRAY_BUFFER, 0);
	 glBindVertexArray(0);
	 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
 }


 // Build a grid of squares (cols x rows). Each square is 4 consecutive VertexSquare entries; returns vertices and indices.
 static void CreateGridSquares(std::vector<VertexSquare>& outVerts, std::vector<GLuint>& outIndices,
	 int cols, int rows, float startX, float startY, float cellW, float cellH,
	 const GLfloat color[4])
 {
	 outVerts.clear();
	 outIndices.clear();
	 outVerts.reserve(static_cast<size_t>(cols) * rows * 4);
	 outIndices.reserve(static_cast<size_t>(cols) * rows * 6);

	 for (int y = 0; y < rows; ++y) {
		 for (int x = 0; x < cols; ++x) {
			 float cx = startX + (x + 0.5f) * cellW;
			 float cy = startY + (y + 0.5f) * cellH;
			 float halfW = cellW * 0.5f;
			 float halfH = cellH * 0.5f;

			 VertexSquare v0 = { { cx + halfW, cy + halfH, 0.0f }, { color[0], color[1], color[2], color[3] } }; // top-right
			 VertexSquare v1 = { { cx + halfW, cy - halfH, 0.0f }, { color[0], color[1], color[2], color[3] } }; // bottom-right
			 VertexSquare v2 = { { cx - halfW, cy - halfH, 0.0f }, { color[0], color[1], color[2], color[3] } }; // bottom-left
			 VertexSquare v3 = { { cx - halfW, cy + halfH, 0.0f }, { color[0], color[1], color[2], color[3] } }; // top-left

			 GLuint base = static_cast<GLuint>(outVerts.size());
			 outVerts.push_back(v0);
			 outVerts.push_back(v1);
			 outVerts.push_back(v2);
			 outVerts.push_back(v3);

			 outIndices.push_back(base + 0);
			 outIndices.push_back(base + 1);
			 outIndices.push_back(base + 2);
			 outIndices.push_back(base + 0);
			 outIndices.push_back(base + 2);
			 outIndices.push_back(base + 3);
		 }
	 }
 }

 // Add this helper (place near CreateGridSquares in the file)
 static void CreateGridSquaresForViewport(std::vector<VertexSquare>& outVerts, std::vector<GLuint>& outIndices,
	 int cols, int rows, const GLfloat color[4],
	 float& outStartX, float& outStartY, float& outCellW, float& outCellH)
 {
	 // Query current viewport to ensure grid maps to full NDC -> window mapping
	 GLint vp[4] = { 0,0,800,600 };
	 glGetIntegerv(GL_VIEWPORT, vp); // vp = {x, y, width, height} (must be called after context is current)
	 // We generate the grid in NDC space [-1, +1] for both axes. The viewport maps NDC to pixels.
	 // Compute cell size in NDC so grid covers entire viewport.
	 const float ndcWidth = 2.0f;   // -1 .. +1
	 const float ndcHeight = 2.0f;  // -1 .. +1

	 outCellW = ndcWidth / static_cast<float>(cols);
	 outCellH = ndcHeight / static_cast<float>(rows);

	 // startX/startY are the lower-left NDC corner used by CreateGridSquares.
	 outStartX = -1.0f;
	 outStartY = -1.0f;

	 // Delegate to existing function that builds verts/indices using NDC coords.
	 CreateGridSquares(outVerts, outIndices, cols, rows, outStartX, outStartY, outCellW, outCellH, color);
 }

 

 // Paint a line on the grid of squares using Bresenham integer algorithm.
 // verts: vector created by CreateGridSquares (4 vertices per cell).
 // cols/rows: grid dimensions in cells.
 // x0,y0,x1,y1: integer cell coordinates (0..cols-1 / 0..rows-1).
 // color: RGBA to apply to the square's vertices.
 static void PaintLineBresenham(std::vector<VertexSquare>& verts, int cols, int rows,
	 int x0, int y0, int x1, int y1, const GLfloat color[4])
 {
	 auto setCellColor = [&](int cx, int cy) {
		 if (cx < 0 || cx >= cols || cy < 0 || cy >= rows) return;
		 size_t cellIndex = static_cast<size_t>(cy) * cols + static_cast<size_t>(cx);
		 size_t base = cellIndex * 4; // 4 vertices per square
			 for (size_t i = 0; i < 4; ++i) {
				 verts[base + i].color[0] = color[0];
				 verts[base + i].color[1] = color[1];
				 verts[base + i].color[2] = color[2];
				 verts[base + i].color[3] = color[3];
			 }
		 };

	 int dx = std::abs(x1 - x0);
	 int sx = x0 < x1 ? 1 : -1;
	 int dy = -std::abs(y1 - y0);
	 int sy = y0 < y1 ? 1 : -1;
	 int err = dx + dy;

	 while (true) {
		 setCellColor(x0, y0);
		 if (x0 == x1 && y0 == y1) break;
		 int e2 = 2 * err;
		 if (e2 >= dy) { err += dy; x0 += sx; }
		 if (e2 <= dx) { err += dx; y0 += sy; }
	 }
 }

 // Recreate or update the grid VAO/VBO/EBO to match the current viewport.
// Call after glViewport(...) has been applied.
 static void RecreateGridForViewport(GLFWwindow* /*window*/)
 {
	 // Build grid geometry sized to NDC [-1,+1] using helper
	 CreateGridSquaresForViewport(g_gridVerts, g_gridIndices, g_gridCols, g_gridRows, g_baseColor, g_startX, g_startY, g_cellW, g_cellH);

	 // Apply the Bresenham paint on CPU (repaint after resizing)
	 PaintLineBresenham(g_gridVerts, g_gridCols, g_gridRows, g_line_x0, g_line_y0, g_line_x1, g_line_y1, g_lineColor);

	 // delete old buffers if exist
	 if (g_VAO_sq) {
		 glDeleteVertexArrays(1, &g_VAO_sq);
		 glDeleteBuffers(1, &g_VBO_sq);
		 glDeleteBuffers(1, &g_EBO_sq);
		 g_VAO_sq = g_VBO_sq = g_EBO_sq = 0;
	 }

	 // Create new VAO/VBO/EBO containing the updated grid
	 CreateSquareVAO(g_VAO_sq, g_VBO_sq, g_EBO_sq, g_gridVerts.data(), g_gridVerts.size(), g_gridIndices.data(), g_gridIndices.size());
	 g_squareIndexCount = static_cast<GLsizei>(g_gridIndices.size());
 }

 // GLFW framebuffer-size callback: update viewport and rebuild grid
 static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
 {
	 // Update GL viewport to new framebuffer size
	 glViewport(0, 0, width, height);

	 // Recreate grid geometry/buffers so NDC->grid mapping stays exact
	 RecreateGridForViewport(window);
 }


 
 
int main() {
	// Initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	// --- square data using VertexSquare ---
	VertexSquare squareVerts[] = {
		{ {  0.01f,  0.01f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // top-right
		{ {  0.01f, -0.01f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // bottom-right
		{ { -0.01f, -0.01f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // bottom-left
		{ { -0.01f,  0.01f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }  // top-left
	};

	GLuint squareIndices[] = {
		0, 1, 2, // first triangle
		0, 2, 3  // second triangle
	};
	// ----------------------------------------


	//render window setup (CONSTRUCT)
	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);
	if (window == nullptr) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	gladLoadGL();

	// resize window callback to adjust viewport and grid
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// recreate grid to match current viewport
	RecreateGridForViewport(window);

	ShaderClass shader("vertex.vs", "fragment.fs");

	// x axis and y axis lines for reference, z is not visible because we are in 2D
	Origin origin;

	CreateGridSquaresForViewport(g_gridVerts, g_gridIndices, g_gridCols, g_gridRows, g_baseColor, g_startX, g_startY, g_cellW, g_cellH);

	// paint a horizontal Bresenham line across the row (cell coords 0..cols-1)
	PaintLineBresenham(g_gridVerts, g_gridCols, g_gridRows, 25, 12, g_gridCols - 10, 25, g_lineColor);

	// upload the painted grid to GPU
	CreateSquareVAO(g_VAO_sq,g_VBO_sq,g_EBO_sq, g_gridVerts.data(), g_gridVerts.size(), g_gridIndices.data(), g_gridIndices.size());

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers(window);

	//****************************render loop (CONTINUUM)*****************************//
	while(!glfwWindowShouldClose(window)) {
		
		glClear(GL_COLOR_BUFFER_BIT);
		shader.Activate(shader.ID);

		glBindVertexArray(g_VAO_sq);
		glDrawElements(GL_TRIANGLES, g_squareIndexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(0));

		// Draw the origin axes on top of the grid
		origin.draw();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	shader.Delete(shader.ID);
	
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}	