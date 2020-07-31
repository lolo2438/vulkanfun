/*
 *
 *
 *
 * */
#include "vkRenderer.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_ALLOCATION_SUCCESSFUL(var) if(!(var)) exit(-69);

#define CLAMP(value, max, min) ((value) = (((value) < (min)) ? (min) : ((max) < (value)) ? (max) : (value)));

/* ================= */
/*  Local variables  */
/* ================= */
// TODO: Remove from here glfw
static GLFWwindow *glfwWindow        = NULL;
static uint32_t    glfwScreenWidth   = 1024;  //todo handle resizing
static uint32_t    glfwScreenHeight  = 820;


static const char *applicationName   = "VkFun";

static VkInstance vkInstance = VK_NULL_HANDLE;

static VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;

static VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;

static VkDevice vkDevice = VK_NULL_HANDLE;

static struct vkQueueFamily{
  VkQueueFamilyProperties properties;
  VkQueue *vkQueues;
  VkBool32 canPresent;
} *vkQueueFamily = NULL;
static uint32_t vkQueueFamilyCount = 0;

static VkSurfaceKHR vkSurface = VK_NULL_HANDLE;

static VkSwapchainKHR vkSwapchain        = VK_NULL_HANDLE;
static VkFormat       vkSwapchainFormat  = VK_FORMAT_UNDEFINED;
static VkExtent2D     vkSwapchainExtent  = {0, 0};

static VkImage        *vkSwapchainImages       = NULL;
static VkImageView    *vkSwapchainImageViews   = NULL;
static VkFramebuffer  *vkSwapchainFramebuffers = NULL;
static uint32_t        vkSwapchainImageCount   = 0;

static VkRenderPass vkRenderPass = VK_NULL_HANDLE;

static VkPipelineLayout vkPipelineLayout       = VK_NULL_HANDLE;
static VkPipeline      *vkPipelines            = NULL;
static uint32_t         vkPipelineCount        = 0;
static const uint32_t   vkGraphicPipelineIndex = 0;

static VkCommandPool   *vkCommandPools         = NULL;
static uint32_t         vkCommandPoolCount     = 0;

// TODO: Create standard commandbuffer
static VkCommandBuffer *vkCommandBuffers      = NULL;
static uint32_t         vkCommandBufferCount  = 0;

static VkSemaphore vkImageAvailableSemaphore  = VK_NULL_HANDLE;
static VkSemaphore vkRenderFinishedSemaphore  = VK_NULL_HANDLE;


