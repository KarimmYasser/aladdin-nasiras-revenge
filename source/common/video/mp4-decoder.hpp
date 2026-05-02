#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace our {

/// FFmpeg-backed MP4 reader: first video stream to RGBA8; optional full audio decode on open.
class Mp4Decoder {
public:
    Mp4Decoder();
    ~Mp4Decoder();

    Mp4Decoder(const Mp4Decoder&) = delete;
    Mp4Decoder& operator=(const Mp4Decoder&) = delete;

    /// @param audioOutputSampleRate engine sample rate (e.g. from miniaudio) for decoded PCM.
    bool open(const std::string& path, unsigned audioOutputSampleRate = 48000);
    void close();

    /// Advance playback by @p deltaSeconds (wall clock). Fills @p outRgba when a new
    /// frame should be displayed. Returns false when the stream has ended.
    bool advance(float deltaSeconds, std::vector<uint8_t>& outRgba, int& outWidth, int& outHeight);

    bool isOpen() const;
    double durationSeconds() const;

    /// If the file had an audio track, moves decoded interleaved f32 PCM (already at @p audioOutputSampleRate
    /// passed to open). Returns false when there was no usable audio.
    bool takeDecodedAudio(std::vector<float>& outInterleavedPcm, unsigned& outChannels, unsigned& outSampleRate);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace our
