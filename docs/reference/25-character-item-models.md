# Character & Item Models

This page catalogs the 3D assets used for characters and interactive items. Character models are typically skeletal meshes (`.fbx`/`.dae`), while item models are often static meshes (`.obj`) with simple logic applied via the `CollectibleSystem`.

## Character Models

### Aladdin

Aladdin is the primary playable character. His assets are split into costume variants:

- **Costume Models:** `aladdin_costume_basic.obj` (Street Rat) and `aladdin_costume_prince.obj` (Prince Ali).
- **Textures:** `aladdin_diff.png` is the primary diffuse map.
- **Animations:** Stored as individual `.fbx` files: `idle`, `walk`, `run`, `jump`, `kick`, `turn`.

### NPCs & Supporting Cast

- **Magic Carpet:** A skinned mesh (`magic_carpet_costume_basic.fbx`) used in specific level segments.
- **Genie:** High-poly model used for dialogue sequences; a `Genie_Jafar` variant exists for the climax.
- **Antagonists/Others:** Includes models for Jafar and Jasmine.

## Item & Collectible Models

Items are generally static meshes associated with a `CollectibleComponent`.

| Item       | File Path                  | Texture                 | Description                      |
| :--------- | :------------------------- | :---------------------- | :------------------------------- |
| **Apple**  | `Apple/apple.obj`          | `Handle1Tex.png`        | Projectile and collectible.      |
| **Coin**   | `Coin/Coin.obj`            | `Item_Coin_Texture.png` | Primary currency.                |
| **Key**    | `Key/key.obj`              | `key_diffuse.png`       | Required for level exits.        |
| **Health** | `Health/health_bottle.obj` | `bottle_diff.png`       | Restores player HP.              |
| **Pot**    | `Pots/pot.obj`             | `pot_diff.png`          | Destructible environment object. |
| **Sword**  | `Swords/sword.obj`         | `sword_metal.png`       | Prop or enemy weapon.            |

## Directory Layout & Loading

The `assets/models/` directory follows a strict hierarchy:

- **Asset Groups:** Each group has its own folder (e.g., `Aladdin/`, `Apple/`).
- **Texture Proximity:** Diffuse and specular maps are stored alongside model files to simplify material resolution.
- **Animation Subfolders:** For complex characters, animations are stored in an `animations/` subfolder, where each file represents a single `AnimationClip`.

## Technical Implementation

When a level loads, the `AnimationLoader`:

1.  Loads the base hierarchy and bind poses from the primary `.fbx`.
2.  Maps bone indices to the `SkinnedVertex` structure.
3.  Registers available clips into the `AnimatorComponent`.
