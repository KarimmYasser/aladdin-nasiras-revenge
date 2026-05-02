# Animation System

The Animation System in the Aladdin engine provides a complete skeletal animation pipeline, enabling complex character movements like Aladdin’s jumping, sword swinging, and enemy patrol cycles. The system handles the transition from offline assets (FBX/DAE) to real-time GPU skinning by interpolating bone transforms and uploading final transformation matrices to specialized shaders.

## System Overview

The animation pipeline is divided into three primary stages:

1.  **Loading:** Importing skeletal hierarchies and animation tracks using Assimp.
2.  **State Management:** Tracking playback time, blending between clips (crossfading), and calculating local bone transforms.
3.  **Skinning:** Computing the final global bone matrices and applying them to vertices on the GPU.

## 1. Animation Loading & Data Types

The engine utilizes **Assimp** to parse industry-standard formats. During the loading process, the `AnimationLoader` extracts the bone hierarchy and keyframe data into engine-specific structures.

- **Bone Hierarchy:** Represented by `NodeData`, which stores the transformation and children of each joint in the skeleton.
- **Keyframes:** Stored as `KeyFrame` objects containing timestamps and transformation data (position, rotation, scale).
- **Skinned Mesh:** Unlike static meshes, these include `SkinnedVertex` data, which maps each vertex to up to four bones using weights.

## 2. Runtime Animator & System

The runtime execution of animations is handled by the `Animator` class and orchestrated by the `AnimationSystem`.

- **Animator Class:** Manages the playback state for a single entity. It supports playing clips, pausing, and smooth **crossfading** between two different animations (e.g., transitioning from `idle` to `run`).
- **AnimationSystem:** An ECS system that runs every frame. It iterates through all entities possessing an `AnimatorComponent` and a `SkinnedMeshRendererComponent`, triggers the matrix calculations, and ensures the resulting matrices are sent to the renderer.
- **GPU Skinning:** The final bone matrices are passed to the `skinned.vert` shader, which transforms vertices in real-time based on the bone weights and the current pose.

## Key Features

| Feature                  | Implementation Detail                                                         |
| :----------------------- | :---------------------------------------------------------------------------- |
| **Crossfading**          | Smoothly interpolates between two animation states over a specified duration. |
| **Root Motion**          | Option to suppress or utilize root bone movement for in-place locomotion.     |
| **Multi-Bone Influence** | Supports up to 4 bones per vertex for high-quality skinning.                  |
| **Assimp Integration**   | Supports `.fbx` and `.dae` (Collada) formats.                                 |
