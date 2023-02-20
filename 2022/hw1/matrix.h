#pragma once


#include <GL/glew.h>
#include <algorithm>
#include <cassert>
#include <cmath>

class matrix {
public:
    matrix(int n, int m, std::vector<GLfloat> &&data = {});

    matrix(const matrix &other) = default;

    matrix(matrix &&other) = default;

    matrix &operator=(const matrix &other) = default;

    matrix &operator=(matrix &&other) = default;

    GLfloat get(int i, int j) const {
        assert(i < n && j < m);
        return bytes[i * 4 + j];
    }

    void set(int i, int j, GLfloat item) {
        assert(i < n && j < m);
        bytes[i * 4 + j] = item;
    }

    GLfloat *data() {
        return bytes.data();
    }

    matrix operator*(const matrix &other) const;

private:
    int n = 0, m = 0;
    std::vector<GLfloat> bytes;
};

matrix get_transform_rotation_by_x(GLfloat angle);

matrix get_transform_rotation_by_y(GLfloat angle);

matrix get_transform_rotation_by_z(GLfloat angle);

matrix get_transform_scaling(GLfloat scale_x, GLfloat scale_y, GLfloat scale_z);

matrix get_transform_shift(GLfloat dx, GLfloat dy, GLfloat dz);

matrix get_view_for_camera_params(GLfloat near, GLfloat far, GLfloat right, GLfloat top, GLfloat left, GLfloat bottom);

matrix get_view_for_symmetric_camera_params(GLfloat near, GLfloat far, GLfloat right, GLfloat top);
