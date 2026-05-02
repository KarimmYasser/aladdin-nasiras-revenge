# Rendering Pipeline

The rendering pipeline in the GFX-LAB engine is a modular system designed to transform 3D scene data into a final rendered frame. It utilizes a forward rendering architecture with support for shadow mapping, skeletal animation, and post-processing. The pipeline is managed primarily by the `ForwardRenderer` system, which coordinates the interaction between ECS components, materials, and OpenGL state.

## High-Level Architecture

The pipeline collects data from the `World`, specifically targeting `CameraComponent`, `MeshRendererComponent`, `SkinnedMeshRendererComponent`, and `LightComponent`. The rendering process is divided into distinct phases: shadow map generation, opaque geometry pass, transparent geometry pass, and a final post-processing blit.

## Core Pipeline Components

### 1. Forward Renderer & Shadow Mapping

The `ForwardRenderer` is the central coordinator of the frame.

- **Shadow Mapping:** It performs light collection to find the primary directional light for shadows and generates a high-resolution depth map (16k).
- **Command Queues:** It manages two `RenderCommand` queues—one for opaque objects and one for transparent objects (sorted back-to-front)—to ensure correct alpha blending.

### 2. Materials, Shaders & Pipeline State

The engine uses a flexible material system:

- **Material:** Encapsulates a `ShaderProgram` and a `PipelineState`.
- **LitMaterial:** Supports the Blinn-Phong reflection model, utilizing albedo, specular, and emission maps.
- **PipelineState:** Manages OpenGL global settings like depth masking, blending functions, and face culling.
- **Shaders:** Uses `light.vert/frag` for standard lighting and `skinned.vert` for GPU-side vertex skinning.

### 3. Mesh, Skinned Mesh & Textures

This layer handles the raw GPU data:

- **Mesh & SkinnedMesh:** Store Vertex Array Objects (VAOs) and Element Buffer Objects (EBOs).
- **Data Types:** Supports standard `Vertex` and `SkinnedVertex` data (including bone IDs and weights).
- **Textures:** Managed via `Texture2D` and `Sampler` objects to control filtering and wrapping.

## Frame Execution Sequence

The `ForwardRenderer::render()` function executes the following sequence every frame:

| Step | Action               | Description                                                                     |
| :--- | :------------------- | :------------------------------------------------------------------------------ |
| 1    | **Light Collection** | Identifies all `LightComponent` entities and selects the primary shadow caster. |
| 2    | **Shadow Pass**      | Renders scene depth from the light's perspective into the shadow map FBO.       |
| 3    | **Setup Camera**     | Updates camera matrices and configures the main viewport.                       |
| 4    | **Opaque Pass**      | Renders all opaque `RenderCommand` objects using their assigned materials.      |
| 5    | **Skybox**           | Renders the sky sphere if a sky material is provided.                           |
| 6    | **Transparent Pass** | Renders sorted transparent objects with alpha blending enabled.                 |
| 7    | **Post-Process**     | Applies full-screen effects (e.g., tinting, distortion) before final output.    |
