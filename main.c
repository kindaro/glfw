#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

struct point {int x; int y;};
struct devices
{
     GLFWwindow * window;
     VkInstance vulkan;
     VkSurfaceKHR surface;
     VkPhysicalDevice card;
     VkDevice logic;
     VkQueue queue;
};

void try (int code, const char * location)
{
     if (code)
     {
          fprintf (stderr, "Error %d in %s!\n", code, location);
          exit (-1);
     }
}

void checkGlfwError (const char * location)
{
     const char * message;
     const int code = glfwGetError (&message);
     if (code) printf ("GLFW error %X, %s in %s.\n", code, message, location);
}

VkPhysicalDevice getSomePhysicalDevice (VkInstance vulkan)
{
     unsigned int numberOfRequiredDevices;
     try (vkEnumeratePhysicalDevices (vulkan, &numberOfRequiredDevices, NULL), "Vulkan physical device count");
     try (numberOfRequiredDevices == 0, "No devices at all");
     VkPhysicalDevice cards [numberOfRequiredDevices];
     try (vkEnumeratePhysicalDevices (vulkan, &numberOfRequiredDevices, cards), "Vulkan physical device acquisition");
     return cards[0];
}

void getLogicAndQueue (VkPhysicalDevice card, VkDevice logic, VkQueue queue)
{
     unsigned int numberOfAvailableQueueFamilies;
     vkGetPhysicalDeviceQueueFamilyProperties (card, &numberOfAvailableQueueFamilies, NULL);
     VkQueueFamilyProperties queueFamilies [numberOfAvailableQueueFamilies];
     vkGetPhysicalDeviceQueueFamilyProperties (card, &numberOfAvailableQueueFamilies, queueFamilies);
     int queueFamilyIndex = -1;
     for (unsigned int i = numberOfAvailableQueueFamilies - 1; i >= 0; i--)
          if (queueFamilies [i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
               {queueFamilyIndex = i; break;}
     try (queueFamilyIndex == -1, "search for a queue");
     float priority = 1;
     const VkDeviceQueueCreateInfo queueCreateInfo =
          {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .queueFamilyIndex = queueFamilyIndex,
           .queueCount = 1,
           .pQueuePriorities = &priority
          };
     const VkDeviceCreateInfo deviceCreateInfo =
          {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .queueCreateInfoCount = 1,
           .pQueueCreateInfos = &queueCreateInfo,
           .enabledLayerCount = 0,
           .ppEnabledLayerNames = NULL,
           .enabledExtensionCount = 0,
           .ppEnabledExtensionNames = NULL,
           .pEnabledFeatures = NULL
          };
     try (vkCreateDevice (card, &deviceCreateInfo, NULL, &logic), "Vulkan logical device initialization");
     vkGetDeviceQueue (logic, queueFamilyIndex, 0, &queue);
}

const struct devices enter (const struct point size)
{
     struct devices devices = { };
     if (! glfwInit ( )) exit (-1);
     glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
     glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);
     devices.window = glfwCreateWindow (size.x, size.y, "Hello World", NULL, NULL);
     if (! devices.window)
     {
          glfwTerminate ( );
          exit (-1);
     }
     glfwMakeContextCurrent (devices.window);

     unsigned int enabledExtensionCount;
     const char ** enabledExtensionNames = glfwGetRequiredInstanceExtensions (&enabledExtensionCount);
     const VkInstanceCreateInfo info =
          {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .pApplicationInfo = NULL,
           .enabledLayerCount = 0,
           .ppEnabledLayerNames = NULL,
           .enabledExtensionCount = enabledExtensionCount,
           .ppEnabledExtensionNames = enabledExtensionNames,
          };
     checkGlfwError ("Query Vulkan extensions");
     for (int i = info.enabledExtensionCount - 1; i >= 0; i--) printf ("Required extension: %s.\n", info.ppEnabledExtensionNames [i]);
     try (vkCreateInstance(&info, NULL, &devices.vulkan), "Vulkan initialization");
     try (glfwCreateWindowSurface (devices.vulkan, devices.window, NULL, &devices.surface), "Vulkan surface initialization");
     devices.card = getSomePhysicalDevice (devices.vulkan);
     getLogicAndQueue (devices.card, devices.logic, devices.queue);
     return devices;
}

int leave (const struct devices devices)
{
     vkDestroyDevice (devices.logic, NULL);
     vkDestroyInstance (devices.vulkan, NULL);
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
