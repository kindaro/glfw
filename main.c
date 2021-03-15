#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

struct point {int x; int y;};
struct images
{
     unsigned int const count;
     VkImage * const images;
     VkImageView * const views;
     VkFramebuffer * const frames;
};
struct devices
{
     GLFWwindow * window;
     VkInstance vulkan;
     VkSurfaceKHR surface;
     VkPhysicalDevice card;
     VkDevice logic;
     VkQueue queue;
     unsigned int queueFamilyIndex;
     VkSurfaceFormatKHR format;
     VkSwapchainKHR chain;
     VkCommandPool pool;
     VkCommandBuffer buffer;
     VkRenderPass renderPass;
     struct images * pointerToImages;
};

void try (int const code, char const * const pointerToLocation)
{
     if (code)
     {
          fprintf (stderr, "Error %d in %s!\n", code, pointerToLocation);
          exit (-1);
     }
}

void * xcalloc (unsigned int count, unsigned int size)
{
     void * pointerToMemory = calloc (count, size);
     if (! pointerToMemory)
     {
          perror ("xcalloc");
          exit (EXIT_FAILURE);
     }
     return pointerToMemory;
}

void * xmalloc0 (unsigned int size)
{
     return xcalloc (1, size);
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
     VkPhysicalDevice * pointerToCards = xcalloc (numberOfRequiredDevices, sizeof (VkPhysicalDevice));
     try (vkEnumeratePhysicalDevices (vulkan, &numberOfRequiredDevices, pointerToCards), "Vulkan physical device acquisition");
     VkPhysicalDevice card = pointerToCards [0];
     free (pointerToCards);
     return card;
}

