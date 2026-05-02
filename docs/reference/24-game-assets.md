# Game Assets

This page provides an overview of the `assets/` directory structure and how resources are organized. The project relies on **Git LFS** (Large File Storage) to manage high-resolution meshes and video files.

## Asset Directory Structure

The `assets/` directory is the central repository for all resources. The `AssetLoader<T>` and various systems use these files to populate the game world based on JSON configurations.

| Directory          | Content Type           | Formats                | Key Systems                          |
| :----------------- | :--------------------- | :--------------------- | :----------------------------------- |
| `assets/models/`   | 3D Meshes & Animations | `.obj`, `.fbx`, `.dae` | `ForwardRenderer`, `AnimationSystem` |
| `assets/textures/` | Image Data             | `.png`, `.jpg`, `.tga` | `LitMaterial`, `TexturedMaterial`    |
| `assets/audio/`    | Sound & Music          | `.mp3`, `.wav`         | `AudioSystem`                        |
| `assets/video/`    | Cutscenes              | `.mp4`                 | `VideoCutsceneOverlay`               |
| `assets/shaders/`  | GLSL Code              | `.vert`, `.frag`       | `ShaderProgram`                      |

## Asset Categories

### 3D Models and Animations

- **Static Models:** Environment objects and collectibles typically use the `.obj` format.
- **Animated Characters:** Characters like Aladdin and enemies use `.fbx` or `.dae` to store skeletal hierarchies and keyframes.

### Character and Item Assets

- **Aladdin:** Features multiple costume variants (Basic, Prince) and a full suite of animations (Idle, Walk, Run, Jump, Kick, Turn).
- **Items:** Optimized meshes for gameplay feedback (e.g., spinning coins, gems, keys).

### Enemy Assets

The game features three distinct enemy types, ranging from static sand golems to fully animated slime creatures, closely coupled with the `EnemyComponent` state machine.

### Environment and Maps

- **Map Geometry:** Large maps (e.g., `pembroke_castle.obj`) are managed via Git LFS. These serve as the structural foundation, while smaller objects (shops, fountains, lamps) are instanced throughout.
- **Texture Library:** Over 390 Agrabah-themed textures in `assets/textures/agrabah/` are used by `LitMaterial` for albedo, specular, and emissive properties.

### Audio and Video

- **BGM/SFX:** `assets/audio/` contains BGM for levels and SFX for actions like jumping, collecting items, and combat.
- **Video Cutscenes:** FMV cutscenes are stored in `assets/video/` and decoded via **FFmpeg**.
