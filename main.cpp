#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstddef>
#include <cmath>

// LIBRERÍAS DE DEAR IMGUI (INSTALADAS CON VCPKG)
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ShaderClass.h"
#include "Origin.h"

/*************** ESTRUCTURA PARA LOS PÍXELES DEL LIENZO ***************/
struct VertexSquare {
    GLfloat pos[3];    // x, y, z
    GLfloat color[4];  // color RGBA 
};

/*************** HISTORIAL DE FIGURAS ***************/
enum TipoFigura { FIG_LINEA, FIG_CIRCULO, FIG_TRIANGULO };
struct Figura {
    TipoFigura tipo;
    int x0, y0;
    int x1, y1;
    int x2, y2;
    GLfloat color[4];
    bool rellenar;
};

// --- Variables Globales ---
static GLuint g_VAO_sq = 0, g_VBO_sq = 0, g_EBO_sq = 0;
static std::vector<VertexSquare> g_gridVerts;
static std::vector<GLuint> g_gridIndices;
static int g_gridCols = 80;
static int g_gridRows = 80;
static GLfloat g_baseColor[4] = { 0.15f, 0.15f, 0.15f, 1.0f }; // Fondo gris oscuro
static GLsizei g_squareIndexCount = 0;
static float g_startX = 0.0f, g_startY = 0.0f, g_cellW = 0.0f, g_cellH = 0.0f;

// --- Interfaz y Modos ---
enum ModoDibujo { LINEA, CIRCULO, TRIANGULO };
static ModoDibujo g_modoActual = LINEA;
static bool g_rellenarFiguras = false;

static std::vector<Figura> g_historialFiguras;
static std::vector<Figura> g_historialRehacer;

// Color inicial del pincel (Blanco por defecto)
static GLfloat g_colorActivo[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

// Estado de los clics en el lienzo
static int g_clickX0 = -1, g_clickY0 = -1;
static int g_clickX1 = -1, g_clickY1 = -1;
static int g_estadoClic = 0;

/*************** PINTAR UNA CELDA ESPECÍFICA ***************/
static void SetCellColor(std::vector<VertexSquare>& verts, int cols, int rows, int cx, int cy, const GLfloat color[4]) {
    if (cx < 0 || cx >= cols || cy < 0 || cy >= rows) return;
    size_t cellIndex = static_cast<size_t>(cy) * cols + static_cast<size_t>(cx);
    size_t base = cellIndex * 4;
    for (size_t i = 0; i < 4; ++i) {
        verts[base + i].color[0] = color[0];
        verts[base + i].color[1] = color[1];
        verts[base + i].color[2] = color[2];
        verts[base + i].color[3] = color[3];
    }
}

/*************** DIBUJAR RECTÁNGULOS (PARA LIMPIAR EL LIENZO) ***************/
static void DrawUIRect(std::vector<VertexSquare>& verts, int cols, int rows, int x0, int y0, int x1, int y1, const GLfloat color[4]) {
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            SetCellColor(verts, cols, rows, x, y, color);
        }
    }
}

