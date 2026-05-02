# Vendor Libraries

The Aladdin: Nasira's Revenge project utilizes a curated set of third-party libraries to handle low-level systems. These libraries are located in the `vendor/` directory and are integrated via the main `CMakeLists.txt` file.

The engine follows a "bring your own source" or "prebuilt dependency" approach to ensure build reproducibility across different environments.

## Rendering & Windowing Libraries

These libraries form the backbone of the engine's interaction with the operating system and the GPU.

- **GLFW:** Manages window creation, OpenGL context management, and hardware input (keyboard, mouse, and gamepads).
- **GLAD:** A multi-language GL/GLES/EGL/GLX/WGL loader-generator. It provides the function pointers for OpenGL 3.3+ commands.
- **GLM (OpenGL Mathematics):** A header-only C++ mathematics library based on the GLSL specification. Used for `vec3`, `mat4`, and `quat` types.
- **Dear ImGui:** An immediate-mode graphical user interface library used for the game's HUD, debug menus, and UI overlays.

## Asset & Utility Libraries

These libraries handle the parsing of complex file formats and the simulation of physical systems.

- **Assimp (Open Asset Import Library):** Used for loading 3D models. The build is restricted to COLLADA (.dae), FBX, and OBJ importers to minimize binary size.
- **ReactPhysics3D:** A C++ physics engine used for rigid body dynamics and collision detection (Version 0.9.3).
- **FFmpeg:** Utilized for decoding MP4 video files for cutscenes.
- **miniaudio:** A single-header audio playback and mixing library that handles the game's BGM and SFX.
- **nlohmann/json:** A header-only JSON library used for parsing configuration files, level definitions, and ECS entity templates.
- **stb:** Single-file public domain libraries, primarily `stb_image.h` for loading texture files.

## Summary Table

| Library            | Version | Integration Type      | Primary Role            |
| :----------------- | :------ | :-------------------- | :---------------------- |
| **GLFW**           | 3.x     | Subdirectory (Static) | Window & Input          |
| **GLAD**           | 0.1.x   | Source File           | OpenGL Loader           |
| **GLM**            | 0.9.x   | Header-only           | Math (Vectors/Matrices) |
| **Dear ImGui**     | 1.8x    | Source Files          | Debug & Game UI         |
| **Assimp**         | 5.x     | Subdirectory (Static) | 3D Model Loading        |
| **ReactPhysics3D** | 0.9.3   | Subdirectory (Static) | Physics Simulation      |
| **FFmpeg**         | 8.1     | Prebuilt Shared       | Video Decoding          |
| **miniaudio**      | Latest  | Header-only           | Audio Engine            |
| **nlohmann/json**  | 3.x     | Header-only           | Config/ECS Parsing      |
