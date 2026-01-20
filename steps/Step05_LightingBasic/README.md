# Step05_LightingBasic

## What you learn

- Vertex normals for lighting calculations
- Normal matrix transformation (transpose of inverse of model-view matrix)
- Lambert diffuse lighting model
- Light direction in view space
- Ambient + diffuse lighting components
- Fragment shader lighting calculations

## Where you are on the GPU pipeline

- CPU computes MVP, ModelView, and Normal matrices
- CPU specifies light direction
- CPU uploads all data to uniform buffer
- Vertex shader transforms position (MVP) and normal (normal matrix)
- Rasterizer interpolates normals across triangle surface
- Fragment shader:
  - Normalizes interpolated normal
  - Computes dot product with light direction (Lambert's law)
  - Calculates ambient + diffuse lighting
  - Outputs final lit color

## Vulkan objects added in this step

Same as Step04, but with extended uniform buffer:
- Uniform buffer now contains: MVP, ModelView, NormalMatrix, LightDir
- Descriptor set stages now include `VK_SHADER_STAGE_FRAGMENT_BIT`

## Object dependencies and lifetime

Same as Step04, with these additions:
1. Normal matrix computed from ModelView matrix (transpose of upper-left 3x3)
2. Light direction stored in uniform buffer
3. Fragment shader reads uniform buffer for lighting calculations

## Why this configuration

- **Normal vectors**: Represent surface orientation, essential for lighting
- **Normal matrix**: Correctly transforms normals under non-uniform scaling
  - For rotation-only transforms, transpose(mat3(ModelView)) is sufficient
  - For general transforms, transpose(inverse(mat3(ModelView))) is required
- **View space lighting**: Light direction in view space simplifies calculations
- **Lambert diffuse**: `ndotl = max(dot(N, L), 0.0)` - basic physically-based lighting
  - N: surface normal (normalized)
  - L: light direction (normalized)
  - ndotl: amount of light hitting surface (0 = perpendicular, 1 = direct)
- **Ambient component**: Prevents completely black surfaces (simulates indirect lighting)

## Design intent

This step demonstrates:
1. How to add vertex normals to geometry
2. How to properly transform normals
3. How to implement basic per-fragment lighting
4. The difference between ambient and diffuse lighting

The triangle now shows shading variation across its surface:
- Brighter where normal faces the light
- Darker where normal points away
- Never completely black (ambient prevents this)

## Windows-specific notes

- Normal and lighting math is platform-independent
- Matrix operations use simple C++ (no DirectX Math or similar)
- Educational focus: understanding GPU lighting pipeline

## Vulkan-specific notes

**Coordinate system:**
- Vulkan's NDC Y-axis points downward (requires negated Y in perspective matrix)
- Same coordinate system considerations as Step04
- Normal transformations are unaffected by this (they're in view space)

## Lighting model details

**Lambert's Cosine Law**:
- Intensity = BaseColor × max(N·L, 0)
- Models diffuse (matte) surfaces
- Foundation for more complex models (Phong, Blinn-Phong, PBR)

**This implementation**:
- Ambient: 20% of base color (constant across surface)
- Diffuse: 80% of base color × N·L (varies per fragment)
- Final = Ambient + Diffuse
