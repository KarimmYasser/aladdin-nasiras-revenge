# Application & State Machine

The `Application` class serves as the central engine controller, managing the GLFW window lifecycle, input dispatching, and the finite state machine (FSM) that governs the game's flow. It bridges the low-level hardware abstraction layer (GLFW/OpenGL) with the high-level game logic encapsulated in `State` objects.

## The Application Class

The `our::Application` class is responsible for the main execution loop. It handles window creation via GLFW, initializes the OpenGL context using GLAD, and maintains a registry of available game states.

### Key Responsibilities

- **Window Management:** Creates and configures the GLFW window and handles resize events.
- **Main Loop:** Executes the `run()` function, which continuously processes input, updates the active state, and renders frames.
- **State Transition:** Manages switching between different game states (e.g., from `MenuState` to `LoadingState`).
- **Input Handling:** Captures keyboard and mouse events and forwards them to the currently active state.

## The State Lifecycle

The `our::State` class is an abstract base class that defines the interface for different game phases. Every major screen or mode in the game (Menu, Play, Test) inherits from this class.

### Lifecycle Methods

The `Application` calls these methods at specific points in the state's life:

| Method             | Description                                                                                      |
| :----------------- | :----------------------------------------------------------------------------------------------- |
| `onInitialize()`   | Called once when the state becomes active. Used for loading assets and setting up the ECS world. |
| `onUpdate(dt)`     | Called every frame. Used for logic, physics simulation, and system updates.                      |
| `onDraw(dt)`       | Called every frame. Used for invoking the `ForwardRenderer`.                                     |
| `onImmediateGui()` | Called every frame to render ImGui overlays (HUD, menus, debug info).                            |
| `onDestroy()`      | Called when transitioning away. Used to clear the ECS world and free resources.                  |

**Input Callbacks:** States also receive raw input events via `onKeyEvent`, `onCursorMoveEvent`, `onMouseButtonEvent`, and `onScrollEvent`.

## State Registration & Transitions

States are registered in `main.cpp` using a unique string ID via `Application::registerState<T>(id)`.

### Transition Logic

A state can request a transition by calling `getApp()->changeState("target_id")`. To prevent use-after-free errors during the update loop, the `Application` performs the swap at the start of the next frame.

## Registered Game States

### Production States

- **MenuState:** The main title screen with "Play" and "Exit" options.
- **LoadingState:** A progressive asset loader that prevents the application from freezing while loading heavy resources.
- **PlayState:** The primary game loop where the ECS world is active, managing Aladdin, enemies, and physics.
- **GameOverState / VictoryState:** Post-game screens triggered by health depletion or level completion.

### Test States

Used for automated testing and visual verification:

- **ShaderTestState / MaterialTestState:** Verifies GLSL uniform passing and material properties.
- **MeshTestState / TextureTestState:** Validates asset loading and GPU buffer uploads.
- **PipelineTestState:** Tests OpenGL state changes (depth testing, face culling).

## GLFW Window & Context Configuration

The engine configures the OpenGL context before window creation:

- **OpenGL Version:** 3.3 Core Profile.
- **Depth/Stencil:** 24-bit depth and 8-bit stencil buffers.
- **Samples:** 4x MSAA for anti-aliasing.
- **Input:** Initializes `Keyboard` and `Mouse` singletons for a simplified polling API.
