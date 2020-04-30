#include "vkRenderer.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#define CHECK_ALLOCATION_SUCCESSFUL(var) if(!(var)) exit(-69);

static GLFWwindow *glfwWindow = NULL;
const uint32_t glfwScreenWidth = 640;
const uint32_t glfwScreenHeight = 320;

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
	VkBool32 canPresent;
	uint32_t queueCount;
	VkQueue *queues;
} *vkQueueFamily = NULL;
static uint32_t vkQueueFamilyIndexCount = 0;

static VkSurfaceKHR vkSurface;

static VkSwapchainKHR vkSwapchain;
static VkFormat vkSwapchainFormat;
static VkExtent2D vkSwapchainExtent;

static VkImage *vkSwapchainImages = NULL;
static VkImageView *vkSwapchainImageViews = NULL;
static uint32_t vkSwapchainImageCount = 0;

static VkRenderPass vkRenderPass;

static VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
static VkPipeline *vkPipelines = NULL;
static uint32_t vkPipelineCount = 0;
static const uint32_t vkGraphicPipelineIndex = 0;

static VkFramebuffer *vkSwapchainFramebuffers = NULL;
static uint32_t vkSwapchainFramebufferCount = 0;

static VkCommandPool *vkCommandPools = NULL;
static uint32_t vkCommandPoolCount = 0;

static VkCommandBuffer *vkCommandBuffers = NULL;
static uint32_t vkCommandBufferCount = 0;

static VkSemaphore vkImageAvailableSemaphore;
static VkSemaphore vkRenderFinishedSemaphore;

static void createInstance(void){

	VkApplicationInfo applicationInfo = { 0 };
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = NULL;
	applicationInfo.pApplicationName = applicationName;
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
	applicationInfo.pEngineName = NULL;
	applicationInfo.engineVersion = 0;
	applicationInfo.apiVersion = VK_API_VERSION_1_1;

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
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &(physicalDevicesCharact[i].queueFamilyPropertiesCount), physicalDevicesCharact[i].queueFamilyProperties);
		}

		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i], vkSurface, &(physicalDevicesCharact[i].surfaceFormatCount), NULL);
		if(physicalDevicesCharact[i].surfaceFormatCount != 0){
			physicalDevicesCharact[i].surfaceFormats = malloc(physicalDevicesCharact[i].surfaceFormatCount * sizeof(*(physicalDevicesCharact[i].surfaceFormats)));
			CHECK_ALLOCATION_SUCCESSFUL(physicalDevicesCharact[i].surfaceFormats);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i], vkSurface, &(physicalDevicesCharact[i].surfaceFormatCount), physicalDevicesCharact[i].surfaceFormats);
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

			//FIXME: Use me!!
			queueCreateInfoCount += 1;
		}
	}

	const uint32_t enabledExtensionCount = 1;
	const char *enabledExtensionNames[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

	VkResult res = vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, NULL, &vkDevice);

	free(queueCreateInfo);

	/* Get queues */
	vkQueueFamily = malloc(vkQueueFamilyIndexCount * sizeof(*vkQueueFamily));
	CHECK_ALLOCATION_SUCCESSFUL(vkQueueFamily);

	// i = QueueFamilyIndex
	// j = QueueIndex
	for(uint32_t i = 0; i < vkQueueFamilyIndexCount; i += 1){

		vkQueueFamily[i].queueCount = queueFamilyProperties[i].queueCount;

		vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, i, vkSurface, &(vkQueueFamily[i].canPresent));

		vkQueueFamily[i].queues = malloc(vkQueueFamily[i].queueCount * sizeof(*(vkQueueFamily[i].queues)));
		CHECK_ALLOCATION_SUCCESSFUL(vkQueueFamily[i].queues);

		for(uint32_t j = 0; j < vkQueueFamily[i].queueCount; j += 1){

			vkGetDeviceQueue(vkDevice, i, j, (vkQueueFamily[i].queues + j));

		}
	}
}


