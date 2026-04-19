# Disney's Aladdin in Nasira's Revenge (Phase 2): Implementation Plan & Architecture

This plan outlines the architecture, feature breakdown, and workload distribution for Phase 2 of the 3D game engine project to implement a Disney's Aladdin in Nasira's Revenge PS1-style clone.

## Goal Description
To successfully deliver Phase 2 of CMP3060, transitioning from a basic rendering framework into a fully functional 3D action-platformer featuring custom Aladdin models, Lighting, Collision, and Gameplay Logic using the ECS paradigm.

## User Review Required
> [!IMPORTANT]
> Since you have 4 team members, please review the Workload Distribution section below to ensure the assignment of tasks aligns with everyone's strengths. 
> Also, confirm any additional external libraries you plan to use for physics/collision (e.g., specific libraries or custom AABB implementations) or audio.


## 1. Feature Organization

To build an Aladdin in Nasira's Revenge clone, Phase 2 will be broken down into the following major features:

### A. Core Engine Enhancements
- **Lighting System implementation**: Shaders must handle ambient, directional, and point/spotlights for dynamic scenes.
- **Lit Materials**: New `LitMaterial` class adding albedo, specular, roughness, ambient occlusion, and emission maps.
- **Advanced Post-Processing**: Implementing at least one new effect (e.g., bloom, motion blur, or depth-of-field) as required.

### B. Game Systems & Logic (ECS)
- **Physics & Collision System**: Implement Axis-Aligned Bounding Box (AABB) or Sphere-based collision detection.
- **Aladdin Player Controller**: A new ECS System and Component (`AladdinControllerComponent`) for player movement, jumping, sword attacks, and apple throwing.
- **Enemy AI System**: Basic patrol logic or chasing behavior using custom Components (e.g., `PatrolComponent`) for palace guards and skeleton warriors.
- **Collectible System**: Logic to handle ancient coins, gems, and clay pots (breaking them via collision & updating score).

### C. Game Architecture & States
- **State Flow**: Fully flesh out `MenuState`, `PlayState`, and transition logic (Game Over, Victory screens) handled by the `Application` base class.
- **Scene Deserialization Layout**: Using JSON configurations (`config/`) to load Aladdin models, enemy models, breakable props, and Level Layout without hardcoding.


---

## 2. Architecture & Design Patterns

The engine's architecture provides a clean separation. Here's how to map the new features into the existing patterns:

### Entity-Component-System (ECS)
*   **Components** are plain data. You should add:
    *   `LightComponent`: Stores light properties (color, intensity, type, cone angles).
    *   `ColliderComponent`: Stores collision shape type and bounds.
    *   `AladdinControllerComponent`: Stores player-specific data like velocity, jump height, isGrounded, swordTimer, appleCount.
    *   `EnemyComponent`: Stores health, damage, and movement speed.
*   **Systems** handle logic. You should add:
    *   `PhysicsSystem`: Loops through all entities that have a `ColliderComponent` to resolve intersections/gravity.
    *   `AladdinControllerSystem`: Listens to `Keyboard` inputs from the `Application`, updates Aladdin's velocity, and transitions his state (idle, run, sword-slash, throw).
    *   `ForwardRenderer` (Exisiting): Will be updated to loop through all `LightComponents` and pass their parameters as uniforms into the shaders.

### Serialization & Deserialization
*   Your engine relies on `deserializeAllAssets()` and `world.deserialize()`. 
*   Update `source/common/components/component-deserializer.hpp` to parse your newly added components (e.g., parsing `type: "Light"` to instantiate a `LightComponent`).
*   Create new `.json` layout files in `config/` that act as your Aladdin levels (Agrabah streets, palace dungeons, etc.).

### Application Game States
*   Inherit specific states from `our::State`.
*   `MenuState`: Displays your UI (using ImGui or rendered textures over a 2D quad), listens to 'Enter' key to call `getApp()->changeState("play");`.
*   `PlayState`: Manages initialization (loads Agrabah level #1 config), tracks score/health, handles pause menus, and calls the updates of your various Logic Systems (`PhysicsSystem`, `AladdinControllerSystem`, etc.).

---

## 3. Workload Distribution (Team of 4)

To work efficiently and minimize GitHub merge conflicts, divide the tasks by module:

### Dev 1: Graphics & Engine Lead (The Renderer)
*   **Domain:** Shaders, Forward Renderer, Materials.
*   **Tasks:** 
    *   Implement `LightComponent` and modify the Forward Renderer to send multi-light uniforms.
    *   Create the `LitMaterial` and support PBR-like or Phong shading in `.frag` shaders.
    *   Develop the new custom post-processing effect required for Phase 2.

### Dev 2: Physics & Collision Lead (The Interactor)
*   **Domain:** Physics System, Math, Collision Components.
*   **Tasks:**
    *   Research and implement 3D Collision Detection (either manually with AABB/Spheres or integrating a lightweight external physics library as permitted).
    *   Build the `PhysicsSystem` to handle entity overlaps, gravity, and ground-checking.
    *   Implement hit detection for Aladdin's sword slash and apple projectiles hitting objects.

### Dev 3: Gameplay Programmer (The Aladdin Controller)
*   **Domain:** Player Logic, Enemy Logic, Input.
*   **Tasks:**
    *   Build the `AladdinControllerSystem` and bind the keyboard/mouse inputs for jumping, sword-slashing, and apple-throwing.
    *   Develop the logic for enemies (palace guards, skeletons) moving around the map (AI Patrol constraints).
    *   Write the system that handles collecting items (coins, gems, health pickups) and managing Player Health/Lives.

### Dev 4: Level Designer & UI/State Manager (The World Builder)
*   **Domain:** Deserialization, Assets, Game States.
*   **Tasks:**
    *   Source 3D Models (Aladdin, Palace Guards, Pots, Coins, Agrabah environment geometry) and prepare their textures.
    *   Write the JSON scene definitions (`config/agrabah_level1.json`) pulling together models, lights, and colliders.
    *   Manage the `MenuState`, `GameOverState`, the UI overlays (Score, Health, Lives), and the transition logic.

## Open Questions

> [!WARNING]
> 1. Will you be writing your own Math for collision/gravity or integrating an external library like Bullet3D or ReactPhysics3D?
> 2. Have you settled on a specific new Post-Processing effect to implement (e.g. Bloom, Depth of Field, warm Arabian color-grading)?
> 3. Does the team agree with the skill distribution outlined in the Workload section?

## Verification Plan
1. **Graphics Core Test**: Setup a simple JSON scene with a Sphere, plane, and multiple light sources to verify the new `LitMaterial`.
2. **Controller Test**: A barebones plain level where Aladdin can jump, slash, and fall. Verify gravity and basic collision.
3. **Integration Test**: Pulling the full JSON scene level with models and lighting. Proceeding from "Main Menu" -> "Agrabah Level 1" -> "Win Screen".
