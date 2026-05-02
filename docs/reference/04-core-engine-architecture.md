# Core Engine Architecture

The GFX-LAB engine is a custom C++ framework built on OpenGL 3.3, designed with a modular architecture that separates application lifecycle, game state logic, and entity management. The engine follows a hybrid approach, combining a state machine for high-level flow control with an Entity Component System (ECS) for gameplay logic and scene management.

## System Overview

The engine is structured into three primary layers that work in tandem to drive the game experience:

1.  **Application Layer:** Manages the window lifecycle (via GLFW), input handling, and the high-level State Machine.
2.  **State Machine:** Handles transitions between different game modes (e.g., Menu, Loading, Play) and manages the active `World` instance.
3.  **ECS (Entity Component System):** The foundational data structure for the game world, where **Entities** are containers for **Components**, and **Systems** process them.

## 1. Application & State Machine

The `Application` class is the heart of the engine. It initializes the OpenGL context, manages the main loop, and routes input events to the current `State`. The state machine allows the game to transition between different contexts, such as moving from the `LoadingState` to the `PlayState` once assets are ready.

- **Lifecycle:** Each state implements `onInitialize`, `onDraw`, and `onDestroy`.
- **Input:** Key and mouse events are dispatched via `onKeyEvent` and `onCursorMoveEvent`.

## 2. Entity Component System (ECS)

The ECS architecture provides a flexible way to define game objects. Instead of deep inheritance hierarchies, functionality is added to Entities through Components (e.g., `Transform`, `CameraComponent`, `MeshRendererComponent`).

- **World:** Acts as a container for all entities and provides the `update()` loop where systems are executed.
- **Transform:** Every entity has a `Transform` component that handles parent-child relationships and matrix calculations (`getLocalToWorldMatrix`).
- **Serialization:** The world can be populated dynamically from JSON files using the `deserialize` utility.

## 3. Asset Loading & Deserialization

The engine uses a centralized `AssetLoader` to manage the lifecycle of textures, meshes, and shaders. This system is integrated with the ECS deserializer, allowing entire levels to be defined in `.jsonc` files.

- **Progressive Loading:** The `LoadingState` utilizes the `AssetLoader` to load heavy resources (like 16k shadow maps or skeletal animations) without freezing the main thread.
- **Resource Mapping:** Assets are identified by unique strings in JSON configurations, which the `AssetLoader` maps to GPU handles.

## Subsystem Integration

| Subsystem     | Primary Code Entity  | Role                                                   |
| :------------ | :------------------- | :----------------------------------------------------- |
| **Rendering** | `ForwardRenderer`    | Processes `MeshRendererComponent` and `LightComponent` |
| **Physics**   | `PhysicsSystem`      | Wraps `ReactPhysics3D` and syncs `Transform`           |
| **Animation** | `AnimationSystem`    | Updates `AnimatorComponent` and `SkinnedMeshRenderer`  |
| **Input**     | `Keyboard` / `Mouse` | Polled by `Application` and passed to `State`          |
