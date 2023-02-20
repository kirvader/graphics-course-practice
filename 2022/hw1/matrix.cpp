//
// Created by kir on 13.02.23.
//

#include "matrix.h"

matrix get_view_for_camera_params(GLfloat near, GLfloat far, GLfloat right, GLfloat top, GLfloat left, GLfloat bottom) {
    return matrix(4, 4, {
            2 * near / (right - left), 0.f, (right + left) / (right - left), 0.f,
            0.f, 2 * near / (top - bottom), (top + bottom) / (top - bottom), 0.f,
            0.f, 0.f, -(far + near) / (far - near), -2 * far * near / (far - near),
            0.f, 0.f, -1.f, 0.f
    });
}

matrix get_view_for_symmetric_camera_params(GLfloat near, GLfloat far, GLfloat right, GLfloat top) {
    return get_view_for_camera_params(near, far, right, top, -right, -top);
}

matrix get_transform_shift(GLfloat dx, GLfloat dy, GLfloat dz) {
    return matrix(4, 4, {
            1.f, 0.f, 0.f, dx,
            0.f, 1.f, 0.f, dy,
            0.f, 0.f, 1.f, dz,
            0.f, 0.f, 0.f, 1.f
    });
}

matrix get_transform_scaling(GLfloat scale_x, GLfloat scale_y, GLfloat scale_z) {
    return matrix(4, 4, {
            scale_x, 0.f, 0.f, 0.f,
            0.f, scale_y, 0.f, 0.f,
            0.f, 0.f, scale_z, 0.f,
            0.f, 0.f, 0.f, 1.f
    });
}

matrix get_transform_rotation_by_z(GLfloat angle) {
    return matrix(4, 4, {
            (float)cos(angle), (float)-sin(angle), 0.f, 0.f,
            (float)sin(angle), (float)cos(angle), 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f
    });
}

matrix get_transform_rotation_by_y(GLfloat angle) {
    return matrix(4, 4, {
            (float)cos(angle), 0.f, (float)sin(angle), 0.f,
            0.f, 1.f, 0.f, 0.f,
            (float)-sin(angle), 0.f, (float)cos(angle), 0.f,
            0.f, 0.f, 0.f, 1.f
    });
}

matrix get_transform_rotation_by_x(GLfloat angle) {
    return matrix(4, 4, {
            1.f, 0.f, 0.f, 0.f,
            0.f, (float)cos(angle), (float)-sin(angle), 0.f,
            0.f, (float)sin(angle), (float)cos(angle), 0.f,
            0.f, 0.f, 0.f, 1.f
    });
}

matrix::matrix(int n, int m, std::vector<GLfloat> &&data) : n(n), m(m) {
    if (data.empty()) {
        if (n * m == 0) {
            n = 0;
            m = 0;
            this->bytes.clear();
            return;
        }
        bytes.resize(n * m, 0.f);
        return;
    }
    assert(data.size() == n * m);
    this->bytes = std::move(data);
}

matrix matrix::operator*(const matrix &other) const {
    assert(this->m == other.n);
    matrix result(this->n, other.m);
    for (int i = 0; i < this->n; i++) {
        for (int j = 0; j < other.m; j++) {
            GLfloat temp = 0.f;
            for (int k = 0; k < this->m; k++) {
                temp += this->get(i, k) * other.get(k, j);
            }
            result.set(i, j, temp);
        }
    }
    return result;
}
