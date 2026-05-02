# Asset & Utility Libraries

This page details the third-party libraries that handle asset loading, video decoding, audio playback, physics simulation, and configuration parsing. These libraries provide the foundational capabilities for the engine's multimedia and mechanical features.

## Assimp (Open Asset Import Library)

Assimp is used for importing 3D models and skeletal animation data.

- **Restricted Importers:** To optimize build times, only COLLADA (`.dae`), FBX (`.fbx`), and OBJ (`.obj`) importers are enabled.
- **Integration:** Assimp is the core of the `AnimationLoader`, processing model hierarchies and extracting bone weights and offset matrices for the animation system.

## FFmpeg (Video Decoding)

The engine integrates **FFmpeg 8.1** to decode MP4 cutscenes (e.g., Jafar meeting and ending cinematics).

- **`Mp4Decoder`:** Wraps FFmpeg's `libavcodec` and `libavformat`. It opens video streams, decodes packets into raw frames, and converts them to RGB using `swscale`.
- **`VideoCutsceneOverlay`:** Uploads decoded frames to an OpenGL texture for full-screen rendering.

## miniaudio (Audio Engine)

The **miniaudio** library powers the `AudioSystem`, providing a high-level API for BGM and SFX.

- **BGM:** Uses `ma_sound` with streaming enabled to play long tracks directly from disk.
- **SFX:** Uses `ma_engine_play_sound` for fire-and-forget effects like coin pickups or jump sounds.

## ReactPhysics3D (Physics Engine)

The engine uses **ReactPhysics3D (RP3D) version 0.9.3** for rigid body dynamics and collision detection. The `PhysicsWorld` class acts as a wrapper around RP3D's core objects.

## Utility Libraries

### nlohmann/json (JSONC Parsing)

Used for all configuration files. It supports **JSON with Comments (JSONC)**, allowing for documented config files. `deserialize-utils.hpp` provides helpers to map JSON data to GLM types.

### stb_image

A single-header library used to load image files (PNG, JPG, BMP) into CPU memory before they are uploaded to the GPU as `Texture2D` objects.

### flags (CLI Parser)

A header-only library used to parse command-line arguments, allowing for runtime configuration overrides (e.g., `--config`).

### tinyobjloader

Used for simple OBJ parsing in basic mesh utility functions, complementing the more complex Assimp pipeline.

## Library Role Mapping

| Library            | Role            | Primary Use Case                        |
| :----------------- | :-------------- | :-------------------------------------- |
| **Assimp**         | Asset Loading   | Skeletal animations and FBX/DAE models. |
| **FFmpeg**         | Video Decoding  | MP4 cinematic cutscenes.                |
| **miniaudio**      | Audio Engine    | BGM streaming and SFX playback.         |
| **ReactPhysics3D** | Physics Engine  | Rigid body dynamics and collisions.     |
| **nlohmann/json**  | Config Parsing  | Level and application configuration.    |
| **stb_image**      | Texture Loading | Loading image data into GPU textures.   |
