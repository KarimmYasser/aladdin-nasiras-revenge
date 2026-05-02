# Collectibles, Hazards & Level Progression

This page details the implementation of interactive gameplay elements, environmental dangers, and the logic governing player advancement. These systems are built upon the GFX-LAB ECS architecture and utilize a mix of trigger-based logic and persistent data management.

## Collectible System

The `CollectibleSystem` manages interactive items like Coins, Apples, Gems, and Keys.

### Implementation Details
Collectibles are entities with a `CollectibleComponent`. The system performs two primary tasks every frame:
*   **Animation:** Applies a continuous rotation (spinning) and a sinusoidal vertical movement (bobbing) to make items visually distinct.
*   **Proximity Detection:** Calculates the distance between the player and active collectibles. If below a threshold, the item is collected.

### Data Flow & Feedback
When an item is picked up:
1.  **Inventory Update:** The corresponding field in `AladdinControllerComponent` (e.g., `coins`, `apples`) is incremented.
2.  **Audio Feedback:** A fire-and-forget sound effect is triggered (e.g., `coinCollected.wav`).
3.  **Entity Removal:** The collectible is marked for removal from the world.

## Hazard & Checkpoint Systems

### HazardSystem
Hazards (e.g., lava pits, spikes) use collision triggers. When the player's `RigidBody` enters a hazard's trigger volume, their health is decremented. If health reaches zero, the respawn logic is triggered.

### CheckpointSystem
*   **Activation:** When Aladdin overlaps a checkpoint entity, the `lastCheckpoint` position in `AladdinControllerComponent` is updated.
*   **Respawn:** Upon death, the player's transform is reset to the `lastCheckpoint` coordinates.

## Level Progression & Exit

The transition between levels is managed by the `LevelExitSystem`.

*   **Key-Gated Exit:** Most exits (portals/doors) require the player to have the `hasKey` flag set in their inventory.
*   **Transition:** Upon validation, the `PlayState` loads the next level configuration.
*   **RoomPortalSystem:** Handles intra-level teleportation (e.g., moving between rooms). Includes a "blackout" effect where the screen fades to black during the coordinate swap.

## Save System & Persistence

The `SaveSystem` manages game progress across sessions, stored in `config/save.json`.
*   **Data Tracked:** Highest level unlocked and cumulative star ratings.
*   **Persistence:** Upon level completion, the system increments the `unlockedLevel` index and writes the data using `nlohmann::json`.

## Technical Summary

| System | Component / Class | Key Functionality | Audio Trigger |
| :--- | :--- | :--- | :--- |
| **Collectible** | `CollectibleComponent` | Proximity pickup, Sinusoidal bobbing | `coinCollected.wav` |
| **Hazard** | `AladdinControllerComponent` | Health reduction on trigger enter | `aladdinHurt.mp3` |
| **Checkpoint** | `Transform` | Stores respawn coordinates | `checkpoint.wav` |
| **Level Exit** | `LevelExitSystem` | Gated by `inventory.hasKey` | `levelExit.wav` |
| **Persistence** | `SaveSystem` | Writes `unlockedLevel` to `save.json` | N/A |
