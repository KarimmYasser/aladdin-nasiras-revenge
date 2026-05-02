# Environment & Map Assets

The world of Aladdin: Nasira's Revenge is constructed from modular 3D environmental models, large-scale map geometry, and an extensive texture library. These assets are categorized into reusable environment objects and unique map layouts.

## Environment Object Library

Located in `assets/models/Environment objects/`, these modular `.obj` assets are used to populate levels with interactive and decorative elements.

### Buildings and Shops

- **Desert House:** A multi-story residential structure using `Brick1_baseColor.jpg` and `HouseMain_baseColor.jpg`.
- **Desert Shop:** A specialized building variant for market areas, utilizing `Ground_baseColor.png`.
- **Agrabah Market Shop:** Smaller stall structures used to densify the maze environments.

### Decorative & Interactive Props

- **Fountains:** Water features used as level landmarks.
- **Aladdin Lamp:** A high-detail prop used as a key collectible or narrative focus.

## Map Assets

The game features three primary map environments. Large-scale maps are managed via **Git LFS** due to their complexity.

| Map Name            | File Path                                  | Usage                                                             |
| :------------------ | :----------------------------------------- | :---------------------------------------------------------------- |
| **Pembroke Castle** | `Maps/pembroke_castle/pembroke_castle.obj` | Expansive exterior testing and specific story segments.           |
| **Raby Castle**     | `Maps/raby_castle/raby_castle.obj`         | Secondary large-scale environment.                                |
| **Level 3 Map**     | N/A                                        | Specialized arena designed for the boss confrontation with Jafar. |

## Texture Library

Visual consistency is maintained by a massive collection of **397 textures** located in `assets/textures/agrabah/`.

- **Wall Textures:** Sandstone, brick, and plastered patterns for maze generation.
- **Ground Textures:** Sand, stone tiles, and carpeted floor patterns.
- **Skybox/Sky Textures:** High-resolution textures used for rendering the sky sphere.

### Renderer Texture Usage

| Texture Type       | Usage                  | Key Files                                      |
| :----------------- | :--------------------- | :--------------------------------------------- |
| **Albedo/Diffuse** | Primary surface color  | `Brick1_baseColor.jpg`, `Ground_baseColor.png` |
| **Sky Sphere**     | Background environment | `assets/textures/sky/`                         |
| **Post-Process**   | Framebuffer effects    | Rendered at runtime to the post-process FBO    |

## Technical Implementation

Textures are loaded using `stb_image` and managed by the `Texture2D` class. They are sampled in shaders via the `Sampler` class, which defines filtering (linear/nearest) and wrapping (repeat/clamp) behaviors.
