//
// Created by kir on 19.02.23.
//

#include <iostream>
#include "BufferHolder.h"

GridBufferHolder::GridBufferHolder() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);



    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, bunny.indices.size() * sizeof(std::uint32_t), bunny.indices.data(),
//                 GL_STATIC_DRAW);
//    GLuint vbos[2];
//    glGenBuffers(2, vbos);
//    vbo_vertices = vbos[0];
//    vbo_colors = vbos[1];
    glGenBuffers(1, &vbo_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);

    glGenBuffers(1, &vbo_colors);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void *) 0);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
}

void GridBufferHolder::update_vertices_colors_indexes(const std::vector<vec2> &vertices,
                                                      const std::vector<color_data> &colors,
                                                      const std::vector<GLuint> &indices) {
    if (vertices.size() != colors.size()) {
        throw std::runtime_error("GridBufferHolder: vertices.size() != colors.size() in updating all");
    }

    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(),
                 GL_DYNAMIC_DRAW);
    indices_size = indices.size();

    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec2), vertices.data(),
                 GL_STATIC_DRAW);
    vertices_size = vertices.size();

    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(color_data), colors.data(),
                 GL_DYNAMIC_DRAW);

//    glDrawElements(GL_LINE, indices_size, GL_UNSIGNED_INT, NULL);
}

void GridBufferHolder::update_colors(const std::vector<color_data> &colors) {
    if (colors.size() != vertices_size) {
        throw std::runtime_error("GridBufferHolder: vertices.size() != colors.size() in updating colors");
    }
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(color_data), colors.data(),
                 GL_DYNAMIC_DRAW);
}

void GridBufferHolder::draw() const {
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glDrawElements(GL_TRIANGLES, indices_size, GL_UNSIGNED_INT, 0);
}



IsolinesBuffersStruct::IsolinesBuffersStruct() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);

}

void IsolinesBuffersStruct::draw(const std::vector<std::vector<vec2>> &vertices) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    for (const auto &path: vertices) {
        glBufferData(GL_ARRAY_BUFFER, path.size() * sizeof(vec2), path.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINE_STRIP, 0, path.size());
    }
}
