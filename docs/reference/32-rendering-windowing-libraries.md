# Rendering & Windowing Libraries

This page details the core third-party libraries used by the GFX-LAB engine to manage windowing, input, OpenGL loading, and mathematics. These libraries form the Hardware Abstraction Layer (HAL) of the engine.

## GLFW (Graphics Library Framework)

GLFW is the primary library for window creation, OpenGL context management, and input handling.

### Window Management

The `Application` class encapsulates the `GLFWwindow*` handle. It configures window hints (e.g., OpenGL 3.3 Core Profile) before creation.

### Input & Callbacks

The engine utilizes a callback-based system to bridge GLFW events to internal states:

- **Keyboard & Mouse:** Routed to `Keyboard` and `Mouse` utility classes.
- **Gamepad Support:** Polled by the `AladdinControllerSystem` to handle movement and actions.

## GLAD (OpenGL Loader)

GLAD is used to load OpenGL function pointers at runtime.

- **Initialization:** Initialized immediately after the GLFW context is made current.
- **Functionality:** Provides implementations for all `gl*` functions (e.g., `glDrawElements`, `glGenBuffers`).
- **Build Integration:** The source file `vendor/glad/src/gl.c` is compiled directly into the project.

## GLM (OpenGL Mathematics)

GLM is a header-only library for linear algebra operations, based on the GLSL specification.

### Core Types

- **`glm::vec2/3/4`:** Used for positions, normals, colors, and UV coordinates.
- **`glm::mat4`:** Used for Model, View, and Projection matrices, as well as bone matrices for skinning.
- **`glm::quat`:** Used for representing rotations to avoid gimbal lock.

**Usage in ECS:** The `Transform` component uses GLM to calculate the local-to-world matrix by combining translation, rotation (quaternion), and scale.

## Dear ImGui

Dear ImGui is an immediate-mode graphical user interface library used for HUDs and debug overlays.

### Integration & Lifecycle

The engine integrates ImGui using the GLFW and OpenGL3 backends. Every frame, the backends are updated before the `State::onImmediateGui` method is executed.

### Use Cases in Aladdin

| Feature               | Implementation Detail                                          |
| :-------------------- | :------------------------------------------------------------- |
| **HUD**               | Displays health, lives, and coin count during `PlayState`.     |
| **Enemy Health Bars** | Rendered as screen-space overlays by the `EnemySystem`.        |
| **Pause Menu**        | Provides "Resume", "Settings", and "Exit" buttons.             |
| **Debug Overlays**    | Used in test states to tweak material properties in real-time. |

## Summary of Vendor Integration

| Library   | Role           | Location       |
| :-------- | :------------- | :------------- |
| **GLFW**  | Window & Input | `vendor/glfw`  |
| **GLAD**  | GL Loader      | `vendor/glad`  |
| **GLM**   | Math           | `vendor/glm`   |
| **ImGui** | UI             | `vendor/imgui` |