/* ================== */
/*  Vulkan Functions  */
/* ================== */
#ifdef DEBUG

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,VkDebugUtilsMessengerEXT* pDebugMessenger) {

  PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func)
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator){

  PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

  if(func) func(instance, messenger, pAllocator);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                          const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData){

  printf("Validation layer ");

  if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)   printf("(VERBOSE) ");
  if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)     printf("(INFO) ");
  if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)   printf("(WARNING) ");
  if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)    printf("(ERROR) ");

  if(messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)      printf("(GENERAL) ");
  if(messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)    printf("(VALIDATION) ");
  if(messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)    printf("(PERFORMANCE) ");

    printf(": %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}
#endif

/* ================= */
/*  Local Functions  */
/* ================= */
static void createInstance(void){

  VkApplicationInfo applicationInfo = {
      .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext              = NULL,
      .pApplicationName   = applicationName,
      .applicationVersion = VK_MAKE_VERSION(1,0,0),
      .pEngineName        = NULL,
      .engineVersion      = 0,
      .apiVersion         = VK_API_VERSION_1_1
  };

  void *pNext;

  uint32_t enabledLayerCount;
  char **ppEnabledLayerNames;

  uint32_t enabledExtensionCount;
  char **ppEnabledExtensionNames;

#ifdef DEBUG

  /* Debug MSG */
  VkDebugUtilsMessageSeverityFlagsEXT messageSeverityFlags = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  VkDebugUtilsMessageTypeFlagsEXT messageTypeFlags = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
      .sType         = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext         = NULL,
      .flags         = 0,
      .messageSeverity  = messageSeverityFlags,
      .messageType     = messageTypeFlags,
      .pfnUserCallback   = debugCallback,
      .pUserData       = NULL
  };

  pNext = (void*) &debugUtilsMessengerCreateInfo;

  /* Layers */
  vkEnumerateInstanceLayerProperties(&enabledLayerCount, NULL);
  VkLayerProperties *layerProperties = malloc(enabledLayerCount * sizeof(*layerProperties));
  CHECK_ALLOCATION_SUCCESSFUL(layerProperties);
  vkEnumerateInstanceLayerProperties(&enabledLayerCount, layerProperties);

  ppEnabledLayerNames = malloc(enabledLayerCount * sizeof(*ppEnabledLayerNames));
  CHECK_ALLOCATION_SUCCESSFUL(ppEnabledLayerNames);

  for(uint32_t i = 0; i < enabledLayerCount; i += 1){
    ppEnabledLayerNames[i] = malloc(VK_MAX_EXTENSION_NAME_SIZE * sizeof(*(ppEnabledLayerNames[i])));
    CHECK_ALLOCATION_SUCCESSFUL(ppEnabledLayerNames[i]);

    memcpy(ppEnabledLayerNames[i], layerProperties[i].layerName, VK_MAX_EXTENSION_NAME_SIZE * sizeof(*(ppEnabledLayerNames[i])));
  }

  /* Extensions */
  uint32_t glfwExtensionCount;
  const char **glfwExtensionName = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  enabledExtensionCount = glfwExtensionCount + 1;

  ppEnabledExtensionNames = malloc(enabledExtensionCount * sizeof(*ppEnabledExtensionNames));
  CHECK_ALLOCATION_SUCCESSFUL(ppEnabledExtensionNames);

  for(uint32_t i = 0; i < glfwExtensionCount; i += 1){
    ppEnabledExtensionNames[i] = malloc(VK_MAX_EXTENSION_NAME_SIZE * sizeof(*(ppEnabledExtensionNames[i])));
    CHECK_ALLOCATION_SUCCESSFUL(ppEnabledExtensionNames[i]);
    memcpy(ppEnabledExtensionNames[i], glfwExtensionName[i], (VK_MAX_EXTENSION_NAME_SIZE * sizeof(*(ppEnabledExtensionNames[i]))));
  }
  ppEnabledExtensionNames[enabledExtensionCount - 1] = malloc(VK_MAX_EXTENSION_NAME_SIZE * sizeof(*(ppEnabledExtensionNames[enabledExtensionCount - 1])));
  CHECK_ALLOCATION_SUCCESSFUL(ppEnabledExtensionNames[enabledExtensionCount - 1]);
  memcpy(ppEnabledExtensionNames[enabledExtensionCount - 1], VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_MAX_EXTENSION_NAME_SIZE);
#else
  pNext = NULL;
  enabledLayerCount = 0;
  ppEnabledLayerNames = NULL;
  ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&enabledExtensionCount)
#endif

  VkInstanceCreateInfo instanceCreateInfo = {
      .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext                   = pNext,
      .flags                   = 0,
      .pApplicationInfo        = &applicationInfo,
      .enabledLayerCount       = enabledLayerCount,
      .ppEnabledLayerNames     = (const char**) ppEnabledLayerNames,
      .enabledExtensionCount   = enabledExtensionCount,
      .ppEnabledExtensionNames = (const char**) ppEnabledExtensionNames
  };

  vkCreateInstance(&instanceCreateInfo, NULL, &vkInstance);

#ifdef DEBUG
  free(layerProperties);

  for(uint32_t i = 0; i < enabledLayerCount; i += 1){
    free(ppEnabledLayerNames[i]);
  }
  free(ppEnabledLayerNames);

  for(uint32_t i = 0; i < enabledExtensionCount; i += 1){
    free(ppEnabledExtensionNames[i]);
  }
  free(ppEnabledExtensionNames);
#endif

}

#ifdef DEBUG
static void setupDebugMessenger(void){

  VkDebugUtilsMessageSeverityFlagsEXT messageSeverityFlags = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                               | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  VkDebugUtilsMessageTypeFlagsEXT messageTypeFlags = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
      .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext           = NULL,
      .flags           = 0,
      .messageSeverity = messageSeverityFlags,
      .messageType     = messageTypeFlags,
      .pfnUserCallback = debugCallback,
      .pUserData       = NULL
  };

  vkCreateDebugUtilsMessengerEXT(vkInstance, &debugUtilsMessengerCreateInfo, NULL, &debugUtilsMessenger);
}
#endif

static void createSurface(void){

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindow = glfwCreateWindow(glfwScreenWidth, glfwScreenHeight, applicationName, NULL, NULL);

  glfwCreateWindowSurface(vkInstance, glfwWindow, NULL, &vkSurface);
}