void createSwapChain(void){

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {0};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = NULL;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = vkSurface;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VkSurfaceFormatKHR *surfaceFormats = vkPhysicalDeviceCharacteristics.surfaceFormats;
	uint32_t surfaceFormatCount = vkPhysicalDeviceCharacteristics.surfaceFormatCount;

	VkBool32 foundFormat = VK_FALSE;
	for(uint32_t i = 0; (i < surfaceFormatCount) || (foundFormat == VK_FALSE); i += 1){
		if(surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			swapchainCreateInfo.imageFormat = surfaceFormats[i].format;
			vkSwapchainFormat = surfaceFormats[i].format;
			swapchainCreateInfo.imageColorSpace = surfaceFormats[i].colorSpace;
			foundFormat = VK_TRUE;
		}
	}
	if(foundFormat == VK_FALSE){
		swapchainCreateInfo.imageFormat = surfaceFormats[0].format;
		vkSwapchainFormat = surfaceFormats[0].format;
		swapchainCreateInfo.imageColorSpace = surfaceFormats[0].colorSpace;
	}

	VkSurfaceCapabilitiesKHR surfaceCapabilities = vkPhysicalDeviceCharacteristics.surfaceCapabilities;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;

	if(surfaceCapabilities.currentExtent.width != 0xFFFFFFFF && surfaceCapabilities.currentExtent.height != 0xFFFFFFFF){
		swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
		vkSwapchainExtent = surfaceCapabilities.currentExtent;;
	}
	else{
		VkExtent2D actualExtent;

		if(glfwScreenWidth > surfaceCapabilities.maxImageExtent.width) actualExtent.width = surfaceCapabilities.maxImageExtent.width;
		else if(glfwScreenWidth < surfaceCapabilities.minImageExtent.width) actualExtent.width = surfaceCapabilities.minImageExtent.width;
		else actualExtent.width = glfwScreenWidth;

		if(glfwScreenHeight > surfaceCapabilities.maxImageExtent.height) actualExtent.height = surfaceCapabilities.maxImageExtent.height;
		else if(glfwScreenHeight < surfaceCapabilities.minImageExtent.height) actualExtent.height = surfaceCapabilities.minImageExtent.height;
		else actualExtent.height = glfwScreenHeight;

		swapchainCreateInfo.imageExtent = actualExtent;
		vkSwapchainExtent = actualExtent;
	}

	/* Todo later: Get the supported usage and create UI to select*/
	swapchainCreateInfo.imageUsage = surfaceCapabilities.supportedUsageFlags & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;	//supported transform
	swapchainCreateInfo.compositeAlpha = surfaceCapabilities.supportedCompositeAlpha & (VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

	/* TODO: if queue family can't present and do graphic in the same queue family -> sharing concurent
	 * Must say how many family index there is and what index will present and which one will do graphics */
	if(0){
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = NULL;
	}
	else{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = NULL;
	}

	VkPresentModeKHR *presentModes = vkPhysicalDeviceCharacteristics.presentModes;
	uint32_t presentModeCount = vkPhysicalDeviceCharacteristics.presentModeCount;
	VkBool32 presentModeFound = VK_FALSE;

	for(uint32_t i = 0; (i < presentModeCount) || (presentModeFound == VK_FALSE); i += 1){
		if(presentModes[i] == VK_PRESENT_MODE_FIFO_KHR){
			swapchainCreateInfo.presentMode = presentModes[i];
			presentModeFound = VK_TRUE;
		}
	}
	if(presentModeFound == VK_FALSE) swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

	vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, NULL, &vkSwapchain);

	vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &vkSwapchainImageCount, NULL);
	vkSwapchainImages = malloc(vkSwapchainImageCount * sizeof(*vkSwapchainImages));
	CHECK_ALLOCATION_SUCCESSFUL(vkSwapchainImages);
	vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &vkSwapchainImageCount, vkSwapchainImages);

	vkSwapchainImageViews = malloc(vkSwapchainImageCount * sizeof(*vkSwapchainImageViews));
	CHECK_ALLOCATION_SUCCESSFUL(vkSwapchainImageViews);

	for(uint32_t i = 0; i < vkSwapchainImageCount; i += 1){
		VkImageViewCreateInfo imageViewCreateInfo = { 0 };
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = NULL;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = vkSwapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = swapchainCreateInfo.imageFormat;

		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(vkDevice, &imageViewCreateInfo, NULL, &(vkSwapchainImageViews[i]));
	}
}

void createRenderPass(void){

	VkAttachmentDescription colorAttachmentDescription = {0};
	colorAttachmentDescription.flags = 0;
	colorAttachmentDescription.format = vkSwapchainFormat;
	colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	// Operation before starting to load our framebuffer
	colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Content of the framebuffer will be stored after the render
	colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference = {0};
	colorAttachmentReference.attachment = 0;	//Index of color attachmentDescription
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {0};
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = 0;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = NULL;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.pResolveAttachments = NULL;
	subpassDescription.pDepthStencilAttachment = NULL;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = NULL;

	VkSubpassDependency subpassDependency = {0};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency.dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassCreateInfo = { 0 };
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = NULL;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	vkCreateRenderPass(vkDevice, &renderPassCreateInfo, NULL, &vkRenderPass);
}

