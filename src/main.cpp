#include <version.hpp>

#include <io/keyboard_input.hpp>
#include <shared/counter.hpp>
#include <shared/lookup_table.hpp>

#include <cassert>
#include <cstddef>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

#define GLAD_GL_IMPLEMENTATION
#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Shader stuff
#define BUFFER_OFFSET(o) ((void*) (o))

#ifndef DEFAULT_SHADER_PATH
    #define SHADER_PATH ""
#else
    #define SHADER_PATH DEFAULT_SHADER_PATH
#endif

#define CHECK_UNIFORM(u) \
    do \
    { \
        if (u == -1) \
            std::cerr << "warn: no location for " #u "\n"; \
    } while (0)

#define UNIFORM(p, u) \
    const GLint u = glGetUniformLocation (p, #u); \
    CHECK_UNIFORM (u)

static void
check_shader_status (GLuint shader, GLuint what)
{
    int res = GL_TRUE;
    glGetShaderiv (shader, what, &res);

    if (res != GL_TRUE)
    {
        int log_size = 0;
        glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &log_size);

        if (! log_size)
            return;

        char *message = new char[log_size];
        glGetShaderInfoLog (shader, log_size, &log_size, message);

        std::cerr << message << '\n';

        delete[] message;
        assert (0);
    }
}

static void
check_program_status (GLuint program, GLuint what)
{
    int res = GL_TRUE;
    glGetProgramiv (program, what, &res);

    if (res != GL_TRUE)
    {
        int log_size = 0;
        glGetProgramiv (program, GL_INFO_LOG_LENGTH, &log_size);

        if (! log_size)
            return;

        char *message = new char[log_size];
        glGetProgramInfoLog (program, log_size, &log_size, message);

        std::cerr << message << '\n';

        delete[] message;
        assert (0);
    }
}

struct shader_info
{
    const char *filepath;
    GLuint type;
};

static GLuint
compile_shader (const shader_info &s)
{
    // Read shader file into local buffer.
    std::ifstream f;
    std::cout << "Loading shader: " << s.filepath << '\n';

    f.open (s.filepath);
    assert (f.good ());

    std::string source;
    {
        std::stringstream ss;
        while (! f.eof ())
        {
            std::string line;
            std::getline (f, line);
            ss << line << '\n';
        }
        source = ss.str ();
    }
    const char *sz_source = source.c_str ();

    // Prepare and compile shader
    GLuint shader = glCreateShader (s.type);
    glShaderSource (shader, 1, &sz_source, nullptr);
    glCompileShader (shader);
    check_shader_status (shader, GL_COMPILE_STATUS);

    return shader;
}

static GLuint
prepare_program (std::initializer_list <shader_info> shaders)
{
    GLuint program = glCreateProgram ();

    std::cout << "Preparing program " << program << "...\n";

    for (const auto &s : shaders)
    {
        GLuint shader = compile_shader (s);
        glAttachShader (program, shader);
        glDeleteShader (shader);
    }

    std::cout << "Linking program...\n";
    glLinkProgram (program);
    check_program_status (program, GL_LINK_STATUS);

    std::cout << "Validating program...\n";
    glValidateProgram (program);
    check_program_status (program, GL_VALIDATE_STATUS);

    std::cout << "Program " << program << " OK.\n";
    return program;
}

// Camera stuff.

// GLFW stuff.
/* Callbacks. */
static void
glfw_error_callback (int error, const char *desc)
{
    std::cerr << "Error [" << error << "]: " << desc << '\n';
}

static GLboolean g_is_wireframe = GL_FALSE;

static void
key_callback (GLFWwindow *window,
              int key, int scancode, int action, int mods)

{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose (window, GLFW_TRUE);
                break;
            case GLFW_KEY_W:
                if (mods & GLFW_MOD_SHIFT)
                {
                    if (g_is_wireframe)
                    {
                        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
                        // std::cout << "Wireframe disabled\n";
                    }
                    else
                    {
                        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
                        // std::cout << "Wireframe enabled\n";
                    }
                    g_is_wireframe = ! g_is_wireframe;
                }
                break;
            case GLFW_KEY_ENTER:
                if (mods & GLFW_MOD_ALT)
                {
                    std::cout << "Fullscreen\n";
                }
            default:
                break;
        }
    }
    if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_T)
    {
        std::cout << "Repeat\n";
    }
}

