#ifdef WIN32
#include <SDL.h>
#undef main
#else

#include <SDL2/SDL.h>

#endif

#include <GL/glew.h>

#include <string_view>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <vector>
#include <map>
#include <cassert>

#include "obj_parser.hpp"

std::string to_string(std::string_view str) {
    return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message) {
    throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error) {
    throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

const char vertex_shader_source[] =
        R"(#version 330 core

uniform mat4 view;
uniform mat4 transform;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;

out vec3 normal;

void main()
{
    gl_Position = view * transform * vec4(in_position, 1.0);
    normal = mat3(transform) * in_normal;
}
)";

const char fragment_shader_source[] =
        R"(#version 330 core

in vec3 normal;

layout (location = 0) out vec4 out_color;

void main()
{
    float lightness = 0.5 + 0.5 * dot(normalize(normal), normalize(vec3(1.0, 2.0, 3.0)));
    out_color = vec4(vec3(lightness), 1.0);
}
)";

GLuint create_shader(GLenum type, const char *source) {
    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, &source, nullptr);
    glCompileShader(result);
    GLint status;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint info_log_length;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Shader compilation failed: " + info_log);
    }
    return result;
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint result = glCreateProgram();
    glAttachShader(result, vertex_shader);
    glAttachShader(result, fragment_shader);
    glLinkProgram(result);

    GLint status;
    glGetProgramiv(result, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLint info_log_length;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Program linkage failed: " + info_log);
    }

    return result;
}

class matrix {
public:
    matrix(int n, int m, std::vector<GLfloat> &&data = {}) : n(n), m(m) {
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

    matrix operator*(const matrix &other) const {
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

private:
    int n = 0, m = 0;
    std::vector<GLfloat> bytes;
};

matrix get_transform_rotation_by_x(GLfloat angle) {
    return matrix(4, 4, {
            1.f, 0.f, 0.f, 0.f,
            0.f, cos(angle), -sin(angle), 0.f,
            0.f, sin(angle), cos(angle), 0.f,
            0.f, 0.f, 0.f, 1.f
    });
}

matrix get_transform_rotation_by_y(GLfloat angle) {
    return matrix(4, 4, {
            cos(angle), 0.f, sin(angle), 0.f,
            0.f, 1.f, 0.f, 0.f,
            -sin(angle), 0.f, cos(angle), 0.f,
            0.f, 0.f, 0.f, 1.f
    });
}

matrix get_transform_rotation_by_z(GLfloat angle) {
    return matrix(4, 4, {
            cos(angle), -sin(angle), 0.f, 0.f,
            sin(angle), cos(angle), 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
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

std::pair<std::pair<GLfloat, GLfloat>, std::pair<GLfloat, GLfloat>>
get_transform_by_input(std::map<SDL_Keycode, bool> &input) {
    GLfloat angle_dy = 0.f, angle_dz = 0.f;
    if (input[SDLK_LEFT]) {
        angle_dy -= 1.f;
    }
    if (input[SDLK_RIGHT]) {
        angle_dy += 1.f;
    }
    if (input[SDLK_DOWN]) {
        angle_dz -= 1.f;
    }
    if (input[SDLK_UP]) {
        angle_dz += 1.f;
    }
    GLfloat shift_x = 0.f, shift_y = 0.f;
    if (input[SDLK_s]) {
        shift_y -= 1.f;
    }
    if (input[SDLK_w]) {
        shift_y += 1.f;
    }
    if (input[SDLK_a]) {
        shift_x -= 1.f;
    }
    if (input[SDLK_d]) {
        shift_x += 1.f;
    }
    input.clear();

    return {{shift_x,  shift_y},
            {angle_dy, angle_dz}};
}

int main() try {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");

    GLfloat rotation_speed = 2.f;
    GLfloat shift_speed = 2.f;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window *window = SDL_CreateWindow("Graphics course practice 4",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          800, 600,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
        sdl2_fail("SDL_CreateWindow: ");

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    auto program = create_program(vertex_shader, fragment_shader);

    GLuint view_location = glGetUniformLocation(program, "view");
    GLuint transform_location = glGetUniformLocation(program, "transform");

    std::string project_root = PROJECT_ROOT;
    obj_data bunny = parse_obj(project_root + "/bunny.obj");

    GLuint vao, vbo, ebo;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, bunny.indices.size() * sizeof(std::uint32_t), bunny.indices.data(),
                 GL_STATIC_DRAW);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void *) 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex), (void *) (3 * sizeof(GLfloat)));

    glBufferData(GL_ARRAY_BUFFER, bunny.vertices.size() * sizeof(obj_data::vertex), bunny.vertices.data(),
                 GL_STATIC_DRAW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);


    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;
    GLfloat scale_factor = 0.5f;
    GLfloat angle_dx = 0.f, angle_dy = 0.f;
    GLfloat all_shift_dx = 0.f, all_shift_dy = 0.f;

    std::map<SDL_Keycode, bool> button_down;
    std::vector<std::pair<GLfloat, GLfloat>> delta_positions;
    delta_positions.emplace_back(0.f, 1.2f);
    delta_positions.emplace_back(1.f, -0.5f);
    delta_positions.emplace_back(-1.f, -0.5f);

    bool running = true;
    while (running) {
        for (SDL_Event event; SDL_PollEvent(&event);)
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_RESIZED:
                            width = event.window.data1;
                            height = event.window.data2;
                            glViewport(0, 0, width, height);
                            break;
                    }
                    break;
                case SDL_KEYDOWN:
                    button_down[event.key.keysym.sym] = true;
                    break;
                case SDL_KEYUP:
                    button_down[event.key.keysym.sym] = false;
                    break;
            }

        if (!running)
            break;


        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        last_frame_start = now;
        time += dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        matrix view_matrix = get_view_for_symmetric_camera_params(0.01f, 100.f, 0.015f, 0.012f);

        GLfloat shift_x, shift_y, delta_angle_y, delta_angle_z;
        auto inputs = get_transform_by_input(button_down);
        std::tie(shift_x, shift_y) = inputs.first;
        std::tie(delta_angle_y, delta_angle_z) = inputs.second;
        angle_dx += delta_angle_y * rotation_speed * dt;
        angle_dy += delta_angle_z * rotation_speed * dt;
        all_shift_dx += shift_x * shift_speed * dt;
        all_shift_dy += shift_y * shift_speed * dt;

        matrix transform_matrix =
                get_transform_shift(all_shift_dx, all_shift_dy, 0.f) * get_transform_shift(0.f, 0.f, -1.5f) *
                get_transform_rotation_by_y(angle_dx) * get_transform_rotation_by_z(angle_dy) *
                get_transform_scaling(scale_factor, scale_factor, scale_factor);

        glUseProgram(program);
        for (auto &[dx, dy]: delta_positions) {
            glUniformMatrix4fv(view_location, 1, GL_TRUE, view_matrix.data());
            glUniformMatrix4fv(transform_location, 1, GL_TRUE, (get_transform_shift(dx, dy, 0.f) * transform_matrix).data());

            glDrawElements(GL_TRIANGLES, bunny.indices.size(), GL_UNSIGNED_INT, nullptr);

        }

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
