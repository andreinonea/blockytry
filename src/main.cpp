#include <version.hpp>

#include <iostream>

#define GLAD_GL_IMPLEMENTATION
#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Shader stuff
#ifndef DEFAULT_SHADER_PATH
    #define SHADER_PATH ""
#else
    #define SHADER_PATH DEFAULT_SHADER_PATH
#endif


/* Callbacks. */
static void
glfw_error_callback (int error, const char *desc)
{
    std::cerr << "Error [" << error << "]: " << desc << '\n';
}

static void
key_callback (GLFWwindow *window,
              int key, int scancode, int action, int mods)

{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose (window, GLFW_TRUE);
    }
}

int
main (int argc, char **argv)
{
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

    // Create a windowed mode window and make its context current
    GLFWwindow *window = glfwCreateWindow (640, 480,
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
        glfwDestroyWindow (window);
        glfwTerminate ();
        return 1;
    }

    // Let OpenGL know we want to use the programmable pipeline.
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::cout << "OpenGL " << GLAD_VERSION_MAJOR (glad_version) << "."
              << GLAD_VERSION_MINOR (glad_version) << " Core profile\n";
    std::cout << "GLAD loader " << GLAD_GENERATOR_VERSION << '\n';

    // Print device info and stuff
    std::cout << "Driver vendor " << glGetString (GL_VENDOR) << '\n';
    std::cout << "Driver version " << glGetString (GL_VERSION) << '\n';
    std::cout << "Device vendor " << glGetString (GL_RENDERER) << '\n';

    // Set input callbacks.
    glfwSetKeyCallback (window, key_callback);

    // Loop until the user closes the window
    while (! glfwWindowShouldClose (window))
    {
        glClear (GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers (window);
        glfwPollEvents ();
    }

    glfwDestroyWindow (window);
    glfwTerminate ();
    return 0;
}