/*
static void
window_size_callback (GLFWwindow *window, int width, int height)
{
    // std::cout << "Window size: " << width << "x" << height << '\n';
}
*/

struct eyepoint
{
    eyepoint ()
        : position { 0.0f, 0.0f, 1.0f }
        , direction { 0.0f, 0.0f, -1.0f }
        , up { 0.0f, 1.0f, 0.0f }
        , FOV (45.0f)
        , near (0.1f)
        , far (100.0f)
    {}

    inline glm::mat4 see () const
    {
        return glm::lookAt (position, position + direction, up);
    }

    inline glm::mat4 follow () const
    {
        return glm::lookAt (position, target, up);
    }

    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 target;
    glm::vec3 up;
    GLfloat FOV = 45.0f;
    GLfloat near = 0.1f;
    GLfloat far = 100.0f;
} g_lens;

static glm::mat4 g_projection (1.0f);

static void
framebuffer_size_callback (GLFWwindow *window, int width, int height)
{
    glViewport (0, 0, width, height);
    g_projection = glm::perspective (glm::radians (g_lens.FOV),
                                     (float) width / (float) height,
                                     g_lens.near,
                                     g_lens.far);
    // std::cout << "Framebuffer size: " << width << "x" << height << '\n';
}

/*
void character_callback(GLFWwindow* window, unsigned int codepoint)
{
    std::cerr << codepoint; // cerr is automatically flushed.
}
*/

void
clean_glfw (GLFWwindow *window)
{
    glfwDestroyWindow (window);
    glfwTerminate ();
}


using namespace std::string_view_literals;

static constexpr shared::map <std::string_view, int, 3> actions = 
    {{
        { "jump"sv, GLFW_KEY_SPACE },
        { "walk"sv, GLFW_KEY_W },
        { "run"sv, GLFW_KEY_LEFT_SHIFT }
    }};

static constexpr shared::map <int, double, 3> actions2 = 
    {{
        { GLFW_KEY_SPACE, 2.4 },
        { GLFW_KEY_W, 3.0 },
        { GLFW_KEY_LEFT_SHIFT, 0.3213 }
    }};