static void createDevicesAndQueues(void){

  /* Select physical device */
  uint32_t physicalDeviceCount;

  vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, NULL);
  VkPhysicalDevice *physicalDevices = malloc(physicalDeviceCount * sizeof(*physicalDevices));
  CHECK_ALLOCATION_SUCCESSFUL(physicalDevices);
  vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, physicalDevices);

  uint32_t bestPhysicalDeviceIndex = 0;
  VkPhysicalDeviceProperties bestPhysicalDeviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevices[bestPhysicalDeviceIndex], &bestPhysicalDeviceProperties);

  for(uint32_t i = 1; i < physicalDeviceCount; i += 1){

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevices[i], &physicalDeviceProperties);

    if(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
      if(bestPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
        // Check limits to see the best
      }
      else{
        bestPhysicalDeviceIndex = i;
        bestPhysicalDeviceProperties = physicalDeviceProperties;
      }
    }
    else if(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU){
      if(bestPhysicalDeviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
        bestPhysicalDeviceProperties = physicalDeviceProperties;
        bestPhysicalDeviceIndex = i;
      }
    }
  }

  vkPhysicalDevice = physicalDevices[bestPhysicalDeviceIndex];
  free(physicalDevices);

  vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, NULL);
  VkQueueFamilyProperties *queueFamilyProperties = malloc(vkQueueFamilyCount * sizeof(*queueFamilyProperties));
  CHECK_ALLOCATION_SUCCESSFUL(queueFamilyProperties);
  vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, queueFamilyProperties);

  /* Create logical device*/
  VkDeviceQueueCreateInfo *queueCreateInfo = calloc(vkQueueFamilyCount, sizeof(*queueCreateInfo));
  CHECK_ALLOCATION_SUCCESSFUL(queueCreateInfo);

  float **queuePriority = malloc(vkQueueFamilyCount * sizeof(*queuePriority));
  CHECK_ALLOCATION_SUCCESSFUL(queuePriority);
  for(uint32_t i = 0; i < vkQueueFamilyCount; i += 1){
    queuePriority[i] = malloc(queueFamilyProperties[i].queueCount * sizeof(*(queuePriority[i])));
    CHECK_ALLOCATION_SUCCESSFUL(queuePriority[i]);

    /* Todo: Select what queue is what and give priority */
    for(uint32_t j = 0; j < queueFamilyProperties[i].queueCount; j += 1){
      queuePriority[i][j] = 1.0f;
    }
  }

  for(uint32_t i = 0; i < vkQueueFamilyCount; i += 1){
    VkDeviceQueueCreateInfo _queueCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext            = NULL,
        .flags            = 0,
        .queueFamilyIndex = i,
        .queueCount       = queueFamilyProperties[i].queueCount,
        .pQueuePriorities = queuePriority[i]
    };

    queueCreateInfo[i] = _queueCreateInfo;
  }

  const char *enabledExtensionNames[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  const uint32_t enabledExtensionCount = sizeof(enabledExtensionNames) / sizeof(*enabledExtensionNames);

  VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueCreateInfoCount = vkQueueFamilyCount,
      .pQueueCreateInfos = queueCreateInfo,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = NULL,
      .enabledExtensionCount = enabledExtensionCount,
      .ppEnabledExtensionNames = enabledExtensionNames,
      .pEnabledFeatures = NULL
  };

  vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, NULL, &vkDevice);

  free(queueCreateInfo);
  for(uint32_t i = 0; i < vkQueueFamilyCount; i += 1) free(queuePriority[i]);
  free(queuePriority);

  /* Get queues */
  vkQueueFamily = malloc(vkQueueFamilyCount * sizeof(*vkQueueFamily));
  CHECK_ALLOCATION_SUCCESSFUL(vkQueueFamily);

  // i = QueueFamilyIndex
  // j = QueueIndex
  for(uint32_t i = 0; i < vkQueueFamilyCount; i += 1){

    vkQueueFamily[i].properties = queueFamilyProperties[i];

    vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, i, vkSurface, &(vkQueueFamily[i].canPresent));

    vkQueueFamily[i].vkQueues = malloc(vkQueueFamily[i].properties.queueCount * sizeof(*(vkQueueFamily[i].vkQueues)));
    CHECK_ALLOCATION_SUCCESSFUL(vkQueueFamily[i].vkQueues);

    for(uint32_t j = 0; j < vkQueueFamily[i].properties.queueCount; j += 1){
      vkGetDeviceQueue(vkDevice, i, j, (vkQueueFamily[i].vkQueues + j));
    }
  }

  free(queueFamilyProperties);
}

