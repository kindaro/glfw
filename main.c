#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

struct point {int x; int y;};

GLFWwindow * initializeWindow (const struct point size)
{
     GLFWwindow * window;
     if (! glfwInit ( )) exit (-1);
     window = glfwCreateWindow (size.x, size.y, "Hello World", NULL, NULL);
     if (! window)
     {
          glfwTerminate ( );
          exit (-1);
     }
     glfwMakeContextCurrent (window);
     return window;
}

int leave (GLFWwindow * window)
{
     glfwDestroyWindow (window);
     glfwTerminate ( );
     return 0;
}

void mainLoop (GLFWwindow * window)
{
     glfwSwapBuffers (window);
     glfwPollEvents ( );
}

int main (void)
{
     const struct point size = {.x = 800, .y = 600};
     GLFWwindow * window;
     window = initializeWindow (size);
     while (! glfwWindowShouldClose (window)) mainLoop (window);
     return leave (window);
}
