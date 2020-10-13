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
     unsigned int queueFamilyIndex;
     VkSwapchainKHR chain;
};

void try (int const code, char const * const pointerToLocation)
{
     if (code)
     {
          fprintf (stderr, "Error %d in %s!\n", code, pointerToLocation);
          exit (-1);
     }
}

void checkGlfwError (char const * const pointerToLocation)
{
     const char * message;
     const int code = glfwGetError (&message);
     if (code) printf ("GLFW error %X, %s in %s.\n", code, message, pointerToLocation);
}

VkPhysicalDevice getSomePhysicalDevice (VkInstance const vulkan)
{
     unsigned int numberOfRequiredDevices;
     try (vkEnumeratePhysicalDevices (vulkan, &numberOfRequiredDevices, NULL), "Vulkan physical device count");
     try (numberOfRequiredDevices == 0, "No devices at all");
     VkPhysicalDevice cards [numberOfRequiredDevices];
     try (vkEnumeratePhysicalDevices (vulkan, &numberOfRequiredDevices, cards), "Vulkan physical device acquisition");
     return cards[0];
}

void getLogicAndQueue (VkPhysicalDevice const card, VkSurfaceKHR const surface, VkDevice * const pointerToLogic, VkQueue * const pointerToQueue, unsigned int * const pointerToQueueFamilyIndex)
{
     unsigned int numberOfAvailableQueueFamilies;
     vkGetPhysicalDeviceQueueFamilyProperties (card, &numberOfAvailableQueueFamilies, NULL);
     VkQueueFamilyProperties queueFamilies [numberOfAvailableQueueFamilies];
     vkGetPhysicalDeviceQueueFamilyProperties (card, &numberOfAvailableQueueFamilies, queueFamilies);
     {
          * pointerToQueueFamilyIndex = -1;
          for (unsigned int i = numberOfAvailableQueueFamilies - 1; i >= 0; i--)
               if (queueFamilies [i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
               {* pointerToQueueFamilyIndex = i; break;}
          try (* pointerToQueueFamilyIndex == -1, "search for a queue");
     }
     {
          float priority = 1;
          const VkDeviceQueueCreateInfo queueCreateInfo =
               {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .queueFamilyIndex = * pointerToQueueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &priority
               };
          char const * const extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
          const VkDeviceCreateInfo deviceCreateInfo =
               {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &queueCreateInfo,
                .enabledLayerCount = 0,
                .ppEnabledLayerNames = NULL,
                .enabledExtensionCount = 1,
                .ppEnabledExtensionNames = extensions,
                .pEnabledFeatures = NULL
               };
          try (vkCreateDevice (card, &deviceCreateInfo, NULL, pointerToLogic), "Vulkan logical device initialization");
     }
     vkGetDeviceQueue (* pointerToLogic, * pointerToQueueFamilyIndex, 0, pointerToQueue);
     {
          VkBool32 isSurfaceSupported;
          try (vkGetPhysicalDeviceSurfaceSupportKHR (card, * pointerToQueueFamilyIndex, surface, &isSurfaceSupported), "Vulkan surface support check");
          try (isSurfaceSupported == VK_FALSE, "Vulkan surface not supported by queue family");
     }
}

VkSwapchainKHR getSwapchain (VkPhysicalDevice const card, VkDevice const logic, VkSurfaceKHR const surface, struct point const size)
{
     VkSurfaceCapabilitiesKHR capabilities;
     try (vkGetPhysicalDeviceSurfaceCapabilitiesKHR (card, surface, &capabilities), "Vulkan surface capabilities query");
     unsigned int numberOfFormats;
     try (vkGetPhysicalDeviceSurfaceFormatsKHR (card, surface, &numberOfFormats, NULL), "Vulkan surface formats number query");
     VkSurfaceFormatKHR formats [numberOfFormats];
     try (vkGetPhysicalDeviceSurfaceFormatsKHR (card, surface, &numberOfFormats, formats), "Vulkan surface formats query");
     VkSurfaceFormatKHR format = formats [1];
     unsigned int numberOfPresentationModes;
     try (vkGetPhysicalDeviceSurfacePresentModesKHR (card, surface, &numberOfPresentationModes, NULL), "Vulkan surface presentationModes number query");
     VkPresentModeKHR presentationModes [numberOfPresentationModes];
     try (vkGetPhysicalDeviceSurfacePresentModesKHR (card, surface, &numberOfPresentationModes, presentationModes), "Vulkan surface presentationModes query");
     VkPresentModeKHR presentationMode = presentationModes [0];
     printf ("Smallest extent: %x, %x.\n", capabilities.minImageExtent.width, capabilities.minImageExtent.height);
     printf ("Largest extent: %x, %x.\n", capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);
     VkSwapchainKHR chain;
     {
          const VkSwapchainCreateInfoKHR info =
               {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = NULL,
                .flags = 0,
                .surface = surface,
                .minImageCount = capabilities.minImageCount + 1,
                .imageFormat = format.format,
                .imageColorSpace = format.colorSpace,
                .imageExtent = capabilities.minImageExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = NULL,
                .preTransform = capabilities.currentTransform,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = presentationMode,
                .clipped = VK_TRUE,
                .oldSwapchain = VK_NULL_HANDLE,
               };
          try (vkCreateSwapchainKHR (logic, &info, NULL, &chain), "Vulkan swapchain acquisition");
     }
     return chain;
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

     {
          unsigned int enabledExtensionCount;
          char const * const * enabledExtensionNames = glfwGetRequiredInstanceExtensions (&enabledExtensionCount);
          char const * const layers [1] = {"VK_LAYER_KHRONOS_validation"};
          const VkInstanceCreateInfo info =
               {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .pApplicationInfo = NULL,
                .enabledLayerCount = 1,
                .ppEnabledLayerNames = layers,
                .enabledExtensionCount = enabledExtensionCount,
                .ppEnabledExtensionNames = enabledExtensionNames,
               };
          checkGlfwError ("Query Vulkan extensions");
          for (int i = info.enabledExtensionCount - 1; i >= 0; i--) printf ("Required extension: %s.\n", info.ppEnabledExtensionNames [i]);
          try (vkCreateInstance(&info, NULL, &devices.vulkan), "Vulkan initialization");
     }
     try (glfwCreateWindowSurface (devices.vulkan, devices.window, NULL, &devices.surface), "Vulkan surface initialization");
     devices.card = getSomePhysicalDevice (devices.vulkan);
     getLogicAndQueue (devices.card, devices.surface, &devices.logic, &devices.queue, &devices.queueFamilyIndex);
     devices.chain = getSwapchain (devices.card, devices.logic, devices.surface, size);
     return devices;
}

int leave (const struct devices devices)
{
     vkDestroySwapchainKHR (devices.logic, devices.chain, NULL);
     vkDestroySurfaceKHR (devices.vulkan, devices.surface, NULL);
     try (vkDeviceWaitIdle (devices.logic), "waiting for device to finish work");
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
