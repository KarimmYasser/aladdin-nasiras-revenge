# Audio System

The Audio System provides the engine's sound infrastructure, utilizing the **miniaudio** library. It is implemented as a singleton managed by the `AudioSystem` class, providing a centralized interface for background music (BGM) streaming and fire-and-forget sound effects (SFX).

## AudioSystem Architecture

The system is built around the `ma_engine` from miniaudio, which handles device connection, mixing, and resource management.

| Component            | Code Entity            | Description                                              |
| :------------------- | :--------------------- | :------------------------------------------------------- |
| **Engine Singleton** | `AudioSystem`          | Manages the lifecycle of the miniaudio engine.           |
| **Music Handle**     | `ma_sound`             | Used for streaming long audio files (BGM) with looping.  |
| **SFX Playback**     | `ma_engine_play_sound` | Low-latency, fire-and-forget playback for short samples. |

## Background Music (BGM) vs. Sound Effects (SFX)

### Background Music (BGM)

Music is loaded and started using `playMusic(path, loop)`.

- **Streaming:** Uses the `MA_SOUND_FLAG_STREAM` flag to stream files from disk rather than loading them entirely into RAM.
- **Management:** If a previous track is playing, it is uninitialized and freed before the new track starts.
- **Looping:** Configured via `ma_sound_set_looping`.

### Sound Effects (SFX)

SFX are triggered via `playSound(path)`.

- **Fire-and-Forget:** Uses `ma_engine_play_sound`, which allows the engine to manage the sound's lifecycle automatically once playback begins.

## Audio Asset Catalogue

| File                | Type | Usage                                               |
| :------------------ | :--- | :-------------------------------------------------- |
| `bg.wav`            | BGM  | Main gameplay background music.                     |
| `menu.mp3`          | BGM  | Main menu theme.                                    |
| `attack.wav`        | SFX  | Aladdin's sword swing sound.                        |
| `hit.wav`           | SFX  | Sound played when Aladdin or an enemy takes damage. |
| `death.wav`         | SFX  | Played during the death animation.                  |
| `coinCollected.wav` | SFX  | Feedback for collecting coins.                      |
| `levelExit.wav`     | SFX  | Sound played when entering the level exit portal.   |
| `buttonSelect.mp3`  | SFX  | UI feedback for menu navigation.                    |

## Volume Management

The system provides separate controls for volume:

- **Master Volume:** `setMasterVolume(float)` affects every sound produced by the engine.
- **Music Volume:** `setMusicVolume(float)` specifically updates the active `ma_sound` handle for the BGM.