/*************** ALGORITMO BRESENHAM: LÍNEAS ***************/
static void PaintLineBresenham(std::vector<VertexSquare>& verts, int cols, int rows, int x0, int y0, int x1, int y1, const GLfloat color[4]) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        SetCellColor(verts, cols, rows, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Simetría para círculos
static void DrawCircleSymmetry(std::vector<VertexSquare>& verts, int cols, int rows, int xc, int yc, int x, int y, const GLfloat color[4], bool fill) {
    if (fill) {
        PaintLineBresenham(verts, cols, rows, xc - x, yc + y, xc + x, yc + y, color);
        PaintLineBresenham(verts, cols, rows, xc - x, yc - y, xc + x, yc - y, color);
        PaintLineBresenham(verts, cols, rows, xc - y, yc + x, xc + y, yc + x, color);
        PaintLineBresenham(verts, cols, rows, xc - y, yc - x, xc + y, yc - x, color);
    }
    else {
        SetCellColor(verts, cols, rows, xc + x, yc + y, color);
        SetCellColor(verts, cols, rows, xc - x, yc + y, color);
        SetCellColor(verts, cols, rows, xc + x, yc - y, color);
        SetCellColor(verts, cols, rows, xc - x, yc - y, color);
        SetCellColor(verts, cols, rows, xc + y, yc + x, color);
        SetCellColor(verts, cols, rows, xc - y, yc + x, color);
        SetCellColor(verts, cols, rows, xc + y, yc - x, color);
        SetCellColor(verts, cols, rows, xc - y, yc - x, color);
    }
}

/*************** ALGORITMO BRESENHAM: CÍRCULOS ***************/
static void PaintCircleBresenham(std::vector<VertexSquare>& verts, int cols, int rows, int xc, int yc, int r, const GLfloat color[4], bool fill) {
    int x = 0, y = r, d = 3 - 2 * r;
    DrawCircleSymmetry(verts, cols, rows, xc, yc, x, y, color, fill);
    while (y >= x) {
        x++;
        if (d > 0) { y--; d = d + 4 * (x - y) + 10; }
        else { d = d + 4 * x + 6; }
        DrawCircleSymmetry(verts, cols, rows, xc, yc, x, y, color, fill);
    }
}

/*************** ALGORITMO: TRIÁNGULOS (SCANLINE) ***************/
static void PaintTriangleBresenham(std::vector<VertexSquare>& verts, int cols, int rows, int x0, int y0, int x1, int y1, int x2, int y2, const GLfloat color[4], bool fill) {
    if (!fill) {
        PaintLineBresenham(verts, cols, rows, x0, y0, x1, y1, color);
        PaintLineBresenham(verts, cols, rows, x1, y1, x2, y2, color);
        PaintLineBresenham(verts, cols, rows, x2, y2, x0, y0, color);
    }
    else {
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
        if (y0 > y2) { std::swap(y0, y2); std::swap(x0, x2); }
        if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }
        for (int y = y0; y <= y2; ++y) {
            if (y < 0 || y >= rows) continue;
            float t1 = (y2 - y0 == 0) ? 1.0f : (float)(y - y0) / (y2 - y0);
            int xa = x0 + static_cast<int>((x2 - x0) * t1);
            if (y < y1) {
                float t2 = (y1 - y0 == 0) ? 1.0f : (float)(y - y0) / (y1 - y0);
                int xb = x0 + static_cast<int>((x1 - x0) * t2);
                PaintLineBresenham(verts, cols, rows, xa, y, xb, y, color);
            }
            else {
                float t2 = (y2 - y1 == 0) ? 1.0f : (float)(y - y1) / (y2 - y1);
                int xb = x1 + static_cast<int>((x2 - x1) * t2);
                PaintLineBresenham(verts, cols, rows, xa, y, xb, y, color);
            }
        }
    }
}

/*************** RECONSTRUIR EL LIENZO DESDE EL HISTORIAL ***************/
static void RedrawCanvasFromHistory(std::vector<VertexSquare>& verts, int cols, int rows) {
    DrawUIRect(verts, cols, rows, 0, 0, cols - 1, rows - 1, g_baseColor);
    for (const auto& fig : g_historialFiguras) {
        if (fig.tipo == FIG_LINEA) PaintLineBresenham(verts, cols, rows, fig.x0, fig.y0, fig.x1, fig.y1, fig.color);
        else if (fig.tipo == FIG_CIRCULO) {
            int radius = static_cast<int>(std::sqrt(std::pow(fig.x1 - fig.x0, 2) + std::pow(fig.y1 - fig.y0, 2)));
            PaintCircleBresenham(verts, cols, rows, fig.x0, fig.y0, radius, fig.color, fig.rellenar);
        }
        else if (fig.tipo == FIG_TRIANGULO) PaintTriangleBresenham(verts, cols, rows, fig.x0, fig.y0, fig.x1, fig.y1, fig.x2, fig.y2, fig.color, fig.rellenar);
    }
}

static void CreateSquareVAO(GLuint& VAO, GLuint& VBO, GLuint& EBO, const VertexSquare* verts, size_t vertCount, const GLuint* indices, size_t indexCount) {
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertCount * sizeof(VertexSquare), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), indices, GL_STATIC_DRAW);
    GLsizei strideSquare = static_cast<GLsizei>(sizeof(VertexSquare));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideSquare, (GLvoid*)offsetof(VertexSquare, pos)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, strideSquare, (GLvoid*)offsetof(VertexSquare, color)); glEnableVertexAttribArray(1);
}

