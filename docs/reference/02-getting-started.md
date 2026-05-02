# Getting Started: Build & Configuration

This page provides a technical guide for setting up the development environment, building the engine from source, and configuring the application behavior. The project uses a custom engine (GFX-LAB) built on top of OpenGL 3.3, ReactPhysics3D, and a component-based architecture.

## 1. Prerequisites & Environment Setup

### Git LFS (Large File Storage)

The repository contains large binary assets, including 3D models (OBJ/FBX) and MP4 cutscenes. These are managed via Git LFS.

- **Requirement:** Ensure Git LFS is installed before cloning.
- **Tracking:** Files such as `pembroke_castle.obj` and `*.mp4` are tracked via LFS.

### IDE Recommendations

- **CLion:** Native support for CMake. Recommended for integrated debugging and profiling.
- **Visual Studio (2019/2022):** Use "Open Folder" mode or generate a solution. Requires the MSVC toolchain for FFmpeg import library generation.
- **VS Code:** Requires the "CMake Tools" and "C/C++" extensions.

## 2. Building with CMake

The project uses CMake (minimum version 3.5) to manage dependencies and build targets.

### Build Steps

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/KarimmYasser/aladdin-nasiras-revenge.git
    ```
2.  **Configure:** Create a build directory and run CMake.
3.  **Build:** Run the build command (e.g., `make` or build via IDE).

### Vendor Library Integration

The `CMakeLists.txt` script handles the compilation of several vendor libraries:

- **GLFW:** Configured without docs/tests/examples and with hybrid GPU support.
- **Assimp:** Optimized by disabling unnecessary exporters and limiting importers to `.dae`, `.fbx`, and `.obj`.
- **FFmpeg:** The build system expects a prebuilt MSVC shared build in `vendor/ffmpeg`. On Windows, it automatically generates `.lib` import libraries from `.def` files using the MSVC `lib.exe` tool.

## 3. Application Configuration (`config/app.jsonc`)

The application is driven by a central configuration file. This file determines the initial state, window properties, and asset paths.

### Key Configuration Sections

- **Window Settings:** Resolution, title, and fullscreen mode.
- **Initial State:** Which game state to load on startup (e.g., `menu`, `play`, or a test state).
- **Asset Paths:** Locations for shaders, textures, and models.

The `Application` class parses this file during the `onInitialize` phase.

## 4. Running & Command-Line Flags

The executable supports several command-line flags for testing and overrides:
| Flag | Description |
| :--- | :--- |
| `-c`, `--config` | Path to the application configuration JSONC file. |
| `--test` | Runs the application in test mode (often used with expected output comparison). |

## 5. Scripts & Testing Utilities

The `scripts/` directory contains tools for automated testing and content generation.

### Image Comparison (`imgcmp`)

A utility used to compare screenshots taken during test states against "expected" images to verify rendering consistency across different hardware.

### Maze Generation

The `scripts/generate_maze.py` script can generate randomized maze levels and output them directly into the JSON format consumed by the engine.
**Usage:**

```bash
python scripts/generate_maze.py --rows 10 --cols 10 --write-app-config
```

## 6. Directory Structure Overview

- `assets/`: All runtime data (models, textures, audio, shaders).
- `config/`: JSONC files defining levels, entities, and application settings.
- `source/common/`: The core GFX-LAB engine code (ECS, Renderer, Physics).
- `source/states/`: Game-specific logic (MenuState, PlayState, TestStates).
- `vendor/`: Third-party dependencies.
- `bin/`: Output directory for the compiled executable and required DLLs.
