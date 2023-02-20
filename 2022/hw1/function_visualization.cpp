
#include <map>
#include <unordered_set>
#include <iostream>
#include "function_visualization.h"

color_data lerp(const color_data &fst, const color_data &snd, float value) {
    return {(GLubyte) ((float) fst.color[0] * (1 - value) + (float) snd.color[0] * value),
            (GLubyte) ((float) fst.color[1] * (1 - value) + (float) snd.color[1] * value),
            (GLubyte) ((float) fst.color[2] * (1 - value) + (float) snd.color[2] * value),
            (GLubyte) ((float) fst.color[3] * (1 - value) + (float) snd.color[3] * value)};
}

void set_color_by_value(float value, color_data &color, const std::map<float, color_data> &colors_map) {
    auto upper_bound_it = colors_map.upper_bound(value);
    if (upper_bound_it == colors_map.end()) {
        --upper_bound_it;
        color = upper_bound_it->second;
        return;
    }
    if (upper_bound_it == colors_map.begin()) {
        color = upper_bound_it->second;
        return;
    }
    auto lower_bound_it = upper_bound_it;
    --lower_bound_it;
    float fst_value = lower_bound_it->first;
    float snd_value = upper_bound_it->first;
    float lerp_value = (value - fst_value) / (snd_value - fst_value);
    color = lerp(lower_bound_it->second, upper_bound_it->second, lerp_value);
}

GridController::GridController(GLuint w, GLuint h, float x0, float x1, float y0, float y1, const FloatFunctionXYT &func)
        : W(w), H(h),
          transform(matrix(4, 4)),
          x0(x0), x1(x1), y0(y0), y1(y1),
          func(func) {
    update_grid(W, H, x0, x1, y0, y1);
    update_transform_matrix(x0, x1, y0, y1);

    colors[-3] = {11, 19, 43, 255};
    colors[-1] = {28, 37, 65, 255};
    colors[0] = {58, 80, 107, 255};
    colors[1] = {91, 192, 190, 255};
    colors[3] = {255, 255, 255, 255};
}

void GridController::update_grid(GLuint new_W, GLuint new_H, float x0, float x1, float y0, float y1) {
    update_transform_matrix(x0, x1, y0, y1);
    W = new_W;
    H = new_H;
    grid.clear();
    grid.reserve(get_grid_size());
    for (uint i = 0; i <= H; i++) {
        float vert_part = (float) i / (float) H;
        float y = y0 * (1 - vert_part) + y1 * vert_part;

        for (uint j = 0; j <= W; j++) {
            float hor_part = (float) j / (float) W;
            float x = x0 * (1 - hor_part) + x1 * hor_part;
            grid.push_back({x, y});
        }
    }
    update_indexes(W, H);
}

std::vector<vec2> GridController::get_grid_points() const {
    return grid;
}

std::vector<color_data> GridController::get_colors() const {
    std::vector<color_data> result;
    result.reserve(get_grid_size());

    for (uint i = 0; i <= H; i++) {
        for (int j = 0; j <= W; j++) {
            float current_point_value = function_values[get_point_index_in_grid(i, j)];
            result.emplace_back();

            set_color_by_value(current_point_value, result[get_point_index_in_grid(i, j)], colors);
        }
    }

    return result;
}

void GridController::update_grid(GLuint W, GLuint H) {
    update_grid(W, H, x0, x1, y0, y1);
}

matrix GridController::get_transform_matrix() const {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << transform.get(i, j) << " ";
        }
        std::cout << "\n";

    }
    return transform;
}

void GridController::update_transform_matrix(float new_x0, float new_x1, float new_y0, float new_y1) {
    x0 = new_x0;
    x1 = new_x1;
    y0 = new_y0;
    y1 = new_y1;
    transform = get_transform_scaling(2 / (x1 - x0), 2 / (y1 - y0), 1.f) *
                get_transform_shift(-(x0 + x1) / 2, -(y0 + y1) / 2, 0.f);
}

std::vector<GLuint> GridController::get_indexes_order_to_render() const {
    return indexes;
}

void GridController::update_indexes(GLuint new_W, GLuint new_H) {
    W = new_W;
    H = new_H;
    indexes.clear();
    indexes.reserve(2 * W * H);

    for (uint i = 0; i < H; i++) {
        for (uint j = 0; j < W; j++) {
            uint top_left = get_point_index_in_grid(i, j);
            uint top_right = get_point_index_in_grid(i, j + 1);
            uint bottom_left = get_point_index_in_grid(i + 1, j);
            uint bottom_right = get_point_index_in_grid(i + 1, j + 1);

            indexes.push_back(top_left);
            indexes.push_back(bottom_right);
            indexes.push_back(bottom_left);

            indexes.push_back(top_left);
            indexes.push_back(top_right);
            indexes.push_back(bottom_right);
        }
    }
}

void GridController::update_time(float time) {
    function_values.clear();
    function_values.reserve(get_grid_size());

    for (uint i = 0; i <= H; i++) {
        for (int j = 0; j <= W; j++) {
            vec2 point = grid[get_point_index_in_grid(i, j)];

            function_values.push_back(func.get(point.x, point.y, time));
        }
    }
}


