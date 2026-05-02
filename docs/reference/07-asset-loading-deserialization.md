# Asset Loading & Deserialization

The GFX-LAB engine employs a centralized, template-driven asset management system designed to decouple resource loading from game logic. The system supports progressive loading, reference counting through smart pointers, and a robust JSON-to-object deserialization pipeline.

## AssetLoader Template

The core of the system is the `AssetLoader<T>` template class. It acts as a registry for resources of type `T`, mapping unique string identifiers to shared pointers.

### Key Responsibilities:

- **Centralized Storage:** Maintains a `std::unordered_map` of `std::shared_ptr<T>`.
- **Reference Management:** Uses `std::shared_ptr` to ensure assets remain in memory as long as they are referenced.
- **Retrieval:** Provides a `get()` method to fetch an asset by name.
- **Loading Hooks:** Utilizes a `load()` method that can be specialized for different types (e.g., textures vs. shaders).

**Data Flow:** A system requests an asset via `AssetLoader::get(name)`. If it exists, the pointer is returned; otherwise, it returns `nullptr`, allowing the caller to handle the missing resource.

## Deserialization Pipeline

The engine converts JSON configuration files into C++ objects using a tiered approach.

### 1. Primitive and Math Deserialization

Utility functions in `deserialize-utils.hpp` bridge `nlohmann::json` and GLM math types.

- **GLM Types:** `readVector<T>` and `readMatrix<T>` convert JSON arrays into `glm::vec3`, `glm::vec4`, and `glm::mat4`.
- **Safety:** Includes checks to ensure JSON keys exist before reading to prevent runtime crashes.

### 2. Component Deserializer

The `deserializeComponent` function acts as a factory for ECS components:

- **MeshRenderer:** Fetches "mesh" and "material" names from their respective `AssetLoader` instances.
- **Camera:** Parses projection type (perspective/orthographic) and FOV parameters.
- **Light:** Parses light type (directional, point, spot) and color/intensity data.
- **AladdinController:** Configures movement speeds, jump heights, and camera offsets.

## LoadingState & LevelCache

To prevent frame drops, the engine uses specialized loading strategies:

### LoadingState

A specialized application state that processes a queue of assets over multiple frames rather than blocking the main thread. It provides visual feedback (loading bars) while populating the `AssetLoader` instances.

### LevelCache

Ensures assets required for the next level are pre-emptively loaded. It scans upcoming level JSON files for mesh and material requirements and avoids duplicates by checking the existing asset map.

## Asset-Specific Loading

- **Shaders:** Loaded from `.vert` and `.frag` files, compiled, and linked into OpenGL programs.
- **Textures:** Loaded using `stb_image`. `Sampler` objects encapsulate OpenGL state for wrapping and filtering.
- **Meshes:**
  - **Static:** Loaded from `.obj` using `tinyobjloader`.
  - **Skinned:** Loaded from `.dae` or `.fbx` using **Assimp** via the `AnimationLoader`.
