#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

GLFWwindow * initializeWindow ( )
{
     GLFWwindow * window;
     if (! glfwInit ( )) exit (-1);
     window = glfwCreateWindow (640, 480, "Hello World", NULL, NULL);
     if (! window)
     {
          glfwTerminate ( );
          exit (-1);
     }
     glfwMakeContextCurrent (window);
     return window;
}

int leave ( )
{
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
     GLFWwindow * window;
     window = initializeWindow ( );
     while (! glfwWindowShouldClose (window)) mainLoop (window);
     return leave ( );
}