/* Todo: Triple buffering:
 *      2 buffer to write while 1 is rendering:
 *      Ex: Writing to buf[0] while buf[1] is rendering, buf[0] is done writing but buf[1] has not finished rendering, therefore
 *         start writing into buf[2] and when buf[1] is done rendering swap to but[0] for rendering and so on.
 *         wr 0 rd 1
 *       0 done, 1 not donw,
 *       -> wr 2
 *       1 done, swap 0 and 1
 *       2 done wr 1...
 * */
static void createSwapChain(VkSwapchainKHR oldSwapchain){

  /* Surface format */
  uint32_t surfaceFormatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &surfaceFormatCount, NULL);
  VkSurfaceFormatKHR *surfaceFormats = malloc(surfaceFormatCount * sizeof(*surfaceFormats));
  CHECK_ALLOCATION_SUCCESSFUL(surfaceFormats);
  vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &surfaceFormatCount, surfaceFormats);

  VkBool32 foundFormat = VK_FALSE;
  VkFormat swapchainImageFormat = surfaceFormats[0].format;
  VkColorSpaceKHR swapchainColorSpace = surfaceFormats[0].colorSpace;
  for(uint32_t i = 0; (i < surfaceFormatCount) || (foundFormat == VK_FALSE); i += 1){
    if(surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
      swapchainImageFormat = surfaceFormats[i].format;
      swapchainColorSpace = surfaceFormats[i].colorSpace;
      foundFormat = VK_TRUE;
    }
  }
  vkSwapchainFormat = swapchainImageFormat;

  /* Present Mode */
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurface, &presentModeCount, NULL);
  VkPresentModeKHR *presentModes = malloc(presentModeCount* sizeof(*presentModes));
  CHECK_ALLOCATION_SUCCESSFUL(presentModes);
  vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurface, &presentModeCount, presentModes);

  VkBool32 presentModeFound = VK_FALSE;
  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  for(uint32_t i = 0; (i < presentModeCount) && (presentModeFound == VK_FALSE); i += 1){
    if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
      swapchainPresentMode = presentModes[i];
      presentModeFound = VK_TRUE;
    }
  }

  /* SurfaceCapabilities */
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, vkSurface, &surfaceCapabilities);

  vkSwapchainImageCount = (swapchainPresentMode == VK_PRESENT_MODE_MAILBOX_KHR) ? 3 : 2;  //triple or double buffering

  CLAMP(vkSwapchainImageCount, surfaceCapabilities.maxImageCount, surfaceCapabilities.minImageCount + 1);

  VkImageUsageFlags swapchainImageUsage = surfaceCapabilities.supportedUsageFlags & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
  VkSurfaceTransformFlagBitsKHR swapchainPreTransform = surfaceCapabilities.currentTransform;  //supported transform
  VkCompositeAlphaFlagBitsKHR swapchainCompositeAlpha = surfaceCapabilities.supportedCompositeAlpha & (VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

  VkExtent2D swapchainImageExtent;
  if(surfaceCapabilities.currentExtent.width != UINT32_MAX && surfaceCapabilities.currentExtent.height != UINT32_MAX){
    swapchainImageExtent = surfaceCapabilities.currentExtent;
  }
  else{
    swapchainImageExtent.width = glfwScreenWidth;
    CLAMP(swapchainImageExtent.width,surfaceCapabilities.maxImageExtent.width, surfaceCapabilities.minImageExtent.width);

    swapchainImageExtent.height = glfwScreenHeight;
    CLAMP(swapchainImageExtent.height, surfaceCapabilities.maxImageExtent.height, surfaceCapabilities.minImageExtent.height);
  }
  vkSwapchainExtent = swapchainImageExtent;


  /* Swapchain creation */
  /* TODO: if queue family can't present and do graphic in the same queue family -> sharing concurent
   * must say how many family index there is and what index will present and which one will do graphics */
  VkSharingMode swapchainSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  uint32_t swapchainQueueFamilyIndexCount = 0;
  uint32_t *swapchainPtrQueueFamilyIndices = NULL;
  if(0){
    swapchainSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchainQueueFamilyIndexCount = 0;
    swapchainPtrQueueFamilyIndices = NULL;
  }

  VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext                 = NULL,
      .flags                 = 0,
      .surface               = vkSurface,
      .minImageCount         = vkSwapchainImageCount,
      .imageFormat           = swapchainImageFormat,
      .imageColorSpace       = swapchainColorSpace,
      .imageExtent           = swapchainImageExtent,
      .imageArrayLayers      = 1,
      .imageUsage            = swapchainImageUsage,
      .imageSharingMode      = swapchainSharingMode,
      .queueFamilyIndexCount = swapchainQueueFamilyIndexCount,
      .pQueueFamilyIndices   = swapchainPtrQueueFamilyIndices,
      .preTransform          = swapchainPreTransform,
      .compositeAlpha        = swapchainCompositeAlpha,
      .presentMode           = swapchainPresentMode,
      .clipped               = VK_TRUE,
      .oldSwapchain          = oldSwapchain
  };

  vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, NULL, &vkSwapchain);

  free(surfaceFormats);
  free(presentModes);

  /* Get Images and create Image Views*/
  vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &vkSwapchainImageCount, NULL);
  vkSwapchainImages = malloc(vkSwapchainImageCount * sizeof(*vkSwapchainImages));
  CHECK_ALLOCATION_SUCCESSFUL(vkSwapchainImages);
  vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &vkSwapchainImageCount, vkSwapchainImages);

  vkSwapchainImageViews = malloc(vkSwapchainImageCount * sizeof(*vkSwapchainImageViews));
  CHECK_ALLOCATION_SUCCESSFUL(vkSwapchainImageViews);

  for(uint32_t i = 0; i < vkSwapchainImageCount; i += 1){

    VkComponentMapping imageViewComponentMapping = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY
    };

    VkImageSubresourceRange imageViewSubresourceRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    };

    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = NULL,
        .flags            = 0,
        .image            = vkSwapchainImages[i],
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = vkSwapchainFormat,
        .components       = imageViewComponentMapping,
        .subresourceRange = imageViewSubresourceRange
    };

    vkCreateImageView(vkDevice, &imageViewCreateInfo, NULL, vkSwapchainImageViews + i);
  }
}


