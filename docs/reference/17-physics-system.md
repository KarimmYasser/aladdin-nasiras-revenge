# Physics System

The Physics System in the GFX-LAB engine provides a robust integration of the **ReactPhysics3D** library. It is designed to handle collision detection and rigid body dynamics for both the Aladdin player and environmental entities. The system is bridged to the engine's Entity Component System (ECS), allowing physics properties to be defined via JSON configurations and synchronized with the `Transform` component of entities.

## System Architecture

The physics integration is divided into two primary layers: a wrapper for the physics world and a system loop that manages the simulation.

### 1. Physics World & Collision

The `PhysicsWorld` class acts as a high-level wrapper around the `rp3d::PhysicsCommon` and `rp3d::PhysicsWorld` objects. It manages the creation and destruction of rigid bodies and colliders.

- **Body Types:** Supports **Static** (environment), **Kinematic** (scripted movement), and **Dynamic** (fully simulated) bodies.
- **Collider Shapes:** Supports Box, Sphere, and Capsule primitives.
- **Raycasting:** Provides a `RaycastHit` API for line-of-sight checks and ground detection.
- **Event Handling:** Manages contact callbacks for gameplay triggers (e.g., collecting items or hitting hazards).

### 2. Physics System Update Loop

The `PhysicsSystem` is an ECS system that executes every frame. It synchronizes the state between the engine's `Transform` components and the ReactPhysics3D simulation.

The update follows a strict four-phase process:

1.  **Initialization:** Creating physics bodies for newly added components.
2.  **ECS to Physics Sync:** Moving kinematic bodies based on game logic.
3.  **Simulation Stepping:** Advancing the physics clock using a fixed delta time and an accumulator to ensure deterministic behavior.
4.  **Physics to ECS Sync:** Updating entity `Transform` positions/rotations based on the results of the dynamic simulation.

## Related Components

| Component              | Role                | Description                                                                |
| :--------------------- | :------------------ | :------------------------------------------------------------------------- |
| **RigidBodyComponent** | Physical Properties | Holds the RP3D body handle, mass, and damping.                             |
| **ColliderComponent**  | Collision Shape     | Defines the shape, friction, and bounciness.                               |
| **MovementComponent**  | Velocity Injection  | Used to inject velocities before the simulation step.                      |
| **Transform**          | Spatial Truth       | The source of truth for kinematic bodies and destination for dynamic ones. |
