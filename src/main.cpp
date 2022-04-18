#include <version.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>

#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <core/runtime.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// DEBUG macros
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
#ifndef NDEBUG
    #define FOST_DEBUG
#endif

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// LOGGING
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
static bool logging_can_be_used = true;

#ifdef FOST_DEBUG
    #define FOST_ASSERT(expr, msg) assert(( (void)(msg), (expr) ))

    #define FOST_LOG_INFO(...) \
        do \
        { \
            FOST_ASSERT (logging_can_be_used, "Attempt to log time reported by glfwGetTime() when glfw not initialized!"); \
            spdlog::info (__VA_ARGS__); \
        } while (0)
#else
    #define FOST_ASSERT(...)
    #define FOST_LOG_INFO(...)
#endif

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// SHADERS
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
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

static void check_shader_status (GLuint shader, GLuint what)
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

static void check_program_status (GLuint program, GLuint what)
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

static GLuint compile_shader (const shader_info &s)
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

static GLuint prepare_program (std::initializer_list <shader_info> shaders)
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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CALLBACK declarations
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
static void glfw_error_callback (int error, const char *desc);
// static void window_size_callback (GLFWwindow *window, int width, int height);
static void framebuffer_size_callback (GLFWwindow *window, int width, int height);
// static void character_callback (GLFWwindow *window, unsigned int codepoint);
static void key_callback (GLFWwindow *window, int key, int scancode, int action, int mods);
static void mouse_callback (GLFWwindow *window, double xpos, double ypos);

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// INPUT declarations
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class key_event
{
public:
    key_event ()
        : _pressed {fost::clock::now ()}
        , _released {_pressed}
    {}

    ~key_event () = default;

    inline const fost::clock::time_point & pressed() const
    {
        return _pressed;
    }

    inline const fost::clock::time_point & released() const
    {
        return _released;
    }

    inline const bool is_complete () const
    {
        return (_released != _pressed);
    }

    template <class _Unit = std::chrono::milliseconds>
    inline const auto time_elapsed () const
    {
        return std::chrono::duration_cast <_Unit> (fost::clock::now () - _pressed);
    }

    inline const auto ticks_elapsed () const
    {
        return (time_elapsed () / fost::runtime::mspt);
    }

    template <class _Unit = std::chrono::milliseconds>
    inline const auto time_held () const
    {
        assert (is_complete ());
        return std::chrono::duration_cast <_Unit> (_released - _pressed);
    }

    inline const auto ticks_held () const
    {
        assert (is_complete ());
        return (time_held () / fost::runtime::mspt);
    }

private:
    // TODO: Friend input system class/function, not this global one.
    friend void key_callback (GLFWwindow*, int, int, int, int);

    fost::clock::time_point _pressed;
    fost::clock::time_point _released;

    inline void complete ()
    {
        assert (_released == _pressed);
        _released = fost::clock::now ();
    }
};

static constexpr std::size_t MAX_NUM_KEYS = 256;
static std::vector <key_event> g_keys[MAX_NUM_KEYS] = {};

inline const int resolve_scancode (const int key, int scancode);
const std::vector <key_event> & get_events (const int key, int scancode = -1);
const std::vector <key_event> get_complete (const int key, int scancode = -1);
const int count_complete (const int key, int scancode = -1);
const int key_down (int key, int scancode = -1);
const int key_up (int key, int scancode = -1);
const long key_held (int key, int scancode = -1);
void prune_keys ();

static GLboolean g_wireframe = GL_FALSE;
static GLboolean g_vsync = GL_FALSE;

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CAMERA | EYEPOINT | LENS declarations
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
static constexpr glm::vec3 world_up = {0.0f, 1.0f, 0.0f};

class eyepoint
{
public:
    eyepoint ()
        : _position {0.0f, 0.0f, 1.0f}
        , _direction {0.0f, 0.0f, -1.0f}
        , _target {nullptr}
        , _FOV {45.0f}
        , _near {0.1f}
        , _far {100.0f}
    {}

    void cycle (const std::chrono::duration<float> dt);
    void tick (const std::chrono::duration<float> dt);

    inline glm::mat4 see () const
    {
        return glm::lookAt (_position, _position + _direction, world_up);
    }

    inline void lock_on (const glm::vec3 *const target)
    {
        _target = const_cast<glm::vec3*> (target);
    }

    inline void track (const glm::vec3 *const target)
    {
        // TODO: Not implemented;
    }

private:
    glm::vec3 _position;
    glm::vec3 _direction;
    glm::vec3 *_target;
public: // TODO: should not be..
    GLfloat _FOV = 45.0f;
    GLfloat _near = 0.1f;
    GLfloat _far = 100.0f;
private: // TODO: should not exist..
    static std::chrono::duration<float> elapsed;
    static glm::vec3 prev_pos;
};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// GLOBAL variables
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
static auto g_lens = eyepoint {};
static auto g_projection = glm::mat4 {1.0f};

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CALLBACK definitions
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
static void glfw_error_callback (int error, const char *desc)
{
    std::cerr << "Error [" << error << "]: " << desc << '\n';
}

