# Aladdin Player Controller

The Aladdin Player Controller is a complex gameplay system responsible for managing the player's state, movement, combat, and inventory. It is implemented via the `AladdinControllerComponent` (data) and the `AladdinControllerSystem` (logic).

## AladdinControllerComponent

The component acts as a data container for the player's attributes. It is designed to be deserialized from JSON, allowing for tuning without recompilation.

### Key Attributes

- **Movement Speeds:** Defines `walkSpeed` and `runSpeed`.
- **Jump Mechanics:** Configures `jumpForce` and tracks double-jump status.
- **Combat:** Stores `attackDamage` and `appleDamage`.
- **Inventory:** Tracks `coins`, `gems`, `apples`, and `lives`.
- **Health & Status:** Manages `health` (max 100), `invincibilityTimer`, and `isDead` status.

| Attribute           | Type        | Description                                        |
| :------------------ | :---------- | :------------------------------------------------- |
| `cameraDistance`    | `float`     | Distance of the spring-arm camera from the player. |
| `cameraSensitivity` | `float`     | Rotation speed of the orbit camera.                |
| `firstPerson`       | `bool`      | Toggle for 1st/3rd person view modes.              |
| `spawnPosition`     | `glm::vec3` | Coordinates used for player respawn.               |

## AladdinControllerSystem

The system bridges user input with the ECS world, physics engine, and animation system.

### Movement and Physics

The system uses the `MovementComponent` to apply forces to the player's `RigidBody`. Movement vectors are calculated relative to the current camera orientation.

### Camera System: Spring-Arm Orbit

- **Orbit Logic:** The camera rotates around the player based on mouse movement.
- **Spring-Arm:** It maintains a set `cameraDistance` unless obstructed by geometry.
- **First-Person Toggle:** Snaps the camera to the player's head and hides the player mesh.

### Combat Mechanics

- **Melee Attack:** Triggered by input (e.g., 'F' or Mouse Left), it plays an animation and performs a proximity check to damage entities with an `EnemyComponent`.
- **Apple Throwing:** Consumes an apple from inventory and spawns a projectile entity with a high-velocity `MovementComponent`.

## Animation State Management

The controller synchronizes the physical state with skeletal animations using `Animator::play()`.

| Player State  | Animation Clip | Trigger Condition                         |
| :------------ | :------------- | :---------------------------------------- |
| **Idle**      | `Idle.fbx`     | Velocity is near zero and grounded.       |
| **Walking**   | `walk.fbx`     | Velocity > 0 and run modifier not active. |
| **Running**   | `run.fbx`      | Velocity > 0 and Shift key held.          |
| **Jumping**   | `jump.fbx`     | Space pressed; `isGrounded` is false.     |
| **Attacking** | `kick.fbx`     | Attack input detected.                    |

## Health, Respawn, and Inventory

### Health System

When health reaches 0, the `isDead` flag is set. If `lives > 0`, a life is decremented and the player respawns at `spawnPosition`.

- **Invincibility Frames:** After taking damage, an `invincibilityTimer` prevents further damage for ~1.5 seconds.

### Inventory Logic

- **Coins/Gems:** Collected via `CollectibleSystem`, incrementing component counts.
- **Apples:** Used as ammunition for ranged attacks.
- **Key:** A `hasKey` flag required by the `LevelExitSystem` for level progression.
