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

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec4 in_color;
layout (location = 2) in float in_distance;


out vec4 color;
out float distance;

void main()
{
    gl_Position = view * vec4(in_position, 0.0, 1.0);
    color = in_color;
    distance = in_distance;
}
)";

const char main_fragment_shader_source[] =
        R"(#version 330 core

uniform int dotted;
uniform float time;

in vec4 color;
in float distance;

layout (location = 0) out vec4 out_color;

void main()
{
    if (dotted == 1) {
        if (mod(distance + time, 40.0) >= 20.0) {
            discard;
        }
    }
    out_color = vec4(color.xyz, 1.0);
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

struct vec2 {
    float x;
    float y;
};

struct vertex {
    vec2 position;
    GLubyte color[4];
};

struct vertex_with_distance {
    vertex point;
    float distance_from_start;
};

vec2 bezier(const std::vector<vertex> &vertices, float t) {
    std::vector<vec2> points(vertices.size());

    for (std::size_t i = 0; i < vertices.size(); ++i)
        points[i] = vertices[i].position;

    // De Casteljau's algorithm
    for (std::size_t k = 0; k + 1 < vertices.size(); ++k) {
        for (std::size_t i = 0; i + k + 1 < vertices.size(); ++i) {
            points[i].x = points[i].x * (1.f - t) + points[i + 1].x * t;
            points[i].y = points[i].y * (1.f - t) + points[i + 1].y * t;
        }
    }
    return points[0];
}

void update_bezier_points(std::vector<vertex_with_distance> &bezierBuffer, const std::vector<vertex> &verticesBuffer, uint quality) {
    bezierBuffer.clear();
    vec2 previous_point = vec2({0.f, 0.f});
    float current_distance = 0.f;
    for (uint rev_t = 0; rev_t <= quality; rev_t++) {
        vec2 next_bezier_point = bezier(verticesBuffer, 1.f * rev_t / quality);
        float current_segment_length = 0.f;
        if (rev_t != 0) {
            current_segment_length = std::hypot(next_bezier_point.x - previous_point.x, next_bezier_point.y - previous_point.y);
        }
        current_distance += current_segment_length;
        bezierBuffer.push_back({next_bezier_point, {0xFF, 0x00, 0x00, 0xFF}, current_distance});

        previous_point = next_bezier_point;
//        std::cout << current_distance << "--------";
    }
//    std::cout << '\n';
}

int main() try {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_Window *window = SDL_CreateWindow("Graphics course practice 3",
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

    SDL_GL_SetSwapInterval(0);

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto main_segments_fragment_shader = create_shader(GL_FRAGMENT_SHADER, main_fragment_shader_source);

    auto main_segments_program = create_program(vertex_shader, main_segments_fragment_shader);

    GLuint mainVerticesVAOId;
    GLuint mainVerticesVBOId;

    GLuint bezierVerticesVAOId;
    GLuint bezierVerticesVBOId;

    std::vector<vertex> verticesBuffer;
    std::vector<vertex_with_distance> bezierBuffer;
    uint quality = 20;

    glGenVertexArrays(1, &mainVerticesVAOId);
    glBindVertexArray(mainVerticesVAOId);

    glGenBuffers(1, &mainVerticesVBOId);
    glBindBuffer(GL_ARRAY_BUFFER, mainVerticesVBOId);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *) 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex), (void *) sizeof(vec2));

    glGenVertexArrays(1, &bezierVerticesVAOId);
    glBindVertexArray(bezierVerticesVAOId);

    glGenBuffers(1, &bezierVerticesVBOId);
    glBindBuffer(GL_ARRAY_BUFFER, bezierVerticesVBOId);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_with_distance), (void *) 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_with_distance), (void *) sizeof(vec2));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(vertex_with_distance), (void *) sizeof(vertex));

    GLuint view_location = glGetUniformLocation(main_segments_program, "view");
    GLuint dotted_flag_location = glGetUniformLocation(main_segments_program, "dotted");
    GLuint time_location = glGetUniformLocation(main_segments_program, "time");

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;

    bool running = true;
    while (running) {
        bool something_happened = false;
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
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int mouse_x = event.button.x;
                        int mouse_y = event.button.y;
                        verticesBuffer.push_back({(GLfloat) mouse_x, (GLfloat) mouse_y, {0xFF, 0x00, 0x00, 0xFF}});
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        if (!verticesBuffer.empty()) {
                            verticesBuffer.pop_back();
                        }
                    }
                    something_happened = true;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_LEFT) {
                        if (quality != 1) {
                            --quality;
                        }
                    } else if (event.key.keysym.sym == SDLK_RIGHT) {
                        ++quality;
                    }
                    if (verticesBuffer.size() <= 2) {
                        break;
                    }
                    something_happened = true;
                    break;
            }

        if (!running)
            break;

        if (something_happened) {
            update_bezier_points(bezierBuffer, verticesBuffer, quality);

            glBindVertexArray(mainVerticesVAOId);
            glBindBuffer(GL_ARRAY_BUFFER, mainVerticesVBOId);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * verticesBuffer.size(), verticesBuffer.data(), GL_DYNAMIC_DRAW);

            glBindVertexArray(bezierVerticesVAOId);
            glBindBuffer(GL_ARRAY_BUFFER, bezierVerticesVBOId);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_with_distance) * bezierBuffer.size(), bezierBuffer.data(), GL_DYNAMIC_DRAW);
//            for (const auto &ver : bezierBuffer) {
//                std::cout << ver.distance_from_start << "/";
//            }
//            std::cout << "\n";

            std::vector<vertex_with_distance> lol(bezierBuffer.size());

            glGetBufferSubData(GL_ARRAY_BUFFER, 0, bezierBuffer.size() * sizeof(vertex_with_distance), lol.data());
//            for (const auto &ver : lol) {
//                std::cout << ver.distance_from_start << "\\";
//            }
//            std::cout << "\n";
        }

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        last_frame_start = now;
        time += dt;

        glClear(GL_COLOR_BUFFER_BIT);

        float view[16] =
                {
                        2.f / (float) width, 0.f, 0.f, -1.f,
                        0.f, -2.f / (float) height, 0.f, 1.f,
                        0.f, 0.f, 1.f, 0.f,
                        0.f, 0.f, 0.f, 1.f,
                };


        glUniformMatrix4fv(view_location, 1, GL_TRUE, view);
        glUniform1f(time_location, time * 200);
        glUniform1i(dotted_flag_location, 1);
//        glUniform1i(dotted_flag_location, 0);

        glUseProgram(main_segments_program);

//        glBindVertexArray(mainVerticesVAOId);
//        glBindBuffer(GL_ARRAY_BUFFER, mainVerticesVBOId);
//
//        glLineWidth(4.f);
//        glDrawArrays(GL_LINE_STRIP, 0, verticesBuffer.size());
//        glDrawArrays(GL_POINTS, 0, verticesBuffer.size());

        glBindVertexArray(bezierVerticesVAOId);
        glBindBuffer(GL_ARRAY_BUFFER, bezierVerticesVBOId);
        glLineWidth(10.f);

        glDrawArrays(GL_LINE_STRIP, 0, bezierBuffer.size());

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
