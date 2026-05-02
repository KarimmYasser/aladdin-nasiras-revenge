# Animator & Animation System

The animation system provides a framework for skeletal animation playback, blending, and GPU-based skinning. It handles transitions between states (e.g., walking to jumping) through **crossfading** and computes bone transformations for rendering skinned meshes.

## System Architecture

The system consists of three main parts:

- **Animator:** The runtime controller for playback and interpolation.
- **AnimatorComponent:** An ECS wrapper for the `Animator`.
- **AnimationSystem:** An ECS system that updates all animators every frame.

## The Animator Class

The `Animator` manages a library of `AnimationClip` objects and handles time progression.

### Key Responsibilities:

1.  **Time Management:** Increments animation time based on a `playbackSpeed` multiplier.
2.  **Crossfade Logic:** Smoothly blends between "from" and "to" clips over a set duration.
3.  **Local Transform Interpolation:** Performs LERP/SLERP between keyframes for translation, rotation, and scale.
4.  **Global Matrix Calculation:** Traverses the `NodeData` hierarchy to compute world-space matrices for every bone.
5.  **Final Matrix Generation:** Multiplies the global bone matrix by the `BoneInfo::offsetMatrix` to transform vertices into bone space.

## ECS Integration

| Component / System      | Role                                                                                                      |
| :---------------------- | :-------------------------------------------------------------------------------------------------------- |
| **AnimatorComponent**   | Wraps the `Animator` instance and stores the `finalBoneMatrices` vector for the shader.                   |
| **SkinnedMeshRenderer** | Points to a `SkinnedMesh` and a `LitMaterial` configured for skinning.                                    |
| **AnimationSystem**     | Iterates through entities, updates their animators, and ensures bone matrices are ready for the renderer. |

## Locomotion and Root Motion

The system is designed for **in-place locomotion**:

- **Root Motion Suppression:** The engine typically ignores translation data on the root bone (e.g., "Hips"), relying instead on the `PhysicsSystem` or `AladdinControllerSystem` to move the entity's `Transform`.
- **Dynamic Playback Speed:** The `AladdinControllerSystem` can sync the `playbackSpeed` with the character's movement velocity.

## Implementation Highlights

- **Shader Upload:** The `ForwardRenderer` retrieves `finalBoneMatrices` and uploads them to the `u_bones[128]` uniform array.
- **Interpolation:** Uses Spherical Linear Interpolation (SLERP) for rotations to ensure smooth skeletal movement.
- **Entry Point:** `AnimationSystem::update` is called every frame within the state's `onUpdate` loop.