static void createRenderPass(void){

  VkAttachmentDescription colorAttachmentDescription = {
      .flags          = 0,
      .format         = vkSwapchainFormat,
      .samples        = VK_SAMPLE_COUNT_1_BIT,
      .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  };

  VkAttachmentReference colorAttachmentReference = {
      .attachment = 0, //Index of color attachmentDescription
      .layout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };

  VkSubpassDescription subpassDescription = {
      .flags                   = 0,
      .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount    = 0,
      .pInputAttachments       = NULL,
      .colorAttachmentCount    = 1,
      .pColorAttachments       = &colorAttachmentReference,
      .pResolveAttachments     = NULL,
      .pDepthStencilAttachment = NULL,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments    = NULL
  };

  VkSubpassDependency subpassDependency = {
      .srcSubpass      = VK_SUBPASS_EXTERNAL,
      .dstSubpass      = 0,
      .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask   = 0,
      .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dependencyFlags = 0
  };

  VkRenderPassCreateInfo renderPassCreateInfo = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext           = NULL,
      .flags           = 0,
      .attachmentCount = 1,
      .pAttachments    = &colorAttachmentDescription,
      .subpassCount    = 1,
      .pSubpasses      = &subpassDescription,
      .dependencyCount = 1,
      .pDependencies   = &subpassDependency
  };

  vkCreateRenderPass(vkDevice, &renderPassCreateInfo, NULL, &vkRenderPass);
}


static VkResult createShaderModule(const char *codePath, VkShaderModule *shaderModule){

  FILE *f = fopen(codePath, "rb");
  if(!f) return VK_ERROR_OUT_OF_HOST_MEMORY;

  fseek(f, 0, SEEK_END);
  size_t codeSize = (size_t) ftell(f);
  fseek(f, 0, SEEK_SET);

  uint32_t *pCode = malloc(codeSize * sizeof(*pCode));
  if(!pCode) return VK_ERROR_OUT_OF_HOST_MEMORY;

  fread(pCode, sizeof(*pCode), (codeSize)/sizeof(*pCode), f);

  fclose(f);

  VkShaderModuleCreateInfo shaderCreateInfo = {
      .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext    = NULL,
      .flags    = 0,
      .codeSize = codeSize,
      .pCode    = pCode
  };

  VkResult result = vkCreateShaderModule(vkDevice, &shaderCreateInfo, NULL, shaderModule);

  free(pCode);

  return result;
}


