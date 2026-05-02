# Physics World & Collision

The physics system is built as a wrapper around the **ReactPhysics3D** library. It provides a high-level interface for managing 3D physics simulations, including rigid body dynamics, collider shapes, raycasting, and collision event handling.

## Physics World Implementation

The `PhysicsWorld` class manages the lifecycle of the `rp3d::PhysicsCommon` and `rp3d::PhysicsWorld` objects. It is responsible for creating and destroying rigid bodies and managing global properties like gravity.

### Key Internal Components

- **PhysicsCommon:** The factory class for all ReactPhysics3D objects.
- **rp3d::PhysicsWorld:** The actual simulation world where bodies interact.
- **EventListener:** A custom listener used to capture contact events (collisions) during the simulation step.

## Rigid Body Management

The `PhysicsWorld` provides methods to create rigid bodies with different motion types:

| Body Type     | Engine Function   | Description                                                                  |
| :------------ | :---------------- | :--------------------------------------------------------------------------- |
| **Static**    | `createRigidBody` | Infinite mass, zero velocity. Used for floors and walls.                     |
| **Kinematic** | `createRigidBody` | Moved manually via code; affects dynamic bodies but doesn't react to forces. |
| **Dynamic**   | `createRigidBody` | Fully simulated (gravity, forces, collisions).                               |

### Collider Shapes

Colliders define the physical volume of a body. Supported primitives:

- **Box:** Defined by half-extents.
- **Sphere:** Defined by a radius.
- **Capsule:** Defined by radius and height.

## Collision Detection & Raycasting

### Raycasting API

The `Raycast` function allows for line-of-sight checks or projectile pathing. It populates a `RaycastHit` structure:

- **`Entity*`**: The entity that was hit.
- **`distance`**: Distance from the ray origin.
- **`worldNormal`**: The surface normal at the hit point.
- **`worldPoint`**: The exact coordinates of the hit.

### Contact Events

The `EventListener` captures collisions during the simulation step. It stores pairs of colliding entities, which are then cleared at the end of every frame to ensure events do not persist beyond their trigger window.

## Frame Lifecycle & Updates

The `PhysicsWorld` is driven by the `PhysicsSystem`:

1.  **Step Simulation:** `world->update(deltaTime)` advances the physical state of all bodies.
2.  **Clear Events:** `clearEvents()` is called at the end of the frame to flush the `EventListener` buffers.
