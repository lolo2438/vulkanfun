#include "vkRenderer.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#define CHECK_ALLOCATION_SUCCESSFUL(var) if(!(var)) exit(-69);

static GLFWwindow *glfwWindow = NULL;

static const char *applicationName = "VkFun";

static VkInstance vkInstance = VK_NULL_HANDLE;

static struct vkPhysicalDeviceCharacteristics {
	VkPhysicalDeviceProperties physicalDeviceProperties;

	VkQueueFamilyProperties *queueFamilyProperties;
	uint32_t queueFamilyPropertiesCount;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;

	VkSurfaceFormatKHR *surfaceFormats;
	uint32_t surfaceFormatCount;

	VkPresentModeKHR *presentModes;
	uint32_t presentModeCount;

} vkPhysicalDeviceCharacteristics;
static VkPhysicalDevice vkPhysicalDevice;

static VkDevice vkDevice = VK_NULL_HANDLE;

static struct vkQueueFamily{
	uint32_t canPresent : 1;	//Problem with VkBool32...
	uint32_t queueCount;
	VkQueue *queues;
} *vkQueueFamily = NULL;
static uint32_t vkQueueFamilyIndexCount = 0;

static VkCommandPool *vkCommandPools = NULL;

static VkSurfaceKHR vkSurface;

static VkSwapchainKHR vkSwapchain;

static void createInstance(void){

	VkApplicationInfo applicationInfo = { 0 };
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = NULL;
	applicationInfo.pApplicationName = applicationName;
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
	applicationInfo.pEngineName = NULL;
	applicationInfo.engineVersion = 0;
	applicationInfo.apiVersion = VK_MAKE_VERSION(1,1,119);

	uint32_t enabledExtensionCount;
	const char **enabledExtensionNames = glfwGetRequiredInstanceExtensions(&enabledExtensionCount);

	VkInstanceCreateInfo instanceCreateInfo = { 0 };
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.flags = 0; //Reserved for future use
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = 0; 		//TODO
	instanceCreateInfo.ppEnabledLayerNames = NULL; //TODO
	instanceCreateInfo.enabledExtensionCount = enabledExtensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;

	if(vkCreateInstance(&instanceCreateInfo, NULL, &vkInstance) != VK_SUCCESS){
		perror("Failed creating the vulkan instance\n");
	}
}

static void createSurface(void){

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindow = glfwCreateWindow(640, 480, applicationName, NULL, NULL);

	glfwCreateWindowSurface(vkInstance, glfwWindow, NULL, &vkSurface);

}