static void CreateGridSquares(std::vector<VertexSquare>& outVerts, std::vector<GLuint>& outIndices, int cols, int rows, float startX, float startY, float cellW, float cellH, const GLfloat color[4]) {
    outVerts.clear(); outIndices.clear();
    outVerts.reserve(static_cast<size_t>(cols) * rows * 4); outIndices.reserve(static_cast<size_t>(cols) * rows * 6);
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            float cx = startX + (x + 0.5f) * cellW, cy = startY + (y + 0.5f) * cellH;
            float halfW = cellW * 0.5f, halfH = cellH * 0.5f;
            VertexSquare v0 = { { cx + halfW, cy + halfH, 0.0f }, { color[0], color[1], color[2], color[3] } };
            VertexSquare v1 = { { cx + halfW, cy - halfH, 0.0f }, { color[0], color[1], color[2], color[3] } };
            VertexSquare v2 = { { cx - halfW, cy - halfH, 0.0f }, { color[0], color[1], color[2], color[3] } };
            VertexSquare v3 = { { cx - halfW, cy + halfH, 0.0f }, { color[0], color[1], color[2], color[3] } };
            GLuint base = static_cast<GLuint>(outVerts.size());
            outVerts.push_back(v0); outVerts.push_back(v1); outVerts.push_back(v2); outVerts.push_back(v3);
            outIndices.push_back(base + 0); outIndices.push_back(base + 1); outIndices.push_back(base + 2);
            outIndices.push_back(base + 0); outIndices.push_back(base + 2); outIndices.push_back(base + 3);
        }
    }
}

static void CreateGridSquaresForViewport(std::vector<VertexSquare>& outVerts, std::vector<GLuint>& outIndices, int cols, int rows, const GLfloat color[4], float& outStartX, float& outStartY, float& outCellW, float& outCellH) {
    outCellW = 2.0f / static_cast<float>(cols); outCellH = 2.0f / static_cast<float>(rows);
    outStartX = -1.0f; outStartY = -1.0f;
    CreateGridSquares(outVerts, outIndices, cols, rows, outStartX, outStartY, outCellW, outCellH, color);
}

/*************** PROCESAR CLICS EN EL LIENZO ***************/
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos; glfwGetCursorPos(window, &xpos, &ypos);
        int width, height; glfwGetWindowSize(window, &width, &height);

        int cellX = static_cast<int>((xpos / static_cast<double>(width)) * g_gridCols);
        int cellY = static_cast<int>(((static_cast<double>(height) - ypos) / static_cast<double>(height)) * g_gridRows);

        if (g_modoActual == LINEA || g_modoActual == CIRCULO) {
            if (g_estadoClic == 0) { g_clickX0 = cellX; g_clickY0 = cellY; g_estadoClic = 1; }
            else {
                g_historialRehacer.clear();
                if (g_modoActual == LINEA) {
                    PaintLineBresenham(g_gridVerts, g_gridCols, g_gridRows, g_clickX0, g_clickY0, cellX, cellY, g_colorActivo);
                    g_historialFiguras.push_back({ FIG_LINEA, g_clickX0, g_clickY0, cellX, cellY, 0, 0, {g_colorActivo[0], g_colorActivo[1], g_colorActivo[2], g_colorActivo[3]}, g_rellenarFiguras });
                }
                else {
                    int radius = static_cast<int>(std::sqrt(std::pow(cellX - g_clickX0, 2) + std::pow(cellY - g_clickY0, 2)));
                    PaintCircleBresenham(g_gridVerts, g_gridCols, g_gridRows, g_clickX0, g_clickY0, radius, g_colorActivo, g_rellenarFiguras);
                    g_historialFiguras.push_back({ FIG_CIRCULO, g_clickX0, g_clickY0, cellX, cellY, 0, 0, {g_colorActivo[0], g_colorActivo[1], g_colorActivo[2], g_colorActivo[3]}, g_rellenarFiguras });
                }
                g_estadoClic = 0;
            }
        }
        else if (g_modoActual == TRIANGULO) {
            if (g_estadoClic == 0) { g_clickX0 = cellX; g_clickY0 = cellY; g_estadoClic = 1; }
            else if (g_estadoClic == 1) { g_clickX1 = cellX; g_clickY1 = cellY; g_estadoClic = 2; }
            else if (g_estadoClic == 2) {
                g_historialRehacer.clear();
                PaintTriangleBresenham(g_gridVerts, g_gridCols, g_gridRows, g_clickX0, g_clickY0, g_clickX1, g_clickY1, cellX, cellY, g_colorActivo, g_rellenarFiguras);
                g_historialFiguras.push_back({ FIG_TRIANGULO, g_clickX0, g_clickY0, g_clickX1, g_clickY1, cellX, cellY, {g_colorActivo[0], g_colorActivo[1], g_colorActivo[2], g_colorActivo[3]}, g_rellenarFiguras });
                g_estadoClic = 0;
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, g_VBO_sq);
        glBufferSubData(GL_ARRAY_BUFFER, 0, g_gridVerts.size() * sizeof(VertexSquare), g_gridVerts.data());
    }
}

