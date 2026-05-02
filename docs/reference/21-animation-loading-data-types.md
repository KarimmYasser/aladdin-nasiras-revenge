# Animation Loading & Data Types

The animation system imports complex skeletal data and keyframed animations from external formats (DAE, FBX) and converts them into a runtime-efficient internal representation. This process utilizes the **Assimp** library for parsing and populates a hierarchy of data structures that support GPU skinning and smooth interpolation.

## Animation Loading Pipeline

The loading process follows these steps:

1.  **Assimp Import:** `AnimationLoader::load` uses `Assimp::Importer` to read `.dae` or `.fbx` files.
2.  **Hierarchy Construction:** Recursively traverses the `aiNode` tree to build a `NodeData` hierarchy.
3.  **Bone Mapping:** Extracts mesh bone weights and offset matrices into `BoneInfo`.
4.  **Keyframe Extraction:** Parses animation channels into `KeyFrame` vectors (Position, Rotation, Scale).

### Core Data Entities

| Concept           | Code Entity       | Role                                                    |
| :---------------- | :---------------- | :------------------------------------------------------ |
| **Loader Engine** | `AnimationLoader` | Static utility bridging Assimp with the engine.         |
| **Import Result** | `LoadResult`      | Container for the `SkinnedMesh`, clips, and root node.  |
| **Skeletal Bone** | `BoneInfo`        | Maps bones to the mesh; stores offset matrices and IDs. |
| **Animation Set** | `AnimationClip`   | Stores a movement sequence (duration, ticks, channels). |
| **Skeletal Node** | `NodeData`        | Represents the transformation hierarchy of the model.   |

## Core Data Types

### AnimationClip

Represents a single sequence (e.g., "walk", "jump"). It contains the duration and a map of bone names to their respective animation channels.

### KeyFrame Types

Animations are stored as discrete points in time:

- **KeyPosition:** Stores `glm::vec3` and a timestamp.
- **KeyRotation:** Stores `glm::quat` and a timestamp.
- **KeyScale:** Stores `glm::vec3` and a timestamp.

### BoneInfo

Maps a skeletal bone to the mesh:

- **`offset`**: `glm::mat4` that transforms from model space to bone space.
- **`id`**: Integer index used to access the bone's matrix in the GPU uniform array.

## Vertex Skinning Data

During loading, the `AnimationLoader` populates `SkinnedVertex` structures. Each vertex is influenced by up to 4 bones:

- **`boneIDs`**: `glm::ivec4` containing the bone indices.
- **`weights`**: `glm::vec4` containing the influence percentage for each bone.

This data is used by the `skinned.vert` shader to deform the mesh in real-time.
