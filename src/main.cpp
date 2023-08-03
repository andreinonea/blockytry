#include <version.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <list>
#include <sstream>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/implot.h>
#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <core/runtime.hpp>
#include <core/cpu_profiler.hpp>
#include "volcaca.hpp"

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
    const GLint u##_##p = glGetUniformLocation (p, #u); \
    CHECK_UNIFORM (u##_##p)

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

// Key input.
static constexpr std::size_t MAX_NUM_KEYS = 256;
static constexpr std::size_t MAX_NUM_MBUTTONS = 16;
static std::list <key_event> g_keys[MAX_NUM_KEYS] = {};
static std::unordered_set <int> g_completed_events = {};

inline const int resolve_scancode (const int key, int scancode);
const std::list <key_event> & get_events (const int key, int scancode = -1);
const std::list <key_event> get_complete (const int key, int scancode = -1);
const int key_down (int key, int scancode = -1);
const int key_up (int key, int scancode = -1);
const long key_held (int key, int scancode = -1);
void prune_events ();

// Mouse input.
static GLboolean g_cursor_is_first_move = GL_TRUE;
static GLfloat g_cursor_last_x = 0.0f;
static GLfloat g_cursor_last_y = 0.0f;
static GLfloat g_cursor_movement_x = 0.0f;
static GLfloat g_cursor_movement_y = 0.0f;

void cycle_mouse_to_be_renamed ()
{
    g_cursor_movement_x = 0.0f;
    g_cursor_movement_y = 0.0f;
}

GLfloat maxis_horizontal ()
{
    return g_cursor_movement_x;
}
GLfloat maxis_vertical ()
{
    return g_cursor_movement_y;
}

// Configurations.
static GLboolean g_wireframe = GL_FALSE;
static GLboolean g_vsync = GL_FALSE;
static GLboolean g_draw_hud = GL_TRUE;
static GLboolean g_draw_debug_hud = GL_TRUE; // TODO: false default.

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CAMERA | EYEPOINT | LENS declarations
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
static constexpr glm::vec3 world_up = {0.0f, 1.0f, 0.0f};

class eyepoint
{
public:
    eyepoint ()
        : _up {0.0f, 1.0f, 0.0f}
        , _position {0.0f, 0.0f, 1.0f}
        , _direction {0.0f, 0.0f, -1.0f}
        , _target {nullptr}
        , _yaw {-90.0f}
        , _pitch {0.0f}
        , _sensitivity_x {0.1f}
        , _sensitivity_y {0.1f}
        , _FOV {45.0f}
        , _near {0.1f}
        , _far {100.0f}
    {}

    void cycle (const std::chrono::duration<float> dt);
    void tick (const std::chrono::duration<float> dt);

    inline glm::mat4 see () const
    {
        return glm::lookAt (_position, _position + _direction, _up);
    }

    inline glm::mat4 see_from (const glm::vec3 &pos) const
    {
        return glm::lookAt (pos, pos + _direction, _up);
    }

    // Fix eyes on target - strict version.
    inline void lock_on (const glm::vec3 *const target)
    {
        _target = const_cast<glm::vec3*> (target);
    }

    // Follow target as long as physically feasible (e.g. if moving forwards and
    // target leaves possible angles of head rotation, stop following, unless
    // trunk moves as well and tracking becomes possible again).
    inline void track (const glm::vec3 *const target)
    {
        // TODO: Not implemented;
    }

    inline const glm::vec3 & get_upvector ()
    {
        return _up;
    }

    inline const glm::vec3 & get_position ()
    {
        return _position;
    }

    inline const glm::vec3 & get_direction ()
    {
        return _direction;
    }

    inline const glm::vec3 & get_target ()
    {
        return *_target;
    }

    inline const GLfloat get_yaw ()
    {
        return _yaw;
    }

    inline const GLfloat get_pitch ()
    {
        return _pitch;
    }

    inline const GLfloat get_sensitivity_x ()
    {
        return _sensitivity_x;
    }

    inline void set_sensitivity_x (GLfloat sensitivity)
    {
        _sensitivity_x = sensitivity;
    }

    inline const GLfloat get_sensitivity_y ()
    {
        return _sensitivity_y;
    }

    inline void set_sensitivity_y (GLfloat sensitivity)
    {
        _sensitivity_y = sensitivity;
    }

private:
    glm::vec3 _up;
    glm::vec3 _position;
    glm::vec3 _direction;
    glm::vec3 *_target;
    GLfloat _yaw;
    GLfloat _pitch;
    GLfloat _sensitivity_x;
    GLfloat _sensitivity_y;
public: // TODO: should not be here..
    GLfloat _FOV = 45.0f;
    GLfloat _near = 0.1f;
    GLfloat _far = 100.0f;
public: // TODO: should not exist..
    static std::chrono::duration<float> elapsed;
    static glm::vec3 prev_pos;
    static glm::vec3 prev_dir;
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
                        std::cout << "Wireframe disabled\n";
                    }
                    else
                    {
                        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
                        std::cout << "Wireframe enabled\n";
                    }
                    g_wireframe = ! g_wireframe;
                }
                break;
            case GLFW_KEY_V:
                if (mods & GLFW_MOD_CONTROL)
                {
                    g_vsync = ! g_vsync;
                    glfwSwapInterval (g_vsync);
                    std::cout << "Vsync " << static_cast <int> (g_vsync) << '\n';
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
        g_keys[scancode].emplace_back ();
        // std::cout << "[Callback] Key " << scancode << " pressed now\n";
    }
    else if (action == GLFW_RELEASE)
    {
        assert (g_keys[scancode].size () != 0);
        g_keys[scancode].back ().complete ();
        g_completed_events.insert (scancode);
        // const auto dd = g_keys[scancode].back ().time_held <std::chrono::milliseconds> ();
        // const auto ticks = g_keys[scancode].back ().ticks_held ();
        // std::cout << "[Callback] Key " << scancode << " released after " << dd.count () << " ms or " << ticks << " ticks.\n";
    }
}

