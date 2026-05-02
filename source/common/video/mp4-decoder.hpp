#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace our {

/// Minimal FFmpeg-backed MP4 reader: decodes the first video stream to RGBA8.
class Mp4Decoder {
public:
    Mp4Decoder();
    ~Mp4Decoder();

    Mp4Decoder(const Mp4Decoder&) = delete;
    Mp4Decoder& operator=(const Mp4Decoder&) = delete;

    bool open(const std::string& path);
    void close();

    /// Advance playback by @p deltaSeconds (wall clock). Fills @p outRgba when a new
    /// frame should be displayed. Returns false when the stream has ended.
    bool advance(float deltaSeconds, std::vector<uint8_t>& outRgba, int& outWidth, int& outHeight);

    bool isOpen() const;
    double durationSeconds() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace our