static void createDevicesAndQueues(void){

	/* Select physical device */
	uint32_t physicalDeviceCount;

	vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, NULL);

	VkPhysicalDevice *physicalDevices = malloc(physicalDeviceCount * sizeof(*physicalDevices));
	CHECK_ALLOCATION_SUCCESSFUL(physicalDevices);

	vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, physicalDevices);

	struct vkPhysicalDeviceCharacteristics *physicalDevicesCharact = malloc(physicalDeviceCount * sizeof(*physicalDevicesCharact));
	CHECK_ALLOCATION_SUCCESSFUL(physicalDevices);

	// get the characteristics of all physical devices
	for(uint32_t i = 0; i < physicalDeviceCount; i += 1){

		vkGetPhysicalDeviceProperties(physicalDevices[i], &(physicalDevicesCharact[i].physicalDeviceProperties));
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[i],vkSurface,&(physicalDevicesCharact[i].surfaceCapabilities));

		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &(physicalDevicesCharact[i].queueFamilyPropertiesCount), NULL);
		if(physicalDevicesCharact[i].queueFamilyPropertiesCount != 0){
			physicalDevicesCharact[i].queueFamilyProperties = malloc(physicalDevicesCharact[i].queueFamilyPropertiesCount * sizeof(*(physicalDevicesCharact[i].queueFamilyProperties)));
			CHECK_ALLOCATION_SUCCESSFUL(physicalDevicesCharact[i].queueFamilyProperties);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &(physicalDevicesCharact[i].queueFamilyPropertiesCount), &(physicalDevicesCharact[i].queueFamilyProperties));
		}

		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i], vkSurface, &(physicalDevicesCharact[i].surfaceFormatCount), NULL);
		if(physicalDevicesCharact[i].surfaceFormatCount != 0){
			physicalDevicesCharact[i].surfaceFormats = malloc(physicalDevicesCharact[i].surfaceFormatCount * sizeof(*(physicalDevicesCharact[i].surfaceFormats)));
			CHECK_ALLOCATION_SUCCESSFUL(physicalDevicesCharact[i].surfaceFormats);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i], vkSurface, &(physicalDevicesCharact[i].surfaceFormatCount), &(physicalDevicesCharact[i].surfaceFormats));
		}

		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[i], vkSurface, &(physicalDevicesCharact[i].presentModeCount), NULL);
		if(physicalDevicesCharact[i].presentModeCount != 0){
			physicalDevicesCharact[i].presentModes = malloc(physicalDevicesCharact[i].presentModeCount * sizeof(*(physicalDevicesCharact[i].presentModes)));
			CHECK_ALLOCATION_SUCCESSFUL(physicalDevicesCharact[i].presentModes);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[i], vkSurface, &(physicalDevicesCharact[i].presentModeCount), physicalDevicesCharact[i].presentModes);
		}
	}

	// TODO: Pick a physical device using struct vkPhysicalDeviceCharacteristics
	uint32_t selectedDevice = 0;
	vkPhysicalDevice = physicalDevices[selectedDevice];
	vkPhysicalDeviceCharacteristics = physicalDevicesCharact[selectedDevice];

	free(physicalDevices);

	for(uint32_t i = 0; i < physicalDeviceCount; i += 1){
		if(i != selectedDevice) {
			free(physicalDevicesCharact[i].queueFamilyProperties);
			free(physicalDevicesCharact[i].surfaceFormats);
			free(physicalDevicesCharact[i].presentModes);
		}
	}
	free(physicalDevicesCharact);

	/* Create logical device*/
	vkQueueFamilyIndexCount = vkPhysicalDeviceCharacteristics.queueFamilyPropertiesCount;

	VkQueueFamilyProperties *queueFamilyProperties = vkPhysicalDeviceCharacteristics.queueFamilyProperties;

	VkDeviceQueueCreateInfo *queueCreateInfo = calloc(vkQueueFamilyIndexCount, sizeof(*queueCreateInfo));
	CHECK_ALLOCATION_SUCCESSFUL(queueCreateInfo);
	uint32_t queueCreateInfoCount = 0;

	for(uint32_t i = 0; i < vkQueueFamilyIndexCount; i += 1){

		if(queueFamilyProperties[i].queueFlags && VK_QUEUE_GRAPHICS_BIT){
			queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo[queueCreateInfoCount].pNext = NULL;
			queueCreateInfo[queueCreateInfoCount].flags = 0;
			queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = i;
			queueCreateInfo[queueCreateInfoCount].queueCount = queueFamilyProperties[i].queueCount;

			float QueuePriority = 1.0f;	//TODO
			queueCreateInfo[queueCreateInfoCount].pQueuePriorities = &QueuePriority;

			queueCreateInfoCount += 1;
		}
	}

	const uint32_t enabledExtensionCount = 2;
	const char *enabledExtensionNames[2] = { VK_KHR_SURFACE_EXTENSION_NAME , VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	VkDeviceCreateInfo deviceCreateInfo = { 0 };
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = NULL;
	deviceCreateInfo.flags = 0;								//reserved for future uses
	deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 0;	  		 		//deprecated, should be the same as instance for compatibility
	deviceCreateInfo.ppEnabledLayerNames = NULL;			//deprecated, should be the same as instance for compatibility
	deviceCreateInfo.enabledExtensionCount = enabledExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;
	deviceCreateInfo.pEnabledFeatures = NULL;				//Features are GPU specific... maybe later

	vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, NULL, &vkDevice);

	free(queueCreateInfo);

	/* Get queues */
	vkQueueFamily = malloc(vkQueueFamilyIndexCount * sizeof(*vkQueueFamily));
	CHECK_ALLOCATION_SUCCESSFUL(vkQueueFamily);

	// i = QueueFamilyIndex
	// j = QueueIndex
	for(uint32_t i = 0; i < vkQueueFamilyIndexCount; i += 1){

		vkQueueFamily[i].queueCount = queueFamilyProperties[i].queueCount;

		vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, i,vkSurface,&(vkQueueFamily[i].canPresent));

		vkQueueFamily[i].queues = malloc(vkQueueFamily[i].queueCount * sizeof(*(vkQueueFamily[i].queues)));
		CHECK_ALLOCATION_SUCCESSFUL(vkQueueFamily);

		for(uint32_t j = 0; j < vkQueueFamily[i].queueCount; j += 1){

			vkGetDeviceQueue(vkDevice, i, j, vkQueueFamily[i].queues + j);

		}
	}
}

