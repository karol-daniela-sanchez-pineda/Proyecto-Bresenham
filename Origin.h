#pragma once
#include <vector>
#include <cstddef>
#include <glad/glad.h>

struct OriginVertex {
    GLfloat pos[3];
    GLfloat color[4];
};

class Origin
{
public:
    Origin() noexcept;
    Origin(const OriginVertex* data, std::size_t count);
    ~Origin();

    // non-copyable
    Origin(const Origin&) = delete;
    Origin& operator=(const Origin&) = delete;

    // movable
    Origin(Origin&& other) noexcept;
    Origin& operator=(Origin&& other) noexcept;

    // initialize or replace data
    void init(const OriginVertex* data, std::size_t count);
    void update(const OriginVertex* data, std::size_t count); // updates buffer contents
    void draw() const;

    std::size_t vertexCount() const noexcept { return m_count; }

private:
    void destroy() noexcept;

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    std::size_t m_count = 0;
};

