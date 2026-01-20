# Step04_Transform

## What you learn

- 3D coordinate transformations (Model / View / Projection)
- MVP matrix composition
- Uniform buffers (`VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`)
- Updating uniforms per frame
- Basic matrix mathematics for 3D graphics
- Time-based animation

## Where you are on the GPU pipeline

- CPU computes Model, View, and Projection matrices
- CPU multiplies them into a single MVP matrix
- CPU uploads MVP to uniform buffer (host-visible memory)
- Vertex shader reads uniform buffer
- Vertex shader transforms positions: `gl_Position = uMvp * vec4(iPos, 1.0)`
- Rasterizer converts transformed positions to screen space
- Fragment shader receives interpolated color

## Vulkan objects added in this step

- `VkBuffer` with `VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT`
- `VkDeviceMemory` (host-visible + host-coherent for dynamic updates)
- `VkDescriptorSetLayout` with `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- `VkDescriptorSet` binding uniform buffer to vertex shader

## Object dependencies and lifetime

1. Create uniform buffer and allocate host-visible memory
2. Bind memory to buffer
3. Create descriptor set layout (defines UBO binding at set 0, binding 0)
4. Create pipeline layout referencing descriptor set layout
5. Create descriptor pool
6. Allocate descriptor set
7. Update descriptor set with buffer info (`vkUpdateDescriptorSets`)
8. Each frame:
   - Map uniform buffer memory
   - Compute and write MVP matrix
   - Unmap memory
   - Bind descriptor set in command buffer
   - Draw

Cleanup order:
- Descriptor pool
- Descriptor set layout
- Uniform buffer
- Uniform memory

## Why this configuration

- **MVP matrices**: Standard 3D graphics transformation pipeline
  - Model: object space → world space (rotation, translation, scale)
  - View: world space → camera space
  - Projection: camera space → clip space (perspective or orthographic)
- **Uniform buffer**: Efficient way to pass constant data to shaders
- **Host-visible + host-coherent**: Allows CPU to update data every frame without barriers
- **Single MVP matrix**: More efficient than passing three separate matrices (fewer vertex shader operations)

## Design intent

This step demonstrates:
1. How to transform 3D coordinates to screen space
2. How to pass data from CPU to GPU via uniform buffers
3. How to create animated scenes by updating transforms each frame

The triangle now rotates in 3D space, demonstrating:
- Model transformation (rotation around Y-axis)
- View transformation (camera positioned at Z=2)
- Projection transformation (perspective with 45° FOV)

## Windows-specific notes

- Matrix math is platform-independent (pure C++)
- `glfwGetTime()` provides frame timing
- No Win32-specific matrix APIs used (keeping it simple for learning)

## Vulkan-specific notes

**Coordinate system differences from OpenGL:**
- Vulkan's NDC (Normalized Device Coordinates) Y-axis points **downward** (0 at top, 1 at bottom)
- OpenGL's NDC Y-axis points **upward** (0 at bottom, 1 at top)
- This requires negating the Y component in the perspective matrix: `m[5] = -1.0f / tanHalfFovy`
- Without this correction, rendered geometry appears upside-down

**Depth range:**
- Vulkan uses [0, 1] depth range (OpenGL uses [-1, 1])
- Our perspective matrix is designed for Vulkan's [0, 1] range
