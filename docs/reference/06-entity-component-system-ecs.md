# Entity Component System (ECS)

The Entity Component System (ECS) provides a modular architecture for managing game objects and their behaviors. It decouples data (**Components**) from logic (**Systems**) and organizational structure (**Entities**).

The implementation is located in `source/common/ecs/` and supports hierarchical transformations, lifecycle management, and data-driven instantiation from JSON configurations.

## Architectural Overview

The ECS is built around three primary pillars:

- **World:** The container for all entities, managing their lifecycle and global state.
- **Entity:** A unique object in the game world that acts as a container for components.
- **Component:** The base class for all data modules attached to entities.

## The World Class

The `World` class is the root of the ECS. It maintains a set of `Entity` objects and provides the interface for adding and removing them.

### Lifecycle Management

- **Addition:** Entities are added to the world using `add()`.
- **Removal:** Entities are not deleted immediately. Instead, they are marked for removal via `markForRemoval()`. This ensures that references remain valid during a frame's update loop.
- **Cleanup:** The `deleteMarkedEntities()` function iterates through the world and removes any entity marked for deletion, effectively cleaning up memory at the end of a frame.
- **Deserialization:** The world can be populated from a JSON configuration. The `deserialize()` function iterates through a JSON array of entities and calls the `ComponentDeserializer` to attach logic.

## Entity and Transform

Every `Entity` contains a `Transform` component by default, which defines its position, rotation, and scale in 3D space.

### Parent-Child Hierarchy

Entities support a hierarchical structure. An entity can have a **parent** and multiple **children**.

- **Local Space:** Coordinates relative to the parent entity.
- **World Space:** Coordinates relative to the origin of the world.

**Matrix Computation:** The `getLocalToWorldMatrix()` function recursively calculates the transformation matrix by multiplying the entity's local transformation matrix with its parent's world matrix.

## Components

The `Component` class is an abstract base defining the interface for all entity behaviors.

| Method              | Description                                                                                     |
| :------------------ | :---------------------------------------------------------------------------------------------- |
| `deserialize(data)` | Configures the component's data from a JSON object.                                             |
| `getID()`           | Static method returning a unique string ID used during deserialization (e.g., "Mesh Renderer"). |

### Component Deserializer

The `ComponentDeserializer` acts as a factory. It maps string IDs to specific component types and instantiates them onto entities during the world loading phase.

## Summary of ECS Files

| File                | Role                    | Key Symbols                                             |
| :------------------ | :---------------------- | :------------------------------------------------------ |
| `world.hpp/cpp`     | Container and Lifecycle | `World`, `deleteMarkedEntities`, `deserialize`          |
| `entity.hpp/cpp`    | Object and Hierarchy    | `Entity`, `parent`, `children`, `getLocalToWorldMatrix` |
| `component.hpp`     | Base Data Class         | `Component`, `owner`, `deserialize`                     |
| `transform.hpp/cpp` | Spatial Data            | `Transform`, `position`, `rotation`, `scale`, `to_mat4` |
