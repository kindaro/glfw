#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

struct point {int x; int y;};
struct devices {GLFWwindow * window; VkInstance vulkan;};

void try (int code, const char * location)
{
     if (code)
     {
          fprintf (stderr, "Error %d in %s!\n", code, location);
          exit (-1);
     }
}

const struct devices enter (const struct point size)
{
     struct devices devices;
     if (! glfwInit ( )) exit (-1);
     /* glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API); */
     glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);
     devices.window = glfwCreateWindow (size.x, size.y, "Hello World", NULL, NULL);
     if (! devices.window)
     {
          glfwTerminate ( );
          exit (-1);
     }
     glfwMakeContextCurrent (devices.window);

     VkInstanceCreateInfo info =
          {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .pApplicationInfo = NULL,
           .enabledLayerCount = 0,
           .ppEnabledLayerNames = NULL,
           .enabledExtensionCount = 0,
           .ppEnabledExtensionNames = NULL,
          };
     info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions (&info.enabledExtensionCount);
     try (vkCreateInstance(&info, NULL, &devices.vulkan), "Vulkan initialization");

     return devices;
}

int leave (const struct devices devices)
{
     glfwDestroyWindow (devices.window);
     glfwTerminate ( );
     return 0;
}

void mainLoop (const struct devices devices)
{
     glfwSwapBuffers (devices.window);
     glfwPollEvents ( );
}

int main (void)
{
     const struct point size = {.x = 800, .y = 600};
     const struct devices devices = enter (size);
     while (! glfwWindowShouldClose (devices.window)) mainLoop (devices);
     return leave (devices);
}
