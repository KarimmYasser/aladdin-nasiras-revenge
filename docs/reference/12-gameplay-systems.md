# Gameplay Systems

The gameplay logic in Aladdin: Nasira's Revenge is driven by specialized ECS systems. These systems process entities with specific components to handle player movement, enemy AI, world interactions, and narrative delivery. The primary entry point for gameplay execution is the `PlayState`, which manages the lifecycle of these systems and handles level-specific triggers.

## System Architecture Overview

Most systems iterate over a filtered set of entities in the `World` and update their state based on delta time and user input.

| System                      | Primary Component(s)         | Responsibility                                 |
| :-------------------------- | :--------------------------- | :--------------------------------------------- |
| **AladdinControllerSystem** | `AladdinControllerComponent` | Player input, movement, camera, and inventory. |
| **EnemySystem**             | `EnemyComponent`             | AI state machine (Patrol/Chase/Attack).        |
| **CollectibleSystem**       | `CollectibleComponent`       | Item pickup logic and animations.              |
| **HazardSystem**            | `HazardComponent`            | Environmental damage and traps.                |
| **DialogueSystem**          | `DialogueComponent`          | NPC interaction and UI typewriter text.        |
| **PhysicsSystem**           | `RigidBody`, `Collider`      | Collision resolution and kinematic sync.       |

## 1. Aladdin Player Controller

The `AladdinControllerSystem` manages the player's interaction with the world:

- **Camera:** Third-person orbit camera with a "spring-arm" feel and toggleable first-person view.
- **Movement:** Multi-state controller supporting walking, running, and jumping.
- **Inventory:** Manages coins, gems, and apples.

## 2. Enemy AI System

Enemies are governed by the `EnemySystem`, which implements a finite state machine (FSM). Entities transition between **PATROL**, **CHASE**, **ATTACK**, and **DEAD** states. The system handles health, damage application, and triggers animations via the `AnimationSystem`.

## 3. Collectibles, Hazards & Level Progression

Interactive world objects are managed by:

- **CollectibleSystem:** Handles rotation/bobbing and proximity-based pickup.
- **HazardSystem:** Manages threats that deplete Aladdin's health.
- **LevelExitSystem:** Logic for transitioning between levels (requires keys).
- **PortalSystem:** Manages teleports between different rooms or map locations.

## 4. Dialogue & Cutscene Systems

Narrative is delivered through:

- **DialogueSystem:** Triggers typewriter-style text when near NPCs (e.g., the Genie). It can trigger world events like spawning items.
- **VideoCutsceneOverlay:** Uses FFmpeg to decode and render MP4 files for fullscreen cinematic playback.