static void createGraphicsPipeline(void){

  VkShaderModule vertShaderModule, fragShaderModule;

  createShaderModule("spir-v/vert.spv", &vertShaderModule);
  createShaderModule("spir-v/frag.spv", &fragShaderModule);

  VkPipelineShaderStageCreateInfo vertPipelineShaderStageCreateInfo = {
      .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext               = NULL,
      .flags               = 0,
      .stage               = VK_SHADER_STAGE_VERTEX_BIT,
      .module              = vertShaderModule,
      .pName               = "main",
      .pSpecializationInfo = NULL
  };

  VkPipelineShaderStageCreateInfo fragPipelineShaderStageCreateInfo = {
      .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext               = NULL,
      .flags               = 0,
      .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module              = fragShaderModule,
      .pName               = "main",
      .pSpecializationInfo = NULL
  };

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertPipelineShaderStageCreateInfo, fragPipelineShaderStageCreateInfo};

  /* Input state */
  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
      .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext                           = NULL,
      .flags                           = 0,
      .vertexBindingDescriptionCount   = 0,
      .pVertexBindingDescriptions      = NULL,
      .vertexAttributeDescriptionCount = 0,
      .pVertexBindingDescriptions      = NULL

  };

  /* Input assembly */
  VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext                  = NULL,
      .flags                  = 0,
      .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE
  };

  /* View port*/
  VkViewport viewport = {
      .x        = 0.0f,
      .y        = 0.0f,
      .width    = (float) glfwScreenWidth,
      .height   = (float) glfwScreenHeight,
      .minDepth = 0.0f,
      .maxDepth = 1.0f
  };

  VkRect2D scissor = {
      .offset = {0,0},
      .extent = vkSwapchainExtent
  };

  VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext         = NULL,
      .flags         = 0,
      .viewportCount = 1,
      .pViewports    = &viewport,
      .scissorCount  = 1,
      .pScissors     = &scissor
  };

  /* Rasterizer */
  VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {
      .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext                   = NULL,
      .flags                   = 0,
      .depthClampEnable        = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode             = VK_POLYGON_MODE_FILL,
      .cullMode                = VK_CULL_MODE_BACK_BIT,
      .frontFace               = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable         = VK_FALSE,
      .depthBiasConstantFactor = 0.0f,
      .depthBiasClamp          = 0.0f,
      .depthBiasSlopeFactor    = 0.0f,
      .lineWidth               = 1.0f
  };

  /* Multisampling */
  VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {
      .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext                 = NULL,
      .flags                 = 0,
      .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable   = VK_FALSE,
      .minSampleShading      = 1.0f,
      .pSampleMask           = NULL,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable      = VK_FALSE
  };

  /* Colorblend */
  VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState = {
      .blendEnable         = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .colorBlendOp        = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp        = VK_BLEND_OP_ADD,
      .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  // Note: Blend either with logicOpEnable or in pipeline ColorBlendAttachmentState, logicOpEnable priority
  VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo = {
      .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext             = NULL,
      .flags             = 0,
      .logicOpEnable     = VK_FALSE,
      .logicOp           = VK_LOGIC_OP_AND,
      .attachmentCount   = 1,
      .pAttachments      = &PipelineColorBlendAttachmentState,
      .blendConstants[0] = 1.0f,
      .blendConstants[1] = 1.0f,
      .blendConstants[2] = 1.0f,
      .blendConstants[3] = 1.0f
  };

  /* Pipeline layout */
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext                  = NULL,
      .flags                  = 0,
      .setLayoutCount         = 0,
      .pSetLayouts            = NULL,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges    = NULL
  };

  vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, NULL, &vkPipelineLayout);

  /* Pipeline */
  VkGraphicsPipelineCreateInfo graphicPipelineCreateInfo = {
      .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext               = NULL,
      .flags               = 0,
      .stageCount          = 2,
      .pStages             = shaderStages,
      .pVertexInputState   = &pipelineVertexInputStateCreateInfo,
      .pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
      .pTessellationState  = NULL,
      .pViewportState      = &pipelineViewportStateCreateInfo,
      .pRasterizationState = &pipelineRasterizationStateCreateInfo,
      .pMultisampleState   = &pipelineMultisampleStateCreateInfo,
      .pDepthStencilState  = NULL,
      .pColorBlendState    = &PipelineColorBlendStateCreateInfo,
      .pDynamicState       = NULL,
      .layout              = vkPipelineLayout,
      .renderPass          = vkRenderPass,
      .subpass             = 0,
      .basePipelineIndex   = -1,
      .basePipelineHandle  = VK_NULL_HANDLE
  };

  VkGraphicsPipelineCreateInfo pipelineCreateInfos[] = { graphicPipelineCreateInfo };
  vkPipelineCount = sizeof(pipelineCreateInfos)/sizeof(*pipelineCreateInfos);

  vkPipelines = malloc(vkPipelineCount * sizeof(*vkPipelines));
  CHECK_ALLOCATION_SUCCESSFUL(vkPipelines);

  vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, vkPipelineCount, pipelineCreateInfos, NULL, vkPipelines);

  vkDestroyShaderModule(vkDevice, vertShaderModule, NULL);
  vkDestroyShaderModule(vkDevice, fragShaderModule, NULL);
}


