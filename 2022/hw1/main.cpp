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
#include "matrix.h"
#include "BufferHolder.h"

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

uniform mat4 transform;

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec4 in_color;

out vec4 color;

void main()
{
    gl_Position = transform *vec4(in_position, 0.0, 1.0);
    color = in_color;
}
)";

const char fragment_shader_source[] =
        R"(#version 330 core

in vec4 color;

layout (location = 0) out vec4 out_color;

void main()
{
     out_color = color;
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

    SDL_Window *window = SDL_CreateWindow("Graphics course hw 1",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          800, 600,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

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


    std::string project_root = PROJECT_ROOT;
//    obj_data bunny = parse_obj(project_root + "/bunny.obj");

    glEnable(GL_CULL_FACE);

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;


    GridBufferHolder grid_buffer_holder;
    FloatFunctionXYT func;
    GridController grid_controller(20, 20, -1.f, 1.f, -1.f, 1.f, func);

    matrix transform_matrix = grid_controller.get_transform_matrix();


    grid_controller.update_time(time);
    std::vector<vec2> grid_vertices = grid_controller.get_grid_points();
    std::vector<color_data> grid_colors = grid_controller.get_colors();
    std::vector<GLuint> grid_indices = grid_controller.get_indexes_order_to_render();

    grid_buffer_holder.update_vertices_colors_indexes(grid_vertices, grid_colors, grid_indices);
    IsolinesBuffersStruct isolines_buffer_holder;


    std::map<SDL_Keycode, bool> button_down;
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
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glLineWidth(2);

        GLint transform_location = glGetUniformLocation(program, "transform");

        glUniformMatrix4fv(transform_location, 1, GL_TRUE, transform_matrix.data());
        grid_controller.update_time(time);
        grid_colors = grid_controller.get_colors();
        grid_buffer_holder.update_colors(grid_colors);
        grid_buffer_holder.draw();

        auto full_paths = grid_controller.get_isolines({0.5});
        isolines_buffer_holder.draw(full_paths);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
