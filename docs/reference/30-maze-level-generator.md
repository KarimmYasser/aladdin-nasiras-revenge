# Maze Level Generator

The **Maze Level Generator** is a standalone Python utility (`scripts/generate_maze.py`) that automates the creation of complex, playable maze environments. It generates JSONC files defining the structural geometry, gameplay items, NPCs, and level transitions.

## Implementation Details

The generator translates a logical grid into 3D world coordinates and ECS entities.

### Maze Generation Algorithm

Uses an **Iterative Backtracking** approach:

- **Initialization:** Creates a 2D grid of `Cell` objects, each with four walls.
- **Carving:** Selects random cells, pushes them to a stack, and carves paths by removing walls between neighbors.
- **Backtracking:** When no unvisited neighbors remain, it pops from the stack to backtrack.
- **Validation:** Ensures all cells are reachable, creating a solvable maze.

### Wall Merging Optimization

To prevent **Z-fighting** and reduce draw call overhead, the script performs **Greedy Rectangular Merging**. It identifies contiguous wall segments and merges them into a single, scaled rectangular prism entity.

## Entity Builder Functions

These functions generate the dictionary structures that the engine parses into game entities:

| Function              | Logic                    | Key Components                      |
| :-------------------- | :----------------------- | :---------------------------------- |
| `build_wall_entity`   | Creates a static box.    | `MeshRenderer`, static `RigidBody`. |
| `build_coin_entity`   | Places a collectible.    | `CollectibleComponent`.             |
| `build_enemy_entity`  | Spawns a Golem or Slime. | `EnemyComponent`, patrol waypoints. |
| `build_portal_entity` | Creates a trigger zone.  | `RoomPortalComponent`.              |

## CLI Arguments and Usage

| Argument             | Description                                  | Default                         |
| :------------------- | :------------------------------------------- | :------------------------------ |
| `--rows`             | Number of cells along the Z-axis.            | 10                              |
| `--cols`             | Number of cells along the X-axis.            | 10                              |
| `--seed`             | Integer seed for reproducibility.            | Random                          |
| `--output`           | Path to save the resulting `.jsonc`.         | `config/levels/generated.jsonc` |
| `--write-app-config` | Updates `app.json` to point to the new maze. | False                           |

## Data Flow: From Script to Engine

1.  **Generation:** The script outputs a JSONC file containing a `"scene"` array of entities.
2.  **Loading:** The `PlayState` calls `AssetLoader` to read the file.
3.  **Deserialization:** `World::deserialize` iterates through the JSON array.
4.  **Component Creation:** `ComponentDeserializer` instantiates the corresponding C++ classes (e.g., `MeshRenderer`).
5.  **Physics Sync:** Entities with physics properties are added to the ReactPhysics3D world via the `PhysicsSystem`.
