# Physics System Documentation & Usage Guide

**Date:** April 18, 2026  
**Engine:** Crash Bandicoot Game Engine  
**Physics Library:** ReactPhysics3D 0.9.3

---

## 📋 Table of Contents
1. [System Overview](#system-overview)
2. [Key Components](#key-components)
3. [Physics Loop (4 Phases)](#physics-loop-4-phases)
4. [Configuration & Setup](#configuration--setup)
5. [JSON Configuration Examples](#json-configuration-examples)
6. [Common Usage Patterns](#common-usage-patterns)
7. [Important Notes & Gotchas](#important-notes--gotchas)
8. [API Reference](#api-reference)

---

## System Overview

The physics system is a bridge between your **ECS (Entity Component System)** and **ReactPhysics3D** simulation engine. It manages:
- Rigid body creation and destruction
- Collision shape attachment
- Gravity and forces
- Synchronization between visual entities and physics bodies

### Architecture
```
Entity (ECS World)
    ├── localTransform (native to Entity)
    ├── RigidBodyComponent (stores mass, type, gravity settings)
    └── ColliderComponent (stores collision shape info)
          ↓
    PhysicsSystem (updates each frame)
          ↓
    PhysicsWorld (manages ReactPhysics3D engine)
          ↓
    ReactPhysics3D (handles collision, gravity, forces)
```

---

## Key Components

### 1. **PhysicsSystem** (`physics-system.hpp`)
**Responsibility:** Orchestrates the entire physics pipeline each frame

**Public Methods:**
- `bool initialize()` - Initializes the physics world
- `void update(World* ecsWorld, float deltaTime)` - Main update loop (call this every frame!)
- `void shutdown(World* ecsWorld = nullptr)` - Cleanly destroys all physics bodies
- `PhysicsWorld& getPhysicsWorld()` - Access the underlying physics world

**How to Use:**
```cpp
PhysicsSystem physicsSystem;
physicsSystem.initialize();

// Each frame:
physicsSystem.update(ecsWorld, deltaTime);

// On shutdown:
physicsSystem.shutdown(ecsWorld);
```

### 2. **PhysicsWorld** (`physics-world.hpp` / `physics-world.cpp`)
**Responsibility:** Wraps ReactPhysics3D and manages bodies/colliders

**Key Methods:**
- `bool initialize()` - Creates the physics world with gravity (0, -9.81, 0)
- `void step(float dt)` - Advances simulation by dt seconds (called 60x per second)
- `void createRigidBody(Entity*, const RigidBodyDesc&)` - Creates a physics body
- `void createCollider(Entity*, const ColliderDesc&)` - Attaches collision shape
- `void setLinearVelocity(Entity*, const glm::vec3&)` - Sets body velocity
- `glm::vec3 getLinearVelocity(Entity*)` - Gets current velocity
- `RaycastHit raycast(origin, direction, maxDistance)` - Casts a ray for hit detection

### 3. **RigidBodyComponent** (`rigid-body.hpp`)
**Responsibility:** Stores physics properties for an entity

**Properties:**
```cpp
RigidBodyType type;          // Static, Dynamic, or Kinematic
float mass;                  // Weight (affects gravity response)
bool useGravity;             // Enable/disable gravity
bool lockRotation;           // Prevent spinning
glm::vec3 velocity;          // Initial/current velocity
rp3d::RigidBody* bodyHandle; // Pointer to actual physics body
```

### 4. **ColliderComponent** (`collider.hpp`)
**Responsibility:** Defines the collision shape attached to a body

**Properties:**
```cpp
ColliderShape shape;         // Box, Sphere, or Capsule
glm::vec3 halfExtents;       // For boxes: width/2, height/2, depth/2
float radius;                // For spheres and capsules
float height;                // For capsules only
bool isTrigger;              // Trigger = no physical collision, only overlap detection
float friction;              // How slippery (0.0 = ice, 1.0 = rough)
float restitution;           // Bounciness (0.0 = no bounce, 1.0 = perfect bounce)
rp3d::Collider* colliderHandle; // Pointer to actual physics collider
```

---

## Physics Loop (4 Phases)

Every frame, the physics system runs in **strict order**:

### Phase 1: Initialize Missing Bodies/Colliders
- Scans all entities for `RigidBodyComponent` and `ColliderComponent`
- If a component exists but no physics body exists → creates it
- **First frame per entity:** Bodies and colliders are created here

### Phase 2: Sync ECS → Physics (Kinematic Only)
- **Only kinematic bodies** are updated from ECS
- Kinematic bodies are moved by **game code**, not physics forces
- We push their new position/rotation to ReactPhysics3D **before** stepping

```
Game Code moves entity → Phase 2 pushes to physics → Phase 3 steps → Result
```

### Phase 3: Step Physics Simulation
- Accumulates deltaTime into a fixed timestep accumulator (60 FPS)
- Steps the physics engine 0, 1, or multiple times based on accumulated time
- Gravity, collisions, and forces are computed **here**

### Phase 4: Sync Physics → ECS (Dynamic Only)
- **Only dynamic bodies** are pulled back from ReactPhysics3D
- New positions and rotations are written to `entity->localTransform`
- Velocities are cached in `RigidBodyComponent::velocity`
- The renderer now draws the physics-calculated positions

### Visual Representation:
```
Entity A (Dynamic)          Entity B (Kinematic)
   ↓                           ↓
[Phase 1] Create body      [Phase 1] Create body
   ↓                           ↓
[Phase 2] Skip              [Phase 2] Push to physics
   ↓                           ↓
[Phase 3] Physics step (gravity affects A, B moves as commanded)
   ↓                           ↓
[Phase 4] Pull from physics [Phase 4] Skip
   ↓                           ↓
Update localTransform       Use manual position
```

---

## Configuration & Setup

### Step 1: Add Components to Entity
When loading an entity from JSON:

```cpp
// In your entity loader/parser:
entity->addComponent<RigidBodyComponent>();
entity->addComponent<ColliderComponent>();
```

### Step 2: Configure via JSON
Your entity's config file should have `RigidBody` and `Collider` sections (see examples below).

### Step 3: Initialize Physics System
```cpp
PhysicsSystem physicsSystem;
if (!physicsSystem.initialize()) {
    Logger::error("Failed to initialize physics");
    return false;
}
```

### Step 4: Call Update Every Frame
```cpp
// In your main game loop:
physicsSystem.update(ecsWorld, deltaTime);
```

### Step 5: Shutdown Gracefully
```cpp
physicsSystem.shutdown(ecsWorld);
```

---

## JSON Configuration Examples

### Example 1: A Falling Box (Dynamic)
```jsonc
{
  "name": "falling_box",
  "position": [0, 5, 0],
  "rotation": [0, 0, 0],
  "scale": [1, 1, 1],
  "components": {
    "RigidBody": {
      "bodyType": "dynamic",     // or: 1 (integer)
      "mass": 2.0,
      "useGravity": true,
      "lockRotation": false,     // Allow spinning
      "velocity": [0, 0, 0]
    },
    "Collider": {
      "shape": "box",            // or: "sphere", "capsule"
      "halfExtents": [0.5, 0.5, 0.5],  // (width/2, height/2, depth/2)
      "friction": 0.5,
      "restitution": 0.3,        // Bounces a little
      "isTrigger": false
    }
  }
}
```

### Example 2: A Platform (Static)
```jsonc
{
  "name": "ground_platform",
  "position": [0, -1, 0],
  "rotation": [0, 0, 0],
  "scale": [10, 1, 10],
  "components": {
    "RigidBody": {
      "bodyType": "static",      // or: 0 (integer)
      "mass": 0.0                // Ignored for static bodies
    },
    "Collider": {
      "shape": "box",
      "halfExtents": [5, 0.5, 5],
      "friction": 0.8,
      "restitution": 0.0         // No bouncing
    }
  }
}
```

### Example 3: A Player Character (Kinematic)
```jsonc
{
  "name": "player",
  "position": [0, 2, 0],
  "components": {
    "RigidBody": {
      "bodyType": "kinematic",   // or: 2 (integer)
      "mass": 1.0,
      "useGravity": false,       // Kinematic bodies ignore gravity
      "lockRotation": true       // Keep upright
    },
    "Collider": {
      "shape": "capsule",
      "radius": 0.4,
      "height": 2.0,
      "friction": 0.6,
      "restitution": 0.0
    }
  }
}
```

### Example 4: A Bouncy Ball (Dynamic with Restitution)
```jsonc
{
  "name": "ball",
  "position": [0, 10, 0],
  "components": {
    "RigidBody": {
      "bodyType": "dynamic",
      "mass": 0.5,
      "useGravity": true,
      "lockRotation": false
    },
    "Collider": {
      "shape": "sphere",
      "radius": 0.3,
      "friction": 0.3,           // Low friction = rolls easily
      "restitution": 0.9         // High bounce
    }
  }
}
```

### Example 5: A Trigger Zone (No Physical Collision)
```jsonc
{
  "name": "damage_zone",
  "components": {
    "RigidBody": {
      "bodyType": "static"
    },
    "Collider": {
      "shape": "box",
      "halfExtents": [5, 2, 5],
      "isTrigger": true          // IMPORTANT: Only detects overlaps, no collision response
    }
  }
}
```

---

## Common Usage Patterns

### Pattern 1: Moving a Kinematic Body (Player Character)
```cpp
// In your input handler:
Entity* player = ecsWorld->findEntity("player");
auto* transform = &player->localTransform;
auto* rbComp = player->getComponent<RigidBodyComponent>();

// Check body type
if (rbComp && rbComp->type == RigidBodyType::Kinematic) {
    // Phase 2 will push this to physics automatically
    transform->position += glm::vec3(moveSpeed * deltaTime, 0, 0);
}
```

### Pattern 2: Applying Force to Dynamic Body
```cpp
// In your game logic:
Entity* box = ecsWorld->findEntity("falling_box");
auto* rbComp = box->getComponent<RigidBodyComponent>();

if (rbComp && rbComp->bodyHandle && rbComp->type == RigidBodyType::Dynamic) {
    // Get current velocity and modify it
    glm::vec3 currentVel = physicsSystem.getPhysicsWorld().getLinearVelocity(box);
    glm::vec3 newVel = currentVel + glm::vec3(impulse, 0, 0);
    physicsSystem.getPhysicsWorld().setLinearVelocity(box, newVel);
}
```

### Pattern 3: Raycasting for Hit Detection
```cpp
// From anywhere in your code:
glm::vec3 origin = camera->position;
glm::vec3 direction = glm::normalize(camera->front);
float maxDist = 100.0f;

RaycastHit hit = physicsSystem.getPhysicsWorld().raycast(origin, direction, maxDist);

if (hit.hasHit) {
    Logger::info("Hit entity: ", hit.entity->name);
    Logger::info("Distance: ", hit.distance);
    Logger::info("Normal: ", hit.normal.x, ", ", hit.normal.y, ", ", hit.normal.z);
}
```

### Pattern 4: Detecting Trigger Overlaps
```cpp
// Note: Current implementation doesn't have contact callbacks yet
// For now, use raycasts or spatial queries to detect overlaps
```

---

## Important Notes & Gotchas

### ⚠️ CRITICAL: Transform vs getComponent<Transform>
**WRONG:**
```cpp
auto* transform = entity->getComponent<Transform>();  // ❌ Returns nullptr!
```

**CORRECT:**
```cpp
auto* transform = &entity->localTransform;  // ✅ Works!
```

**Why:** `Transform` is NOT a `Component`. It's hardcoded directly in `Entity`. Use the `&` operator to get the pointer.

---

### ⚠️ RigidBodyType Enum Mapping
Your enum **does NOT directly match** ReactPhysics3D's internal values. The mapping is handled in `physics-world.cpp`:

```cpp
switch (desc.type) {
    case RigidBodyType::Static:    body->setType(reactphysics3d::BodyType::STATIC);     break;
    case RigidBodyType::Dynamic:   body->setType(reactphysics3d::BodyType::DYNAMIC);    break;
    case RigidBodyType::Kinematic: body->setType(reactphysics3d::BodyType::KINEMATIC);  break;
}
```

**Your enum:**
```
Static = 0, Dynamic = 1, Kinematic = 2
```

**ReactPhysics3D's enum:**
```
STATIC = 0, KINEMATIC = 1, DYNAMIC = 2  ← Different order!
```

This was the bug that made dynamic bodies behave like kinematic! Now it's fixed with explicit mapping.

---

### ⚠️ Only Dynamic Bodies Fall with Gravity
**Correct behavior:**
- `Static` bodies: Frozen in place, unaffected by gravity
- `Dynamic` bodies: Fall due to gravity, affected by collisions
- `Kinematic` bodies: Moved by game code, NOT affected by gravity

If your dynamic body isn't falling, check:
1. `useGravity` is `true` in JSON
2. `type` is `"dynamic"` (not `"kinematic"`)
3. Body has a `ColliderComponent`

---

### ⚠️ Rotation is in RADIANS
Your `Transform::rotation` is stored in **radians**, not degrees.

```cpp
// JSON expects degrees:
"rotation": [45, 0, 0]  // 45 degrees

// Internally converted to radians:
transform->rotation = glm::radians(glm::vec3(45, 0, 0));
```

---

### ⚠️ Collider Must Have RigidBody
You cannot create a collider without a rigid body:

```cpp
// Phase 1 checks this:
if (!rbComp || !rbComp->bodyHandle) {
    Logger::error("Cannot add collider without rigid body");
    return;
}
```

Always add `RigidBody` component before `Collider` in JSON.

---

### ⚠️ Fixed Timestep (60 FPS)
The physics engine steps at a **fixed 60 FPS** regardless of your frame rate:

```cpp
const float fixedDeltaTime = 1.0f / 60.0f;  // ~0.0167 seconds

// If frame takes 0.05 seconds (20 FPS):
accumulator += 0.05;       // Now 0.05
while (accumulator >= 0.0167) {
    physicsWorld.step(0.0167);
    accumulator -= 0.0167;
}
// Runs 2-3 physics steps per frame to catch up
```

This ensures consistent physics results regardless of frame rate.

---

### ⚠️ Quaternion Order (w, x, y, z)
When converting between GLM and ReactPhysics3D quaternions, order matters:

```cpp
// ReactPhysics3D quaternion (w, x, y, z)
reactphysics3d::Quaternion rp3dQuat;

// GLM quaternion - MUST use this exact order
glm::quat glmQuat(rp3dQuat.w, rp3dQuat.x, rp3dQuat.y, rp3dQuat.z);
```

**Wrong order** = Incorrect rotation!

---

### ⚠️ Deserialization Safety
All deserialization functions safely check for key existence:

```cpp
if (data.contains("mass")) {
    mass = data["mass"];
}
// If "mass" missing, uses default (1.0f)
```

Missing JSON keys won't crash. They use defaults.

---

### ⚠️ Half-Extents for Boxes
Box colliders use **half-extents** (half the actual size):

```jsonc
// Physical box is 2×2×2 units:
"halfExtents": [1, 1, 1]

// Physical box is 10×2×10 units:
"halfExtents": [5, 1, 5]
```

This is ReactPhysics3D's convention.

---

### ⚠️ Trigger Colliders Don't Push
When `isTrigger: true`:
- Colliders **detect overlaps** but don't produce collision responses
- Objects pass through each other
- Use for damage zones, pickup areas, etc.
- Useful for raycasting and overlap queries

---

## API Reference

### PhysicsSystem
```cpp
class PhysicsSystem {
    bool initialize();
    void update(World* ecsWorld, float deltaTime);
    void shutdown(World* ecsWorld = nullptr);
    PhysicsWorld& getPhysicsWorld();
};
```

### PhysicsWorld
```cpp
class PhysicsWorld {
    bool initialize();
    void shutdown();
    void step(float dt) const;
    void createRigidBody(Entity* entity, const RigidBodyDesc& desc);
    void createCollider(Entity* entity, const ColliderDesc& desc);
    void setLinearVelocity(Entity* entity, const glm::vec3& velocity) const;
    glm::vec3 getLinearVelocity(Entity* entity) const;
    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const;
};
```

### RigidBodyDesc
```cpp
struct RigidBodyDesc {
    RigidBodyType type = RigidBodyType::Dynamic;
    float mass = 1.0f;
    bool useGravity = true;
    bool lockRotation = true;
    glm::vec3 initialVelocity{0.0f};
};
```

### ColliderDesc
```cpp
struct ColliderDesc {
    ColliderShape shape = ColliderShape::Box;
    glm::vec3 halfExtents{.5f};
    float radius = .5f;
    float height = 1.8f;
    bool isTrigger = false;
    float friction = .5f;
    float restitution = 0.0f;
};
```

### RaycastHit
```cpp
struct RaycastHit {
    bool hasHit = false;
    Entity* entity{nullptr};
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    float distance = 0.0f;
};
```

---

## Summary Checklist

- ✅ Transform accessed via `&entity->localTransform` (not `getComponent`)
- ✅ RigidBodyComponent added before ColliderComponent
- ✅ Dynamic bodies have `useGravity: true`
- ✅ Kinematic bodies moved via game code (Phase 2)
- ✅ Collider halfExtents are HALF the actual size
- ✅ Rotation is in **radians** internally
- ✅ Physics system called every frame: `physicsSystem.update(ecsWorld, deltaTime)`
- ✅ Shutdown called on exit: `physicsSystem.shutdown(ecsWorld)`
- ✅ Body types enum mapped correctly (no direct casting!)

---

## Files Structure
```
source/common/
├── physics/
│   ├── physics-system.hpp      ← Main orchestrator
│   ├── physics-world.hpp       ← Wrapper for ReactPhysics3D
│   ├── physics-world.cpp       ← Implementation
│   └── physics-types.hpp       ← Enums & descriptors
├── components/
│   ├── rigid-body.hpp          ← RigidBodyComponent
│   └── collider.hpp            ← ColliderComponent
└── ecs/
    └── transform.hpp           ← Transform struct (NOT a component!)
```

---

**Last Updated:** April 18, 2026  
**Status:** ✅ Production Ready

