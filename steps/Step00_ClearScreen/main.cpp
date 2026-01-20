#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <VgtConfig.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::fprintf(stderr, "[Validation] %s\n", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

static std::vector<const char*> GetRequiredInstanceExtensions()
{
    uint32_t count = 0;
    const char** glfwExt = glfwGetRequiredInstanceExtensions(&count);

    std::vector<const char*> exts;
    exts.reserve(count + 1);
    for (uint32_t i = 0; i < count; ++i)
        exts.push_back(glfwExt[i]);

    // Debug utils for validation messages.
    exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return exts;
}

int main()
{
    if (!glfwInit())
    {
        std::fprintf(stderr, "Failed to init GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Step00_ClearScreen", nullptr, nullptr);
    if (!window)
    {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    // Instance
    VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "vulkan-glsl-tutorial";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "none";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> instanceExtensions = GetRequiredInstanceExtensions();

    std::vector<const char*> validationLayers;
#if VGT_ENABLE_VALIDATION
    validationLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    VkInstanceCreateInfo instanceCI{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instanceCI.pApplicationInfo = &appInfo;
    instanceCI.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCI.ppEnabledExtensionNames = instanceExtensions.data();
    instanceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    instanceCI.ppEnabledLayerNames = validationLayers.empty() ? nullptr : validationLayers.data();

    VkInstance instance = VK_NULL_HANDLE;
    VkResult res = vkCreateInstance(&instanceCI, nullptr, &instance);
    if (res != VK_SUCCESS)
    {
        std::fprintf(stderr, "vkCreateInstance failed: %d\n", res);
        return 1;
    }

    // Debug messenger
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT_ =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT_ =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

#if VGT_ENABLE_VALIDATION
    if (vkCreateDebugUtilsMessengerEXT_)
    {
        VkDebugUtilsMessengerCreateInfoEXT dbgCI{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        dbgCI.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbgCI.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbgCI.pfnUserCallback = &DebugCallback;

        vkCreateDebugUtilsMessengerEXT_(instance, &dbgCI, nullptr, &debugMessenger);
    }
#endif

    // Surface (Win32 via GLFW)
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (res != VK_SUCCESS)
    {
        std::fprintf(stderr, "glfwCreateWindowSurface failed: %d\n", res);
        return 1;
    }

    // Physical device
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
    if (gpuCount == 0)
    {
        std::fprintf(stderr, "No Vulkan physical devices found\n");
        return 1;
    }
    std::vector<VkPhysicalDevice> gpus(gpuCount);
    vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data());
    VkPhysicalDevice physicalDevice = gpus[0];

    // Queue family selection: graphics + present
    uint32_t qCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> qProps(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qCount, qProps.data());

    uint32_t graphicsQ = UINT32_MAX;
    uint32_t presentQ = UINT32_MAX;
    for (uint32_t i = 0; i < qCount; ++i)
    {
        if ((qProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && graphicsQ == UINT32_MAX)
            graphicsQ = i;

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport && presentQ == UINT32_MAX)
            presentQ = i;
    }

    if (graphicsQ == UINT32_MAX || presentQ == UINT32_MAX)
    {
        std::fprintf(stderr, "No suitable queue family found\n");
        return 1;
    }

    float qPriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCIs;
    {
        VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        qci.queueFamilyIndex = graphicsQ;
        qci.queueCount = 1;
        qci.pQueuePriorities = &qPriority;
        queueCIs.push_back(qci);

        if (presentQ != graphicsQ)
        {
            VkDeviceQueueCreateInfo pqci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            pqci.queueFamilyIndex = presentQ;
            pqci.queueCount = 1;
            pqci.pQueuePriorities = &qPriority;
            queueCIs.push_back(pqci);
        }
    }

    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCI{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceCI.queueCreateInfoCount = static_cast<uint32_t>(queueCIs.size());
    deviceCI.pQueueCreateInfos = queueCIs.data();
    deviceCI.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCI.pEnabledFeatures = &deviceFeatures;

    // For older loaders, enabling layers on the device can help tooling.
    deviceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    deviceCI.ppEnabledLayerNames = validationLayers.empty() ? nullptr : validationLayers.data();

    VkDevice device = VK_NULL_HANDLE;
    res = vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device);
    if (res != VK_SUCCESS)
    {
        std::fprintf(stderr, "vkCreateDevice failed: %d\n", res);
        return 1;
    }

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, graphicsQ, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQ, 0, &presentQueue);

    // Swapchain
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& f : formats)
    {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surfaceFormat = f;
            break;
        }
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto m : presentModes)
    {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = m;
            break;
        }
    }

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX)
    {
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        extent.width = static_cast<uint32_t>(w);
        extent.height = static_cast<uint32_t>(h);
    }

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR swapCI{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapCI.surface = surface;
    swapCI.minImageCount = imageCount;
    swapCI.imageFormat = surfaceFormat.format;
    swapCI.imageColorSpace = surfaceFormat.colorSpace;
    swapCI.imageExtent = extent;
    swapCI.imageArrayLayers = 1;
    swapCI.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapCI.preTransform = caps.currentTransform;
    swapCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapCI.presentMode = presentMode;
    swapCI.clipped = VK_TRUE;

    uint32_t qFamilyIndices[] = { graphicsQ, presentQ };
    if (graphicsQ != presentQ)
    {
        swapCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapCI.queueFamilyIndexCount = 2;
        swapCI.pQueueFamilyIndices = qFamilyIndices;
    }
    else
    {
        swapCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    res = vkCreateSwapchainKHR(device, &swapCI, nullptr, &swapchain);
    if (res != VK_SUCCESS)
    {
        std::fprintf(stderr, "vkCreateSwapchainKHR failed: %d\n", res);
        return 1;
    }

    uint32_t swapImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, nullptr);
    std::vector<VkImage> swapImages(swapImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, swapImages.data());

    // Command pool / buffer
    VkCommandPoolCreateInfo poolCI{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCI.queueFamilyIndex = graphicsQ;

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    vkCreateCommandPool(device, &poolCI, nullptr, &cmdPool);

    VkCommandBufferAllocateInfo cmdAI{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdAI.commandPool = cmdPool;
    cmdAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAI.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device, &cmdAI, &cmd);

    // Sync
    VkSemaphoreCreateInfo semCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence inFlight = VK_NULL_HANDLE;

    vkCreateSemaphore(device, &semCI, nullptr, &imageAvailable);
    vkCreateSemaphore(device, &semCI, nullptr, &renderFinished);
    vkCreateFence(device, &fenceCI, nullptr, &inFlight);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        vkWaitForFences(device, 1, &inFlight, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlight);

        uint32_t imageIndex = 0;
        res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);
        if (res != VK_SUCCESS)
            break;

        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.levelCount = 1;
        range.layerCount = 1;

        // Transition: undefined -> transfer dst
        VkImageMemoryBarrier toTransfer{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.image = swapImages[imageIndex];
        toTransfer.subresourceRange = range;
        toTransfer.srcAccessMask = 0;
        toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &toTransfer);

        VkClearColorValue clearColor{ {0.1f, 0.2f, 0.4f, 1.0f} };
        vkCmdClearColorImage(cmd, swapImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

        // Transition: transfer dst -> present
        VkImageMemoryBarrier toPresent{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toPresent.image = swapImages[imageIndex];
        toPresent.subresourceRange = range;
        toPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toPresent.dstAccessMask = 0;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &toPresent);

        vkEndCommandBuffer(cmd);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &imageAvailable;
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &renderFinished;

        vkQueueSubmit(graphicsQueue, 1, &submit, inFlight);

        VkPresentInfoKHR present{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &renderFinished;
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &imageIndex;
        vkQueuePresentKHR(presentQueue, &present);
    }

    vkDeviceWaitIdle(device);

    vkDestroyFence(device, inFlight, nullptr);
    vkDestroySemaphore(device, renderFinished, nullptr);
    vkDestroySemaphore(device, imageAvailable, nullptr);

    vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
    vkDestroyCommandPool(device, cmdPool, nullptr);

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

#if VGT_ENABLE_VALIDATION
    if (vkDestroyDebugUtilsMessengerEXT_ && debugMessenger)
        vkDestroyDebugUtilsMessengerEXT_(instance, debugMessenger, nullptr);
#endif

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
