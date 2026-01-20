# Step03_Texture

## What you learn

- UV coordinates for texture mapping
- Loading image data with `stb_image`
- Creating and uploading to `VkImage` via staging buffer
- Image layout transitions (`UNDEFINED` → `TRANSFER_DST_OPTIMAL` → `SHADER_READ_ONLY_OPTIMAL`)
- Creating `VkImageView` and `VkSampler`
- Descriptor sets: `VkDescriptorSetLayout`, `VkDescriptorPool`, `VkDescriptorSet`
- Binding combined image sampler to fragment shader

## Where you are on the GPU pipeline

- CPU loads texture data from file (stb_image)
- CPU uploads texture to GPU memory via staging buffer
- CPU sets up descriptor binding
- Vertex shader passes UV coordinates to fragment shader
- Fragment shader samples texture using `texture(sampler2D, uv)`
- Rasterizer interpolates UV between vertices
- Fragment shader outputs textured color to framebuffer

## Vulkan objects added in this step

- `VkImage` (texture storage)
- `VkDeviceMemory` (GPU memory for image)
- `VkImageView` (how to interpret the image)
- `VkSampler` (filtering, addressing modes)
- `VkDescriptorSetLayout` (defines binding structure)
- `VkDescriptorPool` (allocates descriptor sets)
- `VkDescriptorSet` (binds texture + sampler to shader)
- `VkPipelineLayout` now references the descriptor set layout

## Object dependencies and lifetime

1. Load image data with `stb_image` → staging buffer
2. Create `VkImage` with `VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT`
3. Allocate GPU memory and bind to image
4. Record command buffer: transition layout, copy from staging buffer, transition layout again
5. Create `VkImageView` (after image is ready)
6. Create `VkSampler` (independent of image content)
7. Create `VkDescriptorSetLayout` (defines what bindings exist)
8. Create `VkPipelineLayout` referencing the descriptor set layout
9. Create `VkDescriptorPool` and allocate `VkDescriptorSet`
10. Update descriptor set with image view and sampler (`vkUpdateDescriptorSets`)
11. Bind descriptor set during rendering (`vkCmdBindDescriptorSets`)

Cleanup order:
- Descriptor pool (frees all descriptor sets)
- Descriptor set layout
- Sampler
- Image view
- Image
- Image memory

## Why this configuration

- **Image layout transitions**: Vulkan requires explicit layout transitions for optimal GPU access patterns
  - `UNDEFINED` → `TRANSFER_DST_OPTIMAL`: prepare for upload
  - `TRANSFER_DST_OPTIMAL` → `SHADER_READ_ONLY_OPTIMAL`: prepare for sampling
- **Staging buffer**: Most efficient way to upload texture data (host-visible → device-local)
- **Descriptor sets**: Vulkan's mechanism for binding resources (textures, buffers) to shaders
- **Combined image sampler**: Single descriptor type that includes both texture and sampling parameters

## Design intent

This step demonstrates the complete texture pipeline:
1. Load texture from file
2. Upload to GPU
3. Bind to shader
4. Sample in fragment shader

The vertex buffer now includes UV coordinates instead of colors, showing how vertex attributes can be repurposed.

## Windows-specific notes

- `stb_image` works cross-platform (no Win32-specific code)
- Texture file path uses forward slashes or relative paths
- Build system copies/references textures via CMake