VkResult createShaderModule(const char *codePath, VkShaderModule *shaderModule){

	FILE *f = fopen(codePath, "rb");
	if(!f) return VK_ERROR_OUT_OF_HOST_MEMORY;

	fseek(f,0,SEEK_END);
	size_t codeSize = (size_t) ftell(f);
	fseek(f,0,SEEK_SET);

	uint32_t *pCode = malloc(codeSize * sizeof(*pCode));
	if(!pCode) return VK_ERROR_OUT_OF_HOST_MEMORY;

	fread(pCode, sizeof(*pCode), (codeSize)/sizeof(*pCode), f);

	fclose(f);

	VkShaderModuleCreateInfo shaderCreateInfo = {0};
	shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderCreateInfo.pNext = NULL;
	shaderCreateInfo.flags = 0;
	shaderCreateInfo.codeSize = codeSize;
	shaderCreateInfo.pCode = pCode;

	VkResult res = vkCreateShaderModule(vkDevice, &shaderCreateInfo, NULL, shaderModule);

	free(pCode);

	return res;
}

void createGraphicsPipeline(void){

	VkShaderModule vertShaderModule, fragShaderModule;

	createShaderModule("spir-v/vert.spv", &vertShaderModule);
	createShaderModule("spir-v/frag.spv", &fragShaderModule);

	VkPipelineShaderStageCreateInfo vertPipelineShaderStageCreateInfo = {0};
	vertPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertPipelineShaderStageCreateInfo.pNext = NULL;
	vertPipelineShaderStageCreateInfo.flags = 0;
	vertPipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertPipelineShaderStageCreateInfo.module = vertShaderModule;
	vertPipelineShaderStageCreateInfo.pName = "main";
	vertPipelineShaderStageCreateInfo.pSpecializationInfo = NULL;

	VkPipelineShaderStageCreateInfo fragPipelineShaderStageCreateInfo = {0};
	fragPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragPipelineShaderStageCreateInfo.pNext = NULL;
	fragPipelineShaderStageCreateInfo.flags = 0;
	fragPipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragPipelineShaderStageCreateInfo.module = fragShaderModule;
	fragPipelineShaderStageCreateInfo.pName = "main";
	fragPipelineShaderStageCreateInfo.pSpecializationInfo = NULL;

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertPipelineShaderStageCreateInfo, fragPipelineShaderStageCreateInfo};

	/* Input state */
	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {0};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineVertexInputStateCreateInfo.pNext = NULL;
	pipelineVertexInputStateCreateInfo.flags = 0;
	pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
	pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = NULL;
	pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
	pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = NULL;

	/* Input assembly */
	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {0};
	pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineInputAssemblyStateCreateInfo.pNext = NULL;
	pipelineInputAssemblyStateCreateInfo.flags = 0;
	pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	/* View port*/
	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) glfwScreenWidth;
	viewport.height = (float) glfwScreenHeight;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {0};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = vkSwapchainExtent;

	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {0};
	pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.pNext = NULL;
	pipelineViewportStateCreateInfo.flags = 0;
	pipelineViewportStateCreateInfo.viewportCount = 1;
	pipelineViewportStateCreateInfo.pViewports = &viewport;
	pipelineViewportStateCreateInfo.scissorCount = 1;
	pipelineViewportStateCreateInfo.pScissors = &scissor;

	/* Rasterizer */
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {0};
	pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineRasterizationStateCreateInfo.pNext = NULL;
	pipelineRasterizationStateCreateInfo.flags = 0;
	pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

	/* Multisampling */
	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {0};
	pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMultisampleStateCreateInfo.pNext = NULL;
	pipelineMultisampleStateCreateInfo.flags = 0;
	pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	pipelineMultisampleStateCreateInfo.minSampleShading = 1.0f;
	pipelineMultisampleStateCreateInfo.pSampleMask = NULL;
	pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	/* Colorblend */
	VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState = {0};
	PipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
	PipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	PipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	PipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	PipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	PipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	PipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	PipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	// Note: Blend either with logicOpEnable or in pipeline ColorBlendAttachmentState, logicOpEnable priority
	VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo = {0};
	PipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	PipelineColorBlendStateCreateInfo.pNext = NULL;
	PipelineColorBlendStateCreateInfo.flags = 0;
	PipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	PipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_AND;
	PipelineColorBlendStateCreateInfo.attachmentCount = 1;
	PipelineColorBlendStateCreateInfo.pAttachments = &PipelineColorBlendAttachmentState;
	PipelineColorBlendStateCreateInfo.blendConstants[0] = 1.0f;	//R
	PipelineColorBlendStateCreateInfo.blendConstants[1] = 1.0f;	//G
	PipelineColorBlendStateCreateInfo.blendConstants[2] = 1.0f;	//B
	PipelineColorBlendStateCreateInfo.blendConstants[3] = 1.0f; //A

	/* Pipeline layout */
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {0};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = NULL;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = NULL;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

	vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, NULL, &vkPipelineLayout);

	/* Pipeline */
	VkGraphicsPipelineCreateInfo graphicPipelineCreateInfo;
	graphicPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicPipelineCreateInfo.pNext = NULL;
	graphicPipelineCreateInfo.flags = 0;
	graphicPipelineCreateInfo.stageCount = 2;
	graphicPipelineCreateInfo.pStages = shaderStages;
	graphicPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
	graphicPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
	graphicPipelineCreateInfo.pTessellationState = NULL;
	graphicPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
	graphicPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
	graphicPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	graphicPipelineCreateInfo.pDepthStencilState = NULL;
	graphicPipelineCreateInfo.pColorBlendState = &PipelineColorBlendStateCreateInfo;
	graphicPipelineCreateInfo.pDynamicState = NULL;	// Can modify some of the structs without recreating the whole pipeline
	graphicPipelineCreateInfo.layout = vkPipelineLayout;
	graphicPipelineCreateInfo.renderPass = vkRenderPass;
	graphicPipelineCreateInfo.subpass = 0;
	graphicPipelineCreateInfo.basePipelineIndex = -1;
	graphicPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkGraphicsPipelineCreateInfo pipelineCreateInfos[] = { graphicPipelineCreateInfo };
	vkPipelineCount = sizeof(pipelineCreateInfos)/sizeof(*pipelineCreateInfos);

	vkPipelines = malloc(vkPipelineCount * sizeof(*vkPipelines));
	CHECK_ALLOCATION_SUCCESSFUL(vkPipelines);

	vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, vkPipelineCount, pipelineCreateInfos, NULL, vkPipelines);

	vkDestroyShaderModule(vkDevice, vertShaderModule, NULL);
	vkDestroyShaderModule(vkDevice, fragShaderModule, NULL);
}

