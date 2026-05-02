#include "video-cutscene-overlay.hpp"

#include "../shader/shader.hpp"
#include "../input/keyboard.hpp"
#include "../audio/audio-system.hpp"

#include <GLFW/glfw3.h>
#include <iostream>

namespace our {

VideoCutsceneOverlay::~VideoCutsceneOverlay() {
    stop();
    destroyGpu();
}

bool VideoCutsceneOverlay::ensureGpu() {
    if (gpuReady) return true;

    shader = new ShaderProgram();
    if (!shader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER) ||
        !shader->attach("assets/shaders/video_cutscene.frag", GL_FRAGMENT_SHADER) ||
        !shader->link()) {
        std::cerr << "[VideoCutsceneOverlay] Shader failed to link\n";
        delete shader;
        shader = nullptr;
        return false;
    }

    glGenVertexArrays(1, &vao);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    gpuReady = true;
    return true;
}

void VideoCutsceneOverlay::destroyGpu() {
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (shader) {
        delete shader;
        shader = nullptr;
    }
    texW = texH = 0;
    gpuReady = false;
}

bool VideoCutsceneOverlay::start(const std::string& path) {
    stop();
    pausedBgmForCutscene = false;

    unsigned audioRate = 48000;
    if (our::AudioSystem::instance().isInitialized()) {
        audioRate = our::AudioSystem::instance().getEngineSampleRate();
    }

    if (!decoder.open(path, audioRate)) {
        std::cerr << "[VideoCutsceneOverlay] open failed: " << path << "\n";
        return false;
    }
    if (!ensureGpu()) {
        decoder.close();
        return false;
    }

    std::vector<float> pcm;
    unsigned ch = 0;
    unsigned rate = 0;
    if (decoder.takeDecodedAudio(pcm, ch, rate) && !pcm.empty() && our::AudioSystem::instance().isInitialized()) {
        our::AudioSystem::instance().pauseMusic();
        pausedBgmForCutscene = true;
        if (!our::AudioSystem::instance().playCutsceneInterleavedF32(std::move(pcm), (ma_uint32)ch, (ma_uint32)rate)) {
            our::AudioSystem::instance().resumeMusic();
            pausedBgmForCutscene = false;
        }
    }

    active = true;
    return true;
}

void VideoCutsceneOverlay::stop() {
    active = false;
    decoder.close();
    rgbaScratch.clear();
    texW = 0;
    texH = 0;

    if (our::AudioSystem::instance().isInitialized()) {
        our::AudioSystem::instance().stopCutscenePlayback();
        if (pausedBgmForCutscene) {
            our::AudioSystem::instance().resumeMusic();
            pausedBgmForCutscene = false;
        }
    }
}

void VideoCutsceneOverlay::update(float deltaSeconds, const Keyboard& keyboard, bool& skipRequested) {
    skipRequested = false;
    if (!active) return;

    if (keyboard.justPressed(GLFW_KEY_ESCAPE)) {
        skipRequested = true;
        stop();
        return;
    }

    int w = 0, h = 0;
    if (!decoder.advance(deltaSeconds, rgbaScratch, w, h)) {
        stop();
        return;
    }

    if (w <= 0 || h <= 0 || rgbaScratch.empty()) return;

    if (w != texW || h != texH) {
        texW = w;
        texH = h;
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaScratch.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_BYTE, rgbaScratch.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void VideoCutsceneOverlay::render(const glm::ivec2& windowSize) {
    if (!active || !gpuReady || !shader || texW <= 0 || texH <= 0) return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, windowSize.x, windowSize.y);
    glBindVertexArray(vao);
    shader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    shader->set("tex", 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

} // namespace our
