# Step01_MinimalTriangle

## What you learn

- Add the first programmable stages (GLSL vertex/fragment shaders).
- Create a graphics pipeline:
  - shader modules
  - pipeline layout
  - render pass
  - framebuffers
- Draw a minimal triangle using `gl_VertexIndex` (no vertex buffer yet).

## Where you are on the GPU pipeline

- CPU submits a draw call (`vkCmdDraw`)
- Vertex shader runs (generates positions and colors)
- Rasterizer
- Fragment shader writes into the framebuffer
- Swapchain image is presented

## Vulkan objects added in this step

- `VkRenderPass`
- `VkPipelineLayout`
- `VkShaderModule`
- `VkPipeline`
- `VkFramebuffer`

## Shader build

Shaders in `shaders/` are compiled by CMake using `glslangValidator`.
The generated `.spv` files are copied next to the executable in `shaders/`.

## Notes

This step intentionally keeps the data path minimal:

- No `VkBuffer` yet.
- No descriptors yet.
- The vertex shader uses `gl_VertexIndex` to generate positions.