static void createFramebuffers(void){

  vkSwapchainFramebuffers = malloc(vkSwapchainImageCount * sizeof(*vkSwapchainFramebuffers));
  CHECK_ALLOCATION_SUCCESSFUL(vkSwapchainFramebuffers);

  for(uint32_t i = 0; i < vkSwapchainImageCount; i += 1){

    VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext           = NULL,
        .flags           = 0,
        .renderPass      = vkRenderPass,
        .attachmentCount = 1,
        .pAttachments    = vkSwapchainImageViews + i,
        .width           = vkSwapchainExtent.width,
        .height          = vkSwapchainExtent.height,
        .layers          = 1
    };

    vkCreateFramebuffer(vkDevice, &framebufferCreateInfo, NULL, vkSwapchainFramebuffers + i);
  }
}


static void createCommands(void){

  /* Command pools */
  vkCommandPoolCount = vkQueueFamilyCount;

  vkCommandPools = malloc(vkCommandPoolCount * sizeof(*vkCommandPools));
  CHECK_ALLOCATION_SUCCESSFUL(vkCommandPools);

  /* Should create:
   * L * T + N pools -> L = #buffered frames, T = # of threads, N = extra pools for secondary command buffers
   * At the moment: L = vkSwapchainImageCount, T = 1 (for now), N = 0
   * per family index TODO: L = vkSwapchainImageCount
   */
  for(uint32_t i = 0; i < vkCommandPoolCount; i += 1){

    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = NULL,
        .flags            =  0,
        .queueFamilyIndex = i
    };

    vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, NULL, vkCommandPools + i);
  }

  /* Command Buffers */
  /* For now there is 1 buffer per image, but there should be
   *
   * */
  vkCommandBufferCount = vkSwapchainImageCount;

  vkCommandBuffers = malloc(vkCommandBufferCount * sizeof(*vkCommandBuffers));
  CHECK_ALLOCATION_SUCCESSFUL(vkCommandBuffers);

  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext              = NULL,
      .commandPool        = vkCommandPools[0],
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = vkCommandBufferCount
  };

  vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, vkCommandBuffers);

  /* Command recording */
  for(uint32_t i = 0; i < vkCommandBufferCount; i += 1){
    VkCommandBufferBeginInfo commandBeginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = NULL,
        .flags            = 0,
        .pInheritanceInfo = NULL
    };

    vkBeginCommandBuffer(vkCommandBuffers[i], &commandBeginInfo);

    VkRect2D renderArea = {
        .offset = {0,0},
        .extent = vkSwapchainExtent
    };

    VkClearValue clearColor = {
        .color.float32 = {0.0f, 0.0f, 0.0f, 1.0f}
    };

    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = NULL,
        .renderPass      = vkRenderPass,
        .framebuffer     = vkSwapchainFramebuffers[i],
        .renderArea      = renderArea,
        .clearValueCount = 1,
        .pClearValues    = &clearColor
    };

    vkCmdBeginRenderPass(vkCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelines[vkGraphicPipelineIndex]);

    vkCmdDraw(vkCommandBuffers[i], 3, 1, 0, 0);

    vkCmdEndRenderPass(vkCommandBuffers[i]);

    vkEndCommandBuffer(vkCommandBuffers[i]);
  }
}


static void createSemaphores(void){

  VkSemaphoreCreateInfo semaphoreCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0
  };

  vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NULL, &vkImageAvailableSemaphore);
  vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NULL, &vkRenderFinishedSemaphore);

}

