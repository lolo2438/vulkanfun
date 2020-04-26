#include "vkrend.h"

#define VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
//#include <vulkan_xcb.h>
#include <vulkan.h>

#include <stdio.h>
#include <stdlib.h>

#define CHECK_ALLOCATION_SUCCESSFUL(var) if(!(var)) return;

static VkInstance vkInstance = VK_NULL_HANDLE;
static VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
static VkDevice vkDevice = VK_NULL_HANDLE;

static struct vkQueue{
	VkQueue queue;
	uint32_t queueFamilyIndex;
	uint32_t queueIndex;
} *vkQueue = NULL;
static uint32_t vkQueueCount = 0;
static uint32_t vkQueueFamilyCount = 0;

static VkCommandPool *vkCommandPools = NULL;

static VkSurfaceKHR vkSurface;

static void createInstance(void){

	VkApplicationInfo applicationInfo = { 0 };
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = NULL;
	applicationInfo.pApplicationName = "vkfun";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
	applicationInfo.pEngineName = NULL;
	applicationInfo.engineVersion = 0;
	applicationInfo.apiVersion = VK_MAKE_VERSION(1,1,119);

	const uint32_t enabledExtensionCount = 1;
	const char *enabledExtensionNames[1] = { VK_KHR_SURFACE_EXTENSION_NAME };

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

	//TODO: use a library for cross platform, maybe glfw
	xcb_connection_t *conn = xcb_connect(NULL, NULL);
	if(!conn) return;

	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

	xcb_window_t window = xcb_generate_id(conn);

	xcb_create_window(conn,                          /* Connection          */
					  XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
					  window,                        /* window Id           */
					  screen->root,                  /* parent window       */
					  0, 0,                          /* x, y                */
					  150, 150,                      /* width, height       */
					  10,                            /* border_width        */
					  XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
					  screen->root_visual,           /* visual              */
					  0, NULL);                      /* masks, not used yet */

	xcb_map_window (conn, window);
	xcb_flush(conn);

	VkXcbSurfaceCreateInfoKHR xcbSurfaceCreateInfo = { 0 };
	xcbSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	xcbSurfaceCreateInfo.pNext = NULL;
	xcbSurfaceCreateInfo.flags = 0;
	xcbSurfaceCreateInfo.connection = conn;
	xcbSurfaceCreateInfo.window = window;

	vkCreateXcbSurfaceKHR(vkInstance, &xcbSurfaceCreateInfo, NULL, &vkSurface);
}

static void createDevicesAndQueues(void){

	/* Select physical device */
	uint32_t physicalDeviceCount;

	vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, NULL);

	VkPhysicalDevice *physicalDevices = malloc(physicalDeviceCount * sizeof(*physicalDevices));
	CHECK_ALLOCATION_SUCCESSFUL(physicalDevices);

	vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, physicalDevices);

	/* TODO: Pick a physical device if it can compute graphics and use a priority list for the type of gpu
	 * TODO: Check if physical device api version is the same as application api version */
	vkPhysicalDevice = physicalDevices[0];
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(vkPhysicalDevice, &physicalDeviceProperties);

	// Get queue info from the device
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, NULL);

	VkQueueFamilyProperties *queueProperties = malloc(vkQueueFamilyCount * sizeof(*queueProperties));
	CHECK_ALLOCATION_SUCCESSFUL(queueProperties);

	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, queueProperties);

	VkDeviceQueueCreateInfo *queueCreateInfo = calloc(vkQueueFamilyCount, sizeof(*queueCreateInfo));
	CHECK_ALLOCATION_SUCCESSFUL(queueCreateInfo);
	uint32_t queueCreateInfoCount = 0;

	for(uint32_t i = 0; i < vkQueueFamilyCount; i += 1){

		//NOTE: On peut aussi evaluer la performance de la queue family avec vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR
		if(queueProperties[i].queueFlags && VK_QUEUE_GRAPHICS_BIT){
			queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo[queueCreateInfoCount].pNext = NULL;
			queueCreateInfo[queueCreateInfoCount].flags = 0;
			queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = i;
			queueCreateInfo[queueCreateInfoCount].queueCount = queueProperties[i].queueCount;

			float QueuePriority = 1.0f;	//todo: algorithme pour décider de la priorite
			queueCreateInfo[queueCreateInfoCount].pQueuePriorities = &QueuePriority;

			queueCreateInfoCount += 1;
		}
	}

	const uint32_t enabledExtensionCount = 1;
	const char *enabledExtensionNames[1] = { VK_KHR_SURFACE_EXTENSION_NAME };

	/* Create logical device*/
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

	/* Get queues */
	vkQueueCount = 0;
	for(uint32_t i = 0; i < queueCreateInfoCount; i += 1){
		vkQueueCount += queueCreateInfo[i].queueCount;
	}

	vkQueue = malloc(vkQueueCount * sizeof(*vkQueue));
	CHECK_ALLOCATION_SUCCESSFUL(vkQueue);

	uint32_t queueCountValidation = 0;
	for(uint32_t i = 0; i < queueCreateInfoCount; i += 1){
		for(uint32_t j = 0; j < queueCreateInfo[i].queueCount; j += 1){
			vkQueue[i].queueFamilyIndex = queueCreateInfo[i].queueFamilyIndex;
			vkQueue[i].queueIndex = j;
			vkGetDeviceQueue(vkDevice, vkQueue[i].queueFamilyIndex, vkQueue[i].queueIndex, &(vkQueue[i].queue));
			queueCountValidation += 1;
		}
	}

	if(queueCountValidation != vkQueueCount) return;

	// FIXME: If fail these ressources won't get freed.
	free(queueCreateInfo);
	free(queueProperties);
	free(physicalDevices);
}

void createCommandPools(void){

	vkCommandPools = malloc(vkQueueFamilyCount * sizeof(*vkCommandPools));
	CHECK_ALLOCATION_SUCCESSFUL(vkCommandPools);

	for(uint32_t i = 0; i < vkQueueFamilyCount; i += 1){

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
	renderPassCreateInfo.attachmentCount;
	renderPassCreateInfo.pAttachments;
	renderPassCreateInfo.subpassCount;
	renderPassCreateInfo.pSubpasses;
	renderPassCreateInfo.dependencyCount;
	renderPassCreateInfo.pDependencies;

	VkRenderPass vkRenderPass;
	vkCreateRenderPass(vkDevice, &renderPassCreateInfo, NULL, &vkRenderPass);

}

int vk_renderer_create(void){

	createInstance();
	createSurface();
	createDevicesAndQueues();
	createCommandPools();
	createRenderPass();

	return 0;
}

int vk_renderer_destroy(void){

	free(vkCommandPools);
	free(vkQueue);
	vkDestroyDevice(vkDevice,NULL);
	vkDestroySurfaceKHR(vkInstance, vkSurface, NULL);
	vkDestroyInstance(vkInstance, NULL);
	return 0;
}
