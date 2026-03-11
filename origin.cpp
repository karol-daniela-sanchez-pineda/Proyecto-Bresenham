#include "Origin.h"
#include <utility> // for std::move
#include <cstring> // for std::memcpy

// default unit vectors centered at origin (X,Y,Z)
static const LineVertex kDefaultOrigin[] = {
    // X axis (red)
    { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
    { { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },

    // Y axis (green)
    { { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
    { { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },

    // Z axis (blue)
    { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
    { { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
};

Origin::Origin() noexcept {
    init(kDefaultOrigin, sizeof(kDefaultOrigin) / sizeof(kDefaultOrigin[0]));
}

Origin::Origin(const LineVertex* data, std::size_t count) {
    init(data, count);
}

Origin::~Origin() {
    destroy();
}

Origin::Origin(Origin&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_count(other.m_count) {
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_count = 0;
}

Origin& Origin::operator=(Origin&& other) noexcept {
    if (this != &other) {
        destroy();
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_count = other.m_count;
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_count = 0;
    }
    return *this;
}

void Origin::init(const LineVertex* data, std::size_t count) {
    destroy();
    if (data == nullptr || count == 0) return;

    m_count = count;
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(count * sizeof(LineVertex)), data, GL_STATIC_DRAW);

    // attribute 0 = vec3 position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(LineVertex)), reinterpret_cast<void*>(offsetof(LineVertex, pos)));
    glEnableVertexAttribArray(0);

    // attribute 1 = vec4 color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(LineVertex)), reinterpret_cast<void*>(offsetof(LineVertex, color)));
    glEnableVertexAttribArray(1);

    // ensure shader's angle attribute (location 2) reads zero for lines (harmless if shader lacks location 2)
    glDisableVertexAttribArray(2);
    glVertexAttrib1f(2, 0.0f);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Origin::update(const LineVertex* data, std::size_t count) {
    if (m_vbo == 0) {
        init(data, count);
        return;
    }
    if (data == nullptr || count == 0) {
        return;
    }

    m_count = count;
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // overwrite entire buffer (assumes size <= allocated). For safety we reallocate.
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(count * sizeof(LineVertex)), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Origin::draw() const {
    if (m_vao == 0 || m_count == 0) return;
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_count));
    glBindVertexArray(0);
}

void Origin::destroy() noexcept {
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_count = 0;
}