/* ===================== */
/*  Interface Functions  */
/* ===================== */
int vk_renderer_create(void){

  glfwInit(); // Todo: Create a window module
  createInstance();
#ifdef DEBUG
  setupDebugMessenger();
#endif
  createSurface();
  createDevicesAndQueues();
  createSwapChain(VK_NULL_HANDLE);
  createRenderPass();
  createFramebuffers();
  createGraphicsPipeline();
  createCommands();
  createSemaphores();

  return 0;
}

int vk_draw_frame(void){

  glfwPollEvents();

  uint32_t imageIndex;
  vkAcquireNextImageKHR(vkDevice, vkSwapchain, UINT64_MAX, vkImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

  VkSemaphore waitSemaphores[] = {vkImageAvailableSemaphore};
  uint32_t waitSemaphoreCount = sizeof(waitSemaphores)/sizeof(*waitSemaphores);

  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSemaphore signalSemaphores[] = {vkRenderFinishedSemaphore};
  uint32_t signalSemaphoreCount = sizeof(signalSemaphores)/sizeof(*signalSemaphores);

  VkSubmitInfo submitInfo = {
      .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext                = NULL,
      .waitSemaphoreCount   = waitSemaphoreCount,
      .pWaitSemaphores      = waitSemaphores,
      .pWaitDstStageMask    = waitStages,
      .commandBufferCount   = 1,
      .pCommandBuffers      = (vkCommandBuffers + imageIndex),
      .signalSemaphoreCount = signalSemaphoreCount,
      .pSignalSemaphores    = signalSemaphores
  };

  vkQueueSubmit(vkQueueFamily[0].vkQueues[0], 1, &submitInfo, VK_NULL_HANDLE);

  VkSwapchainKHR swapchains[] = {vkSwapchain};
  uint32_t swapchainCount = sizeof(swapchains)/sizeof(*swapchains);

  VkPresentInfoKHR presentInfo = {
      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext              = NULL,
      .waitSemaphoreCount = signalSemaphoreCount,
      .pWaitSemaphores    = signalSemaphores,
      .swapchainCount     = swapchainCount,
      .pSwapchains        = swapchains,
      .pImageIndices      = &imageIndex,
      .pResults           = NULL
  };

  vkQueuePresentKHR(vkQueueFamily[0].vkQueues[1], &presentInfo);

  vkQueueWaitIdle(vkQueueFamily[0].vkQueues[0]);
  vkQueueWaitIdle(vkQueueFamily[0].vkQueues[1]);

  return !glfwWindowShouldClose(glfwWindow);
}

int vk_renderer_destroy(void){

  vkDestroySemaphore(vkDevice, vkImageAvailableSemaphore, NULL);
  vkDestroySemaphore(vkDevice, vkRenderFinishedSemaphore, NULL);

  //for(uint32_t i = 0; i < vkCommandPoolCount; i+= 1)
  vkFreeCommandBuffers(vkDevice, vkCommandPools[0], vkCommandBufferCount, vkCommandBuffers);
  free(vkCommandBuffers);

  for(uint32_t i = 0; i < vkCommandPoolCount; i += 1) vkDestroyCommandPool(vkDevice, vkCommandPools[i], NULL);
  free(vkCommandPools);

  for(uint32_t i = 0; i < vkPipelineCount; i += 1) vkDestroyPipeline(vkDevice, vkPipelines[i], NULL);
  free(vkPipelines);

  vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, NULL);

  for(uint32_t i = 0; i < vkSwapchainImageCount; i += 1) vkDestroyFramebuffer(vkDevice, vkSwapchainFramebuffers[i], NULL);
  free(vkSwapchainFramebuffers);

  vkDestroyRenderPass(vkDevice, vkRenderPass, NULL);

  for(uint32_t i = 0; i < vkSwapchainImageCount; i += 1) vkDestroyImageView(vkDevice, vkSwapchainImageViews[i], NULL);
  free(vkSwapchainImageViews);
  free(vkSwapchainImages);

  vkDestroySwapchainKHR(vkDevice, vkSwapchain, NULL);

  for(uint32_t i = 0; i < vkQueueFamilyCount; i += 1) free(vkQueueFamily[i].vkQueues);
  free(vkQueueFamily);

  vkDestroyDevice(vkDevice,NULL);
  vkDestroySurfaceKHR(vkInstance, vkSurface, NULL);

  glfwDestroyWindow(glfwWindow);  // Todo: Create a window module
  glfwTerminate();

  vkDestroyDebugUtilsMessengerEXT(vkInstance, debugUtilsMessenger, NULL);
  vkDestroyInstance(vkInstance, NULL);

  return 0;
}