int
main (int argc, char **argv)
{
    std::cout << "CRAPPY BUILD!!!\n";

    double key = shared::lookup (actions2, shared::lookup (actions, "jump"));
    std::cout << key << '\n';

    key = shared::lookup (actions2, shared::lookup (actions, "walk"));
    std::cout << key << '\n';

    key = shared::lookup (actions2, shared::lookup (actions, "run"));
    std::cout << key << '\n';

    return 0;
    std::cout << "Blockytry " << BLOCKYTRY_VERSION_STRING << '\n';

    // Set error callback.
    glfwSetErrorCallback (glfw_error_callback);

    // Init GLFW.
    if (! glfwInit ())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return 1;
    }
    std::cout << "GLFW " << glfwGetVersionString () << '\n';

    // Let OpenGL know we want to use the programmable pipeline.
    // Version 3.3.0 is selected for maximum portability.
    // It may be increased in time if more advanced features are required.
    glfwDefaultWindowHints ();
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and make its context current
    int width = 640;
    int height = 480;
    GLFWwindow *window = glfwCreateWindow (width, height,
                                           "Blockytry",
                                           nullptr, nullptr);
    if (! window)
    {
        std::cerr << "Failed to create OpenGL window.\n";
        glfwTerminate ();
        return 1;
    }
    glfwMakeContextCurrent (window);

    int glad_version = gladLoadGL (glfwGetProcAddress);
    if (! glad_version)
    {
        std::cerr << "Failed to load OpenGL functions with Glad.\n";
        clean_glfw (window);
        return 1;
    }

    std::cout << "OpenGL " << GLAD_VERSION_MAJOR (glad_version) << "."
              << GLAD_VERSION_MINOR (glad_version) << " Core profile\n";
    std::cout << "GLAD loader " << GLAD_GENERATOR_VERSION << '\n';

    // Print device info and stuff
    std::cout << "Driver vendor " << glGetString (GL_VENDOR) << '\n';
    std::cout << "Driver version " << glGetString (GL_VERSION) << '\n';
    std::cout << "Device vendor " << glGetString (GL_RENDERER) << '\n';

    // Set callbacks.
    glfwSetKeyCallback (window, key_callback);
    // glfwSetWindowSizeCallback (window, window_size_callback);
    glfwSetFramebufferSizeCallback (window, framebuffer_size_callback);
    // glfwSetCharCallback(window, character_callback);

    // Setup cube geometry.
    const glm::vec3 caca (0.0f, 0.0f, 0.0f);
    const glm::vec3 pos = {0.0f, 0.0f, 0.0f};
    const GLfloat scale = 0.1f;

    const GLfloat vertices[24] = {
        pos.x - scale / 2, pos.y - scale / 2, pos.z - scale / 2,
        pos.x - scale / 2, pos.y - scale / 2, pos.z + scale / 2,
        pos.x - scale / 2, pos.y + scale / 2, pos.z - scale / 2,
        pos.x - scale / 2, pos.y + scale / 2, pos.z + scale / 2,
        pos.x + scale / 2, pos.y - scale / 2, pos.z - scale / 2,
        pos.x + scale / 2, pos.y - scale / 2, pos.z + scale / 2,
        pos.x + scale / 2, pos.y + scale / 2, pos.z - scale / 2,
        pos.x + scale / 2, pos.y + scale / 2, pos.z + scale / 2
    };

    const GLuint indices[14] = {
        4, 6, 5, 7, 3, 6, 2, 4, 0, 5, 1, 3, 0, 2
    };

    // Init stuff.
    GLuint vao = 0U, vbo = 0U, ibo = 0U;
    glGenVertexArrays (1, &vao);
    glGenBuffers (1, &vbo);
    glGenBuffers (1, &ibo);

    if (! vao)
    {
        std::cerr << "error: could not generate vertex array\n";
        clean_glfw (window);
        return 1;
    }
    if (! vbo)
    {
        std::cerr << "error: could not generate vertex buffer\n";
        clean_glfw (window);
        return 1;
    }
    if (! ibo)
    {
        std::cerr << "error: could not generate index buffer \n";
        clean_glfw (window);
        return 1;
    }

    glBindVertexArray (vao);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, ibo);

    glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);

    // glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*) 0);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET (0));
    glEnableVertexAttribArray (0);

    glBindVertexArray (0);
    glDeleteBuffers (1, &vbo);
    glDeleteBuffers (1, &ibo);

    // Setup shaders.
    GLuint prog = prepare_program ({
        { SHADER_PATH "default.vert", GL_VERTEX_SHADER },
        { SHADER_PATH "default.frag", GL_FRAGMENT_SHADER }
    });

    glUseProgram (prog);
    UNIFORM (prog, u_model);
    UNIFORM (prog, u_view);
    UNIFORM (prog, u_projection);
    glUseProgram (0);

    // Setup camera.
    const GLfloat speed = 2.0f;
    const GLfloat radius = 0.5f;
    GLfloat time = 0.0f;

    g_lens.direction = { 0.0f, 0.0f, 0.0f };

    g_projection = glm::perspective (glm::radians (g_lens.FOV),
                                     (float) width / (float) height,
                                     g_lens.near,
                                     g_lens.far);

    shared::counter_down <GLuint> key_recurrence_counter;
    key_recurrence_counter.reset (10U);

    // Loop until the user closes the window
    while (! glfwWindowShouldClose (window))
    {
        static const GLfloat background_color[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        glClearBufferfv (GL_COLOR, 0, background_color);

        glUseProgram (prog);
        glBindVertexArray (vao);

        time = static_cast<GLfloat> (glfwGetTime () * speed);
        g_lens.position = glm::vec3 (glm::sin (time) * radius,
                                     0.0f,
                                     glm::cos (time) * radius);
        glm::mat4 view = glm::lookAt (g_lens.position, g_lens.direction, g_lens.up);
        glm::mat4 model (1.0f);
        model = glm::translate (model, caca);

        glUniformMatrix4fv (u_view, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv (u_projection, 1, GL_FALSE, &g_projection[0][0]);
        glUniformMatrix4fv (u_model, 1, GL_FALSE, &model[0][0]);

        glDrawElements (GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers (window);
        glfwPollEvents ();
    }

    glDeleteProgram (prog);
    glDeleteVertexArrays (1, &vao);
    clean_glfw (window);
    return 0;
}