// static void window_size_callback (GLFWwindow *window, int width, int height)
// {
//     std::cout << "Window size: " << width << "x" << height << '\n';
// }

static void framebuffer_size_callback (GLFWwindow *window,
                                       int width, int height)
{
    glViewport (0, 0, width, height);
    g_projection = glm::perspective (glm::radians (g_lens._FOV),
                                     (float) width / (float) height,
                                     g_lens._near,
                                     g_lens._far);
    // std::cout << "Framebuffer size: " << width << "x" << height << '\n';
}

// static void character_callback (GLFWwindow *window, unsigned int codepoint)
// {
//     std::cerr << codepoint; // cerr is automatically flushed.
// }

static void key_callback (GLFWwindow *window,
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
                if (mods & GLFW_MOD_CONTROL)
                {
                    if (g_wireframe)
                    {
                        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
                        // std::cout << "Wireframe disabled\n";
                    }
                    else
                    {
                        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
                        // std::cout << "Wireframe enabled\n";
                    }
                    g_wireframe = ! g_wireframe;
                }
                break;
            case GLFW_KEY_V:
                if (mods & GLFW_MOD_CONTROL)
                {
                    g_vsync = ! g_vsync;
                    glfwSwapInterval (g_vsync);
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
        // std::cout << "[Async?] Key " << scancode << " pressed now\n";

        g_keys[scancode].emplace_back (key_event {});
    }
    else if (action == GLFW_RELEASE)
    {
        const auto last_idx = g_keys[scancode].size () - 1;
        if (last_idx > 0)
        {
            assert (g_keys[scancode][last_idx - 1].is_complete ());
        }
        g_keys[scancode][last_idx].complete ();
        // const auto dd = g_keys[scancode][last_idx].time_held <std::chrono::milliseconds> ();
        // const auto ticks = g_keys[scancode][last_idx].ticks_held ();
        // std::cout << "[Async?] Key " << scancode << " released after " << dd.count () << " ms or " << ticks << " ticks.\n";
    }
}

