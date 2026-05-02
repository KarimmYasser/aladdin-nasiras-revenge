# Materials, Shaders & Pipeline State

The rendering engine utilizes a flexible material system that abstracts OpenGL pipeline configurations and shader uniform management. This system allows for diverse visual effects while maintaining a consistent interface for the `ForwardRenderer`.

## Pipeline State Management

The `PipelineState` class encapsulates the global state of the OpenGL fixed-function pipeline.

| Property          | Description                                                                              |
| :---------------- | :--------------------------------------------------------------------------------------- |
| **Face Culling**  | Controls if front, back, or no faces are discarded (`glEnable(GL_CULL_FACE)`).           |
| **Depth Testing** | Determines if a fragment should be discarded based on depth (`glEnable(GL_DEPTH_TEST)`). |
| **Blending**      | Configures color mixing for transparency (`glEnable(GL_BLEND)`).                         |
| **Depth Mask**    | Enables or disables writing to the depth buffer (`glDepthMask`).                         |

The `setup()` function translates these properties into native OpenGL calls (`glEnable`, `glBlendFunc`, etc.) before each draw call.

## Shader System

The `Shader` class manages the lifecycle of OpenGL Shader Programs, including compilation, linking, and uniform management.

### Key Shaders (`assets/shaders/`)

| Shader Path        | Purpose                 | Key Features                                                         |
| :----------------- | :---------------------- | :------------------------------------------------------------------- |
| `light.vert/frag`  | Standard lit rendering. | Supports up to 16 lights, Blinn-Phong shading, and shadow sampling.  |
| `skinned.vert`     | Skeletal animation.     | Uses a `uniform mat4 bones[128]` array to transform vertices.        |
| `shadow.vert/frag` | Shadow map generation.  | Minimal shader that only outputs depth from the light's perspective. |
| `postprocess.frag` | Screen-space effects.   | Applied to a fullscreen quad; handles color grading or filters.      |

## Material Hierarchy

All materials inherit from the `Material` base class and are responsible for:

1.  Providing a `PipelineState` to configure the GPU.
2.  Implementing `setup()` to bind textures and update shader uniforms.

### Key Material Types

- **LitMaterial:** The primary gameplay material. Supports Blinn-Phong lighting with Albedo, Specular, Roughness, Ambient Occlusion, and Emissive maps.
- **TexturedMaterial:** Simple material that maps a single texture without complex lighting (used for UI or props).
- **TintedMaterial:** Multiplies a base color by mesh data for highlights or flash effects.

## Skeletal Animation (`skinned.vert`)

The `skinned.vert` shader is critical for character rendering. It processes `SkinnedVertex` data containing Position, Normal, Bone IDs, and Weights. The vertex position is calculated as:
`v_world_pos = sum(BoneMatrix[BoneID[i]] * Position * BoneWeight[i])`

## Forward Renderer Integration

For each `RenderCommand`, the renderer performs:

1.  **State Setup:** `material->pipelineState.setup()` sets culling, depth, and blending.
2.  **Shader Activation:** `material->shader->use()`.
3.  **Material Setup:** `material->setup()` binds textures to units (e.g., `GL_TEXTURE0` for Albedo) and sets uniforms.
4.  **Transformation:** Sets Model and View-Projection matrices.
5.  **Draw:** `mesh->draw()`.