void createSwapChain(void){

	VkPresentModeKHR presentMode;
	vkGetPresentModeKHR();

	VkSwapchainCreateInfoKHR swapchainCreateInfo = { 0 };
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = NULL;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = vkSurface;
	swapchainCreateInfo.minImageCount = vkPhysicalDeviceCharacteristics.surfaceCapabilities.maxImageCount;

	swapchainCreateInfo.imageFormat = vkPhysicalDeviceCharacteristics.surfaceFormats[0].format;				//todo: Select the format
	swapchainCreateInfo.imageColorSpace = vkPhysicalDeviceCharacteristics.surfaceFormats[0].colorSpace;		//todo slect the format

	swapchainCreateInfo.imageExtent = vkPhysicalDeviceCharacteristics.surfaceCapabilities.maxImageExtent; 	//todo between maximgextent and minimgextent

	swapchainCreateInfo.imageArrayLayers = 1;

	swapchainCreateInfo.imageUsage = vkPhysicalDeviceCharacteristics.surfaceCapabilities.supportedUsageFlags; //Todo: chose what flag according to the supported flags

	/* TODO: if queue family can't present and do graphic in the same queue family -> sharing concurent
	 * Must say how many family index there is and what index will present and which one will do graphics */
	if(){
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = NULL;
	}
	else{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = NULL;
	}

	swapchainCreateInfo.preTransform = vkPhysicalDeviceCharacteristics.surfaceCapabilities.supportedTransforms; // Todo chose what transform according to the supported transforms, tutorial =

	swapchainCreateInfo.compositeAlpha = vkPhysicalDeviceCharacteristics.surfaceCapabilities.supportedCompositeAlpha; //Todo chose what what composite alpha according ot the supported, tutorial = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR

	swapchainCreateInfo.presentMode = vkPhysicalDeviceCharacteristics.presentModes[0];	//Todo: select present mode

	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	vkCreateSwapchainKHR(vkDevice, swapchainCreateInfo, NULL, &vkSwapchain);

}

void createCommandPools(void){

	vkCommandPools = malloc(vkQueueFamilyIndexCount * sizeof(*vkCommandPools));
	CHECK_ALLOCATION_SUCCESSFUL(vkCommandPools);

	for(uint32_t i = 0; i < vkQueueFamilyIndexCount; i += 1){

		VkCommandPoolCreateInfo commandPoolCreateInfo = { 0 };
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.pNext = NULL;
		commandPoolCreateInfo.flags =  0;	//See doc: can be: transient, resetable and protected
		commandPoolCreateInfo.queueFamilyIndex = i;

		vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, NULL, vkCommandPools + i);
	}

}

void createRenderPass(void){

	VkRenderPassCreateInfo renderPassCreateInfo = { 0 };
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = NULL;
	renderPassCreateInfo.flags = 0;
	//renderPassCreateInfo.attachmentCount;
	//renderPassCreateInfo.pAttachments;
	//renderPassCreateInfo.subpassCount;
	//renderPassCreateInfo.pSubpasses;
	//renderPassCreateInfo.dependencyCount;
	//renderPassCreateInfo.pDependencies;

	VkRenderPass vkRenderPass;
	vkCreateRenderPass(vkDevice, &renderPassCreateInfo, NULL, &vkRenderPass);

}

int vk_renderer_create(void){

	createInstance();
	createSurface();
	createDevicesAndQueues();
	createSwapChain();
	createCommandPools();
	createRenderPass();

	return 0;
}

int vk_renderer_destroy(void){

	free(vkCommandPools);

	vkDestroySwapchainKHR(vkDevice, vkSwapchain, NULL);

	free(vkPhysicalDeviceCharacteristics.queueFamilyProperties);
	for(uint32_t i = 0; i < vkQueueFamilyIndexCount; i += 1) free(vkQueueFamily[i].queues);
	free(vkQueueFamily);

	vkDestroyDevice(vkDevice,NULL);
	vkDestroySurfaceKHR(vkInstance, vkSurface, NULL);
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();
	vkDestroyInstance(vkInstance, NULL);
	return 0;
}