void getLogicAndQueue (VkPhysicalDevice const card, VkSurfaceKHR const surface, VkDevice * const pointerToLogic, VkQueue * const pointerToQueue, unsigned int * const pointerToQueueFamilyIndex)
{
     unsigned int numberOfAvailableQueueFamilies;
     vkGetPhysicalDeviceQueueFamilyProperties (card, &numberOfAvailableQueueFamilies, NULL);
     {
          VkQueueFamilyProperties * pointerToQueueFamilies = xcalloc (numberOfAvailableQueueFamilies, sizeof (VkQueueFamilyProperties));
          vkGetPhysicalDeviceQueueFamilyProperties (card, &numberOfAvailableQueueFamilies, pointerToQueueFamilies);
          {
               *pointerToQueueFamilyIndex = ~(unsigned int)0;
               for (unsigned int i = 0; i < numberOfAvailableQueueFamilies; ++i)
                    if (pointerToQueueFamilies [i].queueCount > 0 && pointerToQueueFamilies [i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    {
                         *pointerToQueueFamilyIndex = i;
                         break;
                    }
               try (*pointerToQueueFamilyIndex == ~(unsigned int)0, "search for a queue");
          }
          free (pointerToQueueFamilies);
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

VkSurfaceFormatKHR getFormat (VkPhysicalDevice const card, VkSurfaceKHR const surface)
{
     unsigned int numberOfFormats;
     try (vkGetPhysicalDeviceSurfaceFormatsKHR (card, surface, &numberOfFormats, NULL), "Vulkan surface formats number query");
     VkSurfaceFormatKHR * pointerToFormats = xcalloc (numberOfFormats, sizeof (VkSurfaceFormatKHR));
     try (vkGetPhysicalDeviceSurfaceFormatsKHR (card, surface, &numberOfFormats, pointerToFormats), "Vulkan surface formats query");
     VkSurfaceFormatKHR format = pointerToFormats [1];
     free (pointerToFormats);
     return format;
}

VkSwapchainKHR getSwapchain (VkPhysicalDevice const card, VkDevice const logic, VkSurfaceKHR const surface, VkSurfaceFormatKHR const format, struct point const size)
{
     VkSurfaceCapabilitiesKHR capabilities;
     try (vkGetPhysicalDeviceSurfaceCapabilitiesKHR (card, surface, &capabilities), "Vulkan surface capabilities query");
     unsigned int numberOfPresentationModes;
     try (vkGetPhysicalDeviceSurfacePresentModesKHR (card, surface, &numberOfPresentationModes, NULL), "Vulkan surface presentationModes number query");
     VkPresentModeKHR * pointerToPresentationModes = xcalloc (numberOfPresentationModes, sizeof (VkPresentModeKHR));
     try (vkGetPhysicalDeviceSurfacePresentModesKHR (card, surface, &numberOfPresentationModes, pointerToPresentationModes), "Vulkan surface presentationModes query");
     VkPresentModeKHR presentationMode = pointerToPresentationModes [0];
     free (pointerToPresentationModes);
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
                .imageExtent = {.width = size.x, .height = size.y},
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

struct images * getPointerToImages (VkDevice const logic, VkSwapchainKHR const swapchain, VkFormat const format, VkRenderPass const renderPass, struct point const size)
{
     unsigned int count;
     try (vkGetSwapchainImagesKHR (logic, swapchain, &count, NULL), "Get image count");
     VkImage * images = xcalloc (count, sizeof (VkImage));
     try (vkGetSwapchainImagesKHR (logic, swapchain, &count, images), "Get images");
     VkImageView * views = xcalloc (count, sizeof (VkImageView));
     VkFramebuffer * frames = xcalloc (count, sizeof (VkFramebuffer));
     for (unsigned int i = 0; i < count; ++i)
     {
          printf ("Initializing image %d.\n", i);
          {
               VkComponentMapping components =
                    {.r = VK_COMPONENT_SWIZZLE_R,
                     .g = VK_COMPONENT_SWIZZLE_G,
                     .b = VK_COMPONENT_SWIZZLE_B,
                     .a = VK_COMPONENT_SWIZZLE_A,
                    };
               VkImageSubresourceRange subresourceRange =
                    {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                     .baseMipLevel = 0,
                     .levelCount = 1,
                     .baseArrayLayer = 0,
                     .layerCount = 1
                    };
               VkImageViewCreateInfo info =
                    {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                     .pNext = NULL,
                     .flags = 0,
                     .image = images [i],
                     .viewType = VK_IMAGE_VIEW_TYPE_2D,
                     .format = format,
                     .components = components,
                     .subresourceRange = subresourceRange,
                    };
               try (vkCreateImageView (logic, & info, NULL, & views [i]), "Creating image view");
               printf ("Initializing image view %d at %p.\n", i, & images [i]);
          }
          {
               VkFramebufferCreateInfo info =
                    {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                     .pNext = NULL,
                     .flags = 0,
                     .renderPass = renderPass,
                     .attachmentCount = 1,
                     .pAttachments = & views [i],
                     .width = size.x,
                     .height = size.y,
                     .layers = 1,
                    };
               try (vkCreateFramebuffer (logic, & info, NULL, & frames [i]), "Creating framebuffer");
               printf ("Initializing framebuffer %d at %p.\n", i, & frames [i]);
          }
     }
     struct images * pointerToMemory = xmalloc0 (sizeof (struct images));
     struct images temporaryStructure = {.count = count, .views = views, .images = images, .frames = frames};
     memcpy (pointerToMemory, & temporaryStructure, sizeof (temporaryStructure));
     printf ("pointer to views: %p.", pointerToMemory->views);
     return pointerToMemory;
}

VkCommandPool getPool (VkDevice const logic, unsigned int const queueFamilyIndex)
{
     VkCommandPool pool;
     {
          VkCommandPoolCreateInfo const info =
               {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = NULL,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = queueFamilyIndex,
               };
          try (vkCreateCommandPool (logic, &info, NULL, &pool), "Vulkan command pool acquisition");
     }
     return pool;
}

VkCommandBuffer getBuffer (VkDevice const logic, VkCommandPool const pool)
{
     VkCommandBuffer buffer;
     {
          VkCommandBufferAllocateInfo info =
               {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = NULL,
                .commandPool = pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
               };
               try (vkAllocateCommandBuffers (logic, &info, &buffer), "Vulkan command buffer allocation");
     }
     return buffer;
}

VkRenderPass getRenderPass (VkDevice const logic, VkFormat const format)
{
     VkRenderPass renderPass;
     {
          VkAttachmentDescription attachment =
               {.flags = 0,
                .format = format,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
               };
          VkAttachmentReference reference =
               {.attachment = 0,
                .layout = VK_IMAGE_LAYOUT_GENERAL,
               };
          VkSubpassDescription subpass =
               {.flags = 0,
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount = 0,
                .pInputAttachments = NULL,
                .colorAttachmentCount = 1,
                .pColorAttachments = & reference,
                .pResolveAttachments = NULL,
                .pDepthStencilAttachment = NULL,
                .preserveAttachmentCount = 0,
                .pPreserveAttachments = NULL,
               };
          VkRenderPassCreateInfo info =
               {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .attachmentCount = 1,
                .pAttachments = & attachment,
                .subpassCount = 1,
                .pSubpasses = & subpass,
                .dependencyCount = 0,
                .pDependencies = NULL,
               };
          try (vkCreateRenderPass (logic, & info, NULL, & renderPass), "Create render pass");
     }
     return renderPass;
}

struct devices enter (const struct point size)
{
     struct devices devices;
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
          for (unsigned int i = 0; i < info.enabledExtensionCount; ++i) printf ("Required extension: %s.\n", info.ppEnabledExtensionNames [i]);
          try (vkCreateInstance(&info, NULL, &devices.vulkan), "Vulkan initialization");
     }
     try (glfwCreateWindowSurface (devices.vulkan, devices.window, NULL, &devices.surface), "Vulkan surface initialization");
     devices.card = getSomePhysicalDevice (devices.vulkan);
     getLogicAndQueue (devices.card, devices.surface, &devices.logic, &devices.queue, &devices.queueFamilyIndex);
     devices.format = getFormat (devices.card, devices.surface);
     devices.chain = getSwapchain (devices.card, devices.logic, devices.surface, devices.format, size);
     devices.pool = getPool (devices.logic, devices.queueFamilyIndex);
     devices.buffer = getBuffer (devices.logic, devices.pool);
     devices.renderPass = getRenderPass (devices.logic, devices.format.format);
     devices.pointerToImages = getPointerToImages (devices.logic, devices.chain, devices.format.format, devices.renderPass, size);
     printf ("Images in the swap chain: %d\n", devices.pointerToImages->count);
     return devices;
}

int leave (const struct devices devices)
{
     try (vkDeviceWaitIdle (devices.logic), "waiting for device to finish work");
     for (unsigned int i = 0; i < devices.pointerToImages->count; ++i)
     {
          vkDestroyImageView (devices.logic, devices.pointerToImages->views [i], NULL);
          vkDestroyFramebuffer (devices.logic, devices.pointerToImages->frames [i], NULL);
     }
     free (devices.pointerToImages->images);
     free (devices.pointerToImages->views);
     free (devices.pointerToImages->frames);
     free (devices.pointerToImages);
     vkDestroyCommandPool (devices.logic, devices.pool, NULL);
     vkDestroyRenderPass (devices.logic, devices.renderPass, NULL);
     vkDestroySwapchainKHR (devices.logic, devices.chain, NULL);
     vkDestroySurfaceKHR (devices.vulkan, devices.surface, NULL);
     vkDestroyDevice (devices.logic, NULL);
     vkDestroyInstance (devices.vulkan, NULL);
     glfwDestroyWindow (devices.window);
     glfwTerminate ( );
     return 0;
}

void mainLoop (const struct devices devices, struct point const size)
{
     static unsigned int mainLoopCounter = 0;
     glfwSwapBuffers (devices.window);
     glfwPollEvents ( );
     unsigned int imageIndex;
     {
          VkFence fence;
          {
               VkFenceCreateInfo info = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = NULL, .flags = 0};
               try (vkCreateFence (devices.logic, & info, NULL, & fence), "Get a fence");
          }
          try (vkAcquireNextImageKHR (devices.logic, devices.chain, -1, VK_NULL_HANDLE, fence, &imageIndex), "Acquiring next image");
          try (vkWaitForFences (devices.logic, 1, & fence, VK_TRUE, -1), "Waiting for image acquisition");
          vkDestroyFence (devices.logic, fence, NULL);
          printf ("Image view acquired: %d at %p.\n", imageIndex, & devices.pointerToImages->images [imageIndex]);
     }
     unsigned int imageIndices [ ] = {imageIndex};
     {
          VkCommandBufferBeginInfo info =
               {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = NULL,
                .flags = 0,
                .pInheritanceInfo = NULL,
               };
          try (vkBeginCommandBuffer (devices.buffer, &info), "Begin buffer");
          printf ("Framebuffer pointer: %p.\n", & devices.pointerToImages->frames [imageIndex]);
          {
               VkRenderPassBeginInfo info =
                    {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                     .pNext = NULL,
                     .renderPass = devices.renderPass,
                     .framebuffer = devices.pointerToImages->frames [imageIndex],
                     .renderArea = (VkRect2D) {.offset.x = 0, .offset.y = 0, .extent.width = size.x, .extent.height = size.y},
                     .clearValueCount = 0,
                     .pClearValues = NULL,
                    };
               vkCmdBeginRenderPass (devices.buffer, & info, VK_SUBPASS_CONTENTS_INLINE);
               vkCmdEndRenderPass (devices.buffer);
          }
          try (vkEndCommandBuffer (devices.buffer), "End buffer");
     }
     {
          VkSubmitInfo info =
               {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = NULL,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = NULL,
                .pWaitDstStageMask = NULL,
                .commandBufferCount = 1,
                .pCommandBuffers = &devices.buffer,
                .signalSemaphoreCount = 0,
                .pSignalSemaphores = NULL,
               };
          try (vkQueueSubmit (devices.queue, 1, &info, VK_NULL_HANDLE), "Submission of the command buffer to the queue");
     }
     {
          VkPresentInfoKHR info =
               {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext= NULL,
                .waitSemaphoreCount = 0,
                .pWaitSemaphores = NULL,
                .swapchainCount = 1,
                .pSwapchains = &devices.chain,
                .pImageIndices = imageIndices,
                .pResults = NULL,
               };
          try (vkQueuePresentKHR (devices.queue, &info), "Presentation");
     }
     try (vkQueueWaitIdle (devices.queue), "Waiting for the queue to become idle");
     sleep (1);
     mainLoopCounter++;
}

int main (void)
{
     const struct point size = {.x = 800, .y = 600};
     const struct devices devices = enter (size);
     while (! glfwWindowShouldClose (devices.window)) mainLoop (devices, size);
     return leave (devices);
}
