#pragma once

#include "mp4-decoder.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include <glad/gl.h>

namespace our {

class Keyboard;
class ShaderProgram;

/// Fullscreen video overlay (MP4 via Mp4Decoder + textured triangle).
class VideoCutsceneOverlay {
public:
    VideoCutsceneOverlay() = default;
    ~VideoCutsceneOverlay();

    bool start(const std::string& path);
    void stop();

    /// Decodes video; uploads texture. If ESC is pressed, stops and sets @p skipRequested.
    void update(float deltaSeconds, const Keyboard& keyboard, bool& skipRequested);

    void render(const glm::ivec2& windowSize);

    bool isActive() const { return active; }

private:
    bool ensureGpu();
    void destroyGpu();

    bool active = false;
    bool gpuReady = false;

    ShaderProgram* shader = nullptr;
    GLuint vao = 0;
    GLuint texture = 0;
    int texW = 0;
    int texH = 0;

    Mp4Decoder decoder;
    std::vector<uint8_t> rgbaScratch;
    bool pausedBgmForCutscene = false;
};

} // namespace our
