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

std::string to_string(std::string_view str)
{
    return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message)
{
    throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error)
{
    throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

const char *fragment_shader_source = R"(#version 330 core
layout (location = 0) out vec4 out_color;

in vec3 color;
int len = 16;

void main() {
    float x = floor(color.x * len);
    float y = floor(color.y * len);
    int overall = (int(x) + int(y)) % 2;
    out_color = vec4(overall, overall, overall, 0.0);
}
)";

// vertex shader
const char *vertex_shader_source = R"(#version 330 core

const vec2 VERTICES[3] = vec2[3](
        vec2(-0.5, -0.5),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0)
);

out vec3 color;

void main() {
    gl_Position = vec4(VERTICES[gl_VertexID], 0.0, 1.0);
    color = vec3((gl_Position.x + 1) / 2, (gl_Position.y + 1) / 2, 0.0);
})";

GLuint createShader(GLenum shaderType, const char * shaderSource) {
    auto shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &shaderSource, nullptr);
    glCompileShader(shaderId);
    GLint isCompiled = 0;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLsizei logLength;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
        char msg[logLength];
        glGetShaderInfoLog(shaderId, 512, &logLength, msg);
        throw std::runtime_error(msg);
    }
    return shaderId;
}

GLuint createProgram(GLuint vertexShaderId, GLuint fragmentShaderId) {
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);
    GLint linkedStatus = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkedStatus);
    if (linkedStatus == GL_FALSE) {
        GLsizei logLength = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        char msg[logLength];
        glGetProgramInfoLog(programId, 512, &logLength, msg);
        throw std::runtime_error(msg);
    }
    return programId;

}

int main() try
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");

    SDL_Window * window = SDL_CreateWindow("Graphics course practice 1",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
        sdl2_fail("SDL_CreateWindow: ");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    GLuint fragmentShaderId = createShader(GL_FRAGMENT_SHADER, fragment_shader_source);
    GLuint vertexShaderId = createShader(GL_VERTEX_SHADER, vertex_shader_source);

    GLuint programId = createProgram(vertexShaderId, fragmentShaderId);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);

    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    bool running = true;
    while (running)
    {
        for (SDL_Event event; SDL_PollEvent(&event);) switch (event.type)
        {
        case SDL_QUIT:
            running = false;
            break;
        }

        if (!running)
            break;

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(programId);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
