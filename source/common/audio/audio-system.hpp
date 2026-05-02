#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <miniaudio.h>

namespace our {

    /**
     * @brief Lightweight audio system built on top of miniaudio.
     *
     * Provides two channels:
     *   - **Music** (background loop): a single looping track that persists across frames.
     *   - **Sound effects (SFX)**: one-shot, fire-and-forget sounds triggered by gameplay events.
     *
     * Usage:
     *   AudioSystem::instance().initialize();       // once at startup
     *   AudioSystem::instance().playMusic("assets/audio/bg.wav");
     *   AudioSystem::instance().playSound("assets/audio/hit.wav");
     *   AudioSystem::instance().shutdown();          // once at teardown
     */
    class AudioSystem {
    public:
        // Singleton access — there should only be one audio device.
        static AudioSystem& instance() {
            static AudioSystem inst;
            return inst;
        }

        /// Call once (e.g. in main or the first state's onInitialize).
        /// Returns true on success.
        bool initialize() {
            if (initialized) return true;

            ma_engine_config config = ma_engine_config_init();
            if (ma_engine_init(&config, &engine) != MA_SUCCESS) {
                std::cerr << "[AudioSystem] Failed to initialize miniaudio engine.\n";
                return false;
            }
            initialized = true;
            std::cout << "[AudioSystem] Initialized.\n";
            return true;
        }

        /// Call once when the application is shutting down.
        void shutdown() {
            stopCutscenePlayback();
            stopMusic();
            ma_engine_uninit(&engine);
            initialized = false;
            std::cout << "[AudioSystem] Shut down.\n";
        }

        // ─────────────────────────────────── Music (looping BGM) ──

        /**
         * Start playing a music file in a loop.
         * If a different track is already playing it is stopped first.
         * Calling with the same path that is already playing is a no-op.
         */
        void playMusic(const std::string& path) {
            if (!initialized) return;

            // Already playing this track — do nothing.
            if (musicPlaying && currentMusicPath == path) return;

            stopMusic();

            if (ma_sound_init_from_file(&engine, path.c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, &musicSound) != MA_SUCCESS) {
                std::cerr << "[AudioSystem] Failed to load music: " << path << "\n";
                return;
            }
            ma_sound_set_looping(&musicSound, MA_TRUE);
            ma_sound_set_volume(&musicSound, musicVolume);
            ma_sound_start(&musicSound);
            musicPlaying = true;
            currentMusicPath = path;
        }

        /// Stop the currently playing music track (if any).
        void stopMusic() {
            if (musicPlaying) {
                ma_sound_stop(&musicSound);
                ma_sound_uninit(&musicSound);
                musicPlaying = false;
                currentMusicPath.clear();
            }
        }

        /// Set the music volume (0.0 – 1.0). Takes effect immediately.
        void setMusicVolume(float vol) {
            musicVolume = vol;
            if (musicPlaying) {
                ma_sound_set_volume(&musicSound, musicVolume);
            }
        }

        /// Pause / resume the music without unloading it.
        void pauseMusic()  { if (musicPlaying) ma_sound_stop(&musicSound);  }
        void resumeMusic() { if (musicPlaying) ma_sound_start(&musicSound); }

        // ─────────────────────────────────── Cutscene (memory PCM) ──

        ma_uint32 getEngineSampleRate() const {
            if (!initialized) return 48000;
            return ma_engine_get_sample_rate(&engine);
        }

        /// Stops cutscene PCM playback and releases miniaudio objects (safe if idle).
        void stopCutscenePlayback() {
            if (!cutscenePlaybackActive) return;
            ma_sound_stop(&cutsceneSound);
            ma_sound_uninit(&cutsceneSound);
            ma_audio_buffer_uninit(&cutsceneAudioBuffer);
            cutscenePcm.clear();
            cutscenePlaybackActive = false;
            cutsceneSound = {};
            cutsceneAudioBuffer = {};
        }

        /**
         * Play interleaved f32 PCM through the engine (e.g. FFmpeg-decoded MP4 audio).
         * Stops any previous cutscene playback. @p channels is usually 2.
         */
        bool playCutsceneInterleavedF32(std::vector<float>&& pcm, ma_uint32 channels, ma_uint32 sampleRate) {
            if (!initialized || pcm.empty() || channels == 0 || sampleRate == 0) return false;
            stopCutscenePlayback();

            const ma_uint64 frames = (ma_uint64)(pcm.size() / (size_t)channels);
            if (frames == 0) return false;

            cutscenePcm = std::move(pcm);

            ma_audio_buffer_config bufCfg = ma_audio_buffer_config_init(
                ma_format_f32, channels, frames, cutscenePcm.data(), nullptr);
            bufCfg.sampleRate = sampleRate;

            if (ma_audio_buffer_init(&bufCfg, &cutsceneAudioBuffer) != MA_SUCCESS) {
                cutscenePcm.clear();
                return false;
            }

            if (ma_sound_init_from_data_source(
                    &engine,
                    reinterpret_cast<ma_data_source*>(&cutsceneAudioBuffer.ref),
                    MA_SOUND_FLAG_NO_SPATIALIZATION,
                    nullptr,
                    &cutsceneSound) != MA_SUCCESS) {
                ma_audio_buffer_uninit(&cutsceneAudioBuffer);
                cutscenePcm.clear();
                cutsceneAudioBuffer = {};
                return false;
            }

            const float v = musicVolume * getMasterVolume();
            ma_sound_set_volume(&cutsceneSound, v);
            if (ma_sound_start(&cutsceneSound) != MA_SUCCESS) {
                ma_sound_uninit(&cutsceneSound);
                ma_audio_buffer_uninit(&cutsceneAudioBuffer);
                cutscenePcm.clear();
                cutsceneSound = {};
                cutsceneAudioBuffer = {};
                return false;
            }
            cutscenePlaybackActive = true;
            return true;
        }

        // ─────────────────────────────────── Sound Effects (one-shot) ──

        /**
         * Fire-and-forget sound effect.
         * The sound is played once at the given volume and then cleaned up automatically
         * by miniaudio's internal inlined-sound system.
         */
        void playSound(const std::string& path, float volume = 1.0f) {
            if (!initialized) return;
            // ma_engine_play_sound is the simplest fire-and-forget API in miniaudio.
            if (ma_engine_play_sound(&engine, path.c_str(), nullptr) != MA_SUCCESS) {
                std::cerr << "[AudioSystem] Failed to play sound: " << path << "\n";
            }
        }

        // ─────────────────────────────────── Master ──

        /// Set the master volume for the entire engine (0.0 – 1.0).
        void setMasterVolume(float vol) {
            if (initialized) ma_engine_set_volume(&engine, vol);
        }

        float getMasterVolume() {
            if (initialized) return ma_engine_get_volume(&engine);
            return 1.0f;
        }

        bool isInitialized() const { return initialized; }

    private:
        AudioSystem() = default;
        ~AudioSystem() { if (initialized) shutdown(); }
        AudioSystem(const AudioSystem&) = delete;
        AudioSystem& operator=(const AudioSystem&) = delete;

        ma_engine engine{};
        bool initialized = false;

        // Music state
        ma_sound  musicSound{};
        bool      musicPlaying = false;
        float     musicVolume  = 0.5f;
        std::string currentMusicPath;

        // Cutscene PCM (must outlive ma_audio_buffer while playing)
        std::vector<float> cutscenePcm;
        ma_audio_buffer cutsceneAudioBuffer{};
        ma_sound cutsceneSound{};
        bool cutscenePlaybackActive = false;
    };

} // namespace our
