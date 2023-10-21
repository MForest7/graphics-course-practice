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
#include <map>
#include <chrono>

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

const char vertex_shader_source[] =
R"(#version 330 core

const vec2 VERTICES[8] = vec2[8](
    vec2(0.0, 0.0),
    vec2(-1.0, 0.0),
    vec2(-0.5, -sqrt(0.75)),
    vec2(0.5, -sqrt(0.75)),
    vec2(1.0, 0.0),
    vec2(0.5, sqrt(0.75)),
    vec2(-0.5, sqrt(0.75)),
    vec2(-1.0, 0.0)
);

const vec3 COLORS[8] = vec3[8](
    vec3(0.5, 0.5, 0.5),
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 1.0, 1.0),
    vec3(1.0, 0.0, 1.0),
    vec3(1.0, 1.0, 0.0),
    vec3(1.0, 0.0, 0.0)
);

out vec3 color;

uniform mat4 transform;
uniform mat4 view;

void main()
{
    vec2 position = VERTICES[gl_VertexID];
    gl_Position = view * transform * vec4(position, 0.0, 1.0);
    color = COLORS[gl_VertexID];
}
)";

const char fragment_shader_source[] =
R"(#version 330 core

in vec3 color;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(color, 1.0);
}
)";

const char uniform_scale[] = "scale";
const char uniform_angle[] = "angle";
const char uniform_transform[] = "transform";
const char uniform_view[] = "view";

GLuint create_shader(GLenum type, const char * source)
{
    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, &source, nullptr);
    glCompileShader(result);
    GLint status;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint info_log_length;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Shader compilation failed: " + info_log);
    }
    return result;
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint result = glCreateProgram();
    glAttachShader(result, vertex_shader);
    glAttachShader(result, fragment_shader);
    glLinkProgram(result);

    GLint status;
    glGetProgramiv(result, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint info_log_length;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Program linkage failed: " + info_log);
    }

    return result;
}

int main() try
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");

    SDL_Window * window = SDL_CreateWindow("Graphics course practice 2",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
        sdl2_fail("SDL_CreateWindow: ");

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);
    //SDL_GL_SetSwapInterval(0);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

    GLuint program = create_program(vertex_shader, fragment_shader);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    glUseProgram(program);
    float scale = 0.5f;
    float time = 0.0f;
    
    int transform_location = glGetUniformLocation(program, uniform_transform);
    int view_location = glGetUniformLocation(program, uniform_view);

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float x = 0;
    float y = 0;
    float angle = 0;
    float direction = 0;
    float angle_direction = 0;

    bool running = true;
    while (running)
    {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        std::cout << "duration: " << dt << " s" << std::endl;
        last_frame_start = now;
        time += dt;

        for (SDL_Event event; SDL_PollEvent(&event);) switch (event.type)
        {
        case SDL_QUIT:
            running = false;
            break;
        case SDL_WINDOWEVENT: switch (event.window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
                width = event.window.data1;
                height = event.window.data2;
                glViewport(0, 0, width, height);
                break;
            }
            break;
        case SDL_KEYDOWN: switch (event.key.keysym.scancode) 
            {
            case SDL_SCANCODE_UP:
                direction = 1;
                break;
            case SDL_SCANCODE_DOWN:
                direction = -1;
                break;
            case SDL_SCANCODE_LEFT:
                angle_direction = 1;
                break;
            case SDL_SCANCODE_RIGHT:
                angle_direction = -1;
                break;
            }
            break;
        case SDL_KEYUP: switch (event.key.keysym.scancode) 
            {
            case SDL_SCANCODE_UP:
            case SDL_SCANCODE_DOWN:
                direction = 0;
                break;
            case SDL_SCANCODE_LEFT:
            case SDL_SCANCODE_RIGHT:
                angle_direction = 0;
                break;
            }
            break;
        }

        x += direction * dt * cos(angle);
        y += direction * dt * sin(angle);

        angle += angle_direction * dt;

        if (!running)
            break;

        float transform[16] =
        {
            scale * cos(angle), scale * (- sin(angle)), 0, x, // 1 строка
            scale * sin(angle), scale * cos(angle), 0, y, // 2 строка
            0, 0, 0, 0, // 3 строка
            0, 0, 0, 1, // 4 строка
        };
        glUniformMatrix4fv(transform_location, 1, GL_TRUE, transform);

        float aspect_ratio = (float)width / height;

        float view[16] =
        {
            1 / aspect_ratio, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        glUniformMatrix4fv(view_location, 1, GL_TRUE, view);

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 8);

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