static void RecreateGridForViewport(GLFWwindow*) {
    CreateGridSquaresForViewport(g_gridVerts, g_gridIndices, g_gridCols, g_gridRows, g_baseColor, g_startX, g_startY, g_cellW, g_cellH);
    if (g_VAO_sq) { glDeleteVertexArrays(1, &g_VAO_sq); glDeleteBuffers(1, &g_VBO_sq); glDeleteBuffers(1, &g_EBO_sq); }
    CreateSquareVAO(g_VAO_sq, g_VBO_sq, g_EBO_sq, g_gridVerts.data(), g_gridVerts.size(), g_gridIndices.data(), g_gridIndices.size());
    g_squareIndexCount = static_cast<GLsizei>(g_gridIndices.size());
    RedrawCanvasFromHistory(g_gridVerts, g_gridCols, g_gridRows);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height); RecreateGridForViewport(window);
}

/*************** FUNCIÓN PRINCIPAL MAIN ***************/
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 800, "Paint Studio Interactivo - Bresenham Engine + ImGui", nullptr, nullptr);
    if (window == nullptr) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window); gladLoadGL();

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    RecreateGridForViewport(window);
    ShaderClass shader("vertex.vs", "fragment.fs");

    // =========================================================
    // INICIALIZACIÓN DE DEAR IMGUI
    // =========================================================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // ---> ¡ESTA ES LA LÍNEA NUEVA QUE AGRANDA LAS LETRAS Y BOTONES!
    io.FontGlobalScale = 1.5f; // Escala global al 150% (Súper legible)

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    // =========================================================

    /*************** BUCLE DE RENDERIZADO PRINCIPAL ***************/
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 2. DISEÑO DEL PANEL DE CONTROL DE IMGUI
        // Ajustamos las medidas: 550 píxeles de ancho (bien espacioso) por solo 350 de alto (compacto)
        ImGui::SetNextWindowSize(ImVec2(550.0f, 350.0f), ImGuiCond_Always);

        ImGui::Begin("Panel de Control - Paint Studio");

        ImGui::Text("Herramientas de Dibujo:");
        if (ImGui::Button("Línea")) g_modoActual = LINEA;
        ImGui::SameLine();
        if (ImGui::Button("Círculo")) g_modoActual = CIRCULO;
        ImGui::SameLine();
        if (ImGui::Button("Triángulo")) g_modoActual = TRIANGULO;

        ImGui::Separator();

        if (g_modoActual == LINEA) ImGui::Text("Modo Activo: LINEA");
        else if (g_modoActual == CIRCULO) ImGui::Text("Modo Activo: CIRCULO");
        else if (g_modoActual == TRIANGULO) ImGui::Text("Modo Activo: TRIANGULO");

        ImGui::Separator();

        ImGui::Checkbox("Rellenar Figuras", &g_rellenarFiguras);

        ImGui::Separator();

        ImGui::ColorEdit4("Color", g_colorActivo);

        ImGui::Separator();

        if (ImGui::Button("Deshacer")) {
            if (!g_historialFiguras.empty()) {
                g_historialRehacer.push_back(g_historialFiguras.back());
                g_historialFiguras.pop_back();
                RedrawCanvasFromHistory(g_gridVerts, g_gridCols, g_gridRows);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Rehacer")) {
            if (!g_historialRehacer.empty()) {
                g_historialFiguras.push_back(g_historialRehacer.back());
                g_historialRehacer.pop_back();
                RedrawCanvasFromHistory(g_gridVerts, g_gridCols, g_gridRows);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Limpiar Lienzo")) {
            g_historialFiguras.clear();
            g_historialRehacer.clear();
            DrawUIRect(g_gridVerts, g_gridCols, g_gridRows, 0, 0, g_gridCols - 1, g_gridRows - 1, g_baseColor);
            g_estadoClic = 0;
        }

        ImGui::End();

        // 3. Renderizado de tu Lienzo en OpenGL
        glClear(GL_COLOR_BUFFER_BIT);
        shader.Activate(shader.ID);
        glBindVertexArray(g_VAO_sq);

        glBindBuffer(GL_ARRAY_BUFFER, g_VBO_sq);
        glBufferSubData(GL_ARRAY_BUFFER, 0, g_gridVerts.size() * sizeof(VertexSquare), g_gridVerts.data());

        glDrawElements(GL_TRIANGLES, g_squareIndexCount, GL_UNSIGNED_INT, reinterpret_cast<void*>(0));

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    shader.Delete(shader.ID); glfwDestroyWindow(window); glfwTerminate();
    return 0;
}
