# Game Design & Level Structure

Aladdin: Nasira's Revenge is a three-level maze exploration experience set in Agrabah, utilizing a combination of hand-crafted scenes and procedurally influenced layouts.

## Narrative Premise & Level Progression

Progression is gated by a save system that tracks level unlocks and performance ratings.

- **Level 1: The Streets of Agrabah**
  - **Environment:** Marketplaces and desert houses.
  - **Goal:** Navigate the maze to find the exit portal.
  - **Unlock:** Available by default.
- **Level 2: The Royal Palace**
  - **Environment:** Interior palace structures and garden courtyards.
  - **Goal:** Locate a hidden key to unlock the palace gates and escape.
  - **Unlock:** Complete Level 1.
- **Level 3: The Final Confrontation**
  - **Environment:** The lair of Jafar and Nasira.
  - **Goal:** Trigger the final cinematic and survive the maze.
  - **Unlock:** Complete Level 2.

## Scoring & Star-Rating System

The game evaluates player performance using a 0-3 star rating system, persisted in `save.json`.

| Metric      | Description                              | Component/Logic         |
| :---------- | :--------------------------------------- | :---------------------- |
| **Coins**   | Primary currency; affects score.         | `CollectibleType::COIN` |
| **Enemies** | Points for defeating Golems and Slimes.  | `EnemyComponent`        |
| **Time**    | Speed bonus for quick completion.        | `PlayState::levelTimer` |
| **Gems**    | Rare collectibles for high star ratings. | `CollectibleType::GEM`  |

## Maze Generation Script (`scripts/generate_maze.py`)

Layouts are generated using a Python script that automates the creation of the `World` entity hierarchy.

### Key Algorithm: Iterative Backtracking

The script uses a randomized recursive backtracker. To optimize rendering and physics, it implements **Greedy Rectangular Merging**:

1.  Individual wall segments are generated.
2.  Adjacent segments with the same orientation are merged into single large `Transform` components.
3.  This significantly reduces draw calls and the number of `RigidBody` objects.

### Entity Mapping

The script maps maze cells to ECS configurations:

- **Walls:** Static `MeshRenderer` with box colliders.
- **Collectibles:** Entities with `CollectibleComponent` and Trigger `PhysicsBody`.
- **Enemies:** Entities with `EnemyComponent` and patrol waypoints.
- **Portals:** `RoomPortalSystem` triggers for teleportation.

## Level Configuration

Levels are defined in JSONC files within `config/levels/`. These files act as blueprints for the `World` class to instantiate entities during the `LoadingState`.

- **AladdinControllerComponent:** Defines spawn points and player stats.
- **LightComponent:** Configures directional sun lights and point-light torches.
- **CameraComponent:** Sets up third-person spring-arm parameters.

## Cutscene Integration

Level 3 features narrative-driven cutscenes using the `VideoCutsceneOverlay`.

- **Jafar Meeting:** Triggered in the central chamber.
- **Ending Cinematic:** Triggered upon reaching the final exit portal.
- **Implementation:** The `Mp4Decoder` uses FFmpeg to stream video directly onto an OpenGL texture quad.
