# Dialogue & Cutscene Systems

The dialogue and cutscene systems provide immersive narrative transitions. The `DialogueSystem` handles proximity-based text boxes with typewriter animations, while the `VideoCutsceneOverlay` utilizes FFmpeg to render high-quality MP4 cinematics.

## Dialogue System

The `DialogueSystem` is an ECS-based system that manages on-screen text conversations.

### Implementation Details

- **DialogueComponent:** Stores a queue of nodes, each with a speaker name, text content, and optional styling.
- **Typewriter Effect:** Text is revealed character-by-character based on a timer.
- **Proximity Triggering:** Checks the distance between the player and the NPC (e.g., the Genie).
- **Entity Hooks:** Can be configured to "reveal" or "remove" specific entities upon completion (e.g., spawning a key after a conversation).

### JSON Configuration Features

- **Speaker Styles:** Custom colors and fonts for characters (Genie, Nasira, etc.).
- **Input:** Advance dialogue using the "Interact" key (Space/E).
- **`on_finish_reveal` / `on_finish_remove`:** Lists of entity IDs to enable or destroy after the talk.

## Video Cutscene Overlay

The `VideoCutsceneOverlay` provides a full-screen video playback mechanism for major narrative beats.

### MP4 Decoding with FFmpeg

The engine integrates FFmpeg via the `Mp4Decoder` class:

1.  **Stream Opening:** Locates the video stream in the MP4 file.
2.  **Frame Extraction:** Decodes packets into raw RGB frames.
3.  **Texture Upload:** Converts frames into an OpenGL texture.
4.  **Rendering:** Draws a fullscreen quad using the `Postprocess` shader path.

### Playback Control

- **`play(path)`:** Initializes the decoder and starts playback.
- **`update(dt)`:** Advances the timer and updates the GL texture.
- **Skip Logic:** Players can skip cutscenes by pressing **ESC**, which immediately resumes the game state.

## Narrative Triggers in PlayState

The `PlayState` monitors game objectives and player location to trigger cinematic events.

| Trigger Event       | Action           | Asset Path                         |
| :------------------ | :--------------- | :--------------------------------- |
| **Game Start**      | Intro Cutscene   | `assets/Videos/Intro.mp4`          |
| **Boss Room Entry** | Jafar Meeting    | `assets/Videos/Jafaar Meeting.mp4` |
| **Boss Defeat**     | Victory Cutscene | `assets/Videos/Ending.mp4`         |

When a cutscene starts, the `PlayState` pauses the `PhysicsSystem` and `AladdinControllerSystem` to freeze the game world.
