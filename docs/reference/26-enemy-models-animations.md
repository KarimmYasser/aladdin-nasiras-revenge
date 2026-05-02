# Enemy Models & Animations

This page documents the 3D assets, textures, and skeletal animations used for the three primary enemy types. It details how these assets map to the `EnemyComponent` configurations within the ECS.

## Enemy Asset Overview

The game utilizes three distinct enemy categories, ranging from static environmental hazards to fully animated skeletal meshes.

| Enemy Type  | Internal Name | Format  | Features                                                |
| :---------- | :------------ | :------ | :------------------------------------------------------ |
| **Enemy 1** | Golem of Sand | OBJ/FBX | Static mesh, Diffuse texture.                           |
| **Enemy 2** | Slime Enemy   | OBJ/FBX | Skinned mesh, 4 Animations (Idle, Walk, Attack, Death). |
| **Enemy 3** | Golem         | OBJ/FBX | Skinned mesh, Emissive glow, 4 Animations.              |

---

## Enemy 1: Golem of Sand

A high-poly environmental enemy used primarily in Level 1 as a static obstacle or proximity hazard.

- **Model Path:** `assets/models/Enemies/Enemy 1/Golem_of_sand.fbx`
- **Texture:** `diffuse.jpg`
- **Implementation:** Typically rendered using a `MeshRendererComponent` since it lacks skeletal animations.

## Enemy 2: Slime Enemy

A fully animated character with a complete AI state machine.

- **Base Mesh:** `SlimeEnemy1.obj`
- **Animations (`/animations/`):**
  - `idle.fbx`: Default state.
  - `walk.fbx`: Used during **PATROL** and **CHASE**.
  - `attack.fbx`: Triggered within `attackRange`.
  - `death.fbx`: Triggered when health reaches zero.
- **Texture Mapping:** Uses Base Color (green appearance), Metallic/Roughness (specular), and Normal maps.

## Enemy 3: Golem

A heavy-hitting enemy with emissive glowing cracks, designed for later maze sections.

- **Base Mesh:** `Golem.obj`
- **Animations (`/animations/`):** Includes `idle`, `walk`, `attack` (slam), and `death` (crumbling).
- **Material Properties:** Uses a specialized `LitMaterial` with an Emissive map to provide a "magical glow" effect in the stone cracks.

---

## Technical Implementation

### Animation State Logic

The `EnemySystem` monitors the `EnemyState` and updates the `AnimatorComponent` to request the corresponding FBX clip.

### Vertex Skinning

For Enemy 2 and 3, the `SkinnedMesh` contains bone weights and indices. The `AnimationSystem` calculates final bone matrices every frame for the `skinned.vert` shader.

### Root Motion

Root motion is suppressed; movement is handled by the `PhysicsSystem` or `EnemySystem`, while the animation provides the visual representation of that movement.