static void mouse_callback (GLFWwindow *window, double xpos, double ypos)
{
    if (g_cursor_is_first_move)
    {
        g_cursor_last_x = xpos;
        g_cursor_last_y = ypos;
        g_cursor_is_first_move = GL_FALSE;
    }

    g_cursor_movement_x = xpos - g_cursor_last_x;
    g_cursor_movement_y = g_cursor_last_y - ypos;

    g_cursor_last_x = xpos;
    g_cursor_last_y = ypos;
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

const std::list <key_event> & get_events (const int key, int scancode)
{
    scancode = resolve_scancode (key, scancode);
    return g_keys[scancode];
}

const std::list <key_event> get_complete (const int key, int scancode)
{
    scancode = resolve_scancode (key, scancode);
    auto complete_events = g_keys[scancode];
    if (complete_events.size () > 0 && (! complete_events.back ().is_complete ()))
        complete_events.pop_back ();
    return complete_events;
}

const int key_down (const int key, int scancode)
{
    scancode = resolve_scancode (key, scancode);
    auto downs_in_cur_tick = g_keys[scancode].size ();

    if (downs_in_cur_tick > 0 && g_keys[scancode].front ().ticks_elapsed () > 0L)
        --downs_in_cur_tick;

    return static_cast <int> (downs_in_cur_tick);
}

const int key_up (const int key, int scancode)
{
    scancode = resolve_scancode (key, scancode);
    auto count = g_keys[scancode].size ();
    if (count > 0 && (! g_keys[scancode].back ().is_complete ()))
        --count;
    return static_cast <int> (count);
}

const long key_held (const int key, int scancode)
{
    scancode = resolve_scancode (key, scancode);
    return (g_keys[scancode].size () == 0 || g_keys[scancode].back ().is_complete ())
        ? 0L
        : g_keys[scancode].back ().ticks_elapsed ();
}

void prune_events ()
{
    for (int scancode : g_completed_events)
    {
        assert (g_keys[scancode].size () != 0);
        auto begin = g_keys[scancode].cbegin ();
        auto end = g_keys[scancode].cend ();
        if (! g_keys[scancode].back ().is_complete ())
            --end;
        if (begin != end)
            g_keys[scancode].erase (begin, end);
    }
    g_completed_events.clear ();
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CAMERA | EYEPOINT | LENS definitions
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
std::chrono::duration<float> eyepoint::elapsed {};
glm::vec3 eyepoint::prev_pos {};
glm::vec3 eyepoint::prev_dir {};
void eyepoint::cycle (const std::chrono::duration<float> dt)
{
    // elapsed += dt;
    // if (elapsed >= 1s)
    // {
    //     elapsed = 0s;
    //     // FOST_LOG_INFO ("Moved {} units in 1 second", glm::to_string (glm::abs (_position - prev_pos)));
    //     // FOST_LOG_INFO ("Pos {}", glm::to_string (_position));
    //     prev_pos = _position;
    // }
    // FOST_LOG_INFO ("Pos {}", glm::to_string (_position));

    // TODO: Get mouse input already!
    if (! _target)
    {
        prev_dir = _direction;
        // Get input from mouse for orientation.
        _yaw += maxis_horizontal () * _sensitivity_x;
        _pitch += maxis_vertical () * _sensitivity_y;

        if (_pitch > 89.9f)
            _pitch = 89.9f;
        if (_pitch < -89.9f)
            _pitch = -89.9f;

        if (_yaw > 180.0f)
            _yaw -= 360.0f;
        if (_yaw < -180.0f)
            _yaw += 360.0f;

        const auto pitch_in_radians = glm::radians(_pitch);
        const auto yaw_in_radians = glm::radians(_yaw);
        const auto cos_of_pitch = glm::cos(pitch_in_radians);
        _direction.x = glm::cos(yaw_in_radians) * cos_of_pitch;
        _direction.y = glm::sin(pitch_in_radians);
        _direction.z = glm::sin(yaw_in_radians) * cos_of_pitch;
        _direction = glm::normalize(_direction);
        // std::cout << "[Cycle] Position " << glm::to_string (_position) << '\n';
        // std::cout << "[Cycle] Direction: " << glm::to_string (_direction) << '\n';
        // std::cout << "[Cycle] Prev Direction: " << glm::to_string (prev_dir) << '\n';
    }
}

// TODO: restrict mouse movement when locked on. Require mouse_input for this.
void eyepoint::tick (const std::chrono::duration<float> dt)
{
    prev_pos = _position;

    // std::cout << "[tick] dt: " << (dt / 1s) << '\n';

    static constexpr GLfloat sneaking = {1.31f};
    static constexpr GLfloat walking = {4.317f};
    static constexpr GLfloat sprinting = {5.612f};

    GLfloat speed = walking;

    if (key_held (GLFW_KEY_LEFT_SHIFT))
        speed = sneaking;
    else if (key_held (GLFW_KEY_LEFT_CONTROL))
        speed = sprinting;

    if (key_down (GLFW_KEY_T))
    {
        if (_target)
        {
            lock_on (nullptr);
            std::cout << "[Tick] Stopped following.\n";
        }
        else
        {
            static constexpr glm::vec3 target = {0.0f, 0.0f, 0.0f};
            lock_on (&target);
            _direction = glm::normalize (*_target - _position);
            prev_dir = _direction; // in this case, prev is set after because whenever we have teleportations, we can't blend the vectors, it must be a complete jump.
            std::cout << "[Tick] Following origin.\n";
        }
    }

    const glm::vec3 right_vec = glm::normalize (glm::cross (_direction, world_up));
    _up = glm::normalize (glm::cross (right_vec, _direction));

    if (key_held (GLFW_KEY_PAGE_UP))
    {
        _position += world_up * speed * (dt / 1s);
    }
    if (key_held (GLFW_KEY_PAGE_DOWN))
    {
        _position -= world_up * speed * (dt / 1s);
    }
    if (key_held (GLFW_KEY_W))
    {
        _position += _direction * speed * (dt / 1s);
    }
    if (key_held (GLFW_KEY_S))
    {
        _position -= _direction * speed * (dt / 1s);
    }
    if (key_held (GLFW_KEY_A))
    {
        _position -= right_vec * speed * (dt / 1s);
    }
    if (key_held (GLFW_KEY_D))
    {
        _position += right_vec * speed * (dt / 1s);
    }

    // Known bug: because movement is not happening in a circle, each movement
    // will see the eyepoint moving further away from the target. This is too
    // insignificant to involve trigonometrics into this.
    if (_target)
    {
        prev_dir = _direction;
        _direction = glm::normalize (*_target - _position);
        _pitch = glm::asin (_direction.y);
        _yaw = glm::degrees (glm::asin (_direction.z / glm::cos (_pitch)));
        _pitch = glm::degrees (_pitch);
        if (_direction.x < 0 && _direction.z < 0)
        {
            _yaw = (-180.0f - _yaw);
        }
        else if (_direction.x < 0 && _direction.z >= 0)
        {
            _yaw = (180.0f - _yaw);
        }
    }

    // std::cout << "Position " << glm::to_string (_position) << '\n';
    // std::cout << "[Tick] Direction: " << glm::to_string (_direction) << '\n';
    // std::cout << "[Tick] Prev Direction: " << glm::to_string (prev_dir) << '\n';
    // std::cout << "Pitch: " << _pitch << '\n';
    // std::cout << "Yaw: " << _yaw << '\n';
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

void threadfunc (const std::string &name)
{
    std::cout << fost::get_thread_name () << '\n';
}

#endif

int main (int argc, char **argv)
{
    srand(time(nullptr));

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

    std::thread t1(threadfunc,"thread 1");
    std::thread t2(threadfunc,"thread 2");
    std::thread t3(threadfunc,"thread 3");

    t1.join();
    t2.join();
    t3.join();

    fost::set_thread_name ("Main thread");
    std::cout << fost::get_thread_name () << '\n';

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
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

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

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION ();
    ImGui::CreateContext ();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO (); (void) io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark ();
    //ImGui::StyleColorsClassic ();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL (window, true);
    ImGui_ImplOpenGL3_Init ("#version 330");

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault ();
    //io.Fonts->AddFontFromFileTTF ("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF ("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF ("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF ("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF ("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese ());
    //IM_ASSERT (font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;

    const glm::vec3 origin_vec3 {0.0f, 0.0f, 0.0f};

    // Setup cube geometry.
    const GLfloat vertices[24] = {
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, +0.5f,
        -0.5f, +0.5f, -0.5f,
        -0.5f, +0.5f, +0.5f,
        +0.5f, -0.5f, -0.5f,
        +0.5f, -0.5f, +0.5f,
        +0.5f, +0.5f, -0.5f,
        +0.5f, +0.5f, +0.5f
    };

    const GLuint indices[14] = {4, 6, 5, 7, 3, 6, 2, 4, 0, 5, 1, 3, 0, 2};

    // Setup quad geometry.
    const GLfloat quad_vertices[16] = {
        // positions, texture coords
        -0.5f, -0.5f, 0.0f, 0.0f,
        -0.5f, +0.5f, 0.0f, 1.0f,
        +0.5f, -0.5f, 1.0f, 0.0f,
        +0.5f, +0.5f, 1.0f, 1.0f
    };

    const GLuint quad_indices[4] = {0, 2, 1, 3};

    // Init stuff.
    // Cube
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

    // -------------------------------------------------------------------------

    // Quad
    GLuint quad_vao = 0U, quad_vbo = 0U, quad_ibo = 0U;
    glGenVertexArrays (1, &quad_vao);
    glGenBuffers (1, &quad_vbo);
    glGenBuffers (1, &quad_ibo);

    if (! quad_vao)
    {
        std::cerr << "error: could not generate vertex array\n";
        clean_glfw (window);
        return 1;
    }
    if (! quad_vbo)
    {
        std::cerr << "error: could not generate vertex buffer\n";
        clean_glfw (window);
        return 1;
    }
    if (! quad_ibo)
    {
        std::cerr << "error: could not generate index buffer \n";
        clean_glfw (window);
        return 1;
    }

    glBindVertexArray (quad_vao);
    glBindBuffer (GL_ARRAY_BUFFER, quad_vbo);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, quad_ibo);

    glBufferData (GL_ARRAY_BUFFER, sizeof (quad_vertices), quad_vertices, GL_STATIC_DRAW);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (quad_indices), quad_indices, GL_STATIC_DRAW);

    glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), BUFFER_OFFSET (0));
    glEnableVertexAttribArray (0);
    glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), BUFFER_OFFSET (2 * sizeof(GLfloat)));
    glEnableVertexAttribArray (1);

    glBindVertexArray (0);
    glDeleteBuffers (1, &quad_vbo);
    glDeleteBuffers (1, &quad_ibo);

    // -------------------------------------------------------------------------

    // XYZ axes
    // TODO: reduce number of vertices, optionally keep colors.
    // 1.0f * scale,         0.0f,         0.0f
    //         0.0f, 1.0f * scale,         0.0f
    //         0.0f,         0.0f, 1.0f * scale
    //
    // In shader, colors can be determined simply from which dimension != 0.0f.
    const GLfloat line_vertices[36] = {
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.025f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.025f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.025f, 0.0f, 0.0f, 1.0f,
    };

    // Init stuff.
    GLuint axes_vao = 0U, axes_vbo = 0U;
    glGenVertexArrays (1, &axes_vao);
    glGenBuffers (1, &axes_vbo);

    if (! axes_vao)
    {
        std::cerr << "error: could not generate vertex array\n";
        clean_glfw (window);
        return 1;
    }
    if (! axes_vbo)
    {
        std::cerr << "error: could not generate vertex buffer\n";
        clean_glfw (window);
        return 1;
    }

    glBindVertexArray (axes_vao);
    glBindBuffer (GL_ARRAY_BUFFER, axes_vbo);

    glBufferData (GL_ARRAY_BUFFER, sizeof (line_vertices), line_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, sizeof (GLfloat) * 6, BUFFER_OFFSET (0));
    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, sizeof (GLfloat) * 6, BUFFER_OFFSET (sizeof (GLfloat) * 3));
    glEnableVertexAttribArray (0);
    glEnableVertexAttribArray (1);

    glBindVertexArray (0);
    glDeleteBuffers (1, &axes_vbo);

    // -------------------------------------------------------------------------

    // Setup shaders.
    // Cubes
    GLuint prog = prepare_program ({
        { SHADER_PATH "default.vert", GL_VERTEX_SHADER },
        { SHADER_PATH "default.frag", GL_FRAGMENT_SHADER }
    });

    glUseProgram (prog);
    UNIFORM (prog, u_model);
    UNIFORM (prog, u_view);
    UNIFORM (prog, u_projection);
    UNIFORM (prog, u_some_color);
    UNIFORM (prog, u_camera_pos);
    glUseProgram (0);

    // World axes
    GLuint axes_prog = prepare_program ({
        { SHADER_PATH "axes.vert", GL_VERTEX_SHADER },
        { SHADER_PATH "axes.frag", GL_FRAGMENT_SHADER }
    });

    glUseProgram (axes_prog);
    UNIFORM (axes_prog, u_vp);
    glUseProgram (0);

    // Clouds
    GLuint cloud_prog = prepare_program ({
        { SHADER_PATH "cloud.vert", GL_VERTEX_SHADER },
        { SHADER_PATH "cloud.frag", GL_FRAGMENT_SHADER }
    });

    glUseProgram (cloud_prog);
    UNIFORM (cloud_prog, u_model);
    UNIFORM (cloud_prog, u_view);
    UNIFORM (cloud_prog, u_projection);
    UNIFORM (cloud_prog, u_resolution);
    UNIFORM (cloud_prog, u_camera);
    UNIFORM (cloud_prog, u_num_cells);
    UNIFORM (cloud_prog, u_threshold);
    glUseProgram (0);

    // Quads
    GLuint quad_prog = prepare_program ({
        { SHADER_PATH "quad.vert", GL_VERTEX_SHADER },
        { SHADER_PATH "quad.frag", GL_FRAGMENT_SHADER }
    });

    glUseProgram (quad_prog);
    UNIFORM (quad_prog, u_model);
    UNIFORM (quad_prog, u_view);
    UNIFORM (quad_prog, u_projection);
    UNIFORM (quad_prog, u_num_cells);
    UNIFORM (quad_prog, u_slice);
    glUseProgram (0);

    // Clouds
    GLuint volumetric_prog = prepare_program ({
        { SHADER_PATH "volumetric.vert", GL_VERTEX_SHADER },
        { SHADER_PATH "volumetric.frag", GL_FRAGMENT_SHADER }
    });

    glUseProgram (volumetric_prog);
    UNIFORM (volumetric_prog, u_model);
    UNIFORM (volumetric_prog, u_view);
    UNIFORM (volumetric_prog, u_projection);
    UNIFORM (volumetric_prog, u_resolution);
    UNIFORM (volumetric_prog, u_camera);
    glUseProgram (0);

    // Worley
    GLuint worley_comp = prepare_program ({
        { SHADER_PATH "worley.comp", GL_COMPUTE_SHADER },
    });

    //--------------------------------------------------------------------------

    // Setup cloud texture3D
    const GLint worley_res = 128;
    const std::size_t volume_size = worley_res * worley_res * worley_res;
    float *volume_data = new float[volume_size];

    for (int i = 0; i < volume_size; ++i)
        volume_data[i] = 0.0f;

    volume_data[volume_size / 2] = 1.0f;

    unsigned worley_tex;
    glGenTextures (1, &worley_tex);
    glBindTexture (GL_TEXTURE_3D, worley_tex);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    glTexImage3D (GL_TEXTURE_3D,
        0,
        GL_R32F,
        worley_res, worley_res, worley_res,
        0,
        GL_RED,
        GL_FLOAT,
        volume_data);
    glBindImageTexture (1, worley_tex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);
    glPixelStoref (GL_UNPACK_SWAP_BYTES, false);

    unsigned cloud_volume_tex;
    glGenTextures (1, &cloud_volume_tex);
    glBindTexture (GL_TEXTURE_3D, cloud_volume_tex);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    glTexImage3D (GL_TEXTURE_3D,
        0,
        GL_R32F,
        worley_res, worley_res, worley_res,
        0,
        GL_RED,
        GL_FLOAT,
        volume_data);
    glBindImageTexture (0, cloud_volume_tex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32F);
    glPixelStoref (GL_UNPACK_SWAP_BYTES, false);

    delete[] volume_data;

    std::size_t worley_numcells = 5;
    float worley_slice = 0.0f;
    float transmittance_threshold = 0.0f;
    float *worley_samples = generate_worley_cells_3d(worley_numcells);

    for (int i = 0; i < (worley_numcells * worley_numcells * worley_numcells * 3); ++i)
        std::cout << worley_samples[i] << '\n';

    unsigned test_tex;
    glGenTextures (1, &test_tex);
    glBindTexture (GL_TEXTURE_3D, test_tex);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    glTexImage3D (GL_TEXTURE_3D,
        0,
        GL_RGB32F,
        worley_numcells, worley_numcells, worley_numcells,
        0,
        GL_RGB,
        GL_FLOAT,
        worley_samples);
    // glBindImageTexture (0, test_tex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGB32F);
    // glPixelStoref (GL_UNPACK_SWAP_BYTES, false);

    delete[] worley_samples;

    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_3D, cloud_volume_tex);
    glActiveTexture (GL_TEXTURE1);
    glBindTexture (GL_TEXTURE_3D, worley_tex);
    glActiveTexture (GL_TEXTURE2);
    glBindTexture (GL_TEXTURE_3D, test_tex);

    glUseProgram (quad_prog);
    glUniform1i (u_num_cells_quad_prog, worley_numcells);
    glUseProgram (0);

    glUseProgram (cloud_prog);
    glUniform1i (u_num_cells_cloud_prog, worley_numcells);
    glUseProgram (0);


    // Generate worley noise for cloud
    glUseProgram (worley_comp);
    glDispatchCompute (
        worley_res / 8,
        worley_res / 8,
        worley_res / 8
    );

    // Wait for results
    glMemoryBarrier (GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Copy results to cloud volume texture
    glCopyImageSubData (
        worley_tex, GL_TEXTURE_3D, 0, 0, 0, 0,
        cloud_volume_tex, GL_TEXTURE_3D, 0, 0, 0, 0,
        worley_res, worley_res, worley_res
    );
    glUseProgram(0);
    glDeleteProgram(worley_comp);

    // Setup camera.
    const GLfloat speed = 2.0f;
    const GLfloat radius = 0.5f;
    GLfloat time = 0.0f;

    // TODO:
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_BLEND);

    g_projection = glm::perspective (glm::radians (g_lens._FOV),
                                     (float) width / (float) height,
                                     g_lens._near,
                                     g_lens._far);

    FOST_LOG_INFO ("Tickrate: {} mspt | {} tps", fost::runtime::mspt.count (), fost::runtime::tps);

    fost::runtime::duration accumulator {0s};
    fost::runtime::time_point t {};

    int frame_count = 0;

    // TODO: Figure out game loop.
    glfwSwapInterval (g_vsync);
    // Loop until the user closes the window
    while (! glfwWindowShouldClose (window))
    {
        ++frame_count;
        // std::cout << "[Frame #" << frame_count << "] Start\n";
        // Poll Inputs.
        glfwPollEvents ();

        // FOST_LOG_INFO ("Frame debug: {}ms dt | {} fps", fost::runtime::frametime ().count (), fost::runtime::fps ());
        // IMPORTANT! Must cycle runtime to advance simulation (calculates delta time).
        fost::runtime::cycle ();
        auto delta_time = fost::runtime::frame_time ();
        // std::cout << "[Frame #" << frame_count << "] dt " << std::chrono::duration <float> (delta_time).count () << '\n';

        if (delta_time > 250ms)
            delta_time = 250ms;

        accumulator += delta_time;

        g_lens.cycle (fost::runtime::frame_time ()); // TODO: ?

        // Update
        while (accumulator >= fost::runtime::tick_unit)
        {
            // std::cout << "[Frame #" << frame_count << "] TICK NOW!\n";
            // Window key handling.
            if (key_down (GLFW_KEY_F1))
            {
                g_draw_hud = ! g_draw_hud;
                std::cout << "Draw hud " << static_cast <int> (g_draw_hud) << '\n';
            }
            if (g_draw_hud && key_down (GLFW_KEY_F3))
            {
                g_draw_debug_hud = ! g_draw_debug_hud;
                std::cout << "Draw debug hud " << static_cast <int> (g_draw_debug_hud) << '\n';
            }

            if (const auto amount = key_held (GLFW_KEY_H))
            {
                std::cout << "Held H for " << amount << '\n';
            }

            if (key_held (GLFW_KEY_PERIOD))
            {
                worley_slice += 0.005f;
                if (worley_slice > 1.0f)
                    worley_slice = 1.0f;
                std::cout << "worley_slice = " << worley_slice << '\n';
            }
            if (key_held (GLFW_KEY_COMMA))
            {
                worley_slice -= 0.005f;
                if (worley_slice < 0.0f)
                    worley_slice = 0.0f;
                std::cout << "worley_slice = " << worley_slice << '\n';
            }

            if (key_held (GLFW_KEY_RIGHT_BRACKET))
            {
                transmittance_threshold += 0.05f;
                std::cout << "transmittance_threshold = " << transmittance_threshold << '\n';
            }
            if (key_held (GLFW_KEY_LEFT_BRACKET))
            {
                transmittance_threshold -= 0.05f;
                if (transmittance_threshold < 0.0f)
                    transmittance_threshold = 0.0f;
                std::cout << "transmittance_threshold = " << transmittance_threshold << '\n';
            }

            g_lens.tick (fost::runtime::tick_unit);

            prune_events ();
            t += fost::runtime::tick_unit;
            accumulator -= fost::runtime::tick_unit;
        }
        cycle_mouse_to_be_renamed ();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame ();
        ImGui_ImplGlfw_NewFrame ();
        ImGui::NewFrame ();


        // Finish the Dear ImGui frame
        ImGui::Render ();

        // Rendering
        const double alpha = std::chrono::duration <double> {accumulator} / fost::runtime::tick_unit;
        const glm::vec3 final_pos = glm::mix (g_lens.prev_pos, g_lens.get_position (), alpha);
        const glm::vec3 final_dir = glm::mix (g_lens.prev_dir, g_lens.get_direction (), alpha);
        glm::mat4 view = glm::lookAt (final_pos, final_pos + final_dir, g_lens.get_upvector ());

        static const GLfloat background_color[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        static const GLfloat sky_color[] = { 0.608f, 0.671f, 0.733f };

        glEnable (GL_DEPTH_TEST);
        glDepthFunc (GL_LESS);
        glClear (GL_DEPTH_BUFFER_BIT);
        glClearBufferfv (GL_COLOR, 0, background_color);

        // Rotate camera around radius.
        // time = static_cast<GLfloat> (glfwGetTime () * speed);
        // g_lens.position = glm::vec3 (glm::sin (time) * radius,
        //                              0.0f,
        //                              glm::cos (time) * radius);
        // glm::mat4 view = glm::lookAt (g_lens.position, g_lens.direction, g_lens.up);


#define EXPERIMENT 2

#if EXPERIMENT == 0
        glBindVertexArray (vao);
        glUseProgram (prog);
        glUniformMatrix4fv (u_view_prog, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv (u_projection_prog, 1, GL_FALSE, &g_projection[0][0]);
        glUniform3fv (u_camera_pos_prog, 1, glm::value_ptr (g_lens.get_position ()));
        // Draw white cube in the center
        {
            glm::mat4 model {1.0f};

            glUniformMatrix4fv (u_model_prog, 1, GL_FALSE, &model[0][0]);
            glUniform4fv (u_some_color_prog, 1, glm::value_ptr (glm::vec4 {1.0f, 1.0f, 1.0f, 1.0f}));

            glDrawElements (GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, nullptr);
        }
        // Draw red cube one block to the right.
        {
            glm::mat4 model {1.0f};
            model = glm::translate (model, {1.0f, 0.0f, 0.0f});

            glUniformMatrix4fv (u_model_prog, 1, GL_FALSE, &model[0][0]);
            glUniform4f (u_some_color_prog, 1.0f, 0.0f, 0.0f, 1.0f);

            glDrawElements (GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, nullptr);
        }
        // Draw some other cubes in a line with different colors
        for (int i = 2; i < 20; ++i)
        {
            glm::mat4 model {1.0f};
            model = glm::translate (model, {1.0f * i, 0.0f, 0.0f});

            glUniformMatrix4fv (u_model_prog, 1, GL_FALSE, &model[0][0]);
            glUniform4f (u_some_color_prog, 1.0f, 0.0f, 0.1f * i, 1.0f);

            glDrawElements (GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, nullptr);
        }
        glUseProgram (0);
        glBindVertexArray (0);
#elif EXPERIMENT == 1
        glBindVertexArray (vao);
        glUseProgram (prog);
        glUniformMatrix4fv (u_view_prog, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv (u_projection_prog, 1, GL_FALSE, &g_projection[0][0]);
        glUniform3fv (u_camera_pos_prog, 1, glm::value_ptr (g_lens.get_position ()));
        glClearBufferfv (GL_COLOR, 0, sky_color);
        // Draw "terrain"
        for (int i = -50; i < 50; ++i)
            for (int j = -50; j < 50; ++j)
            {
                glm::mat4 model {1.0f};
                model = glm::translate (model, {1.0f * i, -1.0f, 1.0f * j});

                glUniformMatrix4fv (u_model_prog, 1, GL_FALSE, &model[0][0]);
                glUniform4f (u_some_color_prog, 0.1f, 0.5f, 0.1f, 1.0f);

                glDrawElements (GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, nullptr);
            }
        glUseProgram (0);
        glBindVertexArray (0);
#elif EXPERIMENT == 2
        glDisable (GL_DEPTH_TEST);
        glBindVertexArray (vao);
        glUseProgram (cloud_prog);
        glUniformMatrix4fv (u_view_cloud_prog, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv (u_projection_cloud_prog, 1, GL_FALSE, &g_projection[0][0]);
        glUniform3fv (u_camera_cloud_prog, 1, glm::value_ptr (g_lens.get_position ()));
        glUniform1f (u_threshold_cloud_prog, transmittance_threshold);
        int w = -1, h = -1;
        glfwGetFramebufferSize (window, &w, &h);
        glUniform2f (u_resolution_cloud_prog, w, h);

        // Draw weird green cloud-like cube material (but its not a cloud)
        {
            glm::mat4 model {1.0f};
            glUniformMatrix4fv (u_model_cloud_prog, 1, GL_FALSE, &model[0][0]);

            glDrawElements (GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, nullptr);
        }
        glUseProgram (0);
        glBindVertexArray (0);
        glEnable (GL_DEPTH_TEST);
#elif EXPERIMENT == 3
        glDisable (GL_DEPTH_TEST);
        glBindVertexArray (vao);
        glUseProgram (volumetric_prog);
        glUniformMatrix4fv (u_view_volumetric_prog, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv (u_projection_volumetric_prog, 1, GL_FALSE, &g_projection[0][0]);
        glUniform3fv (u_camera_volumetric_prog, 1, glm::value_ptr (g_lens.get_position ()));
        int w = -1, h = -1;
        glfwGetFramebufferSize (window, &w, &h);
        glUniform2f (u_resolution_volumetric_prog, w, h);

        // Draw weird green cloud-like cube material (but its not a cloud)
        {
            glm::mat4 model {1.0f};
            glUniformMatrix4fv (u_model_volumetric_prog, 1, GL_FALSE, &model[0][0]);

            glDrawElements (GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, nullptr);
        }
        glUseProgram (0);
        glBindVertexArray (0);
        glEnable (GL_DEPTH_TEST);
#elif EXPERIMENT == 4
        glBindVertexArray (quad_vao);
        glUseProgram (quad_prog);
        glUniformMatrix4fv (u_view_quad_prog, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv (u_projection_quad_prog, 1, GL_FALSE, &g_projection[0][0]);
        // Draw white quad in the center
        {
            glm::mat4 model {1.0f};

            glUniformMatrix4fv (u_model_quad_prog, 1, GL_FALSE, &model[0][0]);
            glUniform1f (u_slice_quad_prog, worley_slice);

            glDrawElements (GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, nullptr);
        }
        glUseProgram (0);
        glBindVertexArray (0);
#endif

        // TODO: HUD Drawing last.
        if (g_draw_hud)
        {
            if (g_draw_debug_hud)
            {
                // Draw debug crosshair
                glDisable (GL_DEPTH_TEST);
                glBindVertexArray (axes_vao);
                glUseProgram (axes_prog);

                view = glm::lookAt (-g_lens.get_direction (), origin_vec3, g_lens.get_upvector ());
                glUniformMatrix4fv (u_vp_axes_prog, 1, GL_FALSE, &(g_projection * view)[0][0]);

                glDrawArrays (GL_LINES, 0, 6);

                glUseProgram (0);
                glBindVertexArray (0);
                glEnable (GL_DEPTH_TEST);

                // Draw debug stats
            }
            else
            {
                // Draw normal crosshair
            }

            // Draw rest of hud
        }


        ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());

        glfwSwapBuffers (window);
        // std::cout << "[Frame #" << frame_count << "] End\n";
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown ();
    ImGui_ImplGlfw_Shutdown ();
    ImPlot::DestroyContext ();
    ImGui::DestroyContext ();

    glDeleteProgram (prog);
    glDeleteProgram (cloud_prog);
    glDeleteVertexArrays (1, &vao);
    glDeleteVertexArrays (1, &axes_vao);

    clean_glfw (window);
    return 0;
}