void createFramebuffers(void){

	vkSwapchainFramebufferCount = vkSwapchainImageCount;
	vkSwapchainFramebuffers = malloc(vkSwapchainImageCount * sizeof(*vkSwapchainFramebuffers));
	CHECK_ALLOCATION_SUCCESSFUL(vkSwapchainFramebuffers);

	for(uint32_t i = 0; i < vkSwapchainFramebufferCount; i += 1){

		VkFramebufferCreateInfo framebufferCreateInfo = {0};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = NULL;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = vkRenderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = (vkSwapchainImageViews + i);
		framebufferCreateInfo.width = vkSwapchainExtent.width;
		framebufferCreateInfo.height = vkSwapchainExtent.height;
		framebufferCreateInfo.layers = 1;

		vkCreateFramebuffer(vkDevice, &framebufferCreateInfo, NULL, vkSwapchainFramebuffers + i);
	}
}

void createCommands(void){

	/* Command pools */
	vkCommandPoolCount = vkQueueFamilyIndexCount;

	vkCommandPools = malloc(vkCommandPoolCount * sizeof(*vkCommandPools));
	CHECK_ALLOCATION_SUCCESSFUL(vkCommandPools);

	for(uint32_t i = 0; i < vkCommandPoolCount; i += 1){

		VkCommandPoolCreateInfo commandPoolCreateInfo = { 0 };
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.pNext = NULL;
		commandPoolCreateInfo.flags =  0;	//See doc: can be: transient, resetable and protected
		commandPoolCreateInfo.queueFamilyIndex = i;

		vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, NULL, vkCommandPools + i);
	}

	/* Command Buffers */
	vkCommandBufferCount = vkSwapchainFramebufferCount;

	vkCommandBuffers = malloc(vkCommandBufferCount * sizeof(*vkCommandBuffers));
	CHECK_ALLOCATION_SUCCESSFUL(vkCommandBuffers);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = { 0 };
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = NULL;
	commandBufferAllocateInfo.commandPool = vkCommandPools[0];
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = vkCommandBufferCount;

	vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, vkCommandBuffers);

	/* Command recording */
	for(uint32_t i = 0; i < vkCommandBufferCount; i += 1){
		VkCommandBufferBeginInfo commandBeginInfo = { 0 };
		commandBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBeginInfo.pNext = NULL;
		commandBeginInfo.flags = 0;
		commandBeginInfo.pInheritanceInfo = NULL;

		vkBeginCommandBuffer(vkCommandBuffers[i], &commandBeginInfo);

		VkRenderPassBeginInfo renderPassBeginInfo = { 0 };
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = NULL;
		renderPassBeginInfo.renderPass = vkRenderPass;
		renderPassBeginInfo.framebuffer = vkSwapchainFramebuffers[i];
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent = vkSwapchainExtent;

		VkClearValue clearColor;
		clearColor.color.float32[0] = 0.0f;
		clearColor.color.float32[1] = 0.0f;
		clearColor.color.float32[2] = 0.0f;
		clearColor.color.float32[3] = 1.0f;

		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(vkCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelines[vkGraphicPipelineIndex]);

		vkCmdDraw(vkCommandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(vkCommandBuffers[i]);

		if(vkEndCommandBuffer(vkCommandBuffers[i]) != VK_SUCCESS) printf("Error while drawing");
	}
}

