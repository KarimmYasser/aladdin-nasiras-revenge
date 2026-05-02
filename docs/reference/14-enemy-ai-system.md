# Enemy AI System

The Enemy AI system manages the lifecycle, behavioral states, and physical interactions of NPCs. It utilizes a **Finite State Machine (FSM)** architecture integrated with the ECS and physics engine to provide reactive behaviors.

## EnemyComponent

The `EnemyComponent` stores persistent data and current state for an individual enemy entity.

### Behavioral States (`EnemyState`)

- **PATROL:** Moves between a predefined list of waypoints.
- **CHASE:** Pursues the player when they enter the `detectionRange`.
- **ATTACK:** Engages the player when within `attackRange`.
- **DEAD:** Terminal state where the entity is marked for removal.

### Key Data Fields

| Field            | Description                                          |
| :--------------- | :--------------------------------------------------- |
| `health`         | Current health points.                               |
| `maxHealth`      | Initial health used for HUD scaling.                 |
| `damage`         | Damage dealt to the player per attack.               |
| `detectionRange` | Radius to switch from PATROL to CHASE.               |
| `attackRange`    | Distance required to transition to ATTACK.           |
| `speed`          | Movement speed during patrol and chase.              |
| `waypoints`      | List of `glm::vec3` coordinates for the patrol path. |

## EnemySystem Logic

The `EnemySystem` updates the state of all enemies by interfacing with the `PhysicsSystem` for movement and the `AnimatorComponent` for visual feedback.

### State Transitions

Transitions are evaluated every frame based on the distance to the player:

1.  **Patrol:** Iterates through `waypoints`. Moves to the next target upon reaching a waypoint threshold.
2.  **Chase:** Triggered when `distance < detectionRange`. Calculates direction to player and applies velocity.
3.  **Attack:** Triggered when `distance < attackRange`. Stops movement and triggers an attack animation.
4.  **Death:** Triggered when `health <= 0`. Triggers death animation and marks entity for removal.

### Physics & Grounding

To prevent floating or clipping, the system performs downward raycasts in the `PhysicsWorld`, adjusting the Y-position based on underlying terrain.

## Animation & UI

### Animation Synchronization

The system maps `EnemyState` to specific clips:

- **Idle/Patrol:** Plays "Walk" or "Idle".
- **Chase:** Plays "Run" at increased playback speed.
- **Attack:** Plays "Attack".
- **Dead:** Plays "Death" once before destruction.

### Health Bar HUD (ImGui)

Enemy health is visualized using overhead health bars:

- **Projection:** Projects world position to screen space using the active Camera's View-Projection matrix.
- **Rendering:** Uses ImGui's draw list to render red/green bars.
- **Optimization:** Only rendered for enemies within proximity to the player.

## Enemy Type Reference

| Enemy Type        | Model Asset         | Characteristics                            |
| :---------------- | :------------------ | :----------------------------------------- |
| **Golem of Sand** | `Golem_of_sand.fbx` | High health, slow movement, static patrol. |
| **Slime Enemy**   | `SlimeEnemy.obj`    | Low detection range, fast attack speed.    |
| **Golem**         | `Golem.fbx`         | Large detection range, high damage.        |
