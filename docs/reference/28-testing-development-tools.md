# Testing & Development Tools

The Aladdin: Nasira's Revenge codebase includes a robust suite of automated tests and development utilities to ensure rendering stability and facilitate rapid level iteration.

## Testing Infrastructure

The testing framework validates core engine features (shaders, textures, ECS, physics) by comparing runtime output against known-good "expected" results.

### Core Testing Workflow

Testing operates by executing specialized **Test States** which isolate specific engine features:

1.  **Scene Loading:** A test state loads a minimal JSONC configuration.
2.  **Rendering:** the scene is rendered, and the output is captured.
3.  **Validation:** The captured frame is compared against reference images in the `expected/` directory.

### Key Components

| Component           | Role                                                                                  |
| :------------------ | :------------------------------------------------------------------------------------ |
| **Test States**     | C++ classes in `source/states/` that isolate subsystems (e.g., `mesh-test`).          |
| **JSONC Configs**   | Scene definitions in `config/` used to drive specific test scenarios.                 |
| **Expected Output** | Reference images and data stored in `expected/` for regression testing.               |
| **Scripts**         | PowerShell and Python tools (e.g., `run-all.ps1`) for batch execution and comparison. |

## Feature Isolation & Validation

- **Subsystem Verification:** States like `shader-test`, `mesh-test`, and `texture-test` verify low-level OpenGL abstractions.
- **Automated Comparison:** The `imgcmp` tool automates the process of comparing the current framebuffer against the `expected/` directory.
- **Physics Validation:** A dedicated `run-physics-tests.bat` script validates the ReactPhysics3D integration.

## Maze Level Generator (`scripts/generate_maze.py`)

A Python-based tool that automates the creation of complex gameplay environments, removing the need for manual placement of walls and collectibles.

### Key Features:

- **Algorithmic Generation:** Uses an iterative backtracking algorithm to create non-trivial maze layouts.
- **Wall Merging:** Implements greedy rectangular merging to combine adjacent walls into single meshes, reducing draw calls and preventing Z-fighting.
- **Entity Population:** Automatically places Aladdin's spawn point, enemies, coins, and exits.
- **Config Integration:** Outputs `.jsonc` files directly compatible with the engine's `World` deserializer.