void createSemaphores(void){

	VkSemaphoreCreateInfo semaphoreCreateInfo = {0};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = NULL;
	semaphoreCreateInfo.flags = 0;

	vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NULL, &vkImageAvailableSemaphore);
	vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NULL, &vkRenderFinishedSemaphore);

}

int vk_renderer_create(void){

	glfwInit(); // Todo: Create a window module
	createInstance();
	createSurface();
	createDevicesAndQueues();
	createSwapChain();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommands();
	createSemaphores();

	return 0;
}

int vk_drawFrame(void){

	glfwPollEvents();

	uint32_t imageIndex;
	vkAcquireNextImageKHR(vkDevice, vkSwapchain, UINT64_MAX, vkImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkSemaphore waitSemaphores[] = {vkImageAvailableSemaphore};
	uint32_t waitSemaphoreCount = sizeof(waitSemaphores)/sizeof(*waitSemaphores);

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	VkSemaphore signalSemaphores[] = {vkRenderFinishedSemaphore};
	uint32_t signalSemaphoreCount = sizeof(signalSemaphores)/sizeof(*signalSemaphores);

	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = waitSemaphoreCount;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = vkCommandBufferCount;
	submitInfo.pCommandBuffers = vkCommandBuffers;
	submitInfo.signalSemaphoreCount = signalSemaphoreCount;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkQueueSubmit(vkQueueFamily[0].queues[0], 1, &submitInfo, VK_NULL_HANDLE);

	VkSwapchainKHR swapchains[] = {vkSwapchain};
	uint32_t swapchainCount = sizeof(swapchains)/sizeof(*swapchains);

	VkPresentInfoKHR presentInfo = {0};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.waitSemaphoreCount = signalSemaphoreCount;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = swapchainCount;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = NULL;

	vkQueuePresentKHR(vkQueueFamily[0].queues[1], &presentInfo);

	vkQueueWaitIdle(vkQueueFamily[0].queues[1]);

	return !glfwWindowShouldClose(glfwWindow);
}

int vk_renderer_destroy(void){


	vkDestroySemaphore(vkDevice, vkImageAvailableSemaphore, NULL);
	vkDestroySemaphore(vkDevice, vkRenderFinishedSemaphore, NULL);

	free(vkCommandBuffers);
	free(vkCommandPools);

	for(uint32_t i = 0; i < vkSwapchainFramebufferCount; i += 1) vkDestroyFramebuffer(vkDevice, vkSwapchainFramebuffers[i], NULL);
	free(vkSwapchainFramebuffers);

	for(uint32_t i = 0; i < vkPipelineCount; i += 1) vkDestroyPipeline(vkDevice, vkPipelines[i], NULL);
	free(vkPipelines);

	vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, NULL);

	vkDestroyRenderPass(vkDevice, vkRenderPass, NULL);

	for(uint32_t i = 0; i < vkPipelineCount; i += 1) vkDestroyPipeline(vkDevice, vkPipelines[i], NULL);

	for(uint32_t i = 0; i < vkSwapchainImageCount; i += 1) vkDestroyImageView(vkDevice, vkSwapchainImageViews[i], NULL);
	free(vkSwapchainImageViews);
	free(vkSwapchainImages);

	vkDestroySwapchainKHR(vkDevice, vkSwapchain, NULL);

	free(vkPhysicalDeviceCharacteristics.queueFamilyProperties);
	for(uint32_t i = 0; i < vkQueueFamilyIndexCount; i += 1) free(vkQueueFamily[i].queues);
	free(vkQueueFamily);

	vkDestroyDevice(vkDevice,NULL);
	vkDestroySurfaceKHR(vkInstance, vkSurface, NULL);

	glfwDestroyWindow(glfwWindow);	// Todo: Create a window module
	glfwTerminate();

	vkDestroyInstance(vkInstance, NULL);

	return 0;
}
