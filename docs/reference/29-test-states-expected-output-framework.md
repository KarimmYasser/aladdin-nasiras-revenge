# Test States & Expected Output Framework

The **Test States & Expected Output Framework** is a specialized testing infrastructure designed to validate core engine subsystems (rendering, physics, ECS, and asset loading) through automated visual and functional comparisons.

## Framework Architecture

The framework consists of three primary layers:

1.  **Test States:** Isolated `State` implementations in `source/states/` that focus on a single engine unit (e.g., `MaterialTestState`).
2.  **Configuration & Gold Standards:** JSONC files in `config/` that define test parameters and "Gold Standard" PNG files in `expected/` used for visual regression.
3.  **Tooling:** PowerShell and Python scripts in `scripts/` that automate execution, screenshot capture, and pixel-by-pixel comparison.

## Test States Overview

Each test state is a subclass of `our::State`, registered in `main.cpp`.

| Test State         | Source File          | Purpose                                               |
| :----------------- | :------------------- | :---------------------------------------------------- |
| **Shader Test**    | `shader-test.hpp`    | Validates GLSL compilation and uniform setting.       |
| **Mesh Test**      | `mesh-test.hpp`      | Tests VBO/VAO creation and raw rendering.             |
| **Transform Test** | `transform-test.hpp` | Validates hierarchical matrix calculations in ECS.    |
| **Pipeline Test**  | `pipeline-test.hpp`  | Tests `PipelineState` (depth, culling, blending).     |
| **Texture Test**   | `texture-test.hpp`   | Validates `Texture2D` loading and UV mapping.         |
| **Material Test**  | `material-test.hpp`  | Validates the material system and shader binding.     |
| **Renderer Test**  | `renderer-test.hpp`  | Full-stack test of the `ForwardRenderer` with lights. |

## Automated Tooling & Scripts

The `scripts/` directory contains the automation logic for "headless-like" testing:

### 1. Image Comparison (`imgcmp`)

A utility that compares two images, outputting a similarity score and generating a "diff" image highlighting pixel discrepancies. This is critical for catching subtle rendering regressions.

### 2. PowerShell Automation

- **`run-all.ps1`:** Executes every registered test state via the `--config` flag.
- **`compare-all.ps1`:** Runs `imgcmp` on all generated screenshots against the `expected/` directory.
- **`compare-group.ps1`:** Runs and compares a specific subset of tests.

### 3. Physics Testing (`run-physics-tests.bat`)

Validates the **ReactPhysics3D** integration, specifically checking kinematic-to-dynamic body synchronization and collision detection across frames.

## Single Test Execution Flow

1.  **Launch:** `GFX-LAB.exe --config config/test_name.jsonc`
2.  **Load:** `Application` initializes the specified `State` based on the config.
3.  **Capture:** The `State` renders a frame and calls `screenshot::savePNG("screenshots/test_name.png")`.
4.  **Compare:** `imgcmp.exe` compares the screenshot against `expected/test_name.png`.
