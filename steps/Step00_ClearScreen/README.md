# Step00_ClearScreen

## What you learn

- Create a window with GLFW (Win32 backend).
- Create a minimal Vulkan setup on Windows:
  - `VkInstance`
  - `VkSurfaceKHR` (`VK_KHR_surface` + `VK_KHR_win32_surface` via GLFW)
  - `VkPhysicalDevice` / `VkDevice`
  - `VkSwapchainKHR`
- Record and submit a command buffer that clears the swapchain image.
- Enable Vulkan validation layers.

## Where you are on the GPU pipeline

This step focuses on the **framebuffer/output** part of the pipeline.
There are **no shaders** and **no graphics pipeline** yet. We only clear the swapchain image.

## Vulkan objects and responsibilities

- `VkInstance`: enables extensions and validation layers.
- `VkSurfaceKHR`: OS window surface (created by GLFW).
- `VkPhysicalDevice`: GPU selection.
- `VkDevice`: logical device and queue creation.
- `VkSwapchainKHR`: images presented to the window.
- `VkCommandPool` / `VkCommandBuffer`: record work for GPU.
- `VkSemaphore` / `VkFence`: synchronize acquire / submit / present.

## Lifetime / dependency overview

- `VkInstance` must outlive `VkSurfaceKHR` and `VkDevice`.
- `VkDevice` must outlive swapchain and all device objects.
- Swapchain must be recreated when the window is resized.

## Windows notes

- The surface extension on Windows is `VK_KHR_win32_surface`. With GLFW you still need to enable
  `VK_KHR_surface` and `VK_KHR_win32_surface` extensions at instance creation time.

## How to run

Build from the repository root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

Then run `Step00_ClearScreen` from Visual Studio or:

```powershell
./build/steps/Step00_ClearScreen/Debug/Step00_ClearScreen.exe
```
