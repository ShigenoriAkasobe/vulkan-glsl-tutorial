#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>

#include <Windows.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <VgtConfig.h>

static void PrintVkResult(const char* what, VkResult res)
{
    if (res == VK_SUCCESS)
        return;
    std::fprintf(stderr, "%s failed: VkResult=%d\n", what, static_cast<int>(res));
}

static void ShowFatal(const char* msg)
{
    std::fprintf(stderr, "%s\n", msg);
    MessageBoxA(nullptr, msg, "Step01_MinimalTriangle", MB_OK | MB_ICONERROR);
}

static std::vector<uint32_t> ReadSpirvFile(const char* path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return {};

    const std::streamsize size = file.tellg();
    if (size <= 0)
        return {};

    std::vector<uint32_t> data(static_cast<size_t>(size / 4));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

static std::vector<uint32_t> ReadSpirvWithFallback(const char* relativePath)
{
    // Try runtime layout first (when shaders are copied next to the exe).
    {
        auto data = ReadSpirvFile(relativePath);
        if (!data.empty())
            return data;
    }

    // Fallback: locate shader outputs relative to the executable.
    // This handles cases where the working directory is not the exe directory.
    {
        char exePath[MAX_PATH] = {};
        const DWORD len = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        if (len > 0 && len < MAX_PATH)
        {
            std::string exeDir(exePath);
            const size_t lastSlash = exeDir.find_last_of("\\/");
            if (lastSlash != std::string::npos)
                exeDir.resize(lastSlash + 1);

            std::string fromExeDir = exeDir + std::string("compiled_shaders/") + relativePath;
            {
                auto data = ReadSpirvFile(fromExeDir.c_str());
                if (!data.empty())
                    return data;
            }

            // Common MSBuild layout: <target>/Debug/.. == <target>/
            std::string exeParentDir = exeDir;
            if (!exeParentDir.empty())
            {
                // remove trailing slash
                while (!exeParentDir.empty() && (exeParentDir.back() == '\\' || exeParentDir.back() == '/'))
                    exeParentDir.pop_back();
                const size_t parentSlash = exeParentDir.find_last_of("\\/");
                if (parentSlash != std::string::npos)
                {
                    exeParentDir.resize(parentSlash + 1);
                    std::string fromExeParentDir = exeParentDir + std::string("compiled_shaders/") + relativePath;
                    {
                        auto data = ReadSpirvFile(fromExeParentDir.c_str());
                        if (!data.empty())
                            return data;
                    }
                }
            }
        }
    }

    // Fallback: run from build tree.
    // e.g. build-ninja/.../steps/Step01_MinimalTriangle/compiled_shaders
    std::string alt = std::string("compiled_shaders/") + relativePath;
    {
        auto data = ReadSpirvFile(alt.c_str());
        if (!data.empty())
            return data;
    }

    return {};
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::fprintf(stderr, "[Validation] %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

int main()
{
    bool pauseOnExit = false;
    {
        char* val = nullptr;
        size_t len = 0;
        if (_dupenv_s(&val, &len, "VGT_PAUSE_ON_EXIT") == 0 && val != nullptr)
        {
            pauseOnExit = true;
            std::free(val);
        }
    }

    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Step01_MinimalTriangle", nullptr, nullptr);
    if (!window)
        return 1;

    // Instance
    VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "vulkan-glsl-tutorial";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> instanceExts(glfwExts, glfwExts + glfwExtCount);
    instanceExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    std::vector<const char*> layers;
#if VGT_ENABLE_VALIDATION
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    VkInstanceCreateInfo instanceCI{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instanceCI.pApplicationInfo = &appInfo;
    instanceCI.enabledExtensionCount = static_cast<uint32_t>(instanceExts.size());
    instanceCI.ppEnabledExtensionNames = instanceExts.data();
    instanceCI.enabledLayerCount = static_cast<uint32_t>(layers.size());
    instanceCI.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();

    VkInstance instance = VK_NULL_HANDLE;
    {
        const VkResult res = vkCreateInstance(&instanceCI, nullptr, &instance);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateInstance", res);
            ShowFatal("vkCreateInstance failed");
            if (pauseOnExit)
                (void)std::getchar();
            return 1;
        }
    }

    // Debug messenger
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    auto vkCreateDebugUtilsMessengerEXT_ =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    auto vkDestroyDebugUtilsMessengerEXT_ =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    (void)vkDestroyDebugUtilsMessengerEXT_;

#if VGT_ENABLE_VALIDATION
    if (vkCreateDebugUtilsMessengerEXT_)
    {
        VkDebugUtilsMessengerCreateInfoEXT dbgCI{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        dbgCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbgCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbgCI.pfnUserCallback = &DebugCallback;
        vkCreateDebugUtilsMessengerEXT_(instance, &dbgCI, nullptr, &debugMessenger);
    }
#else
    (void)&DebugCallback;
#endif

    (void)vkCreateDebugUtilsMessengerEXT_;

    (void)debugMessenger;

    // Surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    {
        const VkResult res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("glfwCreateWindowSurface", res);
            ShowFatal("glfwCreateWindowSurface failed");
            if (pauseOnExit)
                (void)std::getchar();
            return 1;
        }
    }

    // Physical device
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
    std::vector<VkPhysicalDevice> gpus(gpuCount);
    vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data());
    VkPhysicalDevice physicalDevice = gpus[0];

    // Queue family selection
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

    const char* deviceExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo deviceCI{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceCI.queueCreateInfoCount = static_cast<uint32_t>(queueCIs.size());
    deviceCI.pQueueCreateInfos = queueCIs.data();
    deviceCI.enabledExtensionCount = 1;
    deviceCI.ppEnabledExtensionNames = deviceExts;
    deviceCI.enabledLayerCount = static_cast<uint32_t>(layers.size());
    deviceCI.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();

    VkDevice device = VK_NULL_HANDLE;
    {
        const VkResult res = vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateDevice", res);
            ShowFatal("vkCreateDevice failed");
            if (pauseOnExit)
                (void)std::getchar();
            return 1;
        }
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

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX)
    {
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        extent.width = static_cast<uint32_t>(w);
        extent.height = static_cast<uint32_t>(h);
    }

    VkSwapchainCreateInfoKHR swapCI{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapCI.surface = surface;
    swapCI.minImageCount = caps.minImageCount + 1;
    swapCI.imageFormat = surfaceFormat.format;
    swapCI.imageColorSpace = surfaceFormat.colorSpace;
    swapCI.imageExtent = extent;
    swapCI.imageArrayLayers = 1;
    swapCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapCI.preTransform = caps.currentTransform;
    swapCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapCI.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapCI.clipped = VK_TRUE;
    swapCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    uint32_t qIndices[] = { graphicsQ, presentQ };
    if (graphicsQ != presentQ)
    {
        swapCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapCI.queueFamilyIndexCount = 2;
        swapCI.pQueueFamilyIndices = qIndices;
    }

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    {
        const VkResult res = vkCreateSwapchainKHR(device, &swapCI, nullptr, &swapchain);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateSwapchainKHR", res);
            ShowFatal("vkCreateSwapchainKHR failed");
            if (pauseOnExit)
                (void)std::getchar();
            return 1;
        }
    }

    uint32_t swapImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, nullptr);
    std::vector<VkImage> swapImages(swapImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &swapImageCount, swapImages.data());

    std::vector<VkImageView> swapImageViews(swapImageCount);
    for (uint32_t i = 0; i < swapImageCount; ++i)
    {
        VkImageViewCreateInfo viewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewCI.image = swapImages[i];
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format = surfaceFormat.format;
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.levelCount = 1;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount = 1;

        {
            const VkResult res = vkCreateImageView(device, &viewCI, nullptr, &swapImageViews[i]);
            if (res != VK_SUCCESS)
            {
                PrintVkResult("vkCreateImageView", res);
                return 1;
            }
        }
    }

    // Render pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpCI{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpCI.attachmentCount = 1;
    rpCI.pAttachments = &colorAttachment;
    rpCI.subpassCount = 1;
    rpCI.pSubpasses = &subpass;
    rpCI.dependencyCount = 1;
    rpCI.pDependencies = &dep;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    {
        const VkResult res = vkCreateRenderPass(device, &rpCI, nullptr, &renderPass);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateRenderPass", res);
            return 1;
        }
    }

    // Framebuffers
    std::vector<VkFramebuffer> framebuffers(swapImageCount);
    for (uint32_t i = 0; i < swapImageCount; ++i)
    {
        VkImageView attachments[] = { swapImageViews[i] };
        VkFramebufferCreateInfo fbCI{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fbCI.renderPass = renderPass;
        fbCI.attachmentCount = 1;
        fbCI.pAttachments = attachments;
        fbCI.width = extent.width;
        fbCI.height = extent.height;
        fbCI.layers = 1;
        {
            const VkResult res = vkCreateFramebuffer(device, &fbCI, nullptr, &framebuffers[i]);
            if (res != VK_SUCCESS)
            {
                PrintVkResult("vkCreateFramebuffer", res);
                return 1;
            }
        }
    }

    // Pipeline layout
    VkPipelineLayoutCreateInfo plCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    {
        const VkResult res = vkCreatePipelineLayout(device, &plCI, nullptr, &pipelineLayout);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreatePipelineLayout", res);
            return 1;
        }
    }

    // Shader modules
    const auto vertSpv = ReadSpirvWithFallback("triangle.vert.spv");
    const auto fragSpv = ReadSpirvWithFallback("triangle.frag.spv");
    if (vertSpv.empty() || fragSpv.empty())
    {
        ShowFatal("Failed to load shaders. Did you build the project (which runs glslangValidator)?");
        if (pauseOnExit)
            (void)std::getchar();
        return 1;
    }

    VkShaderModuleCreateInfo smVertCI{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    smVertCI.codeSize = vertSpv.size() * sizeof(uint32_t);
    smVertCI.pCode = vertSpv.data();
    VkShaderModule vertModule = VK_NULL_HANDLE;
    {
        const VkResult res = vkCreateShaderModule(device, &smVertCI, nullptr, &vertModule);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateShaderModule(vert)", res);
            return 1;
        }
    }

    VkShaderModuleCreateInfo smFragCI{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    smFragCI.codeSize = fragSpv.size() * sizeof(uint32_t);
    smFragCI.pCode = fragSpv.data();
    VkShaderModule fragModule = VK_NULL_HANDLE;
    {
        const VkResult res = vkCreateShaderModule(device, &smFragCI, nullptr, &fragModule);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateShaderModule(frag)", res);
            return 1;
        }
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    // Fixed function states (minimal)
    VkPipelineVertexInputStateCreateInfo vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    vp.viewportCount = 1;
    vp.pViewports = &viewport;
    vp.scissorCount = 1;
    vp.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cbAttach{};
    cbAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    cb.attachmentCount = 1;
    cb.pAttachments = &cbAttach;

    VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dyn.dynamicStateCount = static_cast<uint32_t>(std::size(dynStates));
    dyn.pDynamicStates = dynStates;

    VkGraphicsPipelineCreateInfo gpCI{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    gpCI.stageCount = 2;
    gpCI.pStages = stages;
    gpCI.pVertexInputState = &vi;
    gpCI.pInputAssemblyState = &ia;
    gpCI.pViewportState = &vp;
    gpCI.pRasterizationState = &rs;
    gpCI.pMultisampleState = &ms;
    gpCI.pColorBlendState = &cb;
    gpCI.pDynamicState = &dyn;
    gpCI.layout = pipelineLayout;
    gpCI.renderPass = renderPass;
    gpCI.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    {
        const VkResult res = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpCI, nullptr, &pipeline);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateGraphicsPipelines", res);
            return 1;
        }
    }

    // Command pool / buffers
    VkCommandPoolCreateInfo poolCI{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolCI.queueFamilyIndex = graphicsQ;
    poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    {
        const VkResult res = vkCreateCommandPool(device, &poolCI, nullptr, &cmdPool);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateCommandPool", res);
            return 1;
        }
    }

    std::vector<VkCommandBuffer> cmdBuffers(swapImageCount);
    VkCommandBufferAllocateInfo cmdAI{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdAI.commandPool = cmdPool;
    cmdAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAI.commandBufferCount = swapImageCount;
    {
        const VkResult res = vkAllocateCommandBuffers(device, &cmdAI, cmdBuffers.data());
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkAllocateCommandBuffers", res);
            return 1;
        }
    }

    // Sync primitives (single in-flight frame).
    VkSemaphoreCreateInfo semCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence inFlight = VK_NULL_HANDLE;
    {
        VkResult res = vkCreateSemaphore(device, &semCI, nullptr, &imageAvailable);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateSemaphore(imageAvailable)", res);
            return 1;
        }
        res = vkCreateSemaphore(device, &semCI, nullptr, &renderFinished);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateSemaphore(renderFinished)", res);
            return 1;
        }
        res = vkCreateFence(device, &fenceCI, nullptr, &inFlight);
        if (res != VK_SUCCESS)
        {
            PrintVkResult("vkCreateFence", res);
            return 1;
        }
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        vkWaitForFences(device, 1, &inFlight, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlight);

        uint32_t imageIndex = 0;
        {
            const VkResult res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);
            if (res != VK_SUCCESS)
            {
                PrintVkResult("vkAcquireNextImageKHR", res);
                break;
            }
        }

        VkCommandBuffer cmd = cmdBuffers[imageIndex];
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        {
            const VkResult res = vkBeginCommandBuffer(cmd, &begin);
            if (res != VK_SUCCESS)
            {
                PrintVkResult("vkBeginCommandBuffer", res);
                break;
            }
        }

        // Render a triangle with RenderPass.
        VkClearValue clear{};
        clear.color.float32[0] = 0.05f;
        clear.color.float32[1] = 0.05f;
        clear.color.float32[2] = 0.10f;
        clear.color.float32[3] = 1.0f;

        VkRenderPassBeginInfo rpBegin{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        rpBegin.renderPass = renderPass;
        rpBegin.framebuffer = framebuffers[imageIndex];
        rpBegin.renderArea.offset = { 0, 0 };
        rpBegin.renderArea.extent = extent;
        rpBegin.clearValueCount = 1;
        rpBegin.pClearValues = &clear;

        vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport drawViewport{};
        drawViewport.x = 0.0f;
        drawViewport.y = 0.0f;
        drawViewport.width = static_cast<float>(extent.width);
        drawViewport.height = static_cast<float>(extent.height);
        drawViewport.minDepth = 0.0f;
        drawViewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &drawViewport);

        VkRect2D drawScissor{};
        drawScissor.offset = { 0, 0 };
        drawScissor.extent = extent;
        vkCmdSetScissor(cmd, 0, 1, &drawScissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmd);

        {
            const VkResult res = vkEndCommandBuffer(cmd);
            if (res != VK_SUCCESS)
            {
                PrintVkResult("vkEndCommandBuffer", res);
                break;
            }
        }

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &imageAvailable;
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &renderFinished;

        {
            const VkResult res = vkQueueSubmit(graphicsQueue, 1, &submit, inFlight);
            if (res != VK_SUCCESS)
            {
                PrintVkResult("vkQueueSubmit", res);
                break;
            }
        }

        VkPresentInfoKHR present{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &renderFinished;
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &imageIndex;
        {
            const VkResult res = vkQueuePresentKHR(presentQueue, &present);
            if (res != VK_SUCCESS)
            {
                PrintVkResult("vkQueuePresentKHR", res);
                break;
            }
        }

        // Avoid swapchain semaphore reuse hazards in this minimal sample.
        // Ensure presentation is complete before reusing renderFinished.
        vkQueueWaitIdle(presentQueue);
    }

    vkDeviceWaitIdle(device);

    vkDestroyFence(device, inFlight, nullptr);
    vkDestroySemaphore(device, renderFinished, nullptr);
    vkDestroySemaphore(device, imageAvailable, nullptr);

    vkFreeCommandBuffers(device, cmdPool, swapImageCount, cmdBuffers.data());
    vkDestroyCommandPool(device, cmdPool, nullptr);

    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    for (auto fb : framebuffers)
        vkDestroyFramebuffer(device, fb, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto v : swapImageViews)
        vkDestroyImageView(device, v, nullptr);

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

    if (pauseOnExit)
    {
        std::fprintf(stderr, "Press Enter to exit...\n");
        (void)std::getchar();
    }

    return 0;
}
