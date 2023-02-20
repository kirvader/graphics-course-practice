#pragma once

#include <GL/glew.h>
#include "function_visualization.h"

class GridBufferHolder  {
public:
    GridBufferHolder();


    void update_vertices_colors_indexes(const std::vector<vec2> &vertices,
                                        const std::vector<color_data> &colors,
                                        const std::vector<GLuint> &indices);

    void update_colors(const std::vector<color_data> &colors);

    void draw() const;

private:
    GLuint vao{}, ebo{};
    GLuint vbo_vertices{}, vbo_colors{};

    std::uint32_t vertices_size = 0, indices_size = 0;
};

class IsolinesBuffersStruct {
public:
    IsolinesBuffersStruct();

    void draw(const std::vector<std::vector<vec2>> &vertices);

private:
    GLuint vao{}, vbo{};

};
