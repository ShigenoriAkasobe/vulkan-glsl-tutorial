# Step02_VertexColor

## What you learn

- Add a vertex buffer.
- Define vertex input state:
  - binding
  - attribute locations
  - formats and offsets
- Use `layout(location=...)` in GLSL and match it with `VkVertexInputAttributeDescription`.

## Where you are on the GPU pipeline

- CPU uploads vertex data to a `VkBuffer`.
- Vertex fetch reads attributes.
- Vertex shader consumes `location` inputs.
- Interpolation happens between vertex and fragment stages.

## Vulkan objects added in this step

- `VkBuffer` + `VkDeviceMemory` for vertex data
- `VkPipelineVertexInputStateCreateInfo` settings

## Notes

This step still avoids descriptors/uniforms. We only change the data path for vertex attributes.
