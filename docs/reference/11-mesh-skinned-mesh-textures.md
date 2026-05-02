# Mesh, Skinned Mesh & Textures

This page details the implementation of 3D geometry and texture mapping within the GFX-LAB engine. It covers the data structures for static and skeletal meshes, the OpenGL abstraction for vertex and index buffers, and the texture sampling system.

## Mesh Data Structures

The engine distinguishes between static geometry (`Mesh`) and animated skeletal geometry (`SkinnedMesh`).

### Vertex Layouts

- **Vertex:** Used for static meshes. Contains `position`, `normal`, and `texCoord` (UV).
- **SkinnedVertex:** Extends basic vertex data by adding `bone_ids` and `bone_weights`. Each vertex can be influenced by up to 4 bones.

### Mesh Implementation

The `Mesh` class manages the OpenGL lifecycle of geometry.

1.  **Creation:** Creates and binds a Vertex Array Object (VAO), Vertex Buffer Object (VBO), and Element Buffer Object (EBO).
2.  **Data Upload:** Uploads vertex and index data to the GPU using `GL_STATIC_DRAW`.
3.  **Rendering:** The `draw()` function binds the VAO and issues a `glDrawElements` call.

### Skinned Mesh Implementation

`SkinnedMesh` mirrors the `Mesh` class but utilizes `SkinnedVertex` data. It is specifically designed to work with the `AnimationSystem` and `AnimatorComponent` for GPU skinning.

- **BoneInfo:** Stores the offset matrix (mesh space to bone space) and the bone's index within the hierarchy.

## Mesh Management & Utilities

- **File Loading:** `loadOBJ` uses `tinyobjloader` to parse `.obj` files.
- **Primitives:** Functions like `sphere` and `cuboid` generate vertex/index arrays for standard shapes programmatically.

## Textures and Samplers

The engine handles 2D textures via the `Texture2D` class and configures their behavior via the `Sampler` class.

### Texture2D

- **Creation:** Generates a texture handle via `glGenTextures`.
- **Loading:** Uses `stb_image` to load pixel data from disk (PNG, JPG, etc.). It typically converts data to `GL_RGBA8` or `GL_RGB8`.

### Samplers

The `Sampler` class manages how textures are sampled in shaders:

- **Filtering:** Configures `GL_TEXTURE_MIN_FILTER` and `GL_TEXTURE_MAG_FILTER` (e.g., `GL_NEAREST`, `GL_LINEAR`).
- **Wrapping:** Configures `GL_TEXTURE_WRAP_S` and `GL_TEXTURE_WRAP_T` (e.g., `GL_REPEAT`, `GL_CLAMP_TO_EDGE`).

## Key Utility Functions

| Task                      | Function                   | File                |
| :------------------------ | :------------------------- | :------------------ |
| **Generate Mesh VAO/VBO** | `Mesh::Mesh`               | `mesh.cpp`          |
| **Generate Skinned Mesh** | `SkinnedMesh::SkinnedMesh` | `skinned-mesh.cpp`  |
| **Load Texture Data**     | `texture_utils::loadImage` | `texture-utils.cpp` |
| **Apply Sampler State**   | `Sampler::bind`            | `sampler.cpp`       |
| **Capture Framebuffer**   | `screenshot_png`           | `screenshot.cpp`    |