std::vector<std::pair<std::tuple<uint, uint, bool>, std::tuple<uint, uint, bool>>>
add_edges(bool top_left, bool top_right, bool bottom_left, bool bottom_right) {
    int count = top_left + top_right + bottom_right + bottom_left;
    if (count > 2) {
        top_left = !top_left;
        top_right = !top_right;
        bottom_left = !bottom_left;
        bottom_right = !bottom_right;
        count = 4 - count;
    }
    if (count == 0)
        return {};
    if (count == 2 && ((top_left && top_right) || (bottom_left && bottom_right))) {
        return {{{0, 0, false}, {0, 1, false}}};
    }
    if (count == 2 && ((top_left && bottom_left) || (top_right && bottom_right))) {
        return {{{0, 0, true}, {1, 0, true}}};
    }
    std::vector<std::pair<std::tuple<uint, uint, bool>, std::tuple<uint, uint, bool>>> result;
    if (top_left) {
        result.push_back({{0, 0, true}, {0, 0, false}});
    }
    if (top_right) {
        result.push_back({{0, 0, true}, {0, 1, false}});
    }
    if (bottom_left) {
        result.push_back({{0, 0, false}, {1, 0, true}});
    }
    if (bottom_right) {
        result.push_back({{1, 0, true}, {0, 1, false}});
    }

    return result;
}

uint find_least_edges_vertex(std::unordered_map<uint, std::vector<uint>> &a) {
    uint result = a.begin()->first;
    for (const auto &[index, edges]: a) {
        if (a[result].size() > edges.size()) {
            result = index;
        }
    }
    return result;
}

std::vector<std::vector<uint>> get_all_paths_from_edges(const std::vector<std::pair<uint, uint>> &edges) {
    std::unordered_map<uint, std::vector<uint>> a;

    for (const auto &[v, u] : edges) {
        if (a.find(v) == a.end()) {
            a[v] = std::vector<uint>();
        }
        if (a.find(u) == a.end()) {
            a[u] = std::vector<uint>();
        }
        a[v].push_back(u);
        a[u].push_back(v);
        if (a[v].size() > 2 || a[u].size() > 2) {
            throw "lol";
        }
    }

    std::vector<std::vector<uint>> result;
    while (!a.empty()) {
        // find vertex with 1 edge
        uint current_vertex = find_least_edges_vertex(a);
        std::vector<uint> current_path;
        current_path.push_back(current_vertex);

        while (true) {
            auto &current_vertex_edges = a[current_vertex];

            if (current_vertex_edges.empty()) {
                a.erase(current_vertex);
                break;
            }
            uint next_vertex = current_vertex_edges.back();
            current_vertex_edges.pop_back();
            if (current_vertex_edges.empty()) {
                a.erase(current_vertex);
            }
            current_path.push_back(next_vertex);

            auto pos = std::find(a[next_vertex].begin(), a[next_vertex].end(), current_vertex);
            a[next_vertex].erase(pos);

            current_vertex = next_vertex;
        }
        result.push_back(current_path);
    }
    return result;
}

std::vector<std::vector<vec2>> GridController::get_isolines(const std::vector<float> &constants) const {
    std::vector<std::vector<vec2>> isolines;
    isolines.reserve(constants.size());
    for (const auto &c: constants) {
        std::vector<std::pair<uint, uint>> edges;

        // getting all edges
        for (uint i = 0; i < H; i++) {
            for (uint j = 0; j < W; j++) {
                auto additional_edges = add_edges(function_values[get_point_index_in_grid(i, j)] >= c,
                                                  function_values[get_point_index_in_grid(i, j + 1)] >= c,
                                                  function_values[get_point_index_in_grid(i + 1, j)] >= c,
                                                  function_values[get_point_index_in_grid(i + 1, j + 1)] >= c);
                for (const auto&[seg1, seg2] : additional_edges) {
                    edges.emplace_back(get_edge_index(i + std::get<0>(seg1), j + std::get<1>(seg1), std::get<2>(seg1)),
                                     get_edge_index(i + std::get<0>(seg2), j + std::get<1>(seg2), std::get<2>(seg2)));
                }
            }
        }

        // getting all isoline paths
        auto current_paths = get_all_paths_from_edges(edges);
        for (const auto &path : current_paths) {
            isolines.emplace_back();
            isolines.back().reserve(path.size());
            for (const auto &index: path) {
                std::tuple<uint, uint, bool> edge = get_edge_by_index(index);
                uint i0 = std::get<0>(edge);
                uint j0 = std::get<1>(edge);
                bool is_horizontal = std::get<2>(edge);
                uint i1 = is_horizontal ? i0 : i0 + 1;
                uint j1 = is_horizontal ? j0 + 1 : j0;

                isolines.back().push_back(interpolate_through_edge(i0, j0, i1, j1, c));
            }
        }
    }
    return isolines;
}

vec2 GridController::interpolate_through_edge(uint i0, uint j0, uint i1, uint j1, float c) const {
    float fst_function_value = function_values[get_point_index_in_grid(i0, j0)];
    float snd_function_value = function_values[get_point_index_in_grid(i1, j1)];
    vec2 fst_point = get_coords_by_grid(i0, j0);
    vec2 snd_point = get_coords_by_grid(i1, j1);

    if (fst_function_value == snd_function_value) {
        return {(fst_point.x + snd_point.x) / 2, (fst_point.y + snd_point.y) / 2};
    }

    float part = (c - fst_function_value) / (snd_function_value - fst_function_value);

    return {fst_point.x * (1 - part) + snd_point.x * part, fst_point.y * (1 - part) + snd_point.y * part};
}

