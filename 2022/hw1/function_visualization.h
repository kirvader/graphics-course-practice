#pragma once

#include <vector>
#include <GL/glew.h>
#include "FloatFunctionXYT.h"
#include "matrix.h"
#include <map>

struct color_data {
    GLubyte color[4];
};

struct vec2 {
    float x, y;
};

class GridController {
public:
    GridController(GLuint w, GLuint h, float x0, float x1, float y0, float y1, const FloatFunctionXYT &func);

    void update_grid(GLuint W, GLuint H, float x0, float x1, float y0, float y1);
    void update_grid(GLuint W, GLuint H);

    std::vector<vec2> get_grid_points() const;

    std::vector<color_data> get_colors() const;
    std::vector<std::vector<vec2>> get_isolines(const std::vector<float> &constants) const;

    GLuint get_grid_size() const {
        return (W + 1) * (H + 1);
    }

    matrix get_transform_matrix() const;

    std::vector<GLuint> get_indexes_order_to_render() const;

    void update_time(float time);


private:
    uint get_point_index_in_grid(uint i, uint j) const {
        return i * (W + 1) + j;
    }

    uint get_edge_index(uint i, uint j, bool is_horizontal) const {
        if (is_horizontal) {
            return 2 * (W + 1) * i + j;
        } else {
            return 2 * (W + 1) * i + (W + 1) + j;
        }
    }

    // returns i, j, is_horizontal
    std::tuple<uint, uint, bool> get_edge_by_index(uint index) const {
        uint column = index % (2 * (W + 1));
        if (column > W) {
            return {index / (2 * (W + 1)), column % (W + 1), 0};
        } else {
            return {index / (2 * (W + 1)), column, 1};
        }
    }

    vec2 interpolate_through_edge(uint i0, uint j0, uint i1, uint j1, float c) const;

    vec2 get_coords_by_grid(uint i, uint j) const {
        float hor_part = (float) j / (float) W;
        float ver_part = (float) i / (float) H;

        return {x0 * (1 - hor_part) + x1 * hor_part, y0 * (1 - ver_part) + y1 * ver_part};
    }

    void update_transform_matrix(float x0, float x1, float y0, float y1);
    void update_indexes(GLuint new_W, GLuint new_H);

    GLuint W, H;
    float x0, x1, y0, y1;

    matrix transform;
    std::vector<GLuint> indexes;

    FloatFunctionXYT func;

    std::vector<vec2> grid;
    std::vector<float> function_values;

    std::map<float, color_data> colors;
};

