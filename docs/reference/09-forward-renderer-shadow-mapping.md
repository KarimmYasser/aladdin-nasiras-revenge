# Forward Renderer & Shadow Mapping

The Forward Renderer is the primary rendering engine of the GFX-LAB framework. It manages the complete drawing pipeline, including light collection, shadow map generation, opaque/transparent object sorting, and post-processing.

## Render Command Structure

The renderer uses a `RenderCommand` structure to encapsulate all data required to draw a single mesh instance. This decouples scene traversal from actual OpenGL draw calls.

| Field | Type | Description |
| :--- | :--- | :--- |
| `localToWorld` | `glm::mat4` | Transformation matrix for the mesh. |
| `center` | `glm::vec3` | World-space center (used for depth sorting). |
| `mesh` | `Mesh*` | Pointer to the geometric data. |
| `material` | `Material*` | Pointer to the shading properties. |
| `pipelineState` | `PipelineState` | OpenGL state (depth test, blending, etc.). |
| `boneMatrices` | `std::vector<glm::mat4>` | Optional matrices for skinned meshes. |

## Light Collection & Shadow Mapping

### Shadow Map Generation
The renderer utilizes a high-resolution depth-only Framebuffer Object (FBO) for shadows.
*   **Resolution:** Hardcoded to 16k (**16384 x 16384**) for crisp shadows in large levels.
*   **Light Space Matrix:** For directional lights, an orthographic projection creates the `lightSpaceMatrix` to transform world coordinates into light-clip space.
*   **Shadow Pass:** The scene is rendered using a simplified `shadow.vert/frag` shader to record depth values.

## The Rendering Pipeline Execution

The `ForwardRenderer::render` function follows a strict execution order:

### 1. Command Collection and Sorting
The renderer iterates through the `ecs::World`. Entities with `MeshRendererComponent` or `SkinnedMeshRendererComponent` are converted into `RenderCommand` objects.
*   **Opaque Queue:** Materials with blending disabled.
*   **Transparent Queue:** Materials with blending enabled. These are sorted **back-to-front** by distance from the camera.

### 2. Skinned Mesh Path
If a command contains `boneMatrices`, the `skinned.vert` shader performs vertex skinning on the GPU using bone matrices and vertex weights.

### 3. Sky Sphere Rendering
If a sky material is configured, a large sphere is drawn around the camera. Depth testing ensures the sky always appears behind other geometry.

### 4. Post-process Pass
The scene is rendered into a `postprocessFrameBuffer`. After the main passes, a fullscreen quad is drawn using a post-processing shader for effects like color correction or bloom.

## Implementation Details

*   **`initialize()`:** Allocates the shadow and post-process FBOs.
*   **`collectLights()`:** Scans the ECS for `LightComponent` instances.
*   **`setupPipelineState()`:** Applies culling, depth, and blending settings to the OpenGL context.
