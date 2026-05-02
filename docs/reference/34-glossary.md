# Glossary

This glossary provides a comprehensive reference for technical terms, abbreviations, component names, and system concepts used within the Aladdin: Nasira's Revenge codebase.

## Core Engine & Architecture

### Application & State Machine

The top-level controller for the game lifecycle.

- **Application:** The main class that runs the game loop, handles events, and manages the current State.
- **State:** An abstract base class for game logic modules (e.g., `PlayState`, `MenuState`). Key lifecycle methods: `onInitialize`, `onDraw`, `onDestroy`.

### ECS (Entity Component System)

The architectural pattern used to manage game objects.

- **World:** A container for all active entities. Handles creation, destruction, and system updates.
- **Entity:** A general-purpose object identified by a unique ID. Acts as a container for components.
- **Component:** Data containers attached to entities. Logic is handled by external systems.

## Rendering Subsystem

### Forward Renderer

The primary rendering pipeline that processes entities in two passes: opaque and transparent.

- **ForwardRenderer:** Collects `RenderCommand` objects from the world and submits them to OpenGL.
- **RenderCommand:** A struct containing the mesh, material, and transformation matrix for a single draw call.
- **Shadow Mapping:** Uses a high-resolution depth buffer and a `lightSpaceMatrix` to calculate shadows.

### Material System

Defines how surfaces interact with light and shaders.

- **LitMaterial:** A Blinn-Phong implementation supporting albedo, specular, and emission maps.
- **PipelineState:** Encapsulates OpenGL state settings like depth testing, face culling, and alpha blending.

## Gameplay & Domain Concepts

### Aladdin Controller

- **AladdinControllerComponent:** Stores player-specific data such as health, coins, apples, and movement speeds.
- **Spring-Arm Camera:** Camera logic that follows the player while maintaining a fixed distance and handling collisions.

### Enemy AI

- **EnemyComponent:** Manages states like **PATROL**, **CHASE**, **ATTACK**, and **DEAD**.
- **Waypoints:** Coordinate sets defined in JSON used for patrol movement logic.

### Level Progression

- **LevelExitSystem:** Monitors if the player has reached the exit and possesses the required keys.
- **RoomPortalSystem:** Handles instantaneous teleportation between sections, often with a camera blackout.

## Physics & Animation

### Physics World

- **PhysicsWorld:** Manages the simulation step, gravity, and rigid body creation using ReactPhysics3D.
- **PhysicsSystem:** Synchronizes ECS `Transform` data with rigid bodies in four phases (Init, Kinematic Sync, Stepping, and Dynamic Sync).

### Skeletal Animation

- **SkinnedMesh:** A mesh containing `SkinnedVertex` data (weights and bone IDs).
- **Animator:** Computes bone matrices for a given `AnimationClip` and time offset.
- **BoneInfo:** Contains the `offsetMatrix` used to transform vertices into bone space.

## Utility & External Terms

| Term           | Definition                                                                          |
| :------------- | :---------------------------------------------------------------------------------- |
| **JSONC**      | JSON with Comments. Used for level and application configuration.                   |
| **LFS**        | Git Large File Storage. Used for large assets like `.obj` and `.mp4`.               |
| **ImGui**      | Immediate Mode GUI. Used for HUD and debug overlays.                                |
| **ma_sound**   | Miniaudio sound instance. Used for BGM and SFX.                                     |
| **MVP**        | Model-View-Projection matrix.                                                       |
| **Z-Fighting** | Visual artifact where surfaces overlap; prevented by wall-merging logic in scripts. |
