# Physics System Update Loop

The `PhysicsSystem` bridges the ECS with the ReactPhysics3D engine. It manages body lifecycles, synchronizes transforms, and implements a fixed-timestep simulation loop to ensure deterministic behavior.

## The four-phase update process

The core logic resides in `PhysicsSystem::update()`, which executes the following sequence:

### Phase 1: Body and Collider Initialization

The system ensures entities with a `RigidBodyComponent` have a corresponding `rp3d::RigidBody`. If a component is "dirty" or newly added, it initializes the body and its colliders (Box, Sphere, or Capsule).

### Phase 2: ECS → Physics Kinematic Sync

For **Kinematic** entities (like moving platforms), the system manually updates the `rp3d::Transform` to match the ECS `Transform`. This allows gameplay logic to drive physical objects that still affect dynamic bodies.

### Phase 2.5: Movement Velocity Injection

The system translates intended velocity from the `MovementComponent` into linear and angular velocities for the `rp3d::RigidBody`. This is how character controllers (like Aladdin) move within the simulation.

### Phase 3: Fixed-Timestep Accumulation

To maintain stability, physics steps are decoupled from the variable frame rate using an accumulator:

1.  Add `deltaTime` to the `accumulator`.
2.  While `accumulator >= fixedDeltaTime`:
    - Call `PhysicsWorld::update(fixedDeltaTime)`.
    - Subtract `fixedDeltaTime` from `accumulator`.
3.  Includes a `maxStepsPerFrame` cap to prevent a "spiral of death" during performance drops.

### Phase 4: Physics → ECS Transform Sync

After simulation, the results are written back to the ECS. For **Dynamic** bodies, the new position and rotation calculated by ReactPhysics3D are applied to the entity's `Transform`.

## Simulation Constants

| Constant           | Role                 | Purpose                                                     |
| :----------------- | :------------------- | :---------------------------------------------------------- |
| `fixedDeltaTime`   | Simulation Step Size | Usually `1/60s`. Ensures consistent gravity and collisions. |
| `accumulator`      | Time Buffer          | Stores leftover time to stay in sync with real-time.        |
| `maxStepsPerFrame` | Safety Cap           | Limits steps per update to prevent performance freezes.     |

## System Mapping

| Phase                  | Logic Location          | Key Code Entities                                                         |
| :--------------------- | :---------------------- | :------------------------------------------------------------------------ |
| **Initialization**     | `PhysicsSystem::update` | `RigidBodyComponent::isDirty`, `PhysicsWorld::createRigidBody`            |
| **Kinematic Sync**     | `PhysicsSystem::update` | `rp3d::RigidBody::setTransform`, `Transform::getLocalToWorldMatrix`       |
| **Velocity Injection** | `PhysicsSystem::update` | `MovementComponent::linearVelocity`, `rp3d::RigidBody::setLinearVelocity` |
| **Simulation**         | `PhysicsSystem::update` | `rp3d::PhysicsWorld::update`, `PhysicsSystem::accumulator`                |
| **Dynamic Sync**       | `PhysicsSystem::update` | `rp3d::RigidBody::getTransform`, `Transform::setLocalTranslation`         |
