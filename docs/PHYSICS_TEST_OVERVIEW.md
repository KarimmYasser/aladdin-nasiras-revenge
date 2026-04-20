# 🧪 PHYSICS ENGINE TEST GUIDE

This folder contains a comprehensive suite of individual tests mapping the exact backend physics properties mapped to ReactPhysics3D.

Run the batch menu script `bin\run-physics-scenarios.bat` to launch whichever test you want. Just **close the game window** when you're done observing it, and the script menu will pop right back up for the next test!

---

### **[1] TEST 1: Gravity & Mass** (`test1-gravity.jsonc`)
**Tests:** Gravity pull, `useGravity` flag, and mass effects.
**What to Expect (Visually):**
- **Red Cube:** Drops to the floor. (Standard).
- **Green Cube:** Drops exactly as fast as the Red cube. Even though it is 10x lighter, physics correctly dictates all masses fall at the exact same gravity acceleration.
- **Blue Cube:** Levitates floating because `useGravity: false`.

### **[2] TEST 2: Restitution** (`test2-restitution.jsonc`)
**Tests:** The `restitution` (bounciness) material property.
**What to Expect (Visually):**
- **Red Sphere (0.9 Restitution):** Rebounds aggressively, losing almost no momentum.
- **Green Sphere (0.4 Restitution):** Bounces moderately.
- **Blue Sphere (0.0 Restitution):** Hits the ground like a lump of clay with zero bounce.

### **[3] TEST 3: Friction** (`test3-friction.jsonc`)
**Tests:** The `friction` property dictating sliding on a sloped ramp.
**What to Expect (Visually):**
- You are pointing at a tilted ramp.
- **Red Box (0.0 Friction):** Pure ice; slides down the ramp quickly and cleanly.
- **Green Box (0.9 Friction):** Pure rubber. Resists gravity sliding aggressively by sticking or violently tumbling.

### **[4] TEST 4: Collision Shapes** (`test4-shapes.jsonc`)
**Tests:** Different geometry wrappers bounds (`Box` vs `Sphere`).
**What to Expect (Visually):**
- **Red Cube:** Falls from the sky pre-rotated at a tilted angle to trigger a tumbling behavior, before eventually colliding into the ground and coming to a rest laying entirely flat on it's `halfExtents`.
- **Green Sphere:** Falls onto a slightly tilted corner edge to trigger a seamless circular rolling behavior over its specified `radius`.

### **[5] TEST 5: Body Types** (`test5-bodytypes.jsonc`)
**Tests:** Intersecting responses of `static`, `kinematic`, and `dynamic` rigid bodies.
**What to Expect (Visually):**
- **Blue Platform (`kinematic`):** Sits mid-air stably. Kinematic bodies are purely code-driven, ignoring physics.
- **Red Cube (`dynamic`):** Drops from the sky, colliding into the blue platform. *The force of the falling red cube dynamically halts without shifting the blue Kinematic platform!*

### **[6] TEST 6: Triggers** (`test6-trigger.jsonc`)
**Tests:** `isTrigger = true`. (Used later to detect Player collecting coins or taking damage without bouncing the player away like brick walls do).
**What to Expect (Visually):**
- **Translucent Red Box:** Ghost area representing a trigger zone.
- **Green Sphere:** Drops straight down. It completely ignores the red box, passing seamlessly *through it*, and safely striking the floor underneath! In the codebase, Phase 3 detects this perfectly without moving the object!

### **[7] TEST 7: Velocity & Momentum** (`test7-velocity-collision.jsonc`)
**Tests:** `initialVelocity` forces and collision momentum displacement caused by `mass` differences.
**What to Expect (Visually):**
- **Red Heavy Bullet (Mass: 10, LockRotation: True):** Spawns instantly rocketing to the right at 30 m/s without rotating.
- **Cyan Light Target (Mass: 1):** Spawns drifting to the left at 5 m/s.
- **The Impact:** When the massive, fast red box impacts the small cyan ball moving against it, it completely overpowers the ball's momentum and forcefully launches the cyan ball off the stage at high speeds, demonstrating perfect force conservation.