static void mouse_callback (GLFWwindow *window, double xpos, double ypos)
{
    std::cout << "pos (" << xpos << "," << ypos << ")\n";
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// INPUT definitions
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
inline const int resolve_scancode (const int key, int scancode)
{
    if (key > 0)
    {
        scancode = glfwGetKeyScancode (key);
    }
    assert (scancode > 0);
    return scancode;
}

const std::vector <key_event> & get_events (const int key, int scancode)
{
    scancode = resolve_scancode (key, scancode);
    return g_keys[scancode];
}

const std::vector <key_event> get_complete (const int key, int scancode)
{
    scancode = resolve_scancode (key, scancode);
    auto complete_events = std::vector <key_event> {};

    for (const auto &e : g_keys[scancode])
        if (e.is_complete ())
            complete_events.push_back (e);

    return complete_events;
}

const int count_complete (const int key, int scancode) // TODO: same as key_up
{
    scancode = resolve_scancode (key, scancode);
    int count = 0;

    for (const auto &e : g_keys[scancode])
        if (e.is_complete ())
            ++count;

    return count;
}

const int key_down (const int key, int scancode)
{
    scancode = resolve_scancode (key, scancode);
    std::size_t downs_in_cur_tick = g_keys[scancode].size ();
    if (downs_in_cur_tick == 0)
        return 0;

    const key_event &first = g_keys[scancode].front ();
    if (first.ticks_elapsed () > 0L)
        --downs_in_cur_tick;

    return static_cast <int> (downs_in_cur_tick);
}

const int key_up (const int key, int scancode) // TODO: same as count_complete
{
    return count_complete (key, scancode);
}

const long key_held (const int key, int scancode)
{
    scancode = resolve_scancode (key, scancode);
    if (g_keys[scancode].size () == 0)
        return false;

    const key_event &last = g_keys[scancode].back ();
    return (last.is_complete () ? 0L : last.ticks_elapsed ());
}

void prune_keys ()
{
    for (int i = 0; i < MAX_NUM_KEYS; ++i)
    {
        std::erase_if (g_keys[i], [] (const key_event &e)
        {
            return e.is_complete ();
        });
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CAMERA | EYEPOINT | LENS definitions
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
std::chrono::duration<float> eyepoint::elapsed {};
glm::vec3 eyepoint::prev_pos {};
void eyepoint::cycle (const std::chrono::duration<float> dt)
{
    elapsed += dt;
    if (elapsed >= 1s)
    {
        elapsed = 0s;
        FOST_LOG_INFO ("Moved {} units in 1 second", glm::to_string (glm::abs (_position - prev_pos)));
        FOST_LOG_INFO ("Pos {}", glm::to_string (_position));
        prev_pos = _position;
    }

}

void eyepoint::tick (const std::chrono::duration<float> dt)
{
    // std::cout << "[tick] dit: " << (dt / 1s) << '\n';

    static constexpr GLfloat slow_walk = {0.1f};
    static constexpr GLfloat norm_walk = {1.0f};

    if (key_down (GLFW_KEY_T))
    {
        if (_target && key_down (GLFW_KEY_T))
        {
            lock_on (nullptr);
            _direction = glm::normalize (_direction);
            std::cout << "[Tick] Stopped following.\n";
        }
        else
        {
            static constexpr glm::vec3 target = {0.0f, 0.0f, 0.0f};
            lock_on (&target);
            std::cout << "[Tick] Following origin.\n";
        }
    }

    if (_target)
    {
        _direction = *_target - _position;
    }
    else
    {
        // Get input from mouse for orientation.
        // _direction = glm::normalize (_direction);
    }

    if (key_held (GLFW_KEY_W))
    {
        // std::cout << "[Tick] Bigger step forward.\n";
        _position += norm_walk * _direction * (dt / 1s);
    }
    if (key_held (GLFW_KEY_S))
    {
        // std::cout << "[Tick] Bigger step backward.\n";
        _position -= norm_walk * _direction * (dt / 1s);
    }
    if (key_held (GLFW_KEY_A))
    {
        // std::cout << "[Tick] Bigger step left.\n";
        _position -= norm_walk * glm::normalize(glm::cross(_direction, world_up)) * (dt / 1s);
    }
    if (key_held (GLFW_KEY_D))
    {
        // std::cout << "[Tick] Bigger step right.\n";
        _position += norm_walk * glm::normalize(glm::cross(_direction, world_up)) * (dt / 1s);
    }
    // std::cout << "Position " << glm::to_string (_position) << '\n';
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// MAIN
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void clean_glfw (GLFWwindow *window)
{
    glfwDestroyWindow (window);
    glfwTerminate ();
}

#define ENABLE_CRAPPY_BUILD 0

#if ENABLE_CRAPPY_BUILD

#endif

int main (int argc, char **argv)
{
    FOST_LOG_INFO ("Welcome to {} from spdlog!", "Blockytry");
    std::cout << "Blockytry " << BLOCKYTRY_VERSION_STRING << '\n';

    // Set error callback.
    glfwSetErrorCallback (glfw_error_callback);

    // Init GLFW.
    if (! glfwInit ())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return 1;
    }
    logging_can_be_used = true;
#if ENABLE_CRAPPY_BUILD

    std::cout << fost::runtime::tps () << '\n';

    glfwTerminate ();
    return 0;
#endif
    std::cout << "GLFW " << glfwGetVersionString () << '\n';

    // Let OpenGL know we want to use the programmable pipeline.
    // Version 3.3.0 is selected for maximum portability.
    // It may be increased in time if more advanced features are required.
    glfwDefaultWindowHints ();
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and make its context current
    int width = 1024;
    int height = 720;
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

    glfwSetInputMode (window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported ())
        glfwSetInputMode (window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // Set other callbacks.
    // glfwSetWindowSizeCallback (window, window_size_callback);
    glfwSetFramebufferSizeCallback (window, framebuffer_size_callback);
    // glfwSetCharCallback(window, character_callback);
    glfwSetKeyCallback (window, key_callback);
    glfwSetCursorPosCallback (window, mouse_callback);

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

    g_projection = glm::perspective (glm::radians (g_lens._FOV),
                                     (float) width / (float) height,
                                     g_lens._near,
                                     g_lens._far);

    FOST_LOG_INFO ("Tickrate: {} mspt | {} tps", fost::runtime::mspt.count (), fost::runtime::tps);

    fost::runtime::duration accumulator {0s};
    fost::runtime::time_point t {};

    // TODO: Figure out game loop.
    glfwSwapInterval (g_vsync);
    // Loop until the user closes the window
    while (! glfwWindowShouldClose (window))
    {
        // FOST_LOG_INFO ("Frame debug: {}ms dt | {} fps", fost::runtime::frametime ().count (), fost::runtime::fps ());
        // IMPORTANT! Must cycle runtime to advance simulation (calculates delta time).
        fost::runtime::cycle ();
        auto delta_time = fost::runtime::frame_time ();

        if (delta_time > 250ms)
            delta_time = 250ms;

        accumulator += delta_time;

        // Handle some inputs.
        g_lens.cycle (fost::runtime::frame_time ()); // ???????

        while (accumulator >= fost::runtime::tick_unit)
        {
            // std::cout << "dt " << delta_time.count () << '\n';

            g_lens.tick (fost::runtime::tick_unit);

            prune_keys ();
            t += fost::runtime::tick_unit;
            accumulator -= fost::runtime::tick_unit;
        }

        static const GLfloat background_color[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        glClearBufferfv (GL_COLOR, 0, background_color);

        glUseProgram (prog);
        glBindVertexArray (vao);

        // time = static_cast<GLfloat> (glfwGetTime () * speed);
        // g_lens.position = glm::vec3 (glm::sin (time) * radius,
        //                              0.0f,
        //                              glm::cos (time) * radius);
        // glm::mat4 view = glm::lookAt (g_lens.position, g_lens.direction, g_lens.up);
        glm::mat4 view = g_lens.see ();
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