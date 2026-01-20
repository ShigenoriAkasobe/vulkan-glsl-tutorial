#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <cmath>

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
    MessageBoxA(nullptr, msg, "Step05_LightingBasic", MB_OK | MB_ICONERROR);
}

struct Vertex
{
    float pos[3];
    float normal[3];
};

struct UniformBufferObject
{
    float mvp[16];
    float modelView[16];
    float normalMatrix[16];
    float lightDir[4];  // w component unused, padding for alignment
};

static void Mat4Identity(float* m)
{
    for (int i = 0; i < 16; ++i)
        m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void Mat4RotateY(float* m, float angle)
{
    const float c = std::cosf(angle);
    const float s = std::sinf(angle);
    Mat4Identity(m);
    // Column-major format (GLSL standard)
    m[0] = c;   // m[0][0]
    m[2] = -s;  // m[0][2]
    m[8] = s;   // m[2][0]
    m[10] = c;  // m[2][2]
}

static void Mat4LookAt(float* m, float eyeX, float eyeY, float eyeZ,
                       float centerX, float centerY, float centerZ,
                       float upX, float upY, float upZ)
{
    // Forward vector (normalized)
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;
    float rlf = 1.0f / std::sqrtf(fx * fx + fy * fy + fz * fz);
    fx *= rlf;
    fy *= rlf;
    fz *= rlf;

    // Right vector (normalized)
    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;
    float rls = 1.0f / std::sqrtf(sx * sx + sy * sy + sz * sz);
    sx *= rls;
    sy *= rls;
    sz *= rls;

    // Up vector
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    // Column-major format (GLSL standard)
    m[0] = sx;  // Right.x
    m[1] = sy;  // Right.y
    m[2] = sz;  // Right.z
    m[3] = 0.0f;
    m[4] = ux;  // Up.x
    m[5] = uy;  // Up.y
    m[6] = uz;  // Up.z
    m[7] = 0.0f;
    m[8] = -fx; // -Forward.x
    m[9] = -fy; // -Forward.y
    m[10] = -fz; // -Forward.z
    m[11] = 0.0f;
    m[12] = -(sx * eyeX + sy * eyeY + sz * eyeZ);  // -dot(Right, Eye)
    m[13] = -(ux * eyeX + uy * eyeY + uz * eyeZ);  // -dot(Up, Eye)
    m[14] = (fx * eyeX + fy * eyeY + fz * eyeZ);   // dot(Forward, Eye)
    m[15] = 1.0f;
}

static void Mat4Perspective(float* m, float fovY, float aspect, float zNear, float zFar)
{
    const float tanHalfFovy = std::tanf(fovY / 2.0f);
    for (int i = 0; i < 16; ++i)
        m[i] = 0.0f;
    m[0] = 1.0f / (aspect * tanHalfFovy);
    m[5] = 1.0f / tanHalfFovy;
    m[10] = zFar / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = -(zFar * zNear) / (zFar - zNear);
    
    // Vulkan clip space has inverted Y and depth [0, 1]
    // We handle this by negating Y in the projection
    m[5] = -m[5];
}

static void Mat4Multiply(float* out, const float* a, const float* b)
{
    float temp[16];
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            temp[row * 4 + col] = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                temp[row * 4 + col] += a[row * 4 + k] * b[k * 4 + col];
            }
        }
    }
    std::memcpy(out, temp, sizeof(temp));
}

static void Mat4Transpose(float* out, const float* m)
{
    float temp[16];
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            temp[col * 4 + row] = m[row * 4 + col];
        }
    }
    std::memcpy(out, temp, sizeof(temp));
}

static void Mat3FromMat4(float* out, const float* m)
{
    out[0] = m[0]; out[1] = m[1]; out[2] = m[2];
    out[3] = m[4]; out[4] = m[5]; out[5] = m[6];
    out[6] = m[8]; out[7] = m[9]; out[8] = m[10];
}

static void Mat3Transpose(float* out, const float* m)
{
    float temp[9];
    temp[0] = m[0]; temp[1] = m[3]; temp[2] = m[6];
    temp[3] = m[1]; temp[4] = m[4]; temp[5] = m[7];
    temp[6] = m[2]; temp[7] = m[5]; temp[8] = m[8];
    std::memcpy(out, temp, sizeof(temp));
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
    // e.g. build-ninja/.../steps/Step05_LightingBasic/compiled_shaders
    std::string alt = std::string("compiled_shaders/") + relativePath;
    {
        auto data = ReadSpirvFile(alt.c_str());
        if (!data.empty())
            return data;
    }

    return {};
}

static uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
    {
        if ((typeBits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    return UINT32_MAX;
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
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Step05_LightingBasic", nullptr, nullptr);
    if (!window)
        return 1;

    // Instance
    VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "vulkan-glsl-tutorial";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> instanceExts(glfwExts, glfwExts + glfwExtCount);

    std::vector<const char*> layers;
#if VGT_ENABLE_VALIDATION
    instanceExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
        viewCI.subresourceRange.levelCount = 1;
        viewCI.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewCI, nullptr, &swapImageViews[i]);
    }

    // Uniform buffer
    VkBufferCreateInfo uniformBufCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    uniformBufCI.size = sizeof(UniformBufferObject);
    uniformBufCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniformBufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    vkCreateBuffer(device, &uniformBufCI, nullptr, &uniformBuffer);

    VkMemoryRequirements uniformMemReq{};
    vkGetBufferMemoryRequirements(device, uniformBuffer, &uniformMemReq);

    uint32_t uniformMemType = FindMemoryTypeIndex(
        physicalDevice,
        uniformMemReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo uniformAlloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    uniformAlloc.allocationSize = uniformMemReq.size;
    uniformAlloc.memoryTypeIndex = uniformMemType;

    VkDeviceMemory uniformMemory = VK_NULL_HANDLE;
    vkAllocateMemory(device, &uniformAlloc, nullptr, &uniformMemory);
    vkBindBufferMemory(device, uniformBuffer, uniformMemory, 0);

    // Descriptor set layout
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo descLayoutCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descLayoutCI.bindingCount = 1;
    descLayoutCI.pBindings = &uboBinding;

    VkDescriptorSetLayout descLayout = VK_NULL_HANDLE;
    vkCreateDescriptorSetLayout(device, &descLayoutCI, nullptr, &descLayout);

    // Descriptor pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolCI.poolSizeCount = 1;
    poolCI.pPoolSizes = &poolSize;
    poolCI.maxSets = 1;

    VkDescriptorPool descPool = VK_NULL_HANDLE;
    vkCreateDescriptorPool(device, &poolCI, nullptr, &descPool);

    // Descriptor set
    VkDescriptorSetAllocateInfo descAI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    descAI.descriptorPool = descPool;
    descAI.descriptorSetCount = 1;
    descAI.pSetLayouts = &descLayout;

    VkDescriptorSet descSet = VK_NULL_HANDLE;
    vkAllocateDescriptorSets(device, &descAI, &descSet);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    descWrite.dstSet = descSet;
    descWrite.dstBinding = 0;
    descWrite.dstArrayElement = 0;
    descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descWrite.descriptorCount = 1;
    descWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &descWrite, 0, nullptr);

    // Render pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rpCI{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpCI.attachmentCount = 1;
    rpCI.pAttachments = &colorAttachment;
    rpCI.subpassCount = 1;
    rpCI.pSubpasses = &subpass;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    vkCreateRenderPass(device, &rpCI, nullptr, &renderPass);

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
        vkCreateFramebuffer(device, &fbCI, nullptr, &framebuffers[i]);
    }

    // Vertex buffer (triangle with 3D positions and normals)
    Vertex vertices[3] = {
        { {  0.0f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
    };

    VkBufferCreateInfo bufCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufCI.size = sizeof(vertices);
    bufCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    vkCreateBuffer(device, &bufCI, nullptr, &vertexBuffer);

    VkMemoryRequirements memReq{};
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memReq);

    uint32_t memType = FindMemoryTypeIndex(
        physicalDevice,
        memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo alloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc.allocationSize = memReq.size;
    alloc.memoryTypeIndex = memType;

    VkDeviceMemory vertexMem = VK_NULL_HANDLE;
    vkAllocateMemory(device, &alloc, nullptr, &vertexMem);
    vkBindBufferMemory(device, vertexBuffer, vertexMem, 0);

    void* mapped = nullptr;
    vkMapMemory(device, vertexMem, 0, sizeof(vertices), 0, &mapped);
    std::memcpy(mapped, vertices, sizeof(vertices));
    vkUnmapMemory(device, vertexMem);

    // Pipeline layout
    VkPipelineLayoutCreateInfo plCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    plCI.setLayoutCount = 1;
    plCI.pSetLayouts = &descLayout;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    vkCreatePipelineLayout(device, &plCI, nullptr, &pipelineLayout);

    // Shader modules
    const auto vertSpv = ReadSpirvWithFallback("lighting.vert.spv");
    const auto fragSpv = ReadSpirvWithFallback("lighting.frag.spv");
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
    vkCreateShaderModule(device, &smVertCI, nullptr, &vertModule);

    VkShaderModuleCreateInfo smFragCI{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    smFragCI.codeSize = fragSpv.size() * sizeof(uint32_t);
    smFragCI.pCode = fragSpv.data();
    VkShaderModule fragModule = VK_NULL_HANDLE;
    vkCreateShaderModule(device, &smFragCI, nullptr, &fragModule);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    // Vertex input
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(Vertex, pos);

    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = offsetof(Vertex, normal);

    VkPipelineVertexInputStateCreateInfo vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &binding;
    vi.vertexAttributeDescriptionCount = 2;
    vi.pVertexAttributeDescriptions = attrs;

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

    VkPipeline pipeline = VK_NULL_HANDLE;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpCI, nullptr, &pipeline);

    // Command pool / buffers
    VkCommandPoolCreateInfo cmdPoolCI{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    cmdPoolCI.queueFamilyIndex = graphicsQ;
    cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    vkCreateCommandPool(device, &cmdPoolCI, nullptr, &cmdPool);

    std::vector<VkCommandBuffer> cmdBuffers(swapImageCount);
    VkCommandBufferAllocateInfo cmdAI{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdAI.commandPool = cmdPool;
    cmdAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAI.commandBufferCount = swapImageCount;
    vkAllocateCommandBuffers(device, &cmdAI, cmdBuffers.data());

    VkSemaphoreCreateInfo semCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence inFlight = VK_NULL_HANDLE;
    vkCreateSemaphore(device, &semCI, nullptr, &imageAvailable);
    vkCreateSemaphore(device, &semCI, nullptr, &renderFinished);
    vkCreateFence(device, &fenceCI, nullptr, &inFlight);

    double startTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        double currentTime = glfwGetTime();
        float time = static_cast<float>(currentTime - startTime);

        // Update uniform buffer
        UniformBufferObject ubo{};
        
        float model[16], view[16], proj[16], modelView[16];
        Mat4RotateY(model, time);
        Mat4LookAt(view, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        Mat4Perspective(proj, 0.785398f, static_cast<float>(extent.width) / extent.height, 0.1f, 10.0f);
        
        // ModelView = view * model
        Mat4Multiply(modelView, view, model);
        
        // MVP = proj * modelView
        Mat4Multiply(ubo.mvp, proj, modelView);
        
        // Copy modelView (no transpose)
        std::memcpy(ubo.modelView, modelView, sizeof(modelView));
        
        // Normal matrix = transpose(inverse(mat3(modelView)))
        // For simple rotations, transpose(mat3(modelView)) is sufficient
        float mv3[9];
        Mat3FromMat4(mv3, modelView);
        float normalMat3[9];
        Mat3Transpose(normalMat3, mv3);
        
        // Expand to mat4 (padding with identity for the 4th row/column)
        Mat4Identity(ubo.normalMatrix);
        ubo.normalMatrix[0] = normalMat3[0];
        ubo.normalMatrix[1] = normalMat3[1];
        ubo.normalMatrix[2] = normalMat3[2];
        ubo.normalMatrix[4] = normalMat3[3];
        ubo.normalMatrix[5] = normalMat3[4];
        ubo.normalMatrix[6] = normalMat3[5];
        ubo.normalMatrix[8] = normalMat3[6];
        ubo.normalMatrix[9] = normalMat3[7];
        ubo.normalMatrix[10] = normalMat3[8];
        
        // Light direction in view space (pointing down and to the right)
        ubo.lightDir[0] = 0.5f;
        ubo.lightDir[1] = -0.5f;
        ubo.lightDir[2] = -1.0f;
        ubo.lightDir[3] = 0.0f;

        vkMapMemory(device, uniformMemory, 0, sizeof(ubo), 0, &mapped);
        std::memcpy(mapped, &ubo, sizeof(ubo));
        vkUnmapMemory(device, uniformMemory);

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
        vkBeginCommandBuffer(cmd, &begin);

        VkClearValue clear{};
        clear.color.float32[0] = 0.02f;
        clear.color.float32[1] = 0.02f;
        clear.color.float32[2] = 0.05f;
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
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 0, nullptr);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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

    vkDestroyDescriptorPool(device, descPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descLayout, nullptr);

    vkDestroyBuffer(device, uniformBuffer, nullptr);
    vkFreeMemory(device, uniformMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexMem, nullptr);

    for (auto fb : framebuffers)
        vkDestroyFramebuffer(device, fb, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto v : swapImageViews)
        vkDestroyImageView(device, v, nullptr);

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

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
