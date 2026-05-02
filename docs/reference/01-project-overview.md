# Project Overview

Aladdin: Nasira's Revenge is a 3D action-adventure maze game built using the GFX-LAB engine, a custom-built C++ OpenGL framework. The project recreates the atmosphere of Agrabah through a three-level progression system, featuring skeletal animations, a third-person camera system, and a custom physics integration.

The codebase is designed as a data-driven Entity Component System (ECS), where game worlds and entities are primarily defined in JSONC configuration files and instantiated at runtime.

## Technology Stack

The project leverages several industry-standard libraries to handle low-level subsystems:

| Category      | Libraries                               |
| :------------ | :-------------------------------------- |
| **Rendering** | OpenGL (via GLAD), GLFW, GLM            |
| **Physics**   | ReactPhysics3D 0.9.3                    |
| **Animation** | Assimp (Open Asset Import Library)      |
| **Audio**     | miniaudio                               |
| **Video**     | FFmpeg (for MP4 cutscenes)              |
| **UI**        | Dear ImGui                              |
| **Data**      | nlohmann/json, tinyobjloader, stb_image |

## System Architecture

The engine follows a standard game loop managed by the `Application` class. It utilizes a State Machine to transition between menus, loading screens, and gameplay.

## Major Subsystems

### 1. ECS and Data-Driven Design

The game uses a custom ECS located in `source/common/ecs/`. Entities are containers for components (e.g., `Transform`, `Camera`, `MeshRenderer`). The entire game world, including lighting and object placement, is loaded from JSON configurations via the `ComponentDeserializer`.

### 2. Rendering Pipeline

The `ForwardRenderer` handles the drawing of both static and skinned meshes. It supports shadow mapping (using a 16k depth buffer), Blinn-Phong lighting, and post-processing effects.

### 3. Gameplay & Physics

Gameplay logic is divided into specialized systems like the `AladdinControllerSystem` and `EnemySystem`. Physics is handled by a wrapper around `ReactPhysics3D`, which synchronizes engine transforms with rigid body simulations.

### 4. Animation & Audio

Skeletal animations are processed using `Assimp` and played back via the `Animator` component, which calculates bone matrices for the GPU. Audio is managed by a `miniaudio` singleton supporting background music and spatialized sound effects.

## Development Team (Team 15)

| Name                        | GitHub Profile                                       |
| :-------------------------- | :--------------------------------------------------- |
| Karim Yasser Ali Azab       | [@KarimmYasser](https://github.com/KarimmYasser)     |
| Kerolos Mohsen Alfy         | [@kerolos-mohsen](https://github.com/kerolos-mohsen) |
| Ahmed Kamal Soliman Mahmoud | [@ahmedkamal14](https://github.com/ahmedkamal14)     |
| Mario Raafat Ayad Habib     | [@MarioRaafat](https://github.com/MarioRaafat)       